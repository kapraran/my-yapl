#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// typedef enum {
//   TRUE = 0,
//   FALSE = 1
// } t_bool;

#define FALSE 0
#define TRUE  1
#define MAX_CHILDREN 4

#define EL_WHILE    1000
#define EL_ASSIGN   1001
#define EL_RETURN   1002
#define EL_IF       1003
#define EL_BLOCK    1004
#define EL_METH     1005
#define EL_METH_EX  1006
#define EL_METH_LS  1007
#define EL_BODY     1008
#define EL_DECL     1009
#define EL_STMT     1010
#define EL_PROG     1011
#define EL_PARAMS   1012
#define EL_ARGS     1013
#define EL_BREAK    1014
#define EL_DECL_ST  1015
#define EL_DECL_LS  1016
#define EL_EQ       '='
#define EL_ADD      '+'   
#define EL_SUB      '-'
#define EL_MULT     '*'
#define EL_DIV      '/'
#define EL_LEQT     1017
#define EL_LT       1018
#define EL_GT       1019
#define EL_GEQT     1020
#define EL_EQEQ     1021
#define EL_NEQ      1022

#define SYM_TYPE_INT    2000
#define SYM_CONSTANT_INT    2001
#define SYM_VARIABLE        2002
#define SYM_METHOD          2003

#define DEBUG_NONE  0
#define DEBUG_VIZ   1
#define DEBUG_ALL   2

int debug_level = DEBUG_ALL;

// SYMBOL

typedef struct __symbol {
    int type;
    char name[81];

    // used for SYM_VARIABLE and SYM_METHOD
    union {
        int ret_type;
        int var_type;
    };

    union {
        long location;
        int ivalue;
    };

    char undefined;
} symbol;

symbol *symbol_allocate() {
    symbol *symb = (symbol*) malloc(sizeof(symbol));

    symb->type = 0;
    strcpy(symb->name, "");
    symb->ret_type = 0;
    symb->location = 0;
    symb->undefined = TRUE;

    return symb;
}

symbol *symbol_new_integer(int value) {
    symbol *symb = symbol_allocate();

    symb->type = SYM_CONSTANT_INT;
    symb->ivalue = value;

    return symb;
}

symbol *symbol_new_type(int type) {
    symbol *symb = symbol_allocate();

    symb->type = type;

    return symb;
}

symbol *symbol_new_variable_wtvar(char name[81], int var_type) {
    symbol *symb = symbol_allocate();

    symb->type = SYM_VARIABLE;
    strcpy(symb->name, name);
    symb->var_type = var_type;

    return symb;
}

symbol *symbol_new_variable(char name[81]) {
    return symbol_new_variable_wtvar(name, 0);
}

symbol *symbol_new_method_wtret(char name[81], int ret_type) {
    symbol *symb = symbol_allocate();

    symb->type = SYM_METHOD;
    strcpy(symb->name, name);
    symb->ret_type = ret_type;

    return symb;
}

symbol *symbol_new_method(char name[81]) {
    return symbol_new_method_wtret(name, 0);
}

void symbol_free(symbol *symb) {
    // free(symb->name);
    free(symb);
    symb = NULL;
}

// AST_NODE

typedef struct __ast_node {
    int id;

    char is_symbol;
    symbol *symb;

    int element_type;
    char children_len;
    struct __ast_node *children[MAX_CHILDREN];
} ast_node;

int cmd_has(int argc, char *argv[], char *search) {
    int i;
    for (i=0; i<argc; i++)
        if (strcmp(argv[i], search) == 0)
            return TRUE;
    return FALSE;
}

void debug_level_set(int level) {
    debug_level = level;
}

int debug_level_get() {
    return debug_level;
}

void ast_node_debug(ast_node *node, char rec) {
    if (debug_level < DEBUG_VIZ)
        return;

    if (node == NULL) {
        printf("[yacc] > (epsilon) .................\n");
        
        if (rec)
            printf("[yacc] > {viz} 0:0{}:0:\n");
        return;
    } else if (node->is_symbol) {
        printf("[yacc] > (symbol) ch: %d type: $%d\n", node->children_len, node->symb->type);
        
        if (rec) {
            if (node->symb->type == SYM_CONSTANT_INT)
                printf("[yacc] > {viz} %d:%d{%d}:%d:", node->id, node->symb->type, node->symb->ivalue, node->children_len);
            else
                printf("[yacc] > {viz} %d:%d{%s}:%d:", node->id, node->symb->type, node->symb->name, node->children_len);
        }
    } else {
        printf("[yacc] > (element) ch: %d type: $%d\n", node->children_len, node->element_type);
        
        if (rec)
            printf("[yacc] > {viz} %d:%d{}:%d:", node->id, node->element_type, node->children_len);
    }

    if (rec) {
        int j;
        for (j=0; j<node->children_len; j++) {
            if (node->children[j] != NULL)
                printf(" %d ", node->children[j]->id);
            else
                printf(" 0 ");
        }
        printf("\n");
        
        int i;
        for (i=0; i<node->children_len; i++)
            ast_node_debug(node->children[i], TRUE);
    }
}

int ast_node_id = 1;
ast_node *ast_node_allocate() {
    ast_node *node = (ast_node*) malloc(sizeof(ast_node));

    node->id = ast_node_id++;

    node->is_symbol = FALSE;
    node->symb = NULL;

    node->element_type = 0;
    node->children_len = 0;
    int i;
    for (i=0; i<MAX_CHILDREN; i++)
        node->children[i] = NULL;

    return node;
}

