%{
#include "obj_api.h"
#include "scm_token.flex.h"

extern int yylex(void);
extern void yyerror(const char *);
static obj_t *prog_expr = NULL;
static obj_t **prog_frame;

%}

%token T_LPAREN T_RPAREN T_PERIOD T_QUOTE T_SHARPLPAREN
%token <obj_val> T_EXPR
%type <obj_val> sexpr pair list prog vector

%union {
    obj_t *obj_val;
};

%%

prog: sexpr { $$ = make_pair($1, make_nil()); prog_expr = $$; }
    | sexpr prog { $$ = make_pair($1, $2); prog_expr = $$ }

sexpr: T_EXPR { $$ = $1; }
     | list { $$ = $1; }
     | vector { $$ = $1; }
     | T_QUOTE sexpr { $$ = make_quoted($2); }

vector: T_SHARPLPAREN pair T_RPAREN { $$ = make_vector($2); }

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
sparse_do_string(char *s)
{
    obj_t *retval;
    YY_BUFFER_STATE buf;

    buf = yy_scan_string(s);
    yy_switch_to_buffer(buf);
    gc_set_enabled(0);
    yyparse();
    gc_set_enabled(1);
    yy_delete_buffer(buf);

    retval = prog_expr;
    prog_expr = NULL;
    return retval;
}

obj_t *
sparse_do_file(FILE *fp)
{
    obj_t *retval;
    YY_BUFFER_STATE buf;

    buf = yy_create_buffer(fp, YY_BUF_SIZE);
    yy_switch_to_buffer(buf);
    gc_set_enabled(0);
    yyparse();
    gc_set_enabled(1);
    yy_delete_buffer(buf);

    retval = prog_expr;
    prog_expr = NULL;
    return retval;
}

