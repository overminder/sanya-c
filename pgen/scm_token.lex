%{
#include "obj_api.h"
#include "scm_syntax.bison.h"
%}

%%

\( {
    return T_LPAREN;
}

\) {
    return T_RPAREN;
}

\. {
    return T_PERIOD;
}

"'" {
    return T_QUOTE;
}

"#t" {
    yylval.obj_val = make_true();
    return T_EXPR;
}

"#f" {
    yylval.obj_val = make_false();
    return T_EXPR;
}

-?[0-9]+ {
    yylval.obj_val = make_fixnum(yytext);
    return T_EXPR;
}

[\+\-\*\^\?a-zA-Z!<=>\_~/$%&:][\+\-\*\^\?a-zA-Z0-9!<=>\_~/$%&:]* {
    yylval.obj_val = make_symbol(yytext);
    return T_EXPR;
}

[ \n\t]+ ;

%%

int
yywrap(void)
{
    return 1;
}

void
yyerror(const char *s)
{
    fprintf(stderr, "YYERROR -- %s", s);
}

