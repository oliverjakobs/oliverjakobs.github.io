---
layout: post
title: "Writing a code generation tool (Part 1): The Scanner"
author: oliver
lang: C
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

Let's first talk about the scripting language. The language underwent many changes as I was developing the generator
(mostly during the first attempt) and I do not think this is the final version of the language. But it does what I want 
for now, and thats all that really matters. 

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
generator will generate a function (in our case the function declaration would be 'void RegisterComponents(Ecs* ecs);') 
which calls the function specified after the arrow operator ('->') for every element of the enum specified before the 
arrow.

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

There are various ways this language and the generator could be improved. But these features are either too complicated 
or are simply not needed for now. A possible upgrade would be to specify via index which argument of the enum is needed 
in what order. 

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
    TOKEN_EOF   /* end of file */
} TokenType;
{% endhighlight %}

Be aware that these types are depended of your requirements and syntax choices. The types 'TOKEN_ERROR' and 'TOKEN_EOF' 
are probably useful for every project to identify the end of the file and to help find mistakes in the script file.

To keep track of where we are in the file and what we are currently scanning we define the following struct to help us 
with that:

{% highlight c linenos %}
typedef struct
{
    const char* start;
    const char* current;
    int line;
} Scanner;
{% endhighlight %}

We use 'start' to point to the start of the lexeme we are currently scanning and 'current' to point to the character we 
are currently looking at. The 'line' keeps track of the line in the script where we are at, to help us with debugging.

We initialize the scanner by letting 'start' and 'current' point to the beginning of the script file. We should 
probably use a function to do that for us. We also need to set the line of the scanner to 1. 

{% highlight c linenos %}
void scanner_init(Scanner* scanner, const char* source)
{
    scanner->start = source;
    scanner->current = source;

    scanner->line = 1;
}
{% endhighlight %}

The heart of the scanner lies in the scan function, which takes the scanner as argument and returns a Token. The 
function steps through the script file and searches for patterns which matches the various token types. Once the 
scanner found such a token it creates a token which contains the type, a pointer to the start of the token's lexeme in 
the script and the length of the lexeme. The token also contains the number of the line where it was found for 
debugging purposes.

{% highlight c linenos %}
typedef struct
{
    TokenType type;

    const char* start;
    size_t len;

    int line;
} Token;
{% endhighlight %}

Such tokens are stored in an array and will be used by the generator to decide what code to generate. 

The full scan function is rather complex, so we will break it down to more managable pieces. The simplest version of 
the function would be this:

{% highlight c linenos %}
Token scanner_get_next(Scanner* scanner)
{
    char c = scanner->current[0];
    if (!c) return create_token(scanner, TOKEN_EOF);

    scanner->current++;

    return create_token_error(scanner, "Unexpected character.");
}
{% endhighlight %}

With this function we check if we have reached the end of the file (by checking if 'c' is '\0'). If we are indeed at 
the end, we create a token with the type 'TOKEN_EOF' and if not we return an error token with an according error message.

The creation of these tokens (and other simple token) is pretty straightforward. 

{% highlight c linenos %}
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
{% endhighlight %}

The next step on our way to the full scan function is the creation of single and double character tokens. To do so we 
add a switch statement to check if the current character matches a token. After we added the switch to the function we 
get something like this:

{% highlight c linenos %}
Token scanner_get_next(Scanner* scanner)
{
    char c = scanner->current[0];
    if (!c) return create_token(scanner, TOKEN_EOF);

    scanner->current++;

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
    }

    return create_token_error(scanner, "Unexpected character.");
}
{% endhighlight %}

To scan double character token we need a small helper function, that checks if the current character matches a specific 
character and is not '\0'. If there is a match we advance the scanner by one character. Since the scripting language 
does not support arithmetic operators, we return an error token if a hyphen is not followed by a greater-than sign.

{% highlight c linenos %}
static int scanner_match(Scanner* scanner, char c)
{
    if (scanner->current[0] && scanner->current[0] == c)
    {
        scanner->current++;
        return 1;
    }
    return 0;
}
{% endhighlight %}

### Whitespaces and comments

Currently our scanner is creating a token (more specifically an error token) for every space, tab, and newline. But we 
do not care for these since they are not part of any token's lexeme. So at the start of the scan function we should 
skip every leading whitespace. While we are at it we should possibly skip comments to. A comment starts with an '#' and 
goes until the end of the line.

{% highlight c linenos %}
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
{% endhighlight %}

If we now call this function before we set the start of our scanner we will get all the token while avoiding all 
whitespace and comments. 

### Strings

With comments and whitespaces out of the way, we can continue with adding more token we can scan. The next type we want 
to scan is strings. A string starts and ends with '"' everthing in between is part of the string. For simplicity's sake 
we will not handle multiline strings. So if we find '\0' or '\n' before we reach the closing quote we want to return an 
error token indictating an unterminated was found. If we find a valid string we create a token which contains the 
string including both quotes.

{% highlight c linenos %}
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
{% endhighlight %}

To add this function to our scan function we just need to add another case (for '"') to the switch statement.

### Numbers

The only thing we are missing now is the scanning of numbers, identifiers and keywords. We will start with numbers, 
since they are easier. To look for numbers we first need to define a small helper function to check if a character is a
digit.

{% highlight c linenos %}
static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}
{% endhighlight %}

With the help of this function we can scan numbers. To do so we need to advance the scanner while the current character
is still a number. If the next character is a dot, we need to scan the fractional part as well, which is basically the 
same as the first part of this function. Put that all together and the result should look something like this:

{% highlight c linenos %}
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
{% endhighlight %}

To add this to our scan function we just need to check if our current character is a digit before we enter the switch.

### Identifiers and keywords

Now to identifiers and keywords. Since Keywords are "just" special, reserved indetifiers we can check for both in one function. First we scan the whole identifier, and since we allow numbers in our identifiers we need to check for both 
letters (and underscores) and digits. Like for the numbers we should define a helper function to check for letters.

{% highlight c linenos %}
static int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}
{% endhighlight %}

Additionally we need an another helper function to check if the identifier we scanned is a specific keyword. Basically 
its just a fancy wrapper for 'memcmp'. 

{% highlight c linenos %}
static TokenType check_keyword(const Scanner* scanner, const char* keyword, TokenType type)
{
    size_t len = strlen(keyword);

    if ((scanner->current - scanner->start == len) && memcmp(scanner->start, keyword, len) == 0)
        return type;

    return TOKEN_IDENTIFIER;
}
{% endhighlight %}

With the help of these two functions we can now create the 'create_identifier' function. Since we don't want to compare 
every single identifier with every possible keyword, we will use a switch statement where we will look only at the 
first character of each identifier. If it matches the first character of a keyword we will check the rest of the 
identifier. If it doesn't we just return an identifier token since it can't be any keyword.

{% highlight c linenos %}
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
{% endhighlight %}


----

If we now put all of the above together the scan function should look something like this: 

{% highlight c linenos %}
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
{% endhighlight %}

The complete code can be found on github 
([header](https://github.com/oliverjakobs/code-generator/blob/master/generator/src/scanner.h) and 
[source](https://github.com/oliverjakobs/code-generator/blob/master/generator/src/scanner.c))