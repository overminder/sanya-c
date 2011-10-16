
#include <stdio.h>
#include <stdlib.h>
#include "seval.h"
#include "slib.h"
#include "sgc.h"

int g_argc;
char **g_argv;

static void
repl()
{
    static const char *prog =
        "(define *env* (null-environment 5))"
        "(readline-set-prompt \">>> \")"
        "(define (repl)"
        "  (display (eval (read) *env*))"
        "  (newline)"
        "  (repl))"
        "(repl)";
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_EXTEND_ENV);
    // Factor it.
    slib_primitive_load_string(frame, prog);
}

static void
run_file()
{
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_EXTEND_ENV);
    slib_primitive_load(frame, g_argv[1]);
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

