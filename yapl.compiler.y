%{
#include <stdio.h>
#include <stdlib.h>
#include "utils.compiler.h"
#include "generator.h"

extern int yylex();
extern int yyparse();
void yyerror(char *msg);

ast_node *root;
stack_node *symb_table = NULL;

ast_node *h_nMeth(ast_node *ret_type, char meth_name[81], ast_node *params, ast_node *body);
ast_node *h_nParams(ast_node *rest, ast_node *param_type, char param_name[81]);
ast_node *h_nDecl(ast_node *var_type, char var_name[81], ast_node *expr, ast_node *rest);
ast_node *h_nVars(char var_name[81], ast_node *expr, ast_node *rest);
ast_node *h_nLocation(char var_name[81]);
ast_node *h_nMethod(char meth_name[81]);
void semcheck_break_stmt(ast_node* stmt);
void semcheck_break(ast_node *stmts);
ast_node *h_nBody(ast_node *decls, ast_node *stmts);
ast_node *h_nBlock(ast_node *stmts);
ast_node *h_nProgram(ast_node *meth_list);

int count_args(ast_node *args) {
    ast_node *current = args;
    
    int count = 0;
    while (current != NULL && current->children[1] != NULL) {
        count++;
        current = current->children[0]; // next arg
    }

    return count;
}

ast_node *h_nFactor_method_call(ast_node *method, ast_node *args) {
    int ma = stack_node_count_method_args(symb_table, method->symb->name);
    int ga = count_args(args);

    if (ma != ga) {
        if (ga > ma)
            printf("ERROR: Too many arguments for method '%s'\n", method->symb->name);
        else 
            printf("ERROR: Too few arguments for method '%s'\n", method->symb->name);
        exit(1);
    }

    // TODO type checking

    return ast_node_new_ch2(EL_METH_EX, method, args);
}

ast_node *h_nAssign(ast_node *var, ast_node *val) {
    var->symb->undefined = FALSE;
    return ast_node_new_ch2(EL_ASSIGN, var, val);
}

void semcheck_undefined_expr(ast_node *expr);

ast_node *h_nExpr_2(int element_type, ast_node *addExpr1, ast_node *addExpr2) {
    semcheck_undefined_expr(addExpr1);
    semcheck_undefined_expr(addExpr2);

    return ast_node_new_ch2(element_type, addExpr1, addExpr2);
}

ast_node *h_nExpr_1(ast_node *expr) {
    semcheck_undefined_expr(expr);

    return expr;
}

int semcheck_return_stmt(ast_node* stmt);

int semcheck_return(ast_node *stmts);

%}

%union {
    int  yint;
    char ystr[81];
    ast_node *ynode;
}

%token <yint> T_INT T_RETURN T_IF T_ELSE T_WHILE T_BREAK T_TRUE T_FALSE
%token <yint> T_NUM
%token <yint> '(' ')' '{' '}' ',' ';' '=' 
%left  <yint> '+' '-'
%left  <yint> '*' '/'
%token <yint> T_LEQT T_LT T_GT T_GEQT T_EQEQ T_NEQ
%token <ystr> T_ID
%right UMINUS

%type <ynode> nProgram nMethList nMeth nType nParams nBody nFormals 
%type <ynode> nDecls nStmts nDeclList nDecl nVars nExpr nStmt nAssign 
%type <ynode> nBlock nLocation nMethod nAddExpr nTerm  
%type <ynode> nFactor nActuals nArgs 
%type <yint>  nRelop nAddop nMulop

%%

nProgram    : nMethList                             { root = h_nProgram($1); }
            |                                       { root = NULL; }
            ;

nMethList   : nMeth nMethList                       { $$ = ast_node_new_ch2(EL_METH_LS, $1, $2); }
            | nMeth                                 { $$ = $1; }
            ;

nMeth       : nType T_ID '(' nParams ')' nBody      { $$ = h_nMeth($1, $2, $4, $6); }
            ;

nParams     : nFormals nType T_ID                   { $$ = h_nParams($1, $2, $3); }
            |                                       { $$ = NULL; }
            ;

