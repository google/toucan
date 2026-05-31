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
#include <unordered_set>

#include "ast/ast.h"

#include "parser/lexer.h"
#include "parser/parser.h"

using namespace Toucan;

static void PushFile(const char* filename);

static NodeVector* nodes_;
static ScopeStack scopeStack_;
static std::vector<std::string> includePaths_;
static Stmts* rootStmts_;
static std::unordered_set<std::string> includedFiles_;
static std::stack<FileLocation> fileStack_;
static std::unordered_set<ClassDecl*> definedClasses_;
#define yylex lex

static Expr* BinOp(BinOpNode::Op op, Expr* arg1, Expr* arg2);
static Expr* UnOp(UnaryOp::Op op, Expr* expr);
static Expr* IncDec(IncDecExpr::Op op, bool pre, Expr* expr);
static ASTClassType* DeclareClass(const char* id);
static ASTEnumType* DeclareEnum(const char* id);
static void DeclareUsing(const char* id, ASTType* type);
static void BeginClass(ASTType* type, ASTType* parent);
static ASTClassType* BeginClassTemplate(ASTFormalTemplateArgList* templateArgs, const char* id);
static ClassDecl* EndClass(Decls* body);
static EnumDecl* BeginEnum(ASTType* t, ASTEnumValues* values);
ASTClassTemplateInstance* MakeClassTemplateInstance(ASTType* type, ASTTypeList* typeList);
static MethodDecl* MakeMethodDecl(int modifiers, ArgList* optWorkgroupSize, std::string id,
                                  Stmts* formalArguments, int thisQualifiers, ASTType* returnType, 
                                  Expr* initializer, Stmts* body);
static MethodDecl* MakeConstructor(int modifiers, ASTType* type, Stmts* formalArguments,
                                   Expr* initializer, Stmts* body);
static MethodDecl* MakeDestructor(int modifiers, ASTType* type, Stmts* body);
static void BeginBlock();
static void EndBlock();
static Expr* Load(Expr* expr);
static Stmt* Store(Expr* expr, Expr* value);
static Expr* MakeNewExpr(UnresolvedInitializer* initializer, Expr* length = nullptr);
static Expr* InlineFile(const char* filename);
static Expr* StringLiteral(const char* str);

template <typename T, typename... ARGS> T* Make(ARGS&&... args) {
  T* node = nodes_->Make<T>(std::forward<ARGS>(args)...);
  node->SetFileLocation(fileStack_.top());
  return node;
}

ASTType* FindType(const char* str) {
  for (auto scope : scopeStack_) {
    if (auto type = scope->FindType(str)) return type;
  }
  return nullptr;
}

static void DefineType(std::string id, ASTType* type) {
  scopeStack_.Top()->DefineType(id, type);
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
    Toucan::ASTType*     type;
    Toucan::ASTClassType* classType;
    Toucan::ASTFormalTemplateArgList* formalTemplateArgList;
    Toucan::ASTTypeList* typeList;
    Toucan::ASTEnumValue* enumValue;
    Toucan::ASTEnumValues* enumValues;
};

%type <type> scalar_type type simple_type opt_parent_class class_header enum_header opt_return_type
%type <enumValue> enum_value
%type <classType> template_class_header
%type <expr> expr opt_expr assignable expr_or_list opt_initializer opt_length list_initializer
%type <initializer> initializer initializer_or_type
%type <arg> argument
%type <stmt> statement expr_statement var_decl_statement const_decl_statement for_loop_stmt
%type <stmt> assignment
%type <stmt> if_statement for_statement while_statement do_statement
%type <stmt> opt_else class_decl class_body_decl var_decl const_decl enum_decl
%type <stmt> class_forward_decl
%type <stmts> statements formal_arguments non_empty_formal_arguments method_body
%type <decls> var_decl_list const_decl_list class_body
%type <enumValues> enum_list
%type <stmts> block_statement
%type <argList> arguments non_empty_arguments opt_workgroup_size
%type <typeList> types
%type <formalTemplateArgList> template_formal_arguments
%type <i> type_qualifier type_qualifiers opt_type_qualifiers
%type <i> method_modifier method_modifiers
%token <identifier> T_IDENTIFIER T_STRING_LITERAL
%token <type> T_TYPENAME
%token <i> T_BYTE_LITERAL T_UBYTE_LITERAL T_SHORT_LITERAL T_USHORT_LITERAL
%token <i> T_INT_LITERAL T_UINT_LITERAL
%token <f> T_FLOAT_LITERAL
%token <d> T_DOUBLE_LITERAL
%token T_TRUE T_FALSE T_NULL T_IF T_ELSE T_FOR T_WHILE T_DO T_RETURN T_NEW
%token T_CLASS T_ENUM T_VAR T_CONST T_AS
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
%left T_AS
%right UNARYMINUS '!' T_PLUSPLUS T_MINUSMINUS T_DOTDOT ':' '@'
%left '.' '[' ']' '(' ')' '{' '}'
%expect 2   /* we expect 2 shift/reduce: dangling-else, A<B */
%%
program:
    statements                              { rootStmts_->Splice($1); }

