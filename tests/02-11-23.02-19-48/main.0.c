#include "config.h"

#include <stdio.h>
#include <string.h>

void dbfi_help(char * app)
{
    printf(
        "Usage: %s {options} [filename]\n\n"
        "Available options:\n"
        "  --help,-?               - Show this help message\n"
        "  --version,-v            - Show version information\n"
        "  --compile,-c            - Compile to native program instead of interpreting\n"
        "  --output,-o  [filename] - Alternative output filename for --compile\n"
    , app);
}

void dbfi_version(void)
{
    printf(
        "Dragon BrainFuck Interpreter (dbfi) Version " DBFI_VERSION "\n"
        "Copyright (C) 2013, Felix Bytow <felix.bytow@googlemail.com>\n\n"
        "Git-Hash:   " DBFI_GIT_HASH "\n"
        "Build-Type: " DBFI_BUILD_TYPE "\n"
    );
}

int main(int argc, char ** argv)
{
    char ** arg;
    
    for (arg = argv + 1; arg != argv + argc; ++argv)
    {
        /* show help message */
        if ((!strcmp(*arg, "--help")) || (!strcmp(*arg, "-?")))
        {
            dbfi_help(argv[0]);
            return 0;
        }
        
        if ((!strcmp(*arg, "--version")) || (!strcmp(*arg, "-v")))
        {
            dbfi_version();
            return 0;
        }
    }
}