nFormals    : nFormals nType T_ID ','               { $$ = h_nParams($1, $2, $3); }
            |                                       { $$ = NULL; }
            ;

nType       : T_INT                                 { $$ = ast_node_new_symbol(symbol_new_type(SYM_TYPE_INT)); }
            ;

nBody       : '{' nDecls nStmts '}'                 { $$ = h_nBody($2, $3); }
            ;

nDecls      : nDeclList nDecl                       { $$ = ast_node_new_ch2(EL_DECL_LS, $1, $2); }
            |                                       { $$ = NULL; }
            ;

nDeclList   : nDeclList nDecl                       { $$ = ast_node_new_ch2(EL_DECL_LS, $1, $2); }
            |                                       { $$ = NULL; }
            ;

nDecl       : nType T_ID nVars ';'                  { $$ = h_nDecl($1, $2, NULL, $3); }
            | nType T_ID '=' nExpr nVars ';'        { $$ = h_nDecl($1, $2, $4, $5); }
            ;

nVars       : ',' T_ID nVars                        { $$ = h_nVars($2, NULL, $3); }
            | ',' T_ID '=' nExpr nVars              { $$ = h_nVars($2, $4, $5); }
            |                                       { $$ = NULL; }
            ;

nStmts      : nStmts nStmt                          { $$ = ast_node_new_ch2(EL_STMT, $1, $2); }
            |                                       { $$ = NULL; }
            ;

nStmt       : nAssign ';'                           { $$ = $1; }
            | T_RETURN nExpr ';'                    { $$ = ast_node_new_ch1(EL_RETURN, $2); }
            | T_IF '(' nExpr ')' nStmt T_ELSE nStmt { $$ = ast_node_new_ch3(EL_IF, $3, $5, $7); }
            | T_WHILE '(' nExpr ')' nStmt           { $$ = ast_node_new_ch2(EL_WHILE, $3, $5); }
            | T_BREAK ';'                           { $$ = ast_node_new_ch0(EL_BREAK); }
            | nBlock                                { $$ = $1; }
            | ';'                                   { $$ = NULL; }
            ;

nBlock      : '{' nStmts '}'                        { $$ = h_nBlock($2); } // TODO scope
            ;

nAssign     : nLocation '=' nExpr                   { $$ = h_nAssign($1, $3); }
            ;

nLocation   : T_ID                                  { $$ = h_nLocation($1); }
            ;

nMethod     : T_ID                                  { $$ = h_nMethod($1); }
            ;

nExpr       : nAddExpr nRelop nAddExpr              { $$ = h_nExpr_2($2, $1, $3); }
            | nAddExpr                              { $$ = h_nExpr_1($1); }
            ;

nRelop      : T_LEQT                                { $$ = EL_LEQT; }
            | T_LT                                  { $$ = EL_LT; }
            | T_GT                                  { $$ = EL_GT; }
            | T_GEQT                                { $$ = EL_GEQT; }
            | T_EQEQ                                { $$ = EL_EQEQ; }
            | T_NEQ                                 { $$ = EL_NEQ; }
            ;

nAddExpr    : nAddExpr nAddop nTerm                 { $$ = ast_node_new_ch2($2, $1, $3); }
            | nTerm                                 { $$ = $1; }
            ;

nAddop      : '+'                                   { $$ = EL_ADD; }
            | '-'                                   { $$ = EL_SUB; }
            ;

nTerm       : nTerm nMulop nFactor                  { $$ = ast_node_new_ch2($2, $1, $3); }       
            | nFactor                               { $$ = $1; }
            ;

nMulop      : '*'                                   { $$ = EL_MULT; }
            | '/'                                   { $$ = EL_DIV; }
            ;

nFactor     : '(' nExpr ')'                         { $$ = $2; }
            | nLocation                             { $$ = $1; }
            | T_NUM                                 { $$ = ast_node_new_symbol(symbol_new_integer($1)); }
            | '-' T_NUM                             { $$ = ast_node_new_symbol(symbol_new_integer(-$2)); }
            | T_TRUE                                { $$ = ast_node_new_symbol(symbol_new_integer(1)); }
            | T_FALSE                               { $$ = ast_node_new_symbol(symbol_new_integer(0)); }
            | nMethod '(' nActuals ')'              { $$ = h_nFactor_method_call($1, $3); } // TODO
            ;

