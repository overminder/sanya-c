%{
#include "obj_api.h"
#include "scm_token.flex.h"

extern int yylex(void);
extern void yyerror(const char *);
static obj_t *prog_expr = NULL;
static obj_t **prog_frame;

%}

%token T_LPAREN T_RPAREN T_PERIOD T_QUOTE
%token <obj_val> T_EXPR
%type <obj_val> sexpr pair list prog

%union {
    obj_t *obj_val;
};

%%

prog: sexpr { $$ = make_pair($1, make_nil()); prog_expr = $$; }
    | sexpr prog { $$ = make_pair($1, $2); prog_expr = $$ }

sexpr: T_EXPR { $$ = $1; }
     | list { $$ = $1; }
     | T_QUOTE sexpr { $$ = make_quoted($2); }

list: T_LPAREN pair T_RPAREN { $$ = $2 }

pair: sexpr T_PERIOD sexpr { $$ = make_pair($1, $3); }
    | sexpr pair { $$ = make_pair($1, $2); }
    | { $$ = make_nil(); }

%%

void
sparse_init()
{
    static bool_t initialized = 0;
    if (initialized)
        return;
    else
        initialized = 1;

    prog_frame = gc_get_stack_base();
    --prog_frame;
    *prog_frame = NULL;
    gc_set_stack_base(prog_frame);
}

obj_t *
sparse_get_expr()
{
    obj_t *retval;
    if (!prog_expr || nullp(prog_expr)) {
        return NULL;
    }
    else {
        retval = pair_car(prog_expr);
        prog_expr = pair_cdr(prog_expr);
        *prog_frame = prog_expr;
        return retval;
    }
}

void
sparse_do_string(char *s)
{
    YY_BUFFER_STATE buf;
    buf = yy_scan_string(s);
    yy_switch_to_buffer(buf);
    gc_set_enabled(0);
    yyparse();
    *prog_frame = prog_expr;
    gc_set_enabled(1);
    yy_delete_buffer(buf);
}

void
sparse_do_file(FILE *fp)
{
    yyin = fp;
    gc_set_enabled(0);
    yyparse();
    *prog_frame = prog_expr;
    gc_set_enabled(1);
    yyin = NULL;
}

