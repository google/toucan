// Copyright 2023 The Toucan Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

%{
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <cstring>
#include <filesystem>
#include <optional>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "ast/ast.h"
#include "ast/symbol.h"
#include "ast/type.h"
#include "ast/type_replacement_pass.h"

#include "parser/lexer.h"
#include "parser/parser.h"

using namespace Toucan;

static void PushFile(const char* filename);

static NodeVector* nodes_;
static SymbolTable* symbols_;
static TypeTable* types_;
static std::vector<std::string> includePaths_;
static Stmts** rootStmts_;
static std::unordered_set<std::string> includedFiles_;
static std::stack<FileLocation> fileStack_;

extern int yylex();
extern int yylex_destroy();

static Expr* BinOp(BinOpNode::Op op, Expr* arg1, Expr* arg2);
static Expr* UnOp(UnaryOp::Op op, Expr* expr);
static Expr* IncDec(IncDecExpr::Op op, bool pre, Expr* expr);
static Stmt* MakeReturnStatement(Expr* expr);
static Expr* MakeStaticMethodCall(Type* type, const char *id, ArgList* arguments);
static ClassType* DeclareClass(int native, const char* id);
static void DeclareUsing(const char* id, Type* type);
static void BeginClass(Type* type, ClassType* parent);
static ClassType*  BeginClassTemplate(int native, TypeList* templateArgs, const char* id);
static Stmt* EndClass();
static void BeginEnum(const char *id);
static void AppendEnum(const char* id);
static void AppendEnum(const char* id, int value);
static void EndEnum();
static void BeginMethod(int modifiers, std::string id, ArgList* workgroupSize,
                        Stmts* formalArguments, int thisQualifiers, Type* returnType);
static void BeginConstructor(int modifiers, Type* type, Stmts* formalArguments);
static void BeginDestructor(int modifiers, Type* type);
static Method* EndMethod(Stmts* stmts);
static Method* EndConstructor(Expr* initializer, Stmts* stmts);
static void BeginBlock();
static void EndBlock(Stmts* stmts);
static Expr* ThisExpr();
static Expr* Load(Expr* expr);
static Stmt* Store(Expr* expr, Expr* value);
static Expr* Identifier(const char* id);
static Expr* Dot(Expr* lhs, const char* id);
static Expr* MakeArrayAccess(Expr* lhs, Expr* expr);
static Expr* MakeNewExpr(Type* type, Expr* length, ArgList* arguments);
static Expr* MakeNewArrayExpr(Type* type, Expr* length);
static Expr* InlineFile(const char* filename);
static Expr* StringLiteral(const char* str);
static void CreateFieldsFromVarDecls(Stmts* stmts);
static Type* GetArrayType(Type* elementType, int numElements);
static Type* GetScopedType(Type* type, const char* id);
static TypeList* AddIDToTypeList(const char* id, TypeList* list);
static ClassType* AsClassType(Type* type);
static ClassTemplate* AsClassTemplate(Type* type);
static int AsIntConstant(Expr* expr);

template <typename T, typename... ARGS> T* Make(ARGS&&... args) {
  T* node = nodes_->Make<T>(std::forward<ARGS>(args)...);
  node->SetFileLocation(fileStack_.top());
  return node;
}
inline TypeList* Append(TypeList* typeList) {
  types_->AppendTypeList(typeList); return typeList;
}
Type* FindType(const char* str) {
  return symbols_->FindType(str);
}
%}

%union {
    uint32_t             i;
    float                f;
    double               d;
    const char*          identifier;
    Toucan::Expr*        expr;
    Toucan::Stmt*        stmt;
    Toucan::Stmts*       stmts;
    Toucan::Arg*         arg;
    Toucan::ArgList*     argList;
    Toucan::Type*        type;
    Toucan::ClassType*   classType;
    Toucan::TypeList*    typeList;
};

