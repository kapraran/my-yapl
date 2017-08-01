#ifndef GENERATOR_H
#define GENERATOR_H

#include <ctype.h>
#include "utils.compiler.h"

typedef struct __loc {
    char label[128];
    char operation[16];
    char memory[128];
} t_loc;

const char *ARGS_STACK_str = "ARSTACK";
const char *EXPR_STACK_str = "EXSTACK";

int mx_curr_method = 0;
char mx_curr_method_str[128];
int mx_curr_tmp_label = 0;
char mx_curr_tmp_label_str[128];
int mx_curr_while_id = -1;

void generate(ast_node *node);

void mx_comment(const char *comment) {
    printf("** %s\n", comment);
}

void mx_com(const char *label, const char *op, const char *mem) {
    printf("%s\t%s\t%s\n", label, op, mem); // TODO replace with file
}

void mx_com_lo(char *label, char *op) {
    mx_com(label, op, "");
}

void mx_com_om(char *op, char *mem) {
    mx_com("", op, mem);
}

void mx_com_o(char *op) {
    mx_com("", op, "");
}

int mx_util_uniq_uc(char *str, char *uc_str) {
    strcpy(uc_str, str);
    
    int sum = 0, i;
    char* c;
    for (i=0; i<strlen(str); i++)
        sum += str[i];

    for(c=uc_str; *c=toupper(*c); ++c);
    
    sprintf(uc_str, "%s%d", uc_str, sum);

    return sum;
}

void mx_util_method_var(char *name, char *str) {
    char uc_name[128];
    int code = mx_util_uniq_uc(name, uc_name);
    sprintf(str, "%c%d%d%c%d", uc_name[0], strlen(name) % 100, code % 1000, 'V', mx_curr_method % 1000);
}

void mx_util_method(char *name, char *str) {
    char uc_name[128];    
    int code = mx_util_uniq_uc(name, uc_name);
    sprintf(str, "%c%d%d%c", uc_name[0], strlen(name) % 1000, code % 10000, 'M');
}

void mx_util_method_exit(char *name, char *str) {
    strcpy(str, name);
    strcat(str, "X");
}

void mx_util_mem(const char *label, int i, int f, char *str) {
    char convert[128];

    strcpy(str, label);
    sprintf(convert, ",%d(%d)", i, f);
    strcat(str, convert);
}

void mx_util_mem_li(const char *label, int i, char *str) {
    char convert[128];

    strcpy(str, label);
    sprintf(convert, ",%d", i);
    strcat(str, convert);
}

void mx_util_next_tmp_label(char def) {
    mx_curr_tmp_label++;
    sprintf(mx_curr_tmp_label_str, "LBL%d", mx_curr_tmp_label);
    
    if (def)
        mx_com(mx_curr_tmp_label_str, "CON", "0");
}

void mx_util_symbol(symbol *symb, char *str) {
    if (symb->type == SYM_CNST_INTEGER) {
        sprintf(str, "=%d=", symb->ivalue);
    } else if(symb->type == SYM_VARIABLE) {
        mx_util_method_var(symb->name, str);
    }
}

char mx_gen_op(int op, char *symb) {
    char sta = TRUE;

    if (op == EL_LEQT || op == EL_LT || op == EL_GT || op == EL_GEQT || op == EL_EQEQ || op == EL_NEQ) {
        mx_com_om("CMPA", symb);
        mx_com_om("LDA", "=1=");

        mx_util_next_tmp_label(FALSE);
        if (op == EL_LEQT) {
            mx_com_om("JLE", mx_curr_tmp_label_str);
        } else if (op == EL_LT) {
            mx_com_om("JL", mx_curr_tmp_label_str);
        } else if (op == EL_GT) {
            mx_com_om("JG", mx_curr_tmp_label_str);
        } else if (op == EL_GEQT) {
            mx_com_om("JGE", mx_curr_tmp_label_str);
        } else if (op == EL_EQEQ) {
            mx_com_om("JE", mx_curr_tmp_label_str);
        } else if (op == EL_NEQ) {
            mx_com_om("JNE", mx_curr_tmp_label_str);
        }

        mx_com_om("LDA", "=0=");
        mx_com_lo(mx_curr_tmp_label_str, "NOP");
    } else if (op == EL_ADD) {
        mx_com_om("ADD", symb);
    } else if (op == EL_SUB) {
        mx_com_om("SUB", symb);
    } else if (op == EL_MULT) {
        mx_com_om("MUL", symb);
        sta = FALSE;
    } else if (op == EL_DIV) {
        mx_com_om("ENTA", "0");
        mx_com_om("DIV", symb);
    }

    return sta;
}

