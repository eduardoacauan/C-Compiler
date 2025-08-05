#include "compiler.h"

int main(int argc, char **argv)
{
    if(argc < 2) {
        fprintf(stderr, "File expected\n");
        return EXIT_FAILURE;
    }

    boot(argc, argv);
    
    zfree();
    return EXIT_SUCCESS;
}