%type <type> scalar_type type class_header
%type <type> simple_type opt_return_type
%type <classType> template_class_header
%type <expr> expr opt_expr assignable arith_expr expr_or_list opt_initializer
%type <arg> argument
%type <stmt> statement expr_statement for_loop_stmt
%type <stmt> assignment
%type <stmt> if_statement for_statement while_statement do_statement
%type <stmt> opt_else var_decl class_decl
%type <stmt> class_forward_decl
%type <stmts> statements var_decl_list var_decl_statement formal_arguments non_empty_formal_arguments method_body
%type <argList> arguments non_empty_arguments opt_workgroup_size
%type <typeList> types
%type <typeList> template_formal_arguments
%type <i> type_qualifier type_qualifiers opt_type_qualifiers
%type <i> method_modifier method_modifiers class_or_native_class
%type <type> opt_parent_class
%token <identifier> T_IDENTIFIER T_STRING_LITERAL
%token <type> T_TYPENAME
%token <i> T_BYTE_LITERAL T_UBYTE_LITERAL T_SHORT_LITERAL T_USHORT_LITERAL
%token <i> T_INT_LITERAL T_UINT_LITERAL
%token <f> T_FLOAT_LITERAL
%token <d> T_DOUBLE_LITERAL
%token T_TRUE T_FALSE T_NULL T_IF T_ELSE T_FOR T_WHILE T_DO T_RETURN T_NEW
%token T_CLASS T_ENUM T_VAR
%token T_READONLY T_WRITEONLY T_COHERENT T_DEVICEONLY
%token T_INT T_UINT T_FLOAT T_DOUBLE T_BOOL T_BYTE T_UBYTE T_SHORT T_USHORT
%token T_HALF
%token T_STATIC T_VIRTUAL T_NATIVE T_VERTEX T_FRAGMENT T_COMPUTE T_THIS
%token T_INDEX T_UNIFORM T_STORAGE T_SAMPLEABLE T_RENDERABLE
%token T_USING T_INLINE
%right '=' T_ADD_EQUALS T_SUB_EQUALS T_MUL_EQUALS T_DIV_EQUALS
%left T_LOGICAL_OR
%left T_LOGICAL_AND
%left '|'
%left '^'
%left '&'
%left T_EQ T_NE
%left T_LT T_LE T_GE T_GT
%left '+' '-'
%left '*' '/' '%'
%right UNARYMINUS '!' T_PLUSPLUS T_MINUSMINUS ':'
%left '.' '[' ']' '(' ')'
%expect 1   /* we expect 1 shift/reduce: dangling-else */
%%
program:
    statements                              { *rootStmts_ = $1; }

statements:
    statements statement                    { if ($2) $1->Append($2); $$ = $1; }
  | /* nothing */                           { $$ = Make<Stmts>(); }
  ;
statement:
    ';'                                     { $$ = 0; }
  | expr_statement ';'
  | '{' { BeginBlock(); } statements '}'    { EndBlock($3); $$ = $3; }
  | if_statement
  | for_statement
  | while_statement 
  | do_statement
  | T_RETURN expr ';'                       { $$ = MakeReturnStatement($2); }
  | T_RETURN ';'                            { $$ = MakeReturnStatement(0); }
  | var_decl_statement ';'                  { $$ = $1; }
  | class_decl
  | class_forward_decl
  | enum_decl                               { $$ = 0; }
  | using_decl                              { $$ = 0; }
  | assignment ';'
  ;

expr_statement:
    expr                                    { $$ = Make<ExprStmt>($1); }
  ;

assignment:
    assignable '=' expr_or_list             { $$ = Store($1, $3); }
  | assignable T_ADD_EQUALS expr            { $$ = Store($1, BinOp(BinOpNode::ADD, Load($1), $3)); }
  | assignable T_SUB_EQUALS expr            { $$ = Store($1, BinOp(BinOpNode::SUB, Load($1), $3)); }
  | assignable T_MUL_EQUALS expr            { $$ = Store($1, BinOp(BinOpNode::MUL, Load($1), $3)); }
  | assignable T_DIV_EQUALS expr            { $$ = Store($1, BinOp(BinOpNode::DIV, Load($1), $3)); }
  ;

if_statement:
    T_IF '(' expr ')' statement opt_else    { $$ = Make<IfStatement>($3, $5, $6); }
  ;
opt_else:
    T_ELSE statement                        { $$ = $2; }
  | /* nothing */                           { $$ = 0; }
  ;
for_statement:
    T_FOR '(' { symbols_->PushNewScope(); }
    for_loop_stmt ';' opt_expr ';' for_loop_stmt ')' statement
      {
        Stmts* stmts = Make<Stmts>();
        stmts->Append(Make<ForStatement>($4, $6, $8, $10));
        stmts->SetScope(symbols_->PopScope());
        $$ = stmts;
      }
  ;
opt_expr:
    expr
  | /* nothing */                           { $$ = 0; }
  ;
for_loop_stmt:
    assignment
  | expr_statement
  | var_decl_statement                      { $$ = $1; }
  | /* nothing */                           { $$ = 0; }
  ;
while_statement:
    T_WHILE '(' expr ')' statement          { $$ = Make<WhileStatement>($3, $5); }
  ;
do_statement:
    T_DO statement T_WHILE '(' expr ')' ';' { $$ = Make<DoStatement>($2, $5); }
  ;
var_decl_statement:
    T_VAR var_decl_list                     { $$ = $2; }
  ;

simple_type:
    T_TYPENAME
  | scalar_type
  | simple_type T_LT types T_GT             { $$ = types_->GetClassTemplateInstance(AsClassTemplate($1), *$3); }
  | simple_type T_LT T_INT_LITERAL T_GT     { $$ = types_->GetVector($1, $3); }
  | simple_type T_LT T_INT_LITERAL ',' T_INT_LITERAL T_GT 
    { $$ = types_->GetMatrix(types_->GetVector($1, $3), $5); }
  | simple_type ':' T_IDENTIFIER  { $$ = GetScopedType($1, $3); }
  ;