// applys operator to the top 2 numbers of the stack
void mx_gen_stack_op(int op) {
    char tmp_str[128];
    
    mx_com_om("DEC5", "2");
    mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
    
    if (op != EL_DIV)
        mx_com_om("LDA", tmp_str);
    else
        mx_com_om("LDX", tmp_str);

    mx_com_om("INC5", "1");

    char sta = mx_gen_op(op, tmp_str);
    mx_com_om("DEC5", "1");

    // push result
    if (sta)
        mx_com_om("STA", tmp_str);
    else
        mx_com_om("STX", tmp_str);
    mx_com_om("INC5", "1");
}

void mx_gen_declare(ast_node *left, ast_node *right, char *tmp_str) {
    char var_name[128];

    mx_util_method_var(left->symb->name, var_name);

    if (right != NULL && right->is_symbol) {
        if (right->symb->type == SYM_CNST_INTEGER) {
            sprintf(tmp_str, "%d", right->symb->ivalue);
            mx_com(var_name, "CON", tmp_str);
        } else {
            mx_com(var_name, "CON", "0");
            mx_util_method_var(right->symb->name, tmp_str);
            mx_com_om("LDA", tmp_str);
            mx_com_om("STA", var_name);
        }
    } else if (right != NULL) {
        mx_com(var_name, "CON", "0");

        generate(right);

        mx_com_om("DEC5", "1");
        mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
        mx_com_om("LDA", tmp_str);
        mx_com_om("STA", var_name);
    }
}

