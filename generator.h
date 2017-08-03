#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>
#include <ctype.h>
#include "utils.compiler.h"

typedef struct __loc {
    char label[128];
    char operation[16];
    char memory[128];

    struct __loc *next;
    struct __loc *prev;
} t_loc;

t_loc *loc_first = NULL;
t_loc *loc_last = NULL;

t_loc *loc_create(const char *label, const char *operation, const char *memory) {
    t_loc *loc = (t_loc*) malloc(sizeof(t_loc));

    strcpy(loc->label, label);
    strcpy(loc->operation, operation);
    strcpy(loc->memory, memory);

    loc->next = NULL;
    loc->prev = NULL;

    return loc;
}

void loc_free(t_loc *loc, char rec) {
    if (loc == NULL)
        return;

    if (rec) {
        loc_free(loc->prev, rec);
        loc_free(loc->next, rec);
    }

    free(loc);
}

void loc_append(t_loc *loc) {
    if (loc_last == NULL) {
        loc_first = loc;
        loc_last = loc;
    } else {
        loc_last->next = loc;
        loc->prev = loc_last;
        loc_last = loc;
    }
}

void loc_insert_after(t_loc *prev, t_loc *ins) {
    // TODO check nulls...
    ins->next = prev->next;
    ins->prev = prev;
    prev->next = ins;
}

t_loc *loc_peek(int index, char from_start) {
    t_loc *curr = from_start ? loc_first: loc_last;
    while(curr != NULL && index > -1) {
        if (index == 0)
            return curr;

        index--;
        curr = from_start ? curr->next: curr->prev;
    }
}

t_loc *loc_peek_start(int index) {
    return loc_peek(index, TRUE);
}

t_loc *loc_peek_end(int index) {
    return loc_peek(index, FALSE);
}

char loc_remove(t_loc *loc) {
    if (loc == NULL)
        return FALSE;

    if (loc->prev != NULL)
        loc->prev->next = loc->next;
    else
        loc_first = loc->next;

    if (loc->next != NULL)
        loc->next->prev = loc->prev;
    else
        loc_last = loc->prev;

    loc_free(loc, FALSE);
    return FALSE;
}

void loc_print(char write_file) {
    FILE *mxv_file = fopen("compiled.mix", "w");

    t_loc *curr = loc_first;
    while (curr != NULL) {
        printf("%s\t%s\t%s\n", curr->label, curr->operation, curr->memory);
        if (write_file)
            fprintf(mxv_file, "%s\t%s\t%s\n", curr->label, curr->operation, curr->memory);
        curr = curr->next;
    }
}

#define EXPR_STACK_START 150
#define PROG_START 800
#define ARGS_STACK_ID 6
#define EXPR_STACK_ID 5
const char *ARGS_STACK_str = "ARSTACK";
const char *EXPR_STACK_str = "EXSTACK";
const char *PROG_LABEL_str = "PROGRAM";
const char *IF_ELSE_str = "IF%dE";
const char *IF_EXIT_str = "IF%dX";
const char *WHILE_START_str = "WH%dS";
const char *WHILE_EXIT_str = "WH%dX";

int mxv_method = 0;
char mxv_method_str[128];
int mxv_label = 0;
char mxv_label_str[128];
int mxv_while_id = -1;
int mxv_optimized_lines = 0;
int mxv_lvar_pos = 0;
t_loc *mxv_lvar_last = NULL;

void generate(ast_node *node);

void mxc(const char *label, const char *op, const char *mem) {
    loc_append(loc_create(label, op, mem));
}

void mxc_lo(const char *label, const char *op) {
    mxc(label, op, "");
}

void mxc_om(const char *op, const char *mem) {
    mxc("", op, mem);
}

void mxc_o(const char *op) {
    mxc("", op, "");
}

void mxu_id_ucase(const char *str, char *uc_str) {
    strcpy(uc_str, str);
    char *c;
    for(c=uc_str; *c=toupper(*c); ++c);
}

int mxu_id_hash(char *str) {
    int n = 0, i;
    for (i=0; i<strlen(str); i++)
        n += str[i];

    return n + strlen(str) % str[0];
}

void mxu_var_name(char *name, char *str) {
    char uc_name[128];
    mxu_id_ucase(name, uc_name);
    int hash = mxu_id_hash(name);
    sprintf(str, "%c%d%c%d%d", 'V', mxv_method % 1000, uc_name[0], strlen(name) % 100, hash % 1000);
}