type:
    simple_type
  | type_qualifier type                     { $$ = types_->GetQualifiedType($2, $1); }
  | '*' type                                { $$ = types_->GetStrongPtrType($2); }
  | '^' type                                { $$ = types_->GetWeakPtrType($2); }
  | '[' arith_expr ']' type                 { $$ = GetArrayType($4, AsIntConstant($2)); }
  | '[' ']' type                            { $$ = GetArrayType($3, 0); }
  ;

var_decl_list:
    var_decl_list ',' var_decl              { $$ = $1; if ($3) $1->Append($3); }
  | var_decl                                { $$ = Make<Stmts>(); if ($1) $$->Append($1); }
  ;

class_or_native_class:
    T_CLASS                                 { $$ = 0; }
  | T_NATIVE T_CLASS                        { $$ = 1; }
  ;

class_header:
    class_or_native_class T_IDENTIFIER      { $$ = DeclareClass($1, $2); }
  | class_or_native_class T_TYPENAME        { $$ = AsClassType($2); }
  ;

template_class_header:
    class_or_native_class T_IDENTIFIER T_LT template_formal_arguments T_GT { $$ = BeginClassTemplate($1, $4, $2); }
  ;

class_forward_decl:
    class_header ';'                        { $$ = nullptr; }
  ;

class_decl:
    class_header opt_parent_class '{'       { BeginClass($1, AsClassType($2)); }
    class_body '}'                          { $$ = EndClass(); }
  | template_class_header opt_parent_class  '{' { $1->SetParent(AsClassType($2)); }
    class_body '}'                              { $$ = EndClass(); }
  ;
  ;

