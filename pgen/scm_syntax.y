%{
#include "obj_api.h"
#include "scm_token.flex.h"

extern int yylex(void);
extern void yyerror(const char *);
static obj_t *last_expr;

%}

%token T_LPAREN T_RPAREN T_PERIOD
%token <obj_val> T_SYMBOL T_FIXNUM
%type <obj_val> sexpr pair list prog

%union {
    obj_t *obj_val;
};

%%

prog: sexpr {
    $$ = $1;
    last_expr = $1;
}

sexpr: T_SYMBOL { $$ = $1; }
     | T_FIXNUM { $$ = $1; }
     | list { $$ = $1; }

list: T_LPAREN pair T_RPAREN { $$ = $2 }

pair: sexpr T_PERIOD sexpr { $$ = make_pair($1, $3); }
    | sexpr pair { $$ = make_pair($1, $2); }
    | { $$ = make_nil(); }

%%

obj_t *
scm_parse_string(char *s)
{
    YY_BUFFER_STATE buf;
    buf = yy_scan_string(s);
    yy_switch_to_buffer(buf);
    gc_set_enabled(0);
    yyparse();
    gc_set_enabled(1);
    yy_delete_buffer(buf);
    return last_expr;
}