void mxu_method(char *name, char *str) {
    char uc_name[128];
    mxu_id_ucase(name, uc_name);
    int hash = mxu_id_hash(name);
    sprintf(str, "%c%c%d%d", 'M', uc_name[0], strlen(name) % 1000, hash % 10000);
}

void mxu_method_exit(char *name, char *str) {
    sprintf(str, "%sX", name);
}

void mxu_mem_pos(const char *label, int i, char *str) {
    sprintf(str, "%s,%d", label, i);
}

void mxu_next_label(const char def) {
    mxv_label++;
    sprintf(mxv_label_str, "LBL%d", mxv_label);

    if (def)
        mxc_lo(mxv_label_str, "NOP");
}

void mxu_symbol(symbol *symb, char *str) {
    if (symb->type == SYM_CONSTANT_INT)
        sprintf(str, "=%d=", symb->ivalue);
    else if (symb->type == SYM_VARIABLE)
        mxu_var_name(symb->name, str);
}

char mxg_op(int op, char *symb) {
    char sta = TRUE;

    if (op == EL_LEQT || op == EL_LT || op == EL_GT || op == EL_GEQT || op == EL_EQEQ || op == EL_NEQ) {
        mxc_om("CMPA", symb);
        mxc_om("LDA", "=1=");

        mxu_next_label(FALSE);
        if (op == EL_LEQT) {
            mxc_om("JLE", mxv_label_str);
        } else if (op == EL_LT) {
            mxc_om("JL", mxv_label_str);
        } else if (op == EL_GT) {
            mxc_om("JG", mxv_label_str);
        } else if (op == EL_GEQT) {
            mxc_om("JGE", mxv_label_str);
        } else if (op == EL_EQEQ) {
            mxc_om("JE", mxv_label_str);
        } else if (op == EL_NEQ) {
            mxc_om("JNE", mxv_label_str);
        }

        mxc_om("LDA", "=0=");
        mxc_lo(mxv_label_str, "NOP");
    } else if (op == EL_ADD) {
        mxc_om("ADD", symb);
    } else if (op == EL_SUB) {
        mxc_om("SUB", symb);
    } else if (op == EL_MULT) {
        mxc_om("MUL", symb);
        sta = FALSE;
    } else if (op == EL_DIV) {
        mxc_om("ENTA", "0");
        mxc_om("DIV", symb);
    }

    return sta;
}

// applys operator to the top 2 numbers of the stack
void mxg_apply_op(int op) {
    char tmp_str[128];
    
    mxc_om("DEC5", "2");
    mxu_mem_pos(EXPR_STACK_str, 5, tmp_str);
    
    if (op != EL_DIV)
        mxc_om("LDA", tmp_str);
    else
        mxc_om("LDX", tmp_str);

    mxc_om("INC5", "1");

    char sta = mxg_op(op, tmp_str);
    mxc_om("DEC5", "1");

    // push result
    if (sta)
        mxc_om("STA", tmp_str);
    else
        mxc_om("STX", tmp_str);
    mxc_om("INC5", "1");
}

void mxg_stack_push(const char stack, const char a_reg) {
    char memory[128], inc[16];
    const char *stack_str = stack == ARGS_STACK_ID ? ARGS_STACK_str: EXPR_STACK_str;
    const char *reg = a_reg ? "STA": "STX";
    mxu_mem_pos(stack_str, stack, memory);
    mxc_om(reg, memory);
    sprintf(inc, "INC%d", stack);
    mxc_om(inc, "1");
}

void mxg_stack_pop(const char stack) {
    char memory[128], dec[16];
    const char *stack_str = stack == ARGS_STACK_ID ? ARGS_STACK_str: EXPR_STACK_str;
    sprintf(dec, "DEC%d", stack);
    mxc_om(dec, "1");
    mxu_mem_pos(stack_str, stack, memory);
    mxc_om("LDA", memory);
}

/** special **/
void mxg_define_var(char *orig_name) {
    mxv_lvar_pos++;
    char label[128], memory[128];
    mxu_var_name(orig_name, label);
    sprintf(memory, "%d", PROG_START-mxv_lvar_pos);
    t_loc *loc = loc_create(label, "EQU", memory);
    loc_insert_after(mxv_lvar_last, loc);
}

