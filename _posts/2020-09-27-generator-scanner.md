---
layout: post
author: oliver
lang: C
title: "Writing a code generation tool (Part 1): The Scanner"
---
Introduction and Scanner...

>Disclaimer: This is not the only or best way to do this. This is how I've done it. Maybe it helps with your 
project or maybe you get inspired to do something better.

For some time now I've been working on writing a game in C from scratch (using OpenGL and GLFW). Over the 
course of the development I often needed to call a function a certain number of times with different parameters 
to fill an array and access them afterwards by index. After having to painstakingly add new components and 
systems to my ecs I decided to write a small code generation tool to automate the process.

My first attempt, while producing the desired result, was rather bad and not very extensible. Nothing where I 
could add new features as I needed them. So i rewrote everything. 

For my second attempt I refined the syntax for the scripting language used to generate the code, with what I 
learned from my prior mistake. Futhermore I used some code (and knowledge) from Bob Nystrom's book 
[Crafting Interpreters](https://www.craftinginterpreters.com/).

----

## The Scripting Lanuage

Let's first talk about the scripting language. 

{% highlight c linenos %}
define ECS_LOADER_H
{% endhighlight %}

The first line of every script must be 'define' followed by an identifier. The identifier specifies the include 
guard for the generated C-file.

{% highlight c linenos %}
include
{
    "Ecs/Ecs.h",
    "Components/Animator.h",
    "Components/Transform.h",
    "Components/RigidBody.h",
    "Components/Sprite.h",
    "Components/Movement.h",
    "Components/Camera.h",
    "Components/Inventory.h",
    "Components/Interaction.h"
}
{% endhighlight %}

The inluce block indicates what files need to be included in the generated header file. This basically gets 
translated to a bunch of '#include' statements for all files in the block.

{% highlight c linenos %}
enum ComponentType
{
    [ COMPONENT_TRANSFORM,   sizeof(Transform),     NULL ],
    [ COMPONENT_RIGID_BODY,  sizeof(RigidBody),     NULL ],
    [ COMPONENT_MOVEMENT,    sizeof(Movement),      NULL ],
    [ COMPONENT_SPRITE,      sizeof(Sprite),        NULL ],
    [ COMPONENT_ANIMATOR,    sizeof(Animator),      AnimatorFree ],
    [ COMPONENT_CAMERA,      sizeof(Camera),        NULL ],
    [ COMPONENT_INVENTORY,   sizeof(Inventory),     InventoryFree ],
    [ COMPONENT_INTERACTION, sizeof(Interaction),   NULL ],
    [ COMPONENT_INTERACTOR,  sizeof(Interactor),    NULL ]
}
{% endhighlight %}

The enum is the core of the script. The 'enum' keyword is followed by the name of enum. The body of an enum is a 
collection of arrays (indicated by '[' and ']') encapsuled by braces. Each enum element of a name which corresponds 
with the name for the constant in the resulting C-enum, and an optional list of arguments for the generate function.

{% highlight c linenos %}
generate strings: ComponentType

generate RegisterComponents(Ecs* ecs): 
    ComponentType -> EcsRegisterComponent(ecs)
{% endhighlight %}

The 'generate' keyword is used to generate a function (or more). If 'generate' is followed by a specific keyword 
(for now only strings) a predefined function will be generated. E.g. 'generate strings' will generate two functions 
to allow the conversion from a string to an enum constant and back. If 'generate' is followed by an identifier the 
generator will generate a function (in this case the function declaration would be 'void RegisterComponents(Ecs* ecs);') 
which calls the function specified after the arrow operator ('->') for every element of the enum specified before the arrow.

In our example the generated function would be: 
{% highlight c linenos %}
void RegisterComponents(Ecs* ecs)
{
    EcsRegisterComponent(ecs, sizeof(Transform), NULL);
    EcsRegisterComponent(ecs, sizeof(RigidBody), NULL);
    EcsRegisterComponent(ecs, sizeof(Movement), NULL);
    EcsRegisterComponent(ecs, sizeof(Sprite), NULL);
    EcsRegisterComponent(ecs, sizeof(Animator), AnimatorFree);
    EcsRegisterComponent(ecs, sizeof(Camera), NULL);
    EcsRegisterComponent(ecs, sizeof(Inventory), InventoryFree);
    EcsRegisterComponent(ecs, sizeof(Interaction), NULL);
    EcsRegisterComponent(ecs, sizeof(Interactor), NULL);
}
{% endhighlight %}

>Note: A possible upgrade would be to specify via index which argument of the enum is needed in what order. But 
I had no need for that feature (yet).

----

## The Scanner

Before we can generate any code we need to split the script file into a list of tokens. This makes it easier to 
handle the script. To do so we need to scan the whole file and look for lexemes that correspond with the defined 
set of Tokens. For my requirements I needed the following types of token:

{% highlight c linenos %}
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

    /* keywords */
    TOKEN_ENUM,
    TOKEN_DEFINE,
    TOKEN_INCLUDE,
    TOKEN_GENERATE,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;
{% endhighlight %}

Be aware that these types are depended of your requirements and syntax choices.

To keep track of where we are in the file and what we are currently scanning we define the following struct to help us with 
that:

{% highlight c linenos %}
typedef struct
{
    const char* start;
    const char* current;
    int line;
} Scanner;
{% endhighlight %}

We use 'start' to point to the start of the lexeme we are currently scanning and 'current' to point to the character we are 
currently looking at. The 'line' keeps track of the line in the script where we are at, to help us with debugging.

We initialize the scanner by letting 'start' and 'current' point to the beginning of the script file. We should probably use 
a function to do that for us. We also need to set the line of the scanner to 1. 

{% highlight c linenos %}
void scanner_init(Scanner* scanner, const char* source)
{
    scanner->start = source;
    scanner->current = source;

    scanner->line = 1;
}
{% endhighlight %}

To scan the next token we define a function called 'scanner_get_next' which takes the scanner as argument and returns a Token.

{% highlight c linenos %}
Token scanner_get_next(Scanner* scanner)
{
    scanner_skip_whitespaces(scanner);
    
    scanner->start = scanner->current;

    if (!scanner->current[0]) return make_token(scanner, TOKEN_EOF);

    char c = scanner->current[0];
    scanner->current++;

    if (is_alpha(c)) return make_identifier(scanner);

    switch (c)
    {
    case ':': return make_token(scanner, TOKEN_COLON);
    case ',': return make_token(scanner, TOKEN_COMMA);
    case '*': return make_token(scanner, TOKEN_ASTERISK);

    case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
    case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
    case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
    case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
    case '[': return make_token(scanner, TOKEN_LEFT_BRACKET);
    case ']': return make_token(scanner, TOKEN_RIGHT_BRACKET);

    case '-':
        if (scanner_match(scanner, '>'))
            return make_token(scanner, TOKEN_ARROW);
        else
            return make_token_error(scanner, "Expected '>' after '-'.");

    case '"': return make_string(scanner);
    
    default:
        break;
    }

    return make_token_error(scanner, "Unexpected character.");
}
{% endhighlight %}

----

{% highlight c linenos %}
typedef struct
{
    TokenType type;

    const char* start;
    size_t len;

    int line;
} Token;
{% endhighlight %}

{% highlight c linenos %}
static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static Token make_token(const Scanner* scanner, TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.len = scanner->current - scanner->start;
    token.line = scanner->line;

    return token;
}

static Token make_token_error(const Scanner* scanner, const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.len = strlen(message);
    token.line = scanner->line;

    return token;
}

static Token make_string(Scanner* scanner)
{
    while(scanner->current[0] != '"')
    {
        if (scanner->current[0] == '\0' || scanner->current[0] == '\n')
            return make_token_error(scanner, "Unterminated string.");

        scanner->current++;
    }

    scanner->current++; /* closing quote */
    return make_token(scanner, TOKEN_STRING);
}

static TokenType check_keyword(const Scanner* scanner, const char* keyword, TokenType type)
{
    size_t len = strlen(keyword);
    if ((scanner->current - scanner->start == len) && memcmp(scanner->start, keyword, len) == 0)
        return type;

    return TOKEN_IDENTIFIER;
}

static TokenType get_identifier_type(const Scanner* scanner)
{
    switch (scanner->start[0])
    {
    case 'd': return check_keyword(scanner, "define", TOKEN_DEFINE);
    case 'e': return check_keyword(scanner, "enum", TOKEN_ENUM);
    case 'g': return check_keyword(scanner, "generate", TOKEN_GENERATE);
    case 'i': return check_keyword(scanner, "include", TOKEN_INCLUDE);
    }

    return TOKEN_IDENTIFIER;
}

static Token make_identifier(Scanner* scanner)
{
    while (is_alpha(scanner->current[0]) || is_digit(scanner->current[0])) 
        scanner->current++;

    return make_token(scanner, get_identifier_type(scanner));
}

static int scanner_match(Scanner* scanner, char c)
{
    if (!(scanner->current[0] && scanner->current[0] == c))
        return 0;

    scanner->current++;
    return 1;
}

static void scanner_skip_whitespaces(Scanner* scanner)
{
    while(1)
    {
        switch (scanner->current[0])
        {
        case ' ':
        case '\r':
        case '\t':
            scanner->current++;
            break;

        case '\n':
            scanner->line++;
            scanner->current++;
            break;

        /* Comments */
        case '#':
            while(scanner->current[0] && scanner->current[0] != '\n')
                scanner->current++;
        
        default:
            return;
        }
    }
}

Token scanner_get_next(Scanner* scanner)
{
    scanner_skip_whitespaces(scanner);
    
    scanner->start = scanner->current;

    if (!scanner->current[0]) return make_token(scanner, TOKEN_EOF);

    char c = scanner->current[0];
    scanner->current++;

    if (is_alpha(c)) return make_identifier(scanner);

    switch (c)
    {
    case ':': return make_token(scanner, TOKEN_COLON);
    case ',': return make_token(scanner, TOKEN_COMMA);
    case '*': return make_token(scanner, TOKEN_ASTERISK);

    case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
    case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
    case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
    case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
    case '[': return make_token(scanner, TOKEN_LEFT_BRACKET);
    case ']': return make_token(scanner, TOKEN_RIGHT_BRACKET);

    case '-':
        if (scanner_match(scanner, '>'))
            return make_token(scanner, TOKEN_ARROW);
        else
            return make_token_error(scanner, "Expected '>' after '-'.");

    case '"': return make_string(scanner);
    
    default:
        break;
    }

    return make_token_error(scanner, "Unexpected character.");
}

int token_cmp(Token* token, const char* str)
{
    return strncmp(token->start, str, token->len);
}

void print_token(Token* token)
{
    printf("%2d: %.*s\n", token->type, token->len, token->start);
}
{% endhighlight %}