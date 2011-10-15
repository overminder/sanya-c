
#include <stdio.h>
#include <stdlib.h>
#include "rl.h"
#include "seval.h"
#include "sgc.h"

extern obj_t *scm_parse_string(char *);
extern obj_t *scm_parse_file(FILE *);

int g_argc;
char **g_argv;

void
repl()
{
    obj_t *expr;
    char *line;
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_EXTEND_ENV);

    while (1) {
        line = rl_getstr(">>> ");
        if (!line)
            break;

        obj_t **ext_frame = frame_extend(frame, 1,
                                         FR_SAVE_PREV | FR_EXTEND_ENV);
        expr = scm_parse_string(line);
        if (!expr) {
            puts("");
            continue;
        }

        *frame_ref(ext_frame, 0) = expr;
        expr = eval_frame(ext_frame);

        if (get_type(expr) != TP_UNSPECIFIED) {
            print_repr(expr, stdout);
            puts("");
        }

        if (gc_want_collect())
            gc_collect(frame);
    }
}

void
run_file()
{
    obj_t *expr;
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_EXTEND_ENV);

    FILE *fp = fopen(g_argv[1], "r");
    expr = scm_parse_file(fp);
    fclose(fp);
    *frame_ref(frame, 0) = expr;
    eval_frame(frame);
}

int
main(int argc, char **argv)
{
    seval_init();

    g_argc = argc;
    g_argv = argv;

    if (argc == 1)
        repl();
    else if (argc == 2)
        run_file();
    return 0;
}