nActuals    : nArgs nExpr                           { $$ = ast_node_new_ch2(EL_ARGS, $1, $2); }
            |                                       { $$ = NULL; }
            ;

nArgs       : nArgs nExpr ','                       { $$ = ast_node_new_ch2(EL_ARGS, $1, $2); }
            |                                       { $$ = NULL; }
            ;

%%

int semcheck_return_stmt(ast_node* stmt) {
    // wrap a single statement to a fake list of statements
    ast_node *stmts = ast_node_new_ch2(EL_STMT, NULL, stmt);
    int ret = semcheck_return(stmts);
    free(stmts);
    
    return ret;
}

int semcheck_return(ast_node *stmts) {
    ast_node *current = stmts;
    
    while(current != NULL) {
        ast_node *stmt = current->children[1];
        current = current->children[0];

        if (stmt == NULL)
            continue;

        if (stmt->element_type == EL_WHILE) // TODO we maybe dont care if there's a break inside
            if (semcheck_return(stmt->children[1]))
                return TRUE;

        if (stmt->element_type == EL_BLOCK)
            if (semcheck_return(stmt->children[0]))
                return TRUE;

        if (stmt->element_type == EL_IF) {
            if (semcheck_return_stmt(stmt->children[1]))
                return TRUE;

            if (semcheck_return_stmt(stmt->children[2]))
                return TRUE;
        }

        if (stmt->element_type == EL_RETURN)
            return TRUE;
    }

    return FALSE;
}

void semcheck_undefined_expr(ast_node *expr) {
    if (expr == NULL)
        return;

    int et = expr->element_type;
    // todo, fix this
    if (et == EL_LEQT || et == EL_LT || et == EL_GT || et == EL_GEQT || et == EL_EQEQ || et == EL_NEQ || et == EL_ADD || et == EL_SUB || et == EL_MULT || et == EL_DIV) {
        semcheck_undefined_expr(expr->children[0]);
        semcheck_undefined_expr(expr->children[1]);
        return;
    }

    if (expr->is_symbol && expr->symb->type == SYM_VARIABLE && expr->symb->undefined) {
        printf("ERROR: Variable '%s' has no value\n", expr->symb->name);
        exit(1);
    }
}

ast_node *h_nMeth(ast_node *ret_type, char meth_name[81], ast_node *params, ast_node *body) {
    symbol *symb_meth = symbol_new_method_wtret(meth_name, ret_type->symb->type);

    // todo check if symbol exists
    if (stack_node_find(symb_table, meth_name, FALSE) != NULL) {
        printf("ERROR: There is already a symbol with the same name '%s'\n", meth_name);
        exit(1);
    }

    stack_node_remove_lvl1(&symb_table);

    stack_node_push(&symb_table, symb_meth, 0);

    return ast_node_new_ch4(EL_METH, ret_type, ast_node_new_symbol(symb_meth), params, body);
}

ast_node *h_nParams(ast_node *rest, ast_node *param_type, char param_name[81]) {
    symbol *symb_param = symbol_new_variable_wtvar(param_name, param_type->symb->type);
    symb_param->undefined = FALSE; // TODO quickfix

    // todo check if symbol exists
    if (stack_node_find(symb_table, param_name, FALSE) != NULL) {
        printf("ERROR: There is already a symbol with the same name '%s'\n", param_name);
        exit(1);
    }

    stack_node_push(&symb_table, symb_param, 0);

    return ast_node_new_ch3(EL_PARAMS, rest, param_type, ast_node_new_symbol(symb_param));
}

