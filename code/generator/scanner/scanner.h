#ifndef SCANNER_H
#define SCANNER_H

#include <stdlib.h>

typedef enum
{
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_ASTERISK,

    TOKEN_ARROW,

    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    
    /* literals */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    /* keywords */
    TOKEN_ENUM,
    TOKEN_DEFINE,
    TOKEN_INCLUDE,
    TOKEN_GENERATE,
    TOKEN_STRINGS,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

typedef struct
{
    TokenType type;

    const char* start;
    size_t len;

    int line;
} Token;

typedef struct
{
    const char* start;
    const char* current;
    int line;
} Scanner;

void scanner_init(Scanner* scanner, const char* source);

Token scanner_get_next(Scanner* scanner);

#endif /* !SCANNER_H */
