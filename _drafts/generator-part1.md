---
layout: post
title: "Writing a code generation tool (Part 1): Introduction"
author: oliver
lang: C
---
For some time now I've been working on writing a game in C from scratch (using OpenGL and GLFW). During development I 
often needed to access specific array elements. The easiest way to do so is to define an enum which constants 
corresponds with the index of the element in the array. The only shortcoming of this method the manual, error prone 
process to fill the array.

To register a new component type to the ECS I have to call a function with component specific argmunets. So the order 
in which the components are added has to match the order defined in the enum which contains all component types. Doing 
so manual is tedious and error prone. Every time I want to register a new component type the type needs to be added to 
the enum and the array insertion needs to happen in the right order. A far better way is to automate the process of 
adding new components and let a code generator do the work.

The main idea is that the enum constant and the arguments for the insert function are stored in the same array 
and the tool generates a enum and a function that fills the array in the right order. Additionally the tool can generate
functions to convert enum constants to strings and back for serialisation purposes.

My first attempt, while producing the desired result, was rather bad and not very extensible. Nothing where I could 
add new features as I needed them. So I rewrote everything. For my second attempt I refined the syntax for the 
scripting language used to generate the code, with what I learned from my prior mistake. And with the help of Bob
Nystrom's book [Crafting Interpreters](https://www.craftinginterpreters.com/) I managed to create a program that 
satisfied my requirements.

{% include disclaimer.md %}

Since this tool is written for my game which is written in C the language this tool will generate to will be C. But 
if you need to generate code for another language the changes you need to make to this tool are mostly trivial, as long 
as the language supports some kind of enums.

The whole process will be split into serveral parts the keep every post to a managable length.

## The Scripting Language

Before we can start programming we should first talk about the scripting language. 

The generator is basically a transpiler, that takes in a custom made language and translate it to a _real_ programming 
language (in my case C).

A transpiler is pretty similar to a compiler, but instead of producing machine code as output the transpiler outputs a
different programming lanuage.

If you want to transpile into another language the syntax of the scripting language may need to change, but I think as 
long as the language supports some kind of enums these changes are trivial.

The language needs to fullfill a distinctive set of requirements which are heavilly dependent on the target language
the generator should transpile to.

The language is highly specified and underwent many changes as I was developing the generator (mostly during the first 
attempt) and I do not think this is the final version of the language. But it does what I want for now, and thats all 
that really matters. 

Like every other programming language the scripting language will be in (nearly) constant development. So the language 
will mature and evolve with time.

The language ignores whitespaces like newlines and tabs, so how you want to format the code is up to you.

Comments starts with a '#' and continue to the end of the line.

>   For now the transpiler ignores comments, but it could be useful to just copy them to the C-file.

Since the transpiler just translates the script into header and source file, the language does not support any arithmetic 
operations.

Since header files in C (and C++) should have a way to avoid double inclusion and we want to stay standard (so no 
_#pragma once_), we need some way to define an include guard. I dont think automatically generating the name for the 
guard is worth the effort. So the first line of every script must be 'define' followed by an identifier representing 
the name.

{% highlight c linenos %}
define ECS_LOADER_H
{% endhighlight %}

The next feature most files will need is a way to include files in the generated code files. Since often you need to 
include multiple files I decided to pack all include statements into a block the include block encapsuled by braces. 
Inside the block the several include statements are represented by multiple strings seperated by commas. An include 
block could look something like this:

{% highlight c linenos %}
include
{
    "Ecs/Ecs.h"
    "Components/Transform.h",
    "Components/RigidBody.h",
    "Components/Animator.h",
    "Components/Sprite.h"
}
{% endhighlight %}

## Enums 

Considering that the generator is meant to generate enums and do something with them we need a way to define an enum 
with constants and arguments. A new enum is defined with the keyword 'enum' followed by a name.

Enums are the only thing close to a data type. They are used to generate functions which is explained below.

The body of an enum is a collection of arrays encapsuled by braces. 

Arrays are comma separated values between brackets.

Each enum element contains an a name which will be translated to a constant in the resulting C-enum, and an optional 
list of arguments for the generate function.

{% highlight c linenos %}
enum ComponentType
{
    [ COMPONENT_TRANSFORM,  $SIZE(Transform),   NULL ],
    [ COMPONENT_RIGID_BODY, $SIZE(RigidBody),   NULL ],
    [ COMPONENT_ANIMATOR,   $SIZE(Animator),    AnimatorFree ],
    [ COMPONENT_SPRITE,     $SIZE(Sprite),      NULL ]
}
{% endhighlight %}

## Macros

The above example already uses another feature.
Macros are defined by '$' followed by an identifier.

Currently the only macro supported is _$SIZE_, which translates to the C-operator _sizeof_. But the way macros are implemented
makes it pretty easy to add more.

Macros can have any number of arguments as long as they match the C-function they correspond to. 

The arguments of an macro are just getting copied to the the output file. So nested macros are not possible.

>   In a later version it will probably be possible for the user to define macros. But for now it is easier to just add new macros manually.

## Generate functions

To do something with the enums 

The 'generate' keyword is used to generate functions. 

If 'generate' is followed by a specific keyword (for now only strings) a predefined function will be generated. E.g. 
generate strings' will generate two functions to allow the conversion from a string to an enum constant and back. 

{% highlight c linenos %}
generate strings: ComponentType
{% endhighlight %}

If 'generate' is followed by an identifier the generator will generate a function (in our case the function declaration 
would be 'void RegisterComponents(Ecs* ecs);') which calls the function specified after the arrow operator ('->') for 
every element of the enum specified before the arrow.

{% highlight c linenos %}
generate RegisterComponents(Ecs* ecs): ComponentType -> EcsRegisterComponent(ecs)
{% endhighlight %}

In our example the generated function would be: 

{% highlight c linenos %}
void RegisterComponents(Ecs* ecs)
{
    EcsRegisterComponent(ecs, sizeof(Transform), NULL);
    EcsRegisterComponent(ecs, sizeof(RigidBody), NULL);
    EcsRegisterComponent(ecs, sizeof(Animator), AnimatorFree);
    EcsRegisterComponent(ecs, sizeof(Sprite), NULL);
}
{% endhighlight %}

>   A possibly useful upgrade would be a way to specify which argument of an enum element is needed in what order. 
    This would allow to hove multiple generate calls for one enum.

## What's next

There are various ways this language and the generator could be improved. But these features are either too complicated 
or are simply not needed for now.

Thats all for this part. In the next part I will talk about implementing a scanner.

How we can parse the script file into a more manageable format.

