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
%token INT RETURN LAND LOR CONST
%token <str_val> IDENT RELOP EQOP
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block BlockItem Stmt 
%type <ast_val> Exp UnaryExp PrimaryExp MulExp AddExp RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl VarDecl ConstDef VarDef
%type <ast_val> LVal InitVal

%type <int_val> Number ConstInitVal ConstExp

%type <char_val> UnaryOp MulOp AddOp

%type <str_val> BType

%type <vec_val> ConstDefList VarDefList BlockItemList

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = "int";
    $$ = ast;
  }
  ;

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
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::RETURN;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::LVAL_ASSIGN;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto exp = dynamic_cast<ExpAST*>($1);
    assert(exp->type == ExpAST::NUMBER);
    $$ = exp->number;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    auto lor_exp = dynamic_cast<LOrExpAST*>($1);
    if (lor_exp->type == LOrExpAST::NUMBER) {
      ast->type = ExpAST::NUMBER;
      ast->number = lor_exp->number;
    } else {
      ast->type = ExpAST::LOR;
      ast->lor_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    auto exp = dynamic_cast<ExpAST*>($2);
    if (exp->type == ExpAST::NUMBER) {
      ast->type = PrimaryExpAST::NUMBER;
      ast->number = exp->number;
    } else {
      ast->type = PrimaryExpAST::EXP;
      ast->exp = unique_ptr<BaseAST>($2);
    }
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    auto lval = dynamic_cast<LValAST*>($1);
    auto q = Symbol::query(lval->ident);
    if (q.first == Symbol::TYPE_CONST) {
      ast->number = *(long*)&q.second;
      ast->type = PrimaryExpAST::NUMBER;
    } else {
      ast->lval = unique_ptr<BaseAST>($1);
      ast->type = PrimaryExpAST::LVAL;
    }
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
    auto primary_exp = dynamic_cast<PrimaryExpAST*>($1);
    if (primary_exp->type == PrimaryExpAST::NUMBER) {
      ast->type = UnaryExpAST::NUMBER;
      ast->number = primary_exp->number;
    } else {
      ast->type = UnaryExpAST::PRIMARY;
      ast->primary_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  |
  UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    auto unary_exp = dynamic_cast<UnaryExpAST*>($2);
    if (unary_exp->type == UnaryExpAST::NUMBER) {
      ast->type = UnaryExpAST::NUMBER;
      int num = unary_exp->number;
      switch ($1) {
        case '+':
          ast->number = num;
          break;
        case '-':
          ast->number = -num;
          break;
        case '!':
          ast->number = !num;
          break;
      }
      ast->number = num;
    } else {
      ast->type = UnaryExpAST::UNARY;
      ast->unaryop = $1;
      ast->unary_exp = unique_ptr<BaseAST>($2);
    }
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    auto unary_exp = dynamic_cast<UnaryExpAST*>($1);
    if (unary_exp->type == UnaryExpAST::NUMBER) {
      ast->type = MulExpAST::NUMBER;
      ast->number = unary_exp->number;
    } else {
      ast->type = MulExpAST::UNARY;
      ast->unary_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  |
  MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    auto mul_exp = dynamic_cast<MulExpAST*>($1);
    auto unary_exp = dynamic_cast<UnaryExpAST*>($3);
    if (mul_exp->type == MulExpAST::NUMBER && unary_exp->type == UnaryExpAST::NUMBER) {
      ast->type = MulExpAST::NUMBER;
      int num1 = mul_exp->number;
      int num2 = unary_exp->number;
      switch ($2) {
        case '*':
          ast->number = num1 * num2;
          break;
        case '/':
          ast->number = num1 / num2;
          break;
        case '%':
          ast->number = num1 % num2;
          break;
      }
    } else {
      ast->type = MulExpAST::MUL;
      ast->mul_exp = unique_ptr<BaseAST>($1);
      ast->op = $2;
      ast->unary_exp = unique_ptr<BaseAST>($3);
    }
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    auto mul_exp = dynamic_cast<MulExpAST*>($1);
    if (mul_exp->type == MulExpAST::NUMBER) {
      ast->type = AddExpAST::NUMBER;
      ast->number = mul_exp->number;
    } else {
      ast->type = AddExpAST::MUL;
      ast->mul_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  |
  AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    auto add_exp = dynamic_cast<AddExpAST*>($1);
    auto mul_exp = dynamic_cast<MulExpAST*>($3);
    if (add_exp->type == AddExpAST::NUMBER && mul_exp->type == MulExpAST::NUMBER) {
      ast->type = AddExpAST::NUMBER;
      int num1 = add_exp->number;
      int num2 = mul_exp->number;
      switch ($2) {
        case '+':
          ast->number = num1 + num2;
          break;
        case '-':
          ast->number = num1 - num2;
          break;
      }
    } else {
      ast->type = AddExpAST::ADD;
      ast->add_exp = unique_ptr<BaseAST>($1);
      ast->op = $2;
      ast->mul_exp = unique_ptr<BaseAST>($3);
    }

    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    auto add_exp = dynamic_cast<AddExpAST*>($1);
    if (add_exp->type == AddExpAST::NUMBER) {
      ast->type = RelExpAST::NUMBER;
      ast->number = add_exp->number;
    } else {
      ast->type = RelExpAST::ADD;
      ast->add_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  | RelExp RELOP AddExp {
    auto ast = new RelExpAST();
    auto rel_exp = dynamic_cast<RelExpAST*>($1);
    std::string op = *$2;
    auto add_exp = dynamic_cast<AddExpAST*>($3);
    if (rel_exp->type == RelExpAST::NUMBER && add_exp->type == AddExpAST::NUMBER) {
      ast->type = RelExpAST::NUMBER;
      int num1 = rel_exp->number;
      int num2 = add_exp->number;
      if (op == "<") {
        ast->number = num1 < num2;
      } else if (op == "<=") {
        ast->number = num1 <= num2;
      } else if (op == ">") {
        ast->number = num1 > num2;
      } else if (op == ">=") {
        ast->number = num1 >= num2;
      }
    } else {
      ast->type = RelExpAST::REL;
      ast->rel_exp = unique_ptr<BaseAST>($1);
      ast->op = op;
      ast->add_exp = unique_ptr<BaseAST>($3);
    }
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    auto rel_exp = dynamic_cast<RelExpAST*>($1);
    if (rel_exp->type == RelExpAST::NUMBER) {
      ast->type = EqExpAST::NUMBER;
      ast->number = rel_exp->number;
    } else {
      ast->type = EqExpAST::REL;
      ast->rel_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  | EqExp EQOP RelExp {
    auto ast = new EqExpAST();
    auto eq_exp = dynamic_cast<EqExpAST*>($1);
    std::string op = *$2;
    auto rel_exp = dynamic_cast<RelExpAST*>($3);
    if (eq_exp->type == EqExpAST::NUMBER && rel_exp->type == RelExpAST::NUMBER) {
      ast->type = EqExpAST::NUMBER;
      int num1 = eq_exp->number;
      int num2 = rel_exp->number;
      if (op == "==") {
        ast->number = num1 == num2;
      } else if (op == "!=") {
        ast->number = num1 != num2;
      }
    } else {
      ast->type = EqExpAST::EQ;
      ast->eq_exp = unique_ptr<BaseAST>($1);
      ast->op = op;
      ast->rel_exp = unique_ptr<BaseAST>($3);
    }
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    auto eq_exp = dynamic_cast<EqExpAST*>($1);
    if (eq_exp->type == EqExpAST::NUMBER) {
      ast->type = LAndExpAST::NUMBER;
      ast->number = eq_exp->number;
    } else {
      ast->type = LAndExpAST::EQ;
      ast->eq_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    auto land_exp = dynamic_cast<LAndExpAST*>($1);
    auto eq_exp = dynamic_cast<EqExpAST*>($3);
    if (land_exp->type == LAndExpAST::NUMBER && eq_exp->type == EqExpAST::NUMBER) {
      ast->type = LAndExpAST::NUMBER;
      int num1 = land_exp->number;
      int num2 = eq_exp->number;
      ast->number = num1 && num2;
    } else {
      ast->type = LAndExpAST::LAND;
      ast->land_exp = unique_ptr<BaseAST>($1);
      ast->eq_exp = unique_ptr<BaseAST>($3);
    }
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    auto land_exp = dynamic_cast<LAndExpAST*>($1);
    if (land_exp->type == LAndExpAST::NUMBER) {
      ast->type = LOrExpAST::NUMBER;
      ast->number = land_exp->number;
    } else {
      ast->type = LOrExpAST::LAND;
      ast->land_exp = unique_ptr<BaseAST>($1);
    }
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    auto lor_exp = dynamic_cast<LOrExpAST*>($1);
    auto land_exp = dynamic_cast<LAndExpAST*>($3);
    if (lor_exp->type == LOrExpAST::NUMBER && land_exp->type == LAndExpAST::NUMBER) {
      ast->type = LOrExpAST::NUMBER;
      int num1 = lor_exp->number;
      int num2 = land_exp->number;
      ast->number = num1 || num2;
    } else {
      ast->type = LOrExpAST::LOR;
      ast->lor_exp = unique_ptr<BaseAST>($1);
      ast->land_exp = unique_ptr<BaseAST>($3);
    }
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
  : CONST BType ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast->btype = *$2;
    ast->const_def_list = $3;
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefList ';' {
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

BType
  : INT {
    $$ = new std::string("int");
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    auto ident = *$1;
    auto const_init_val = $3;
    Symbol::insert(ident, Symbol::TYPE_CONST, (void*)const_init_val);
    ast->ident = *unique_ptr<std::string>($1);
    ast->const_init_val = $3;
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
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->has_init_val = true;
    ast->ident = *unique_ptr<std::string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    $$ = $1;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    auto exp = dynamic_cast<ExpAST*>($1);
    if (exp->type == ExpAST::NUMBER) {
      ast->number = exp->number;
      ast->type = InitValAST::NUMBER;
    } else {
      ast->exp = unique_ptr<BaseAST>($1);
      ast->type = InitValAST::EXP;
    }
    $$ = ast;
  }

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<std::string>($1);
    $$ = ast;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
