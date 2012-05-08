
#include <stdio.h>
#include <stdlib.h>
#include "seval.h"
#include "slib.h"
#include "sgc.h"

int g_argc;
char **g_argv;

// let, letrec, let*, cond, and, or...
#define _TOSTR(x) #x
#define TOSTR(x) _TOSTR(x)
static const char *syntax_lib = TOSTR(omscm_ROOTDIR) "/scripts/lambda-syntax.scm";
#undef TOSTR
#undef _TOSTR

static void
repl()
{
    static const char *prog =
        "(define *env* (null-environment 5))"
        "(readline-set-prompt \">>> \")"
        "(define (repl)"
        "  (define expr (read))"
        "  (if (eof? expr)"
        "    (begin"
        "      (display \"received eof, bye.\")"
        "      (newline))"
        "    (begin"
        "      (set-backtrace-base)"
        "      (define result (eval expr *env*))"
        "      (display (if (unspecified? result) \"OK.\" result))"
        "      (newline)"
        "      (repl))))"
        "(repl)";
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_CONTINUE_ENV);
    slib_primitive_load(frame, syntax_lib);
    // Factor it?
    slib_primitive_load_string(frame, prog);
}

static void
run_file()
{
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_CONTINUE_ENV);
    slib_primitive_load(frame, syntax_lib);
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

    sgc_fini();
    return 0;
}