statements:
    statements statement                    { if ($2) $1->Append($2); $$ = $1; }
  | /* nothing */                           { $$ = Make<Stmts>(); }
  ;

block_statement:
    '{' { BeginBlock(); } statements '}'    { EndBlock(); $$ = $3; }
  ;
statement:
    ';'                                     { $$ = 0; }
  | expr_statement ';'
  | block_statement                         { $$ = $1; }
  | if_statement
  | for_statement
  | while_statement 
  | do_statement
  | T_RETURN opt_expr ';'                   { $$ = Make<ReturnStatement>($2); }
  | var_decl_statement ';'                  { $$ = $1; }
  | const_decl_statement ';'                { $$ = $1; }
  | class_decl
  | class_forward_decl
  | enum_decl
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
        EndBlock();
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
  | simple_type T_LT types T_GT             { $$ = MakeClassTemplateInstance($1, $3); }
  | simple_type T_LT T_INT_LITERAL T_GT     { $$ = Make<ASTVectorType>($1, $3); }
  | simple_type T_LT T_INT_LITERAL ',' T_INT_LITERAL T_GT
                                            { auto columnType = Make<ASTVectorType>($1, $3);
                                              $$ = Make<ASTMatrixType>(columnType, $5); }
  | simple_type ':' T_IDENTIFIER            { $$ = Make<ASTScopedType>($1, $3); }
  ;

type:
    simple_type
  | type_qualifier type                     { $$ = Make<ASTQualifiedType>($2, $1); }
  | '*' type                                { $$ = Make<ASTStrongPtrType>($2); }
  | '^' type                                { $$ = Make<ASTWeakPtrType>($2); }
  | '&' type                                { $$ = Make<ASTRawPtrType>($2); }
  | '[' expr ']' type                       { $$ = Make<ASTArrayType>($4, $2); }
  | '[' ']' type                            { $$ = Make<ASTArrayType>($3, nullptr); }
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
  | T_CLASS T_TYPENAME                      { $$ = $2; }
  ;

template_class_header:
    T_CLASS T_IDENTIFIER T_LT template_formal_arguments T_GT
                                            { $$ = BeginClassTemplate($4, $2); }
  ;

class_forward_decl:
    class_header ';'                        { $$ = nullptr; }
  ;

class_decl:
    class_header opt_parent_class '{'           { BeginClass($1, $2); }
    class_body '}'                              { $$ = EndClass($5); }
  | template_class_header opt_parent_class  '{' { if ($2) $1->GetDecl()->SetParent($2); }
    class_body '}'                              { $$ = EndClass($5); }
  ;