void mxg_declare(ast_node *left, ast_node *right, char *tmp_str) {
    char var_name[128];
    mxg_define_var(left->symb->name);
    mxu_var_name(left->symb->name, var_name);

    if (right != NULL && right->is_symbol) {
        symbol *symb = right->symb;

        mxu_symbol(symb, tmp_str);

        // if (symb->type == SYM_CONSTANT_INT)
        //     sprintf(tmp_str, "=%d=", symb->ivalue);
        // else
        //     mxu_var_name(symb->name, tmp_str);

        mxc_om("LDA", tmp_str);
        mxc_om("STA", var_name);
    } else if (right != NULL) {
        generate(right);
        mxg_stack_pop(EXPR_STACK_ID);
        mxc_om("STA", var_name);
    }
}

/*************************/
/** mixal optimizations **/
/*************************/
char mxo_rem_ret_jmp(char *label) {
    if (strcmp(loc_last->operation, "JMP") == 0 && strcmp(loc_last->memory, label) == 0) {
        mxv_optimized_lines++;
        loc_remove(loc_last);
        return TRUE;
    }

    return FALSE;
}

char mxo_only_main(char *orig_name) {
    if (strcmp(orig_name, "main") == 0 && mxv_method == 1) {
        loc_remove(loc_last);
        mxv_optimized_lines += 2;
        return TRUE;
    }

    return FALSE;
}