opt_parent_class:
    ':' simple_type                         { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
  ;

class_body:
    class_body class_body_decl
  | /* nothing */
  ;
enum_decl:
    T_ENUM T_IDENTIFIER '{'                 { BeginEnum($2); }
    enum_list '}'                           { EndEnum(); }
  ;
enum_list:
    enum_list ',' T_IDENTIFIER                     { AppendEnum($3); }
  | enum_list ',' T_IDENTIFIER '=' T_INT_LITERAL   { AppendEnum($3, $5); }
  | T_IDENTIFIER                                   { AppendEnum($1); }
  | T_IDENTIFIER '=' T_INT_LITERAL                 { AppendEnum($1, $3); }
  | /* nothing */
  ;

using_decl:
    T_USING T_IDENTIFIER '=' type ';'       { DeclareUsing($2, $4); }
  ;

opt_return_type:
    ':' type                                { $$ = $2; }
  | /* NOTHING */                           { $$ = types_->GetVoid(); }
  ;

class_body_decl:
    method_modifiers opt_workgroup_size T_IDENTIFIER '(' formal_arguments ')'
    opt_type_qualifiers opt_return_type     { BeginMethod($1, $3, $2, $5, $7, $8); }
    method_body                             { EndMethod($10); }
  | method_modifiers T_TYPENAME '(' formal_arguments ')'
                                            { BeginConstructor($1, $2, $4); }
    opt_initializer method_body             { EndConstructor($7, $8); }
  | method_modifiers '~' T_TYPENAME '(' ')' { BeginDestructor($1, $3); }
    method_body                             { EndMethod($7); }
  | var_decl_statement ';'                  { CreateFieldsFromVarDecls($1); }
  | enum_decl ';'
  | using_decl
  ;

method_body:
    '{' statements '}'                      { $$ = $2; }
  | ';'                                     { $$ = 0; }
  ;

template_formal_arguments:
    T_IDENTIFIER
                                            { $$ = AddIDToTypeList($1, nullptr); }
  | template_formal_arguments ',' T_IDENTIFIER
                                            { $$ = AddIDToTypeList($3, $1); }
  ;

method_modifier:
    T_STATIC                                { $$ = Method::Modifier::Static; }
  | T_VIRTUAL                               { $$ = Method::Modifier::Virtual; }
  | T_DEVICEONLY                            { $$ = Method::Modifier::DeviceOnly; }
  | T_VERTEX                                { $$ = Method::Modifier::Vertex; }
  | T_FRAGMENT                              { $$ = Method::Modifier::Fragment; }
  | T_COMPUTE                               { $$ = Method::Modifier::Compute; }
  ;

opt_type_qualifiers:
    type_qualifiers                         { $$ = $1; }
  | /* NOTHING */                           { $$ = 0; }
  ;

opt_workgroup_size:
    '(' arguments ')'                       { $$ = $2; }
  | /* NOTHING */                           { $$ = nullptr; }
  ;

type_qualifier:
    T_UNIFORM                               { $$ = Type::Qualifier::Uniform; }
  | T_STORAGE                               { $$ = Type::Qualifier::Storage; }
  | T_VERTEX                                { $$ = Type::Qualifier::Vertex; }
  | T_INDEX                                 { $$ = Type::Qualifier::Index; }
  | T_SAMPLEABLE                            { $$ = Type::Qualifier::Sampleable; }
  | T_RENDERABLE                            { $$ = Type::Qualifier::Renderable; }
  | T_READONLY                              { $$ = Type::Qualifier::ReadOnly; }
  | T_WRITEONLY                             { $$ = Type::Qualifier::WriteOnly; }
  | T_COHERENT                              { $$ = Type::Qualifier::Coherent; }
  ;

type_qualifiers:
    type_qualifier type_qualifiers          { $$ = $1 | $2; }
  | type_qualifier

method_modifiers:
    method_modifier method_modifiers        { $$ = $1 | $2; }
  | /* nothing */                           { $$ = 0; }
  ;

formal_arguments:
    non_empty_formal_arguments
  | /* nothing */                           { $$ = nullptr; }
  ;

non_empty_formal_arguments:
    formal_arguments ',' var_decl           { $1->Append($3); $$ = $1; }
  | var_decl                                { $$ = Make<Stmts>(); $$->Append($1); }
  ;

var_decl:
    T_IDENTIFIER ':' type                   { $$ = Make<VarDeclaration>($1, $3, nullptr); }
  | T_IDENTIFIER '=' expr_or_list           { $$ = Make<VarDeclaration>($1, types_->GetAuto(), $3); }
  | T_IDENTIFIER ':' type '=' expr_or_list  { $$ = Make<VarDeclaration>($1, $3, $5); }
  ;

scalar_type:
    T_INT           { $$ = types_->GetInt(); }
  | T_UINT          { $$ = types_->GetUInt(); }
  | T_SHORT         { $$ = types_->GetShort(); }
  | T_USHORT        { $$ = types_->GetUShort(); }
  | T_BYTE          { $$ = types_->GetByte(); }
  | T_UBYTE         { $$ = types_->GetUByte(); }
  | T_FLOAT         { $$ = types_->GetFloat(); }
  | T_DOUBLE        { $$ = types_->GetDouble(); }
  | T_BOOL          { $$ = types_->GetBool(); }
  ;

arguments:
    non_empty_arguments
  | /* nothing */                           { $$ = Make<ArgList>(); }
  ;

non_empty_arguments:
    non_empty_arguments ',' argument        { $1->Append($3); $$ = $1; }
  | argument                                { $$ = Make<ArgList>(); $$->Append($1); }
  ;

argument:
    T_IDENTIFIER '=' expr_or_list           { $$ = Make<Arg>($1, $3); }
  | expr_or_list                            { $$ = Make<Arg>("", $1); }
  ;

arith_expr:
    arith_expr '+' arith_expr               { $$ = BinOp(BinOpNode::ADD, $1, $3); }
  | arith_expr '-' arith_expr               { $$ = BinOp(BinOpNode::SUB, $1, $3); }
  | arith_expr '*' arith_expr               { $$ = BinOp(BinOpNode::MUL, $1, $3); }
  | arith_expr '/' arith_expr               { $$ = BinOp(BinOpNode::DIV, $1, $3); }
  | arith_expr '%' arith_expr               { $$ = BinOp(BinOpNode::MOD, $1, $3); }
  | '-' arith_expr %prec UNARYMINUS         { $$ = UnOp(UnaryOp::Op::Minus, $2); }
  | arith_expr T_LT arith_expr              { $$ = BinOp(BinOpNode::LT, $1, $3); }
  | arith_expr T_LE arith_expr              { $$ = BinOp(BinOpNode::LE, $1, $3); }
  | arith_expr T_EQ arith_expr              { $$ = BinOp(BinOpNode::EQ, $1, $3); }
  | arith_expr T_GT arith_expr              { $$ = BinOp(BinOpNode::GT, $1, $3); }
  | arith_expr T_GE arith_expr              { $$ = BinOp(BinOpNode::GE, $1, $3); }
  | arith_expr T_NE arith_expr              { $$ = BinOp(BinOpNode::NE, $1, $3); }
  | arith_expr T_LOGICAL_AND arith_expr     { $$ = BinOp(BinOpNode::LOGICAL_AND, $1, $3); }
  | arith_expr T_LOGICAL_OR arith_expr      { $$ = BinOp(BinOpNode::LOGICAL_OR, $1, $3); }
  | arith_expr '&' arith_expr               { $$ = BinOp(BinOpNode::BITWISE_AND, $1, $3); }
  | arith_expr '^' arith_expr               { $$ = BinOp(BinOpNode::BITWISE_XOR, $1, $3); }
  | arith_expr '|' arith_expr               { $$ = BinOp(BinOpNode::BITWISE_OR, $1, $3); }
  | '!' arith_expr                          { $$ = UnOp(UnaryOp::Op::Negate, $2); }
  | T_PLUSPLUS assignable                   { $$ = IncDec(IncDecExpr::Op::Inc, true, $2); }
  | T_MINUSMINUS assignable                 { $$ = IncDec(IncDecExpr::Op::Dec, true, $2); }
  | assignable T_PLUSPLUS                   { $$ = IncDec(IncDecExpr::Op::Inc, false, $1); }
  | assignable T_MINUSMINUS                 { $$ = IncDec(IncDecExpr::Op::Dec, false, $1); }
  | '(' arith_expr ')'                      { $$ = $2; }
  | '(' type ')' arith_expr %prec UNARYMINUS      { $$ = Make<CastExpr>($2, $4); }
  | simple_type '(' arguments ')'           { $$ = Make<UnresolvedInitializer>($1, $3, true); }
  | simple_type '{' arguments '}'           { $$ = Make<UnresolvedInitializer>($1, $3, false); }
  | '[' arith_expr ']' type '(' arguments ')'     { $$ = Make<UnresolvedInitializer>(GetArrayType($4, AsIntConstant($2)), $6, true); }
  | T_INT_LITERAL                           { $$ = Make<IntConstant>($1, 32); }
  | T_UINT_LITERAL                          { $$ = Make<UIntConstant>($1, 32); }
  | T_BYTE_LITERAL                          { $$ = Make<IntConstant>($1, 8); }
  | T_UBYTE_LITERAL                         { $$ = Make<UIntConstant>($1, 8); }
  | T_SHORT_LITERAL                         { $$ = Make<IntConstant>($1, 16); }
  | T_USHORT_LITERAL                        { $$ = Make<UIntConstant>($1, 16); }
  | T_FLOAT_LITERAL                         { $$ = Make<FloatConstant>($1); }
  | T_DOUBLE_LITERAL                        { $$ = Make<DoubleConstant>($1); }
  | T_TRUE                                  { $$ = Make<BoolConstant>(true); }
  | T_FALSE                                 { $$ = Make<BoolConstant>(false); }
  | T_NULL                                  { $$ = Make<NullConstant>(); }
  | assignable                              { $$ = Load($1); }
  ;

expr:
    arith_expr
  | T_NEW type                              { $$ = MakeNewExpr($2, nullptr, nullptr); }
  | T_NEW type '(' arguments ')'            { $$ = MakeNewExpr($2, nullptr, $4); }
  | '[' arith_expr ']' T_NEW type           { $$ = MakeNewArrayExpr($5, $2); }
  | '[' arith_expr ']' T_NEW type '(' arguments ')'
                                            { $$ = MakeNewExpr($5, $2, $7); }
  | T_INLINE '(' T_STRING_LITERAL ')'       { $$ = InlineFile($3); }
  | T_STRING_LITERAL                        { $$ = StringLiteral($1); }
  ;

expr_or_list:
    expr
  | '{' arguments '}'                       { $$ = Make<UnresolvedListExpr>($2); }
  ;

opt_initializer:
    ':' expr_or_list                        { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
  ;

types:
    type                                    { $$ = Append(new TypeList()); $$->push_back($1);  }
  | types ',' type                          { $1->push_back($3); $$ = $1; }
  ;

assignable:
    T_IDENTIFIER                            { $$ = Identifier($1); }
  | T_THIS                                  { $$ = ThisExpr(); }
  | assignable '[' expr ']'                 { $$ = MakeArrayAccess($1, $3); }
  | assignable '.' T_IDENTIFIER             { $$ = Dot($1, $3); }
  | assignable '.' T_IDENTIFIER '(' arguments ')'
                                            { $$ = Make<UnresolvedMethodCall>($1, $3, $5); }
  | simple_type '.' T_IDENTIFIER '(' arguments ')'
                                            { $$ = MakeStaticMethodCall($1, $3, $5); }
  | assignable ':'                          { $$ = Make<SmartToRawPtr>(Make<LoadExpr>($1)); }
  | '&' assignable %prec UNARYMINUS         { $$ = Make<RawToWeakPtr>($2); }
  ;

%%

extern "C" int yywrap(void) {
  return 1;
}

static int numSyntaxErrors = 0;

void yyerror(const char *s) {
  std::string filename = std::filesystem::path(GetFileName()).filename().string();
  fprintf(stderr, "%s:%d: %s\n", filename.c_str(), GetLineNum(), s);
  numSyntaxErrors++;
}

static void yyerrorf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string filename = std::filesystem::path(GetFileName()).filename().string();
  fprintf(stderr, "%s:%d: ", filename.c_str(), GetLineNum());
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  numSyntaxErrors++;
}

static Expr* BinOp(BinOpNode::Op op, Expr* arg1, Expr* arg2) {
  if (!arg1 || !arg2) return nullptr;
  return Make<BinOpNode>(op, arg1, arg2);
}

static Expr* UnOp(UnaryOp::Op op, Expr* expr) {
  if (!expr) return nullptr;
  return Make<UnaryOp>(op, expr);
}

static Expr* IncDec(IncDecExpr::Op op, bool pre, Expr* expr) {
  if (!expr) return nullptr;
  return Make<IncDecExpr>(op, expr, !pre);
}

static Expr* Identifier(const char* id) {
  if (!id) return nullptr;
  return Make<UnresolvedIdentifier>(id);
}

static Expr* Dot(Expr* lhs, const char* id) {
  if (!lhs || !id) return nullptr;
  return Make<UnresolvedDot>(lhs, id);
}

static Expr* MakeArrayAccess(Expr* lhs, Expr* expr) {
  if (!lhs) return nullptr;
  return Make<ArrayAccess>(lhs, expr);
}

static Expr* Load(Expr* expr) {
  if (!expr) return nullptr;
  return Make<LoadExpr>(expr);
}

static Expr* MakeNewExpr(Type* type, Expr* length, ArgList* arguments) {
  if (!type) return nullptr;
  return Make<UnresolvedNewExpr>(type, length, arguments);
}

static Expr* MakeNewArrayExpr(Type* type, Expr* length) {
  if (!type || !length) return nullptr;
  return Make<NewArrayExpr>(type, length);
}

static Expr* TryInlineFile(std::string dir, const char* filename) {
  struct stat statbuf;
  std::string path = !dir.empty() ? dir + "/" + filename : filename;
  if (stat(path.c_str(), &statbuf) != 0) {
    return nullptr;
  }
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    yyerrorf("file \"%s\" could not be opened for reading", filename);
    return nullptr;
  }
  off_t size = statbuf.st_size;
  auto buffer = std::make_unique<uint8_t[]>(size);
  fread(buffer.get(), size, 1, f);
  Type* type = types_->GetArrayType(types_->GetUByte(), 0, MemoryLayout::Default);
  return Make<Data>(type, std::move(buffer), size);
}