void generate(ast_node *node) {
    if (node == NULL)
        return;

    char tmp_str[128];

    switch(node->element_type) {
        case EL_PROG:
            mx_com(ARGS_STACK_str, "EQU", "0");     // ri6
            mx_com(EXPR_STACK_str, "EQU", "256");   // ri5
            mx_com("PROG", "EQU", "1024");
            mx_com_om("ORIG", "PROG");
            mx_util_method("main", tmp_str);
            mx_com_om("JMP", tmp_str);
            
            generate(node->children[0]);
            
            mx_com_o("HLT");
            mx_com_om("END", "PROG");
            break;

        case EL_METHLIST:
            generate(node->children[0]);
            generate(node->children[1]);
            break;

        case EL_METH:
            mx_curr_method++;
            char mtname[128];
            mx_util_method(node->children[1]->symb->name, mtname);
            strcpy(mx_curr_method_str, mtname);

            mx_util_method_exit(mtname, tmp_str);
            mx_com(mtname, "STJ", tmp_str);
            
            generate(node->children[2]);
            generate(node->children[3]);

            if (strcmp(node->children[1]->symb->name, "main") != 0)
                mx_com(tmp_str, "JMP", "*");
            else
                mx_com_lo(tmp_str, "NOP");
            break;

        case EL_PARAMS:
            mx_util_method_var(node->children[2]->symb->name, tmp_str);
            mx_com(tmp_str, "CON", "0");
            mx_com_om("DEC6", "1");
            mx_util_mem_li(ARGS_STACK_str, 6, tmp_str);
            mx_com_om("LDA", tmp_str);
            mx_util_method_var(node->children[2]->symb->name, tmp_str);
            mx_com_om("STA", tmp_str);

            generate(node->children[0]);
            break;
  
        case EL_BREAK:
            sprintf(tmp_str, "WHL%dX", mx_curr_while_id);
            mx_com_om("JMP", tmp_str);
            break;

        case EL_WHILE:
        {
            int prev_while_id = mx_curr_while_id;
            mx_curr_while_id = node->id;

            sprintf(tmp_str, "WHL%d", node->id);
            mx_com_lo(tmp_str, "NOP");

            if (node->children[0]->is_symbol) {
                mx_util_symbol(node->children[0]->symb, tmp_str);
            } else {
                generate(node->children[0]);
                mx_com_om("DEC5", "1");
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
            }

            // do the comparison
            mx_com_om("LDA", tmp_str);
            mx_com_om("CMPA", "=0=");
            sprintf(tmp_str, "WHL%dX", node->id);
            mx_com_om("JE", tmp_str);

            generate(node->children[1]);

            sprintf(tmp_str, "WHL%d", node->id);
            mx_com_om("JMP", tmp_str);
            sprintf(tmp_str, "WHL%dX", node->id);
            mx_com_lo(tmp_str, "NOP");

            mx_curr_while_id = prev_while_id;

            break;
        }
        
        case EL_DECL_ST:
            // ignores type...
            mx_gen_declare(node->children[1], node->children[2], tmp_str);
            generate(node->children[3]);
            break;

        case EL_DECL:
            mx_gen_declare(node->children[0], node->children[1], tmp_str);
            generate(node->children[2]);
            break;

        case EL_DECL_LS:
            generate(node->children[0]);
            generate(node->children[1]);
            break;

        case EL_ASSIGN:      
            if (node->children[1]->is_symbol) {
                mx_util_symbol(node->children[1]->symb, tmp_str);
            } else {
                generate(node->children[1]);
                mx_com_om("DEC5", "1");
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
            }

            mx_com_om("LDA", tmp_str);
            mx_util_method_var(node->children[0]->symb->name, tmp_str);
            mx_com_om("STA", tmp_str);

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
                mx_util_symbol(node->children[0]->symb, tmp_str);
            } else {
                generate(node->children[0]);
                mx_com_om("DEC5", "1");
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
            }

            // do the comparison
            mx_com_om("LDA", tmp_str);
            mx_com_om("CMPA", "=0=");
            sprintf(tmp_str, "ELIF%d", node->id);
            mx_com_om("JE", tmp_str);

            generate(node->children[1]);

            sprintf(tmp_str, "ENIF%d", node->id);
            mx_com_om("JMP", tmp_str);

            sprintf(tmp_str, "ELIF%d", node->id);
            mx_com_lo(tmp_str, "NOP");

            generate(node->children[2]);

            sprintf(tmp_str, "ENIF%d", node->id);
            mx_com_lo(tmp_str, "NOP");

            break;
        
        case EL_RETURN:
            if (node->children[0]->is_symbol) {
                mx_util_symbol(node->children[0]->symb, tmp_str);
                mx_com_om("LDA", tmp_str);
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
                mx_com_om("STA", tmp_str);
                mx_com_om("INC5", "1");
            } else {
                generate(node->children[0]);
            }
            
            mx_util_method_exit(mx_curr_method_str, tmp_str);
            mx_com_om("JMP", tmp_str);

            break;

        case EL_METHCALL:
            generate(node->children[1]);
            mx_util_method(node->children[0]->symb->name, tmp_str);
            mx_com_om("JMP", tmp_str);
            break;

        case EL_ARGS:
            generate(node->children[0]);

            // check if arg is symbol or el
            if (node->children[1]->is_symbol) {
                mx_util_symbol(node->children[1]->symb, tmp_str);
            } else {
                generate(node->children[1]);

                mx_com_om("DEC5", "1");
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
            }

            mx_com_om("LDA", tmp_str);
            mx_util_mem_li(ARGS_STACK_str, 6, tmp_str);
            mx_com_om("STA", tmp_str);
            mx_com_om("INC6", "1");

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
            if (left->is_symbol && right->is_symbol) { // TODO check if methods
                mx_util_symbol(left->symb, tmp_str);

                if (node->element_type != EL_DIV)
                    mx_com_om("LDA", tmp_str);
                else
                    mx_com_om("LDX", tmp_str);

                mx_util_symbol(right->symb, tmp_str);
                char sta = mx_gen_op(node->element_type, tmp_str);
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
                if (sta)
                    mx_com_om("STA", tmp_str);
                else
                    mx_com_om("STX", tmp_str);
                mx_com_om("INC5", "1");
                
                return;
            }

            if (left->is_symbol) {
                mx_util_symbol(left->symb, tmp_str);
                
                // push to stack
                mx_com_om("LDA", tmp_str);
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
                mx_com_om("STA", tmp_str);
                mx_com_om("INC5", "1");
            } else {
                generate(left);
            }

            if (right->is_symbol) {
                mx_util_symbol(right->symb, tmp_str);
                
                // push to stack
                mx_com_om("LDA", tmp_str);
                mx_util_mem_li(EXPR_STACK_str, 5, tmp_str);
                mx_com_om("STA", tmp_str);
                mx_com_om("INC5", "1");
            } else {
                generate(right);
            }

            mx_gen_stack_op(node->element_type);
        
            break;
        }

        default:
            break;
    }
}

void generate_mixal(ast_node *root) {
    generate(root);
}

#endif