void generate(ast_node *node) {
    if (node == NULL)
        return;

    char tmp_str[128];

    switch(node->element_type) {
        case EL_PROG:
            mxc(ARGS_STACK_str, "EQU", "0");
            sprintf(tmp_str, "%d", EXPR_STACK_START);
            mxc(EXPR_STACK_str, "EQU", tmp_str);
            mxv_lvar_last = loc_last;
            sprintf(tmp_str, "%d", PROG_START);
            mxc(PROG_LABEL_str, "EQU", tmp_str);
            mxc_om("ORIG", PROG_LABEL_str);
            mxu_method("main", tmp_str);
            mxc_om("JMP", tmp_str);

            generate(node->children[0]);

            mxc_om("END", PROG_LABEL_str);
            break;

        case EL_METH_LS:
            generate(node->children[0]);
            generate(node->children[1]);
            break;

        case EL_METH:
            mxv_method++;
            char mtname[128];
            mxu_method(node->children[1]->symb->name, mtname);
            strcpy(mxv_method_str, mtname);

            mxu_method_exit(mtname, tmp_str);
            if (!mxo_only_main(node->children[1]->symb->name)) {
                mxc(mtname, "STJ", tmp_str);
            }

            generate(node->children[2]);
            generate(node->children[3]);

            mxo_rem_ret_jmp(tmp_str);
            if (strcmp(node->children[1]->symb->name, "main") != 0) {
                mxc(tmp_str, "JMP", "*");
            } else {
                mxc_lo(tmp_str, "HLT");
            }
            break;

        case EL_PARAMS:
            mxg_define_var(node->children[2]->symb->name);
            mxg_stack_pop(ARGS_STACK_ID);
            mxu_var_name(node->children[2]->symb->name, tmp_str);
            mxc_om("STA", tmp_str);

            generate(node->children[0]);
            break;

        case EL_BREAK:
            sprintf(tmp_str, WHILE_EXIT_str, mxv_while_id);
            mxc_om("JMP", tmp_str);
            break;

        case EL_WHILE:
        {
            int prev_while_id = mxv_while_id;
            mxv_while_id = node->id;

            sprintf(tmp_str, WHILE_START_str, node->id);
            mxc_lo(tmp_str, "NOP");

            if (node->children[0]->is_symbol) {
                mxu_symbol(node->children[0]->symb, tmp_str);
                mxc_om("LDA", tmp_str);
            } else {
                generate(node->children[0]);
                mxg_stack_pop(EXPR_STACK_ID);
            }

            mxc_om("CMPA", "=0=");
            sprintf(tmp_str, WHILE_EXIT_str, node->id);
            mxc_om("JE", tmp_str);

            generate(node->children[1]);

            sprintf(tmp_str, WHILE_START_str, node->id);
            mxc_om("JMP", tmp_str);
            sprintf(tmp_str, WHILE_EXIT_str, node->id);
            mxc_lo(tmp_str, "NOP");

            mxv_while_id = prev_while_id;

            break;
        }
        
        case EL_DECL_ST:
            // ignores type...
            mxg_declare(node->children[1], node->children[2], tmp_str);
            generate(node->children[3]);
            break;

        case EL_DECL:
            mxg_declare(node->children[0], node->children[1], tmp_str);
            generate(node->children[2]);
            break;

        case EL_DECL_LS:
            generate(node->children[0]);
            generate(node->children[1]);
            break;

        case EL_ASSIGN:      
            if (node->children[1]->is_symbol) {
                mxu_symbol(node->children[1]->symb, tmp_str);
                mxc_om("LDA", tmp_str);
            } else {
                generate(node->children[1]);
                mxg_stack_pop(EXPR_STACK_ID);
            }

            mxu_var_name(node->children[0]->symb->name, tmp_str);
            mxc_om("STA", tmp_str);

            break;

        case EL_BODY:
            generate(node->children[0]);
            generate(node->children[1]);
            break;

        case EL_BLOCK:
            generate(node->children[0]);
            break;

        case EL_STMT:
            generate(node->children[0]);
            generate(node->children[1]);
            break;

        case EL_IF:
            if (node->children[0]->is_symbol) {
                mxu_symbol(node->children[0]->symb, tmp_str);
                mxc_om("LDA", tmp_str);
            } else {
                generate(node->children[0]);
                mxg_stack_pop(EXPR_STACK_ID);
            }

            // do the comparison
            mxc_om("CMPA", "=0=");
            sprintf(tmp_str, IF_ELSE_str, node->id);
            mxc_om("JE", tmp_str);

            generate(node->children[1]);

            sprintf(tmp_str, IF_EXIT_str, node->id);
            mxc_om("JMP", tmp_str);

            sprintf(tmp_str, IF_ELSE_str, node->id);
            mxc_lo(tmp_str, "NOP");

            generate(node->children[2]);

            sprintf(tmp_str, IF_EXIT_str, node->id);
            mxc_lo(tmp_str, "NOP");

            break;
        
        case EL_RETURN:
            if (node->children[0]->is_symbol) {
                mxu_symbol(node->children[0]->symb, tmp_str);
                mxc_om("LDA", tmp_str);
                mxg_stack_push(EXPR_STACK_ID, TRUE);
            } else {
                generate(node->children[0]);
            }
            
            mxu_method_exit(mxv_method_str, tmp_str);
            mxc_om("JMP", tmp_str);

            break;

        case EL_METH_EX:
            generate(node->children[1]);
            mxu_method(node->children[0]->symb->name, tmp_str);
            mxc_om("JMP", tmp_str);
            break;

        case EL_ARGS:
            generate(node->children[0]);

            // check if arg is symbol or el
            if (node->children[1]->is_symbol) {
                mxu_symbol(node->children[1]->symb, tmp_str);
                mxc_om("LDA", tmp_str);
            } else {
                generate(node->children[1]);

                mxg_stack_pop(EXPR_STACK_ID);
            }

            mxu_mem_pos(ARGS_STACK_str, 6, tmp_str);
            mxc_om("STA", tmp_str);
            mxc_om("INC6", "1");

            break;

        case EL_LEQT:
        case EL_LT:
        case EL_GT:
        case EL_GEQT:
        case EL_EQEQ:
        case EL_NEQ:
        case EL_ADD:
        case EL_SUB:
        case EL_MULT:
        case EL_DIV:
        {
            ast_node *left = node->children[0];
            ast_node *right = node->children[1];

            // optimization #1
            if (left->is_symbol && right->is_symbol) {
                mxu_symbol(left->symb, tmp_str);

                char *mxop = node->element_type != EL_DIV ? "LDA": "LDX";
                mxc_om(mxop, tmp_str);

                mxu_symbol(right->symb, tmp_str);
                char sta = mxg_op(node->element_type, tmp_str);
                char *reg = sta ? "STA": "STX";
                mxg_stack_push(EXPR_STACK_ID, sta);

                return;
            }

            if (left->is_symbol) {
                mxu_symbol(left->symb, tmp_str);
                
                // push to stack
                mxc_om("LDA", tmp_str);
                mxg_stack_push(EXPR_STACK_ID, TRUE);
            } else {
                generate(left);
            }

            if (right->is_symbol) {
                mxu_symbol(right->symb, tmp_str);

                // push to stack
                mxc_om("LDA", tmp_str);
                mxg_stack_push(EXPR_STACK_ID, TRUE);
            } else {
                generate(right);
            }

            mxg_apply_op(node->element_type);

            break;
        }

        default:
            break;
    }
}

void generate_mixal(ast_node *root) {
    generate(root);
    loc_print(TRUE);
    printf("> optimized lines = %d\n", mxv_optimized_lines);
}

#endif