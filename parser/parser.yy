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
static Stmts* rootStmts_;
static std::unordered_set<std::string> includedFiles_;
static std::stack<FileLocation> fileStack_;
static std::queue<ClassType*> instanceQueue_;
static EnumType* currentEnumType_;

extern int yylex();
extern int yylex_destroy();

static Expr* BinOp(BinOpNode::Op op, Expr* arg1, Expr* arg2);
static Expr* UnOp(UnaryOp::Op op, Expr* expr);
static Expr* IncDec(IncDecExpr::Op op, bool pre, Expr* expr);
static Stmt* MakeReturnStatement(Expr* expr);
static Expr* MakeStaticMethodCall(Type* type, const char *id, ArgList* arguments);
static ClassType* DeclareClass(const char* id);
static EnumType* DeclareEnum(const char* id);
static void DeclareUsing(const char* id, Type* type);
static void BeginClass(Type* type, ClassType* parent);
static ClassType*  BeginClassTemplate(TypeList* templateArgs, const char* id);
static Stmt* EndClass(ClassType* classType, Stmts* body);
static void BeginEnum(Type* e);
static void AppendEnum(const char* id);
static void AppendEnum(const char* id, int value);
static void EndEnum();
static MethodDecl* MakeMethodDecl(int modifiers, ArgList* optWorkgroupSize, std::string id,
                                  Stmts* formalArguments, int thisQualifiers, Type* returnType, 
                                  Expr* initializer, Stmts* body);
static MethodDecl* MakeConstructor(int modifiers, Type* type, Stmts* formalArguments,
                                   Expr* initializer, Stmts* body);
static MethodDecl* MakeDestructor(int modifiers, Type* type, Stmts* body);
static void BeginBlock();
static void EndBlock(Stmts* stmts);
static Expr* Load(Expr* expr);
static Stmt* Store(Expr* expr, Expr* value);
static Expr* Identifier(const char* id);
static Expr* MakeArrayAccess(Expr* lhs, Expr* expr);
static Expr* MakeNewExpr(UnresolvedInitializer* initializer, Expr* length = nullptr);
static Expr* InlineFile(const char* filename);
static Expr* StringLiteral(const char* str);
static Type* GetArrayType(Type* elementType, int numElements);
static Type* GetScopedType(Type* type, const char* id);
static TypeList* AddIDToTypeList(const char* id, TypeList* list);
static ClassType* GetClassTemplateInstance(Type* type, const TypeList& templateArgs);
static ClassType* AsClassType(Type* type);
static EnumType* AsEnumType(Type* type);
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
    Toucan::Decls*       decls;
    Toucan::Arg*         arg;
    Toucan::ArgList*     argList;
    Toucan::UnresolvedInitializer* initializer;
    Toucan::Type*        type;
    Toucan::TypeList*    typeList;
};