opt_parent_class:
    ':' simple_type                         { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
  ;

class_body:
    class_body class_body_decl              { if ($2) $1->Append($2); $$ = $1; }
  | /* nothing */                           { $$ = Make<Decls>(); }
  ;

enum_header:
    T_ENUM T_IDENTIFIER                     { $$ = DeclareEnum($2); }
  | T_ENUM T_TYPENAME                       { $$ = $2; }
  ;

enum_decl:
    enum_header '{' enum_list '}'           { $$ = BeginEnum($1, $3); }
  ;

enum_value:
    T_IDENTIFIER                            { $$ = Make<ASTEnumValue>($1); }
  | T_IDENTIFIER '=' T_INT_LITERAL          { $$ = Make<ASTEnumValue>($1, $3); }
  ;

enum_list:
    enum_list ',' enum_value                { $$ = $1; $$->Append($3); }
  | enum_value                              { $$ = Make<ASTEnumValues>(); $$->Append($1); }
  | /* nothing */                           { $$ = Make<ASTEnumValues>(); }
  ;

using_decl:
    T_USING T_IDENTIFIER '=' type ';'       { DeclareUsing($2, $4); }
  ;

opt_return_type:
    ':' type                                { $$ = $2; }
  | /* NOTHING */                           { $$ = Make<ASTVoidType>(); }
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
    T_IDENTIFIER                            { $$ = Make<ASTFormalTemplateArgList>();
                                              $$->Append(Make<ASTFormalTemplateArg>($1)); }
  | template_formal_arguments ',' T_IDENTIFIER
                                            { $$ = $1; $$->Append(Make<ASTFormalTemplateArg>($3)); }
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
  | /* nothing */                           { $$ = Make<Stmts>(); }
  ;

non_empty_formal_arguments:
    formal_arguments ',' var_decl           { $1->Append($3); $$ = $1; }
  | var_decl                                { $$ = Make<Stmts>(); $$->Append($1); }
  ;

var_decl:
    T_IDENTIFIER ':' type                   { $$ = Make<VarDeclaration>($1, $3, nullptr); }
  | T_IDENTIFIER '=' expr_or_list           { $$ = Make<VarDeclaration>($1, Make<ASTAutoType>(), $3); }
  | T_IDENTIFIER ':' type '=' expr_or_list  { $$ = Make<VarDeclaration>($1, $3, $5); }
  ;

const_decl:
    T_IDENTIFIER '=' expr_or_list           { $$ = Make<ConstDecl>($1, $3); }
  ;

scalar_type:
    T_INT           { $$ = Make<ASTIntegerType>(32, true); }
  | T_UINT          { $$ = Make<ASTIntegerType>(32, false); }
  | T_SHORT         { $$ = Make<ASTIntegerType>(16, true); }
  | T_USHORT        { $$ = Make<ASTIntegerType>(16, false); }
  | T_BYTE          { $$ = Make<ASTIntegerType>(8, true); }
  | T_UBYTE         { $$ = Make<ASTIntegerType>(8, false); }
  | T_FLOAT         { $$ = Make<ASTFloatingPointType>(32); }
  | T_DOUBLE        { $$ = Make<ASTFloatingPointType>(64); }
  | T_BOOL          { $$ = Make<ASTBoolType>(); }
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

expr:
    expr '+' expr                           { $$ = BinOp(BinOpNode::ADD, $1, $3); }
  | expr '-' expr                           { $$ = BinOp(BinOpNode::SUB, $1, $3); }
  | expr '*' expr                           { $$ = BinOp(BinOpNode::MUL, $1, $3); }
  | expr '/' expr                           { $$ = BinOp(BinOpNode::DIV, $1, $3); }
  | expr '%' expr                           { $$ = BinOp(BinOpNode::MOD, $1, $3); }
  | '-' expr %prec UNARYMINUS               { $$ = UnOp(UnaryOp::Op::Minus, $2); }
  | expr T_LT expr                          { $$ = BinOp(BinOpNode::LT, $1, $3); }
  | expr T_LE expr                          { $$ = BinOp(BinOpNode::LE, $1, $3); }
  | expr T_EQ expr                          { $$ = BinOp(BinOpNode::EQ, $1, $3); }
  | expr T_GT expr                          { $$ = BinOp(BinOpNode::GT, $1, $3); }
  | expr T_GE expr                          { $$ = BinOp(BinOpNode::GE, $1, $3); }
  | expr T_NE expr                          { $$ = BinOp(BinOpNode::NE, $1, $3); }
  | expr T_LOGICAL_AND expr                 { $$ = BinOp(BinOpNode::LOGICAL_AND, $1, $3); }
  | expr T_LOGICAL_OR expr                  { $$ = BinOp(BinOpNode::LOGICAL_OR, $1, $3); }
  | expr '&' expr                           { $$ = BinOp(BinOpNode::BITWISE_AND, $1, $3); }
  | expr '^' expr                           { $$ = BinOp(BinOpNode::BITWISE_XOR, $1, $3); }
  | expr '|' expr                           { $$ = BinOp(BinOpNode::BITWISE_OR, $1, $3); }
  | '!' expr                                { $$ = UnOp(UnaryOp::Op::Negate, $2); }
  | T_PLUSPLUS assignable                   { $$ = IncDec(IncDecExpr::Op::Inc, true, $2); }
  | T_MINUSMINUS assignable                 { $$ = IncDec(IncDecExpr::Op::Dec, true, $2); }
  | assignable T_PLUSPLUS                   { $$ = IncDec(IncDecExpr::Op::Inc, false, $1); }
  | assignable T_MINUSMINUS                 { $$ = IncDec(IncDecExpr::Op::Dec, false, $1); }
  | '(' expr ')'                            { $$ = $2; }
  | expr T_AS type                          { $$ = Make<UnresolvedCastExpr>($3, $1); }
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
  | opt_length T_NEW initializer_or_type    { $$ = MakeNewExpr($3, $1); }
  | T_INLINE '(' T_STRING_LITERAL ')'       { $$ = InlineFile($3); }
  | T_STRING_LITERAL                        { $$ = StringLiteral($1); }
  ;

opt_length:
    '[' expr ']'                            { $$ = $2; }
  | /* nothing */                           { $$ = nullptr; }
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
    type                                    { $$ = Make<ASTTypeList>(); $$->Append($1);  }
  | types ',' type                          { $1->Append($3); $$ = $1; }
  ;

assignable:
    T_IDENTIFIER                            { $$ = Make<UnresolvedIdentifier>($1); }
  | T_THIS                                  { $$ = Make<UnresolvedIdentifier>("this"); }
  | assignable '[' expr ']'                 { $$ = Make<ArrayAccess>($1, $3); }
  | assignable '[' opt_expr T_DOTDOT opt_expr ']'
                                            { $$ = Make<SliceExpr>($1, $3, $5); }
  | assignable '.' T_IDENTIFIER             { $$ = Make<UnresolvedDot>($1, $3); }
  | simple_type '.' T_IDENTIFIER            { $$ = Make<UnresolvedStaticDot>($1, $3); }
  | assignable '.' T_IDENTIFIER '(' arguments ')'
                                            { $$ = Make<UnresolvedMethodCall>($1, $3, $5); }
  | simple_type '.' T_IDENTIFIER '(' arguments ')'
                                            { $$ = Make<UnresolvedStaticMethodCall>($1, $3, $5); }
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

void yyerrorf(const char *fmt, ...) {
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
  return Make<Data>(std::move(buffer), size);
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
  return Make<Data>(std::move(buffer), length);
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

static ASTClassType* DeclareClass(const char *id) {
  assert(!FindType(id));
  auto classDecl = Make<ClassDecl>(id);
  auto result = Make<ASTClassType>(classDecl);
  DefineType(id, Make<ASTClassType>(classDecl));
  return result;
}

static ASTEnumType* DeclareEnum(const char *id) {
  assert(!FindType(id));
  auto decl = Make<EnumDecl>(id);
  auto enumType = Make<ASTEnumType>(decl);
  DefineType(id, enumType);
  return enumType;
}

static void DeclareUsing(const char *id, ASTType* type) {
  assert(!FindType(id));
  DefineType(id, type);
}

static void BeginClass(ASTType* t, ASTType* parent) {
  if (!t->IsClass()) {
    yyerrorf("type is already declared as non-class");
    return;
  }
  auto decl = static_cast<ASTClassType*>(t)->GetDecl();
  scopeStack_.Push(decl);
  if (definedClasses_.contains(decl)) {
    yyerrorf("class \"%s\" already has a definition", decl->GetName().c_str());
    return;
  }
  definedClasses_.insert(decl);
  if (parent) decl->SetParent(parent);
}

static ASTClassType* BeginClassTemplate(ASTFormalTemplateArgList* templateArgs, const char* id) {
  auto decl = Make<ClassTemplateDecl>(id, templateArgs);
  auto result = Make<ASTClassType>(decl);
  DefineType(id, result);
  scopeStack_.Push(decl);
  for (auto arg : templateArgs->Get()) {
    DefineType(arg->GetName(), arg);
  }
  return result;
}

static ClassDecl* EndClass(Decls* body) {
  auto node = static_cast<ClassDecl*>(scopeStack_.Pop());
  node->SetBody(body);
  return node;
}

static EnumDecl* BeginEnum(ASTType* t, ASTEnumValues* values) {
  if (!t->IsEnum()) {
    yyerrorf("type is already declared as non-enum");
    return nullptr;
  }
  ASTEnumType* enumType = static_cast<ASTEnumType*>(t);
  auto decl = enumType->GetDecl();
  decl->SetValues(values);
  return decl;
}

static void BeginBlock() {
  scopeStack_.Push(Make<Stmts>());
}

static void EndBlock() {
  scopeStack_.Pop();
}

ASTClassTemplateInstance* MakeClassTemplateInstance(ASTType* type, ASTTypeList* typeList) {
  if (!type->IsClass()) {
    yyerror("base type is not a class template");
    return nullptr;
  }
  auto decl = static_cast<ASTClassType*>(type)->GetDecl();
  assert(decl->IsTemplate());

  return Make<ASTClassTemplateInstance>(static_cast<ClassTemplateDecl*>(decl), typeList);
}

static MethodDecl* MakeMethodDecl(int modifiers, ArgList* optWorkgroupSize, std::string id,
                                  Stmts* formalArguments, int thisQualifiers, ASTType* returnType,
                                  Expr* initializer, Stmts* body) {
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

static MethodDecl* MakeConstructor(int modifiers, ASTType* type, Stmts* formalArguments,
                                   Expr* initializer, Stmts* body) {
  if (!type->IsClass()) {
    yyerror("constructor must be of class type");
    return nullptr;
  }
  auto classType = static_cast<ASTClassType*>(type);
  if (classType->GetDecl()->IsTemplate()) {
    auto decl = static_cast<ClassTemplateDecl*>(classType->GetDecl());
    auto templateArgs = Make<ASTTypeList>();
    for (auto arg : decl->GetFormalTemplateArgs()->Get()) {
      templateArgs->Append(arg);
    }
    type = Make<ASTClassTemplateInstance>(decl, templateArgs);
  }
  auto returnType = Make<ASTRawPtrType>(type);
  auto name = classType->GetDecl()->GetName();
  return MakeMethodDecl(modifiers, nullptr, name, formalArguments, 0, returnType,
                        initializer, body);
}

static MethodDecl* MakeDestructor(int modifiers, ASTType* type, Stmts* body) {
  std::string name;
  if (!type->IsClass()) {
    yyerror("destructor must be of class type");
    return nullptr;
  }
  auto classType = static_cast<ASTClassType*>(type);
  name = std::string("~") + classType->GetDecl()->GetName();
  auto formalArguments = Make<Stmts>();
  return MakeMethodDecl(modifiers, nullptr, name.c_str(), formalArguments, 0, Make<ASTVoidType>(),
                        nullptr, body);
}

int ParseProgram(const char* filename,
                 NodeVector* nodes,
                 const std::vector<std::string>& includePaths,
                 Stmts* rootStmts) {
  numSyntaxErrors = 0;
  nodes_ = nodes;
  includePaths_ = includePaths;
  rootStmts_ = rootStmts;
  PushFile(filename);
  scopeStack_.Push(rootStmts);
  yyparse();
  scopeStack_.Pop();
  if (numSyntaxErrors == 0) {
    if (!rootStmts_->ContainsReturn()) {
      rootStmts_->Append(Make<ReturnStatement>(nullptr));
    }
  }
  PopFile();
  nodes_ = nullptr;
  rootStmts_ = nullptr;
  lex_destroy();
  return numSyntaxErrors;
}
