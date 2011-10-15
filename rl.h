#ifndef RL_H
#define RL_H

#include <stdlib.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

static char *
rl_getstr(const char *prompt)
{
    static char *line_read = (char *)NULL;
    if (line_read)
    {
        free(line_read);
        line_read = (char *)NULL;
    }

    line_read = readline(prompt);
    if (line_read && *line_read)
        add_history(line_read);
    return line_read;
}


#endif /* RL_H */