%type <type> scalar_type type class_header template_class_header enum_header
%type <type> simple_type opt_return_type
%type <expr> expr opt_expr assignable arith_expr expr_or_list opt_initializer opt_length list_initializer
%type <initializer> initializer initializer_or_type
%type <arg> argument
%type <stmt> statement expr_statement var_decl_statement const_decl_statement for_loop_stmt
%type <stmt> assignment
%type <stmt> if_statement for_statement while_statement do_statement
%type <stmt> opt_else class_decl class_body_decl var_decl const_decl
%type <stmt> class_forward_decl
%type <stmts> statements formal_arguments non_empty_formal_arguments method_body
%type <decls> var_decl_list const_decl_list
%type <stmts> class_body block_statement
%type <argList> arguments non_empty_arguments opt_workgroup_size
%type <typeList> types
%type <typeList> template_formal_arguments
%type <i> type_qualifier type_qualifiers opt_type_qualifiers
%type <i> method_modifier method_modifiers
%type <type> opt_parent_class
%token <identifier> T_IDENTIFIER T_STRING_LITERAL
%token <type> T_TYPENAME
%token <i> T_BYTE_LITERAL T_UBYTE_LITERAL T_SHORT_LITERAL T_USHORT_LITERAL
%token <i> T_INT_LITERAL T_UINT_LITERAL
%token <f> T_FLOAT_LITERAL
%token <d> T_DOUBLE_LITERAL
%token T_TRUE T_FALSE T_NULL T_IF T_ELSE T_FOR T_WHILE T_DO T_RETURN T_NEW
%token T_CLASS T_ENUM T_VAR T_CONST
%token T_READONLY T_WRITEONLY T_COHERENT T_DEVICEONLY T_HOSTREADABLE T_HOSTWRITEABLE
%token T_INT T_UINT T_FLOAT T_DOUBLE T_BOOL T_BYTE T_UBYTE T_SHORT T_USHORT
%token T_HALF
%token T_STATIC T_VERTEX T_FRAGMENT T_COMPUTE T_THIS
%token T_INDEX T_UNIFORM T_STORAGE T_SAMPLEABLE T_RENDERABLE
%token T_USING T_INLINE T_UNFILTERABLE
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
%right UNARYMINUS '!' T_PLUSPLUS T_MINUSMINUS T_DOTDOT ':' '@'
%left '.' '[' ']' '(' ')' '{' '}'
%expect 1   /* we expect 1 shift/reduce: dangling-else */
%%
program:
    statements                              { rootStmts_->Append($1->GetStmts()); }

statements:
    statements statement                    { if ($2) $1->Append($2); $$ = $1; }
  | /* nothing */                           { $$ = Make<Stmts>(); }
  ;

block_statement:
    '{' { BeginBlock(); } statements '}'    { EndBlock($3); $$ = $3; }
  ;