static Expr* InlineFile(const char* filename) {
  for (auto path : includePaths_) {
    if (Expr* e = TryInlineFile(path, filename)) {
      return e;
    }
  }
  if (Expr* e = TryInlineFile("", filename)) {
    return e;
  }
  yyerrorf("file \"%s\" not found", filename);
  return nullptr;
}

static Expr* StringLiteral(const char* str) {
  size_t length = strlen(str);
  auto buffer = std::make_unique<uint8_t[]>(length);
  memcpy(buffer.get(), str, length);
  Type* type = types_->GetArrayType(types_->GetUByte(), 0, MemoryLayout::Default);
  return Make<Data>(type, std::move(buffer), length);
}

static void PushFile(const char* filename) {
  fileStack_.push(FileLocation(std::make_shared<std::string>(filename), 1));
}

static std::optional<std::string> FindIncludeFile(const char* filename) {
  struct stat statbuf;
  for (auto dir : includePaths_) {
    auto path = std::filesystem::path(dir) / filename;
    if (stat(path.string().c_str(), &statbuf) == 0) {
      return path.string();
    }
  }
  auto path = std::filesystem::path(*fileStack_.top().filename);
  path = path.replace_filename(filename);
  if (stat(path.string().c_str(), &statbuf) == 0) {
    return path.string();
  }
  return std::nullopt;
}

