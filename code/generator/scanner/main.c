#include <stdio.h>
#include <string.h>

#include "scanner.h"

int main(int argc, char** args)
{
    FILE* file = fopen("test_script.cx", "rb");
    if (!file) return 1;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = calloc(size + 1, 1);

    if (!buffer || fread(buffer, 1, size, file) != size)
        return 1;

    fclose(file);

    Scanner scanner;
    scanner_init(&scanner, buffer);

    while (1)
    {
        Token token = scanner_get_next(&scanner);
        if (token.type == TOKEN_EOF) break;

        printf("%2d: %.*s\n", token.type, token.len, token.start);
    }

    free(buffer);

    return 0;
}