ast_node *h_nDecl(ast_node *var_type, char var_name[81], ast_node *expr, ast_node *rest) {
    symbol *symb_var = symbol_new_variable_wtvar(var_name, var_type->symb->type);

    // todo check if symbol exists
    if (stack_node_find(symb_table, var_name, FALSE) != NULL) {
        printf("ERROR: There is already a symbol with the same name '%s'\n", var_name);
        exit(1);
    }

    if (expr != NULL)
        symb_var->undefined = FALSE;

    stack_node_push(&symb_table, symb_var, 1);
    
    // set type to the rest of the variables
    if (rest != NULL) {
        ast_node *tmp = rest;
        do {
            tmp->children[0]->symb->var_type = var_type->symb->type;
            tmp = tmp->children[2];
        } while(tmp != NULL);
    }

    return ast_node_new_ch4(EL_DECL_ST, var_type, ast_node_new_symbol(symb_var), expr, rest);
}

ast_node *h_nVars(char var_name[81], ast_node *expr, ast_node *rest) {
    symbol *symb_var = symbol_new_variable(var_name);

    // todo check if symbol exists
    if (stack_node_find(symb_table, var_name, FALSE) != NULL) {
        printf("ERROR: There is already a symbol with the same name '%s'\n", var_name);
        exit(1);
    }

    if (expr != NULL)
        symb_var->undefined = FALSE;

    stack_node_push(&symb_table, symb_var, 1);

    return ast_node_new_ch3(EL_DECL, ast_node_new_symbol(symb_var), expr, rest);
}

ast_node *h_nLocation(char var_name[81]) {
    symbol *symb_var = stack_node_find(symb_table, var_name, TRUE);

    if (symb_var == NULL) {
        printf("ERROR: Variable '%s' is undefined\n", var_name);
        exit(1);
    }

    return ast_node_new_symbol(symb_var);
}

ast_node *h_nMethod(char meth_name[81]) {
    symbol *symb_meth = stack_node_find_method(symb_table, meth_name);

    if (symb_meth == NULL) {
        printf("ERROR: Method '%s' is undefined\n", meth_name);
        exit(1);
    }

    return ast_node_new_symbol(symb_meth);
}

void semcheck_break_stmt(ast_node* stmt) {
    // wrap a single statement to a fake list of statements
    ast_node *stmts = ast_node_new_ch2(EL_STMT, NULL, stmt);
    semcheck_break(stmts);
    free(stmts);
}

void semcheck_break(ast_node *stmts) {
    ast_node *current = stmts;
    
    while(current != NULL) {
        ast_node *stmt = current->children[1];
        current = current->children[0];

        if (stmt == NULL)
            continue;

        if (stmt->element_type == EL_WHILE) // TODO we maybe dont care if there's a break inside
            continue;

        if (stmt->element_type == EL_BLOCK)
            semcheck_break(stmt->children[0]);

        if (stmt->element_type == EL_IF) {
            semcheck_break_stmt(stmt->children[1]);
            semcheck_break_stmt(stmt->children[2]);
        }

        if (stmt->element_type == EL_BREAK) {
            printf("ERROR: Found a break statement outside of a while loop\n");
            exit(1);
        }
    }
}

ast_node *h_nBody(ast_node *decls, ast_node *stmts) {
    if (!semcheck_return(stmts)) {
        printf("ERROR: There is no return statement\n");
        exit(1);
    }

    semcheck_break(stmts);
    return ast_node_new_ch2(EL_BODY, decls, stmts);
}

ast_node *h_nBlock(ast_node *stmts) {
    // semcheck_break(stmts);
    ast_node_new_ch1(EL_BLOCK, stmts);
}

ast_node *h_nProgram(ast_node *meth_list) {
    if (stack_node_find_method(symb_table, "main") == NULL) {
        printf("ERROR: There is no main method\n");
        exit(1);
    }

    return ast_node_new_ch1(EL_PROG, meth_list);
}

void yyerror(char *msg) {
    fprintf(stderr, "[yacc] > error: %s\n", msg);
    exit(1);
}

int main(void) {
    yyparse();

    printf("[yacc] > -------------- FINAL_TREE --------------\n");
    ast_node_debug(root, TRUE);

    stack_node_debug(symb_table, TRUE);
    
    // ast_node_free(root); // fails

    generate_mixal(root);

    return 0;
}