#ifndef GENERATOR_H
#define GENERATOR_H

#include "utils.compiler.h"

void mx_com(char *label, char *op, char *mem) {
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

int mx_curr_method = 0;

void mx_util_method_param(char *name, char *str) {
    char convert[45]; // 128 - 81 - 2
    sprintf(convert, "%d", mx_curr_method);
    
    strcpy(str, name);
    strcat(str, "_m");
    strcat(str, convert);
}

void mx_util_method_exit(char *name, char *str) {
    strcpy(str, name);
    strcat(str, "_exit");
}

void mx_util_literal(int value, char *str) {
    sprintf(str, "=%d=", mx_curr_method);
}

void generate(ast_node *node) {
    switch(node->element_type) {
        case EL_PROG:
            mx_com("STACK", "EQU", "0");
            mx_com("PROG", "EQU", "1000");
            mx_com_om("ORIG", "PROG");
            mx_com_om("JMP", "main");
            
            if (node->children[0] != NULL)
                generate(node->children[0]);
            
            mx_com_o("HLT");
            mx_com_om("END", "PROG");
            break;

        case EL_METHLIST:
            if (node->children[0] != NULL)
                generate(node->children[0]);

            if (node->children[1] != NULL)
                generate(node->children[1]);

            break;

        case EL_METH:
            mx_curr_method++;
            char method_exit[128];
            char *name = node->children[1]->symb->name;

            mx_util_method_exit(name, method_exit);
            mx_com(name, "STJ", method_exit);

            if (node->children[2] != NULL)
                generate(node->children[2]);

            if (node->children[3] != NULL)
                generate(node->children[3]);

            if (strcmp(name, "main") != 0)
                mx_com(method_exit, "JMP", "*");
            else
                mx_com_lo(method_exit, "NOP");
            break;

        case EL_PARAMS:
            {
            char param[128];

            mx_util_method_param(node->children[2]->symb->name, param);
            mx_com(param, "ENT1", "*");
            mx_com_om("DEC6", "1");
            mx_com_om("MOVE", "STACK,6(1)");

            if (node->children[0] != NULL)
                generate(node->children[0]);
            
            break;
            }

        case EL_DECL_LS:
            break;

        case EL_BODY:
            if (node->children[0] != NULL)
                generate(node->children[0]);
            
            if (node->children[1] != NULL)
                generate(node->children[1]);
            
            break;

        case EL_STMT:
            if (node->children[0] != NULL)
                generate(node->children[0]);

            generate(node->children[1]);
            break;

        case EL_RETURN:
            if (node->children[0]->is_symbol) {
                if (node->children[0]->symb->type == SYM_CNST_INTEGER) {
                    char str[128];
                    mx_util_literal(node->children[0]->symb->ivalue, str);
                    mx_com_om("LDA", str);
                }
            }

            break;
        
        default:
            break;
    }
}

void generate_mixal(ast_node *root) {
    generate(root);
}

#endif