FILE* IncludeFile(const char* filename) {
  auto path = FindIncludeFile(filename);
  if (!path) {
    yyerrorf("file \"%s\" not found", filename);
    return nullptr;
  }
  if (includedFiles_.find(*path) != includedFiles_.end()) {
    // File was previously included
    return nullptr;
  }
  FILE* f = fopen(path->c_str(), "r");
  if (!f) {
    yyerrorf("file \"%s\" could not be opened for reading", filename);
    return nullptr;
  }
  includedFiles_.insert(*path);
  PushFile(path->c_str());
  return f;
}

void PopFile() {
  fileStack_.pop();
}

std::string GetFileName() {
  return *fileStack_.top().filename;
}

int GetLineNum() {
  return fileStack_.top().lineNum;
}

void IncLineNum() {
  fileStack_.top().lineNum++;
}

static Expr* ThisExpr() {
  return Make<UnresolvedIdentifier>("this");
}

static Stmt* Store(Expr* lhs, Expr* rhs) {
  if (!rhs) return nullptr;
  return Make<StoreStmt>(lhs, rhs);
}

static Stmt* MakeReturnStatement(Expr* expr) {
  return Make<ReturnStatement>(expr, symbols_->PeekScope());
}

static Expr* MakeStaticMethodCall(Type* type, const char* id, ArgList* arguments) {
  if (!type->IsClass()) {
    yyerrorf("attempt to call method on non-class\n");
    return nullptr;
  }
  return Make<UnresolvedStaticMethodCall>(static_cast<ClassType*>(type), id, arguments);
}

static ClassType* DeclareClass(int native, const char *id) {
  assert(!symbols_->FindType(id));
  ClassType* c = types_->Make<ClassType>(id);
  c->SetNative(native != 0);
  symbols_->DefineType(id, c);
  return c;
}

static void DeclareUsing(const char *id, Type* type) {
  assert(!symbols_->FindType(id));
  symbols_->DefineType(id, type);
}

