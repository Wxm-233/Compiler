%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include "ast.h"
#include "symtab.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  char char_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST>> *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN LAND LOR CONST IF ELSE WHILE BREAK CONTINUE VOID
%token <str_val> IDENT RELOP EQOP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block BlockItem Stmt OpenStmt ClosedStmt SimpleStmt
%type <ast_val> Exp UnaryExp PrimaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl VarDecl ConstDef VarDef GlobalDef
%type <ast_val> LVal InitVal ConstInitVal ConstExp
%type <ast_val> FuncFParam

%type <int_val> Number

%type <char_val> UnaryOp MulOp AddOp

%type <str_val> Type

%type <vec_val> DimList IndexList ConstDefList VarDefList ConstInitValList InitValList
%type <vec_val> GlobalDefList FuncFParamList FuncRParamList BlockItemList

%%

CompUnit
  : GlobalDefList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->global_def_list = $1;
    ast = move(comp_unit);
  }
  ;

GlobalDefList
  : {
    $$ = new std::vector<unique_ptr<BaseAST>>();
  }
  | GlobalDefList GlobalDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

GlobalDef
  : FuncDef {
    auto ast = new GlobalDefAST();
    ast->type = GlobalDefAST::FUNC_DEF;
    ast->func_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Decl {
    auto ast = new GlobalDefAST();
    ast->type = GlobalDefAST::DECL;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

FuncDef
  : Type IDENT '(' FuncFParamList ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list = $4;
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParamList
  : {
    $$ = new std::vector<unique_ptr<BaseAST>>();
  }
  | FuncFParam {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncFParamList ',' FuncFParam {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }

FuncFParam
  : Type IDENT {
    auto ast = new FuncFParamAST();
    ast->type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->is_array = false;
    $$ = ast;
  }
  | Type IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->is_array = true;
    $$ = ast;
  }
  | Type IDENT '[' ']' DimList {
    auto ast = new FuncFParamAST();
    ast->type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->is_array = true;
    ast->dim_list = $5;
    $$ = ast;
  }

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_item_list = $2;
    $$ = ast;
  }
  ;

BlockItemList
  : {
    $$ = new std::vector<unique_ptr<BaseAST>>();
  }
  | BlockItemList BlockItem {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->type = BlockItemAST::DECL;
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->type = BlockItemAST::STMT;
    $$ = ast;
  }
  ;

Stmt
  : OpenStmt {
    auto ast = new StmtAST();
    ast->open_stmt = unique_ptr<BaseAST>($1);
    ast->type = StmtAST::OPEN_STMT;
    $$ = ast;
  }
  | ClosedStmt {
    auto ast = new StmtAST();
    ast->closed_stmt = unique_ptr<BaseAST>($1);
    ast->type = StmtAST::CLOSED_STMT;
    $$ = ast;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new OpenStmtAST();
    ast->type = OpenStmtAST::IF;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto ast = new OpenStmtAST();
    ast->type = OpenStmtAST::IF_ELSE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->closed_stmt = unique_ptr<BaseAST>($5);
    ast->open_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto ast = new OpenStmtAST();
    ast->type = OpenStmtAST::WHILE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->open_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

ClosedStmt
  : SimpleStmt {
    auto ast = new ClosedStmtAST();
    ast->simple_stmt = unique_ptr<BaseAST>($1);
    ast->type = ClosedStmtAST::SIMPLE_STMT;
    $$ = ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto ast = new ClosedStmtAST();
    ast->type = ClosedStmtAST::IF_ELSE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->closed_stmt = unique_ptr<BaseAST>($5);
    ast->closed_stmt2 = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' ClosedStmt {
    auto ast = new ClosedStmtAST();
    ast->type = ClosedStmtAST::WHILE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->closed_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

SimpleStmt
  : RETURN Exp ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::RETURN;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::LVAL_ASSIGN;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::EMPTY_RETURN;
    $$ = ast;
  }
  | Block {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::BLOCK;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::EXP;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::CONTINUE;
    $$ = ast;
  }
  | ';' {
    auto ast = new SimpleStmtAST();
    ast->type = SimpleStmtAST::EMPTY;
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

DimList
  : '[' ConstExp ']' {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | DimList '[' ConstExp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

IndexList
  : '[' Exp ']' {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  | IndexList '[' Exp ']' {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->type = PrimaryExpAST::EXP;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->type = PrimaryExpAST::LVAL;
    $$ = ast;
  }
  |
  Number {
    auto ast = new PrimaryExpAST();
    ast->number = $1;
    ast->type = PrimaryExpAST::NUMBER;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

UnaryOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  | '!' {
    $$ = '!';
  }
  ;

AddOp
  : '+' {
    $$ = '+';
  }
  | '-' {
    $$ = '-';
  }
  ;

MulOp
  : '*' {
    $$ = '*';
  }
  | '/' {
    $$ = '/';
  }
  | '%' {
    $$ = '%';
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->type = UnaryExpAST::PRIMARY;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  |
  UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = UnaryExpAST::UNARY;
    ast->unaryop = $1;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' FuncRParamList ')' {
    auto ast = new UnaryExpAST();
    ast->type = UnaryExpAST::FUNC_CALL;
    ast->ident = *unique_ptr<string>($1);
    ast->func_r_param_list = $3;
    $$ = ast;
  }
  ;

FuncRParamList
  : {
    $$ = new std::vector<unique_ptr<BaseAST>>();
  }
  | Exp {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | FuncRParamList ',' Exp {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->type = MulExpAST::UNARY;
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  |
  MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->type = MulExpAST::MUL;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->op = $2;
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->type = AddExpAST::MUL;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  |
  AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->type = AddExpAST::ADD;
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->op = $2;
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type = RelExpAST::ADD;
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    ast->type = RelExpAST::REL;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<std::string>($2);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->type = EqExpAST::REL;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    ast->type = EqExpAST::EQ;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->op = *unique_ptr<std::string>($2);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->type = LAndExpAST::EQ;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    ast->type = LAndExpAST::LAND;
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->type = LOrExpAST::LAND;
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    ast->type = LOrExpAST::LOR;
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->type = DeclAST::CONSTDECL;
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->type = DeclAST::VARDECL;
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST Type ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast->btype = *$2;
    ast->const_def_list = $3;
    $$ = ast;
  }
  ;

VarDecl
  : Type VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->btype = *$1;
    ast->var_def_list = $2;
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstDefList ',' ConstDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDefList
  : VarDef {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | VarDefList ',' VarDef {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstInitValList
  : {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | ConstInitVal {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ConstInitValList ',' ConstInitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

InitValList
  : {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    $$ = vec;
  }
  | InitVal {
    auto vec = new std::vector<unique_ptr<BaseAST>>();
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | InitValList ',' InitVal {
    auto vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }

Type
  : INT {
    $$ = new std::string("int");
  }
  | VOID {
    $$ = new std::string("void");
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<std::string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT DimList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<std::string>($1);
    ast->dim_list = $2;
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->has_init_val = false;
    ast->ident = *unique_ptr<std::string>($1);
    $$ = ast;
  }
  | IDENT DimList {
    auto ast = new VarDefAST();
    ast->has_init_val = false;
    ast->ident = *unique_ptr<std::string>($1);
    ast->dim_list = $2;
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->has_init_val = true;
    ast->ident = *unique_ptr<std::string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT DimList '=' InitVal {
    auto ast = new VarDefAST();
    ast->has_init_val = true;
    ast->ident = *unique_ptr<std::string>($1);
    ast->dim_list = $2;
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->is_list = false;
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST();
    ast->is_list = true;
    ast->const_init_val_list = $2;
    $$ = ast;
  } 
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->is_list = false;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST();
    ast->is_list = true;
    ast->init_val_list = $2;
    $$ = ast;
  }

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<std::string>($1);
    $$ = ast;
  }
  | IDENT IndexList {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<std::string>($1);
    ast->index_list = $2;
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