statement:
    ';'                                     { $$ = 0; }
  | expr_statement ';'
  | block_statement                         { $$ = $1; }
  | if_statement
  | for_statement
  | while_statement 
  | do_statement
  | T_RETURN opt_expr ';'                   { $$ = MakeReturnStatement($2); }
  | var_decl_statement ';'                  { $$ = $1; }
  | const_decl_statement ';'                { $$ = $1; }
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
    T_FOR '(' { BeginBlock(); }
    for_loop_stmt ';' opt_expr ';' for_loop_stmt ')' statement
      {
        Stmts* stmts = Make<Stmts>();
        stmts->Append(Make<ForStatement>($4, $6, $8, $10));
        EndBlock(stmts);
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

const_decl_statement:
    T_CONST const_decl_list                 { $$ = $2; }
  ;

simple_type:
    T_TYPENAME
  | scalar_type
  | simple_type T_LT types T_GT             { $$ = GetClassTemplateInstance($1, *$3); }
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
  | '&' type                                { $$ = types_->GetRawPtrType($2); }
  | '[' arith_expr ']' type                 { $$ = GetArrayType($4, AsIntConstant($2)); }
  | '[' ']' type                            { $$ = GetArrayType($3, 0); }
  ;

var_decl_list:
    var_decl_list ',' var_decl              { $$ = $1; if ($3) $1->Append($3); }
  | var_decl                                { $$ = Make<Decls>(); if ($1) $$->Append($1); }
  ;

const_decl_list:
    const_decl_list ',' const_decl          { $$ = $1; if ($3) $1->Append($3); }
  | const_decl                              { $$ = Make<Decls>(); if ($1) $$->Append($1); }
  ;

class_header:
    T_CLASS T_IDENTIFIER                    { $$ = DeclareClass($2); }
  | T_CLASS T_TYPENAME                      { $$ = AsClassType($2); }
  ;

template_class_header:
    T_CLASS T_IDENTIFIER T_LT template_formal_arguments T_GT
                                            { $$ = BeginClassTemplate($4, $2); }
  ;

class_forward_decl:
    class_header ';'                        { $$ = nullptr; }
  ;

class_decl:
    class_header opt_parent_class '{'       { BeginClass($1, AsClassType($2)); }
    class_body '}'                          { $$ = EndClass(AsClassType($1), $5); }
  | template_class_header opt_parent_class  '{' { AsClassType($1)->SetParent(AsClassType($2)); }
    class_body '}'                              { $$ = EndClass(AsClassType($1), $5); }
  ;
  ;

opt_parent_class:
    ':' simple_type                         { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
  ;

class_body:
    class_body class_body_decl              { if ($2) $1->Append($2); $$ = $1; }
  | /* nothing */                           { $$ = Make<Stmts>(); }
  ;

enum_header:
    T_ENUM T_IDENTIFIER                     { $$ = DeclareEnum($2); }
  | T_ENUM T_TYPENAME                       { $$ = AsEnumType($2); }
  ;

enum_decl:
    enum_header '{'                         { BeginEnum($1); }
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
    method_modifiers opt_workgroup_size T_IDENTIFIER '(' formal_arguments ')' opt_type_qualifiers
    opt_return_type method_body
                                            { $$ = MakeMethodDecl($1, $2, $3, $5, $7, $8, 0, $9); }
  | method_modifiers T_TYPENAME '(' formal_arguments ')' opt_initializer method_body
                                            { $$ = MakeConstructor($1, $2, $4, $6, $7); }
  | method_modifiers '~' T_TYPENAME '(' ')' method_body
                                            { $$ = MakeDestructor($1, $3, $6); }
  | var_decl_statement ';'                  { $$ = $1; }
  | const_decl_statement ';'                { $$ = $1; }
  | enum_decl ';'                           { $$ = 0; }
  | using_decl                              { $$ = 0; }
  ;

method_body:
    block_statement                         { $$ = $1; }
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
  | T_HOSTREADABLE                          { $$ = Type::Qualifier::HostReadable; }
  | T_HOSTWRITEABLE                         { $$ = Type::Qualifier::HostWriteable; }
  | T_COHERENT                              { $$ = Type::Qualifier::Coherent; }
  | T_UNFILTERABLE                          { $$ = Type::Qualifier::Unfilterable; }
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

const_decl:
    T_IDENTIFIER '=' expr_or_list           { $$ = Make<ConstDecl>($1, $3); }
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
  | '@' expr_or_list                        { $$ = Make<Arg>("", $2, true); }
  ;

initializer:
    type '(' arguments ')'                  { $$ = Make<UnresolvedInitializer>($1, $3, true); }
  | type '{' arguments '}'                  { $$ = Make<UnresolvedInitializer>($1, $3, false); }
  ;

initializer_or_type:
    initializer
  | type                                    { $$ = Make<UnresolvedInitializer>($1, Make<ArgList>(), false); }
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
  | '&' assignable %prec UNARYMINUS         { $$ = $2; }
  ;

opt_length:
    '[' arith_expr ']'                      { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
  ;

expr:
    arith_expr
  | opt_length T_NEW initializer_or_type    { $$ = MakeNewExpr($3, $1); }
  | T_INLINE '(' T_STRING_LITERAL ')'       { $$ = InlineFile($3); }
  | T_STRING_LITERAL                        { $$ = StringLiteral($1); }
  ;

list_initializer:
    '{' arguments '}'                       { $$ = Make<UnresolvedListExpr>($2); }
  ;

expr_or_list:
    expr
  | list_initializer
  ;

opt_initializer:
    ':' list_initializer                    { $$ = $2; }
  | ':' initializer                         { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
  ;

types:
    type                                    { $$ = Append(new TypeList()); $$->push_back($1);  }
  | types ',' type                          { $1->push_back($3); $$ = $1; }
  ;

assignable:
    T_IDENTIFIER                            { $$ = Identifier($1); }
  | T_THIS                                  { $$ = Make<UnresolvedIdentifier>("this"); }
  | assignable '[' expr ']'                 { $$ = MakeArrayAccess($1, $3); }
  | assignable '[' opt_expr T_DOTDOT opt_expr ']'
                                            { $$ = Make<SliceExpr>($1, $3, $5); }
  | assignable '.' T_IDENTIFIER             { $$ = Make<UnresolvedDot>($1, $3); }
  | simple_type '.' T_IDENTIFIER            { $$ = Make<UnresolvedStaticDot>($1, $3); }
  | assignable '.' T_IDENTIFIER '(' arguments ')'
                                            { $$ = Make<UnresolvedMethodCall>($1, $3, $5); }
  | simple_type '.' T_IDENTIFIER '(' arguments ')'
                                            { $$ = MakeStaticMethodCall($1, $3, $5); }
  | assignable ':'                          { $$ = Make<SmartToRawPtr>(Make<LoadExpr>($1)); }
  | initializer                             { $$ = Make<TempVarExpr>(nullptr, $1); }
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

static Expr* MakeArrayAccess(Expr* lhs, Expr* expr) {
  if (!lhs) return nullptr;
  return Make<ArrayAccess>(lhs, expr);
}

static Expr* Load(Expr* expr) {
  if (!expr) return nullptr;
  return Make<LoadExpr>(expr);
}

static Expr* MakeNewExpr(UnresolvedInitializer* initializer, Expr* length) {
  if (!initializer->GetType()) return nullptr;
  return Make<UnresolvedNewExpr>(initializer->GetType(), length, initializer->GetArgList(), initializer->IsConstructor());
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

static Stmt* Store(Expr* lhs, Expr* rhs) {
  if (!rhs) return nullptr;
  return Make<StoreStmt>(lhs, rhs);
}

static Stmt* MakeReturnStatement(Expr* expr) {
  return Make<ReturnStatement>(expr);
}

static Expr* MakeStaticMethodCall(Type* type, const char* id, ArgList* arguments) {
  if (!type->IsClass()) {
    yyerrorf("attempt to call method on non-class\n");
    return nullptr;
  }
  return Make<UnresolvedStaticMethodCall>(static_cast<ClassType*>(type), id, arguments);
}

static ClassType* DeclareClass(const char *id) {
  assert(!symbols_->FindType(id));
  ClassType* c = types_->Make<ClassType>(id);
  symbols_->DefineType(id, c);
  return c;
}

static EnumType* DeclareEnum(const char *id) {
  assert(!symbols_->FindType(id));
  EnumType* e = types_->Make<EnumType>(id);
  symbols_->DefineType(id, e);
  return e;
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
  BeginBlock();
}

static ClassType* BeginClassTemplate(TypeList* templateArgs, const char* id) {
  ClassTemplate* t = types_->Make<ClassTemplate>(id, *templateArgs);
  symbols_->DefineType(id, t);
  t->SetDefined(true);
  BeginBlock();
  for (Type* const& i : *templateArgs) {
    auto type = static_cast<FormalTemplateArg*>(i);
    symbols_->DefineType(type->GetName(), type);
  }
  return t;
}

class ClassPopulator : public Visitor {
 public:
  ClassPopulator(ClassType* classType) : classType_(classType) {}

 private:
  Result Visit(Stmts* stmts) override {
    for (auto stmt : stmts->GetStmts()) stmt->Accept(this);
    return {};
  }

  Result Visit(Decls* decls) override {
    for (auto decl : decls->Get()) decl->Accept(this);
    return {};
  }

  Result Visit(VarDeclaration* v) override {
    classType_->AddField(v->GetID(), v->GetType(), v->GetInitExpr());
    return {};
  }

  Result Visit(MethodDecl* decl) override {
    classType_->AddMethod(decl->CreateMethod(classType_, types_));
    return {};
  }

  Result Default(ASTNode* node) override { return {}; }

  ClassType*    classType_;
};

static Stmt* EndClass(ClassType* classType, Stmts* body) {
  auto scope = symbols_->PopScope();
  ClassPopulator populator(classType);
  body->Accept(&populator);
  for (auto type : scope->GetTypes()) {
    classType->DefineType(type.first, type.second);
  }
  return Make<UnresolvedClassDefinition>(classType);
}

static void BeginEnum(Type* t) {
  currentEnumType_ = static_cast<EnumType*>(t);
}

static void EndEnum() {
  currentEnumType_ = nullptr;
}

static void AppendEnum(const char *id) {
  if (currentEnumType_) currentEnumType_->Append(id);
}

static void AppendEnum(const char *id, int value) {
  if (currentEnumType_) currentEnumType_->Append(id, value);
}

static void BeginBlock() {
  symbols_->PushScope(Make<Stmts>());
}

static void EndBlock(Stmts* stmts) {
  auto scope = symbols_->PopScope();
  for (auto type : scope->GetTypes()) stmts->DefineType(type.first, type.second);
}

MethodDecl* MakeMethodDecl(int modifiers, ArgList* optWorkgroupSize, std::string id, Stmts* formalArguments,
                    int thisQualifiers, Type* returnType, Expr* initializer, Stmts* body) {
  std::array<uint32_t, 3> workgroupSize;
  if (optWorkgroupSize) {
    auto args = optWorkgroupSize->GetArgs();
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
          workgroupSize[i] = static_cast<IntConstant*>(expr)->GetValue();
        }
      }
    }
  } else if (modifiers & Method::Modifier::Compute) {
    yyerrorf("compute shader requires a workgroup size");
  }
  return Make<MethodDecl>(modifiers, workgroupSize, id, formalArguments, thisQualifiers,
                          returnType, initializer, body);
}

MethodDecl* MakeConstructor(int modifiers, Type* type, Stmts* formalArguments, Expr* initializer,
                            Stmts* body) {
  if (!type->IsClass()) {
    yyerror("constructor must be of class type");
    return nullptr;
  }
  ClassType* classType = static_cast<ClassType*>(type);
  auto returnType = types_->GetRawPtrType(classType);
  return MakeMethodDecl(modifiers, nullptr, classType->GetName(), formalArguments, 0, returnType,
                        initializer, body);
}

MethodDecl* MakeDestructor(int modifiers, Type* type, Stmts* body) {
  if (!type->IsClass()) {
    yyerror("destructor must be of class type");
    return nullptr;
  }
  ClassType* classType = static_cast<ClassType*>(type);
  std::string name(std::string("~") + classType->GetName());
  return MakeMethodDecl(modifiers, nullptr, name.c_str(), nullptr, 0, types_->GetVoid(), nullptr,
                        body);
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

static TypeList* AddIDToTypeList(const char* id, TypeList* list) {
  if (!list) {
    list = Append(new TypeList());
  }
  list->push_back(types_->GetFormalTemplateArg(id));
  return list;
}

static ClassType* GetClassTemplateInstance(Type* type, const TypeList& templateArgs) {
  return types_->GetClassTemplateInstance(AsClassTemplate(type), templateArgs, &instanceQueue_);
}

static ClassType* PopInstanceQueue() {
  if (instanceQueue_.empty()) { return nullptr; }
  ClassType* instance = instanceQueue_.front();
  instanceQueue_.pop();
  return instance;
}

static void InstantiateClassTemplates() {
  while (ClassType* instance = PopInstanceQueue()) {
    ClassTemplate* classTemplate = instance->GetTemplate();
    TypeReplacementPass pass(nodes_, types_, classTemplate->GetFormalTemplateArgs(), instance->GetTemplateArgs(), &instanceQueue_);
    pass.ResolveClassInstance(classTemplate, instance);
    numSyntaxErrors += pass.NumErrors();
    rootStmts_->Append(Make<UnresolvedClassDefinition>(instance));
  }
}

int ParseProgram(const char* filename,
                 NodeVector* nodes,
                 TypeTable* types,
                 const std::vector<std::string>& includePaths,
                 Stmts* rootStmts) {
  numSyntaxErrors = 0;
  nodes_ = nodes;
  SymbolTable symbols;
  symbols_ = &symbols;
  types_ = types;
  includePaths_ = includePaths;
  rootStmts_ = rootStmts;
  PushFile(filename);
  symbols.PushScope(rootStmts);
  yyparse();
  symbols.PopScope();
  if (numSyntaxErrors == 0) {
    if (!rootStmts_->ContainsReturn()) {
      rootStmts_->Append(Make<ReturnStatement>(nullptr));
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

static EnumType* AsEnumType(Type* type) {
  if (type && !type->IsEnum()) {
    yyerrorf("type is already declared as non-enum");
    return nullptr;
  }
  return static_cast<EnumType*>(type);
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