static void BeginClass(Type* t, ClassType* parent) {
  ClassType* c = static_cast<ClassType*>(t);
  if (c->IsDefined()) {
    yyerrorf("class \"%s\" already has a definition", c->GetName().c_str());
    return;
  }
  c->SetParent(parent);
  c->SetDefined(true);
  Scope* scope = symbols_->PushNewScope();
  scope->classType = c;
  c->SetScope(scope);
}

static ClassType* BeginClassTemplate(int native, TypeList* templateArgs, const char* id) {
  ClassTemplate* t = types_->Make<ClassTemplate>(id, *templateArgs);
  symbols_->DefineType(id, t);
  t->SetDefined(true);
  t->SetNative(native != 0);
  Scope* scope = symbols_->PushNewScope();
  scope->classType = t;
  t->SetScope(scope);
  for (Type* const& i : *templateArgs) {
    auto type = static_cast<FormalTemplateArg*>(i);
    symbols_->DefineType(type->GetName(), type);
  }
  return t;
}

static Stmt* EndClass() {
  Scope* scope = symbols_->PopScope();
  assert(scope->classType);
  ClassType* classType = scope->classType;
  return Make<UnresolvedClassDefinition>(scope);
}

static void BeginEnum(const char *id) {
  EnumType* e = types_->Make<EnumType>(id);
  // FIXME:  Check for class or enum of the same name.
  ClassType* classType = symbols_->PeekScope()->classType;
  if (classType) {
    classType->AddEnum(id, e);
  } else {
    // FIXME:  Global enums must die.
    symbols_->DefineType(id, e);
  }
  Scope* scope = symbols_->PushNewScope();
  scope->enumType = e;
}

static void EndEnum() {
  symbols_->PopScope();
}

static void AppendEnum(const char *id) {
  EnumType* enumType = symbols_->PeekScope()->enumType;
  assert(enumType);
  enumType->Append(id);
}

static void AppendEnum(const char *id, int value) {
  EnumType* enumType = symbols_->PeekScope()->enumType;
  assert(enumType);
  enumType->Append(id, value);
}

static void BeginBlock() {
  symbols_->PushNewScope();
}

static void EndBlock(Stmts* stmts) {
  stmts->SetScope(symbols_->PopScope());
}

static void BeginMethod(int modifiers, std::string id, ArgList* workgroupSize,
                        Stmts* formalArguments, int thisQualifiers, Type* returnType) {
  Scope* classScope = symbols_->PeekScope();
  while (!classScope->classType) {
    classScope = classScope->parent;
  }
  ClassType* classType = classScope->classType;
  Method* method = new Method(modifiers, returnType, id, classType);
  if (!(modifiers & Method::Modifier::Static)) {
    Type* thisType = types_->GetQualifiedType(classType, thisQualifiers);
    thisType = types_->GetWeakPtrType(thisType);
    method->AddFormalArg("this", thisType, nullptr);
  }
  if (formalArguments) {
    for (Stmt* const& it : formalArguments->GetStmts()) {
      VarDeclaration* v = static_cast<VarDeclaration*>(it);
      method->AddFormalArg(v->GetID(), v->GetType(), v->GetInitExpr());
    }
  }
  if (workgroupSize) {
    auto args = workgroupSize->GetArgs();
    if (!(modifiers & Method::Modifier::Compute)) {
      yyerror("non-compute shaders do not require a workgroup size");
    } else if (args.size() == 0 || args.size() > 3) {
      yyerror("workgroup size must have 1, 2, or 3 dimensions");
    } else {
      for (int i = 0; i < args.size(); ++i) {
        Expr* expr = args[i]->GetExpr();
        if (!expr->IsIntConstant()) {
          yyerrorf("workgroup size is not an integer constant");
          break;
        } else {
          method->workgroupSize[i] = static_cast<IntConstant*>(expr)->GetValue();
        }
      }
    }
  } else if (method->modifiers & Method::Modifier::Compute) {
    yyerrorf("compute shader requires a workgroup size");
  }
  Scope* scope = symbols_->PushNewScope();
  scope->method = method;
}

static void BeginConstructor(int modifiers, Type* type, Stmts* formalArguments) {
  if (!type->IsClass()) {
    yyerror("constructor must be of class type");
    return;
  }
  ClassType* classType = static_cast<ClassType*>(type);
  if (classType->IsNative()) {
    modifiers |= Method::Modifier::Static;
  }
  auto returnType = types_->GetWeakPtrType(classType);
  BeginMethod(modifiers, classType->GetName(), nullptr, formalArguments, 0, returnType);
}

static void BeginDestructor(int modifiers, Type* type) {
  if (!type->IsClass()) {
    yyerror("destructor must be of class type");
    return;
  }
  ClassType* classType = static_cast<ClassType*>(type);
  modifiers |= Method::Modifier::Virtual;
  std::string name(std::string("~") + classType->GetName());
  Type* returnType = types_->GetVoid();
  BeginMethod(modifiers, name.c_str(), nullptr, nullptr, 0, returnType);
}

