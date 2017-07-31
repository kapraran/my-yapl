%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"

#define STANDALONE 0

int _match(char *txt, int token);
void error_badchar(char *yytext);

#if STANDALONE

YYSTYPE yylval;

#endif

%}

%%

"true"                      { return _match(yytext, T_TRUE); }
"false"                     { return _match(yytext, T_FALSE); }
"int"                       { return _match(yytext, T_INT); }
"return"                    { return _match(yytext, T_RETURN); }
"if"                        { return _match(yytext, T_IF); }
"else"                      { return _match(yytext, T_ELSE); }
"while"                     { return _match(yytext, T_WHILE); }
"break"                     { return _match(yytext, T_BREAK); }

"("                         { return _match(yytext, '('); }
")"                         { return _match(yytext, ')'); }
"{"                         { return _match(yytext, '{'); }
"}"                         { return _match(yytext, '}'); }
","                         { return _match(yytext, ','); }
";"                         { return _match(yytext, ';'); }

"="                         { return _match(yytext, '='); }
"+"                         { return _match(yytext, '+'); }
"-"                         { return _match(yytext, '-'); }
"*"                         { return _match(yytext, '*'); }
"/"                         { return _match(yytext, '/'); }

"<="                        { return _match(yytext, T_LEQT); }
"<"                         { return _match(yytext, T_LT); }
">"                         { return _match(yytext, T_GT); }
">="                        { return _match(yytext, T_GEQT); }
"=="                        { return _match(yytext, T_EQEQ); }
"!="                        { return _match(yytext, T_NEQ); }

([1-9][0-9]*|0)          { yylval.yint=atoi(yytext); return _match(yytext, T_NUM); }
[A-Za-z]([A-Za-z]|[0-9]|_)* { strcpy(yylval.ystr, yytext); return _match(yytext, T_ID); }

"//".*                      { ; }
[ \f\r\n\t\v]+              { ; }
.                           { error_badchar(yytext); }

%%

void error_badchar(char *yytext) {
    printf("[lex]  > error: unknown characters '%s'\n", yytext);
    exit(1);
}

int _match(char *txt, int token) {
    printf("[lex]  > %3d\t%s\n", token, txt);
    return token;
}

int yywrap() {
    return 1;
}

#if STANDALONE

int main(void) {
    while(1) {
        int token = yylex();

        if (token == 0)
            return 0;

        printf("[lex]  > %d\n", token);
    }
}

#endif