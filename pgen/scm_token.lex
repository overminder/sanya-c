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

-?[0-9]+ {
    yylval.obj_val = make_fixnum(yytext);
    return T_FIXNUM;
}

[\+\-\*\^\?a-zA-Z!<=>\_~/$%&:][\+\-\*\^\?a-zA-Z0-9!<=>\_~/$%&:]* {
    yylval.obj_val = make_symbol(yytext);
    return T_SYMBOL;
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