static void CreateFieldsFromVarDecls(Stmts* stmts) {
  ClassType* classType = symbols_->PeekScope()->classType;
  assert(classType);
  for (Stmt* const& it : stmts->GetStmts()) {
    VarDeclaration* v = static_cast<VarDeclaration*>(it);
    classType->AddField(v->GetID(), v->GetType(), v->GetInitExpr());
  }
}

static Type* GetScopedType(Type* type, const char* id) {
  if (type->IsFormalTemplateArg()) {
    return types_->GetUnresolvedScopedType(static_cast<FormalTemplateArg*>(type), id);
  }
  if (!type->IsClass()) {
    yyerrorf("\"%s\" is not a class type", type->ToString().c_str());
    return nullptr;
  }
  Type* scopedType = static_cast<ClassType*>(type)->FindType(id);
  if (!type) {
    yyerrorf("class \"%s\" has no type named \"%s\"", type->ToString().c_str(), id);
    return nullptr;
  }
  return scopedType;
}

static Method* EndConstructor(Expr* initializer, Stmts* stmts) {
  if (stmts) {
    stmts->Append(Make<ReturnStatement>(Load(ThisExpr()), symbols_->PeekScope()));
  }
  Method* method = EndMethod(stmts);
  if (method->stmts) {
    if (!initializer) {
      initializer = Make<UnresolvedListExpr>(Make<ArgList>());
    }
    Expr* rawPtrThis = Make<SmartToRawPtr>(Load(ThisExpr()));
    method->stmts->Prepend(Make<StoreStmt>(rawPtrThis, initializer));
  }
  return method;
}

static Method* EndMethod(Stmts* stmts) {
  Scope* methodScope = symbols_->PopScope();
  Method* method = methodScope->method;
  method->stmts = stmts;
  if (stmts) stmts->SetScope(methodScope);
  Scope* scope = symbols_->PeekScope();
  if (!scope || !scope->classType) {
    yyerror("method definition outside class?!");
    return nullptr;
  }
  scope->classType->AddMethod(method);
  return method;
}

static TypeList* AddIDToTypeList(const char* id, TypeList* list) {
  if (!list) {
    list = Append(new TypeList());
  }
  list->push_back(types_->GetFormalTemplateArg(id));
  return list;
}

static void InstantiateClassTemplates() {
  while (ClassType* instance = types_->PopInstanceQueue()) {
    Scope* scope = symbols_->PushNewScope();
    ClassTemplate* classTemplate = instance->GetTemplate();
    scope->classType = instance;
    TypeReplacementPass pass(nodes_, symbols_, types_, classTemplate->GetFormalTemplateArgs(), instance->GetTemplateArgs());
    pass.ResolveClassInstance(classTemplate, instance);
    numSyntaxErrors += pass.NumErrors();
    (*rootStmts_)->Append(Make<UnresolvedClassDefinition>(scope));
    symbols_->PopScope();
  }
}

int ParseProgram(const char* filename,
                 SymbolTable* symbols,
                 TypeTable* types,
                 NodeVector* nodes,
                 const std::vector<std::string>& includePaths,
                 Stmts** rootStmts) {
  numSyntaxErrors = 0;
  nodes_ = nodes;
  symbols_ = symbols;
  types_ = types;
  includePaths_ = includePaths;
  rootStmts_ = rootStmts;
  PushFile(filename);
  yyparse();
  if (numSyntaxErrors == 0) {
    if (!(*rootStmts_)->ContainsReturn()) {
      (*rootStmts_)->Append(Make<ReturnStatement>(nullptr, symbols_->PeekScope()));
    }
    InstantiateClassTemplates();
  }
  PopFile();
  nodes_ = nullptr;
  symbols_ = nullptr;
  types_ = nullptr;
  rootStmts_ = nullptr;
#ifndef _WIN32
  yylex_destroy();
#endif
  return numSyntaxErrors;
}

static ClassType* AsClassType(Type* type) {
  if (type && !type->IsClass()) {
    yyerrorf("type is already declared as non-class");
    return nullptr;
  }
  return static_cast<ClassType*>(type);
}

static ClassTemplate* AsClassTemplate(Type* type) {
  if (type && !type->IsClassTemplate()) {
    yyerrorf("template is already declared as non-template");
    return nullptr;
  }
  return static_cast<ClassTemplate*>(type);
}

static int AsIntConstant(Expr* expr) {
  if (expr && !expr->IsIntConstant()) {
    yyerrorf("array size is not an integer constant");
  }
  return static_cast<IntConstant*>(expr)->GetValue();
}

static Type* GetArrayType(Type* elementType, int numElements) {
  if (elementType->IsVoid() || elementType->IsAuto()) {
    yyerrorf("invalid array element type \"%s\"", elementType->ToString().c_str());
    return nullptr;
  }
  return types_->GetArrayType(elementType, numElements, MemoryLayout::Default);
}
