#include "scanner.h"

#include <stdio.h>
#include <string.h>

static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static Token create_token(const Scanner* scanner, TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.len = scanner->current - scanner->start;
    token.line = scanner->line;

    return token;
}

static Token create_token_error(const Scanner* scanner, const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.len = strlen(message);
    token.line = scanner->line;

    return token;
}

static Token create_string(Scanner* scanner)
{
    while(scanner->current[0] != '"')
    {
        if (!scanner->current[0] || scanner->current[0] == '\n')
            return create_token_error(scanner, "Unterminated string.");

        scanner->current++;
    }

    scanner->current++; /* closing quote */
    return create_token(scanner, TOKEN_STRING);
}

static Token create_number(Scanner* scanner)
{
    while (is_digit(scanner->current[0])) scanner++;

    /* Look for a fractional part. */
    if (scanner->current[0] && scanner->current[0] == '.' && is_digit(scanner->current[1]))
    {
        scanner++;
        while (is_digit(scanner->current[0])) scanner++;
    }

    return create_token(scanner, TOKEN_NUMBER);
}

static TokenType check_keyword(const Scanner* scanner, const char* keyword, TokenType type)
{
    size_t len = strlen(keyword);

    if ((scanner->current - scanner->start == len) && memcmp(scanner->start, keyword, len) == 0)
        return type;

    return TOKEN_IDENTIFIER;
}

static Token create_identifier(Scanner* scanner)
{
    while (is_alpha(scanner->current[0]) || is_digit(scanner->current[0])) 
        scanner->current++;

    TokenType identifier_type = TOKEN_IDENTIFIER;
    switch (scanner->start[0])
    {
    case 'd': identifier_type = check_keyword(scanner, "define", TOKEN_DEFINE); break;
    case 'e': identifier_type = check_keyword(scanner, "enum", TOKEN_ENUM); break;
    case 'g': identifier_type = check_keyword(scanner, "generate", TOKEN_GENERATE); break;
    case 'i': identifier_type = check_keyword(scanner, "include", TOKEN_INCLUDE); break;
    case 's': identifier_type = check_keyword(scanner, "strings", TOKEN_STRINGS); break;
    }

    return create_token(scanner, identifier_type);
}

void scanner_init(Scanner* scanner, const char* source)
{
    scanner->start = source;
    scanner->current = source;

    scanner->line = 1;
}

static int scanner_match(Scanner* scanner, char c)
{
    if (scanner->current[0] && scanner->current[0] == c)
    {
        scanner->current++;
        return 1;
    }
    
    return 0;
}

static void scanner_skip_whitespaces(Scanner* scanner)
{
    while(1)
    {
        switch (scanner->current[0])
        {
        case '\n':
            scanner->line++;
        case ' ':
        case '\r':
        case '\t':
            scanner->current++;
            break;

        /* Comments */
        case '#':
            while(scanner->current[0] && scanner->current[0] != '\n')
                scanner->current++;
            break;

        default:
            return;
        }
    }
}

Token scanner_get_next(Scanner* scanner)
{
    scanner_skip_whitespaces(scanner);
    scanner->start = scanner->current;

    char c = scanner->current[0];
    if (!c) return create_token(scanner, TOKEN_EOF);

    scanner->current++;

    if (is_alpha(c)) return create_identifier(scanner);
    if (is_digit(c)) return create_number(scanner);

    switch (c)
    {
    /* checking for single character token */
    case ':': return create_token(scanner, TOKEN_COLON);
    case ',': return create_token(scanner, TOKEN_COMMA);
    case '*': return create_token(scanner, TOKEN_ASTERISK);
    case '(': return create_token(scanner, TOKEN_LEFT_PAREN);
    case ')': return create_token(scanner, TOKEN_RIGHT_PAREN);
    case '{': return create_token(scanner, TOKEN_LEFT_BRACE);
    case '}': return create_token(scanner, TOKEN_RIGHT_BRACE);
    case '[': return create_token(scanner, TOKEN_LEFT_BRACKET);
    case ']': return create_token(scanner, TOKEN_RIGHT_BRACKET);

    /* checking for double character token */
    case '-':
        if (scanner_match(scanner, '>'))
            return create_token(scanner, TOKEN_ARROW);
        else
            return create_token_error(scanner, "Expected '>' after '-'.");

    case '"': return create_string(scanner);
    }

    return create_token_error(scanner, "Unexpected character.");
}