ast_node *ast_node_new(int element_type, char children_len, ast_node *ch1, ast_node *ch2, ast_node *ch3, ast_node *ch4) {
    ast_node *node = ast_node_allocate();

    node->element_type = element_type;
    node->children_len = children_len;
    node->children[0] = ch1;
    node->children[1] = ch2;
    node->children[2] = ch3;
    node->children[3] = ch4;

    // ast_node_debug(node, FALSE);

    return node;
}

void ast_node_free(ast_node *node) {
    if (node == NULL)
        return;

    if (node->is_symbol)
        symbol_free(node->symb);

    if (node->children_len > 0) {
        int i;
        for (i=0; i<node->children_len; i++)
            ast_node_free(node->children[i]);
    }

    free(node);
    node = NULL;
}

ast_node *ast_node_new_ch0(int element_type) {
    return ast_node_new(element_type, 0, NULL, NULL, NULL, NULL);
}

ast_node *ast_node_new_ch1(int element_type, ast_node *ch1) {
    return ast_node_new(element_type, 1, ch1, NULL, NULL, NULL);
}

ast_node *ast_node_new_ch2(int element_type, ast_node *ch1, ast_node *ch2) {
    return ast_node_new(element_type, 2, ch1, ch2, NULL, NULL);
}

ast_node *ast_node_new_ch3(int element_type, ast_node *ch1, ast_node *ch2, ast_node *ch3) {
    return ast_node_new(element_type, 3, ch1, ch2, ch3, NULL);
}

ast_node *ast_node_new_ch4(int element_type, ast_node *ch1, ast_node *ch2, ast_node *ch3, ast_node *ch4) {
    return ast_node_new(element_type, 4, ch1, ch2, ch3, ch4);
}

ast_node *ast_node_new_symbol(symbol *symb) {
    ast_node *node = ast_node_allocate();

    node->is_symbol = TRUE;
    node->symb = symb;

    // ast_node_debug(node, FALSE);

    return node;
}

// SYMBOL TABLE
typedef struct __stack_node {
    symbol *symb;
    int level;

    struct __stack_node *before;
    struct __stack_node *after;
} stack_node;

stack_node *stack_node_allocate() {
    stack_node *node = (stack_node*) malloc(sizeof(stack_node));

    node->symb = NULL;
    node->level = 0;
    node->before = NULL;
    node->after = NULL;

    return node;
}

stack_node *stack_node_new(symbol *symb, int level) {
    stack_node *node = stack_node_allocate();

    node->symb = symb;
    node->level = level;

    return node;
}

void stack_node_free(stack_node *node, char rec) {
    if (node == NULL)
        return;
    
    if (rec) {
        symbol_free(node->symb);
        stack_node_free(node->before, FALSE);
    }
    
    free(node);
    node = NULL;
}

void stack_node_push(stack_node **top, symbol *symb, int level) {
    stack_node *node = stack_node_new(symb, level);

    node->before = *top;
    
    if (*top != NULL)
        (*top)->after = node;
    
    *top = node;
}

symbol *stack_node_pop(stack_node **top) {
    symbol *symb = (*top)->symb;
    stack_node *before = (*top)->before;

    stack_node_free(*top, FALSE);
    *top = before;

    return symb;
}

int stack_node_count_method_args(stack_node *symb_table, char meth_name[81]) {
    stack_node *current = symb_table;
    int count = -1;
    char start_counting = FALSE;

    while (current != NULL) {
        if (current->level != 0) {
            current = current->before;
            continue;
        }            

        if (current->symb->type == SYM_METHOD && strcmp(current->symb->name, meth_name) == 0) {
            start_counting = TRUE;
            count = 0;
        }
        
        else if (start_counting && current->symb->type == SYM_VARIABLE)
            count++;

        else if (start_counting && current->symb->type == SYM_METHOD)
            return count;

        current = current->before;
    }

    return count;
}

void stack_node_remove_lvl1(stack_node **symb_table) {
    while(*symb_table != NULL && (*symb_table)->level == 1)
        stack_node_pop(symb_table);
}

symbol *stack_node_find(stack_node *top, char name[81], char in_meth) {
    stack_node *current = top;
    char my_meth = TRUE;

    while (current != NULL) {
        if (my_meth && current->symb->type == SYM_METHOD) {
            my_meth = FALSE;

            if (in_meth)
                return NULL;
        }

        if (strcmp(current->symb->name, name) == 0) {
            if (my_meth || current->symb->type == SYM_METHOD)
                return current->symb;
        }

        current = current->before;
    }

    return NULL;
}

symbol *stack_node_find_method(stack_node *top, char meth_name[81]) {
    symbol *symb = stack_node_find(top, meth_name, FALSE);

    if (symb == NULL)
        return NULL;

    return symb->type == SYM_METHOD ? symb: NULL;
}

void stack_node_debug(stack_node *node, int rec) {
    printf("[yacc] > <stack_node>\n");
    printf("[yacc] >   level = %d\n", node->level);
    printf("[yacc] >   type = $%d\n", node->symb->type);

    if (node->symb->type == SYM_VARIABLE || node->symb->type == SYM_METHOD) {
        printf("[yacc] >   name = %s\n", node->symb->name);

        if (node->symb->type == SYM_VARIABLE)
            printf("[yacc] >   var_type = $%d\n", node->symb->var_type);
        else
            printf("[yacc] >   ret_type = $%d\n", node->symb->ret_type);
    }

    printf("[yacc] > </stack_node>\n");

    if (rec && node->before != NULL)
        stack_node_debug(node->before, rec);
}

#endif // UTILS_H
