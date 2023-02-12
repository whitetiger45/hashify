/************************************************************************
 * Copyright (C) 2013, Felix Bytow <felix.bytow@googlemail.com>         *
 *                                                                      *
 * This file is part of dbfi.                                           *
 *                                                                      *
 * dbfi is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * dbfi is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with dbfi.  If not, see <http://www.gnu.org/licenses/>.        *
 ************************************************************************/
#include "config.h"
#include "lexer.h"
#include "parser.h"
#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

void dbfi_help(char * app)
{
    printf(
        "Usage: %s {options} [filename]\n\n"
        "Available options:\n"
        "  --help,-?               - Show this help message\n"
        "  --version,-v            - Show version information\n"
#if defined(LIBTCC_FOUND)
        "  --compile,-c            - Compile to native program instead of interpreting\n"
        "  --output,-o  [filename] - Alternative output filename for --compile\n"
#endif /* LIBTCC_FOUND */
    , app);
}

void dbfi_version(void)
{
    printf(
        "Dragon BrainFuck Interpreter (dbfi) Version " DBFI_VERSION "\n"
        "Copyright (C) 2013, Felix Bytow <felix.bytow@googlemail.com>\n\n"
        "Git-Hash:         " DBFI_GIT_HASH "\n"
        "Build-Type:       " DBFI_BUILD_TYPE " - %d Bit \n"
        "Compiled with:    " DBFI_COMPILED_WITH "\n"
#if defined(LIBTCC_FOUND)
        "Builtin-Compiler: Yes (" DBFI_LIBTCC ")\n"
#else
        "Builtin-Compiler: No\n"
#endif
        , (int)(sizeof(void*) * CHAR_BIT)
    );
}

char const * dbfi_next_arg(char *** current_arg, char ** end)
{
    if (*current_arg + 1 == end)
    {
        fprintf(stderr, "Error: \"--output,-o\" requires a filename as a parameter.\n");
        exit(1);
    }
    ++(*current_arg);
    return (**current_arg);
}

int dbfi_main(char * filename, int compile, char * output)
{
    dbfi_lexer_t lexer = dbfi_lexer_init(filename);
    dbfi_parser_t parser = dbfi_parser_init();
    dbfi_backend_t backend = dbfi_backend_init(compile ? DBFI_BACKEND_COMPILER : DBFI_BACKEND_INTERPRETER);
    dbfi_parser_tree_t pt;
    assert(lexer);
    assert(parser);
    assert(backend);
    if (*output == '\0')
    {
        strcpy(output, filename);
        strncat(output, ".bin", 1024);
        output[1023] = '\0';
    }
    pt = dbfi_parser_generate_tree(parser, lexer);
    dbfi_parser_release(parser);
    dbfi_lexer_release(lexer);
    assert(pt);
    dbfi_backend_process_parser_tree(backend, pt);
    dbfi_backend_finalize(backend, output);
    dbfi_backend_release(backend);
    dbfi_parser_tree_release(pt);
    return 0;
}

int main(int argc, char ** argv)
{
    char ** arg;
    int compile = 0;
#if defined(__linux__)
    /* on Linux we default to stdin if no filename is given */
    char filename[1024] = "/dev/stdin";
#else
    /* on Windows the filename is mandatory */
    char filename[1024] = "";
#endif /* __linux__ */
    char output[1024] = "";
    for (arg = argv + 1; arg != argv + argc; ++arg)
    {
        /* show help message */
        if ((!strcmp(*arg, "--help")) || (!strcmp(*arg, "-?")))
        {
            dbfi_help(argv[0]);
            return 0;
        }
        /* show version information */
        else if ((!strcmp(*arg, "--version")) || (!strcmp(*arg, "-v")))
        {
            dbfi_version();
            return 0;
        }
#if defined(LIBTCC_FOUND)
        /* switch to compiler mode */
        else if ((!strcmp(*arg, "--compile")) || (!strcmp(*arg, "-c")))
        {
            compile = 1;
        }
        /* set output filename for compiler mode */
        else if ((!strcmp(*arg, "--output")) || (!strcmp(*arg, "-o")))
        {
            strncpy(output, dbfi_next_arg(&arg, argv + argc), sizeof(output));
        }
#endif /* LIBTCC_FOUND */
        /* other things starting with '-' are invalid options */
        else if ((*arg)[0] == '-')
        {
            fprintf(stderr, "Error: Unknown command line option. See --help for available options.\n");
            return 1;
        }
        /* the source filename */
        else
        {
            strncpy(filename, *arg, sizeof(filename));
        }
    }
    if (filename[0] == '\0')
    {
        fprintf(stderr, "Error: No filename of a brainfuck script has been given.\n");
        return 1;
    }
    return dbfi_main(filename, compile, output);
}
