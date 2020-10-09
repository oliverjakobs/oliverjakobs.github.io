---
layout: post
title: "Writing a code generator (Part 1): Introduction"
author: oliver
lang: C
---
For some time now I've been working on writing a game in C from scratch (using OpenGL and GLFW). To make my life easier 
and to prevent some mistakes I have developed a generator and a custom made scripting language to create C-files. 

I think the whole process could be interesting to some people out there, so I decided to explain what I have done step 
by step in this and the next few posts. In this part I will give an overview of the language we will implement and give 
a general outlook what to expect.

During development I often needed to access specific array elements. The easiest way to do so is to define an enum 
which constants corresponds with the index of the element in the array. The only shortcoming of this method the manual, 
error prone process to fill the array.

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

## The Scripting Language

Before we can start programming we should first talk about _the language_. The single purpose of _the language_ is to 
tell the generator how the C file should look. So there is no need to support any arithmetic operations. As a result 
_the language_ is more of a markup language, that tells the generator how the C file should look.

>   I have no idea how to name this language so I'm just gonna call it _the language_, until I come up with something
    better.

The syntax and the features of _the language_ are heavilly dependent on the target language the generator should 
generate. During development of the generator _the language_ underwent many changes (mostly during the first attempt) 
and like every other programming language the scripting language will continue to be in (nearly) constant development. 
So _the language_ will mature and evolve with time, but for now it does everything I need.

Let's start with the syntax of _the language_. The first thing to know is _the language_ ignores whitespaces like 
newlines and tabs, so indentation doesn't matter and you can format the code whatever way you like the most. _The
language_ also supports comments. A comment starts with a '#' and continue to the end of the line.

>   For now the generator ignores comments, but it could be useful to just copy them into the header file.

In C (and C++) header files should have a way to avoid double inclusion and we want to stay standard (so no _#pragma 
once_), we need some way to define an include guard. And I dont think automatically generating the name for the guard 
is worth the effort. So the first line of every script must be _define_ followed by an identifier representing the 
name.

{% highlight c linenos %}
define ECS_LOADER_H
{% endhighlight %}

If you need to include header files into the generated code (which in most cases will be necessary) you can pack them 
all together into one _include_ statement. Inside the block the several include statements are represented by multiple 
strings seperated by commas. An include block could look something like this:

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

## The Core of _the language_ 

The core of _the language_ are enums and generate expressions. A new enum is defined with the keyword _enum_ followed 
by a name. They are the only thing close to a data type in _the language_. The body of an enum is a collection of arrays
encapsuled by braces. Arrays are comma separated values between brackets. And each of these arrays contains a name, 
which will be translated to a constant in the resulting C-enum, and an optional list of arguments for the generate 
function.

Enums are the only thing close to a data type. They are used to generate functions which is explained below.

{% highlight c linenos %}
enum ComponentType
{
    [ COMPONENT_TRANSFORM,  $SIZE(Transform),   NULL ],
    [ COMPONENT_RIGID_BODY, $SIZE(RigidBody),   NULL ],
    [ COMPONENT_ANIMATOR,   $SIZE(Animator),    AnimatorFree ],
    [ COMPONENT_SPRITE,     $SIZE(Sprite),      NULL ]
}
{% endhighlight %}

Each enum will automatically be translated into a C-enum. For everything else we need to use the _generate_ keyword. 


This keyword is used to generate functions and can be used in two different ways.

To use the enums for something more than just generating enum, I am going to introduce the _generate_ keyword. This 
keyword is used to generate functions and can be used in two different ways.

If _generate_ is followed by a specific keyword (for now only strings) a predefined function will be generated. E.g. 
_generate strings_ will generate two functions to allow the conversion from a string to an enum constant and back. 

{% highlight c linenos %}
generate strings: ComponentType
{% endhighlight %}

If _generate_ is followed by an identifier the generator will generate a function with the name given by the identifier 
which calls the function specified after the arrow operator ('->') for every element of the enum specified before the 
arrow. 

This is actually the feature that made me write this tool. Everythin else is just a bonus.

{% highlight c linenos %}
generate RegisterComponents(Ecs* ecs): ComponentType -> EcsRegisterComponent(ecs)
{% endhighlight %}

To fully understand how the _generate_ keyword works, let's just take a look at what the output for our example looks:

{% highlight c linenos %}
void RegisterComponents(Ecs* ecs)
{
    EcsRegisterComponent(ecs, sizeof(Transform), NULL);
    EcsRegisterComponent(ecs, sizeof(RigidBody), NULL);
    EcsRegisterComponent(ecs, sizeof(Animator), AnimatorFree);
    EcsRegisterComponent(ecs, sizeof(Sprite), NULL);
}
{% endhighlight %}

If there are still things unclear just wait untill we implement the generator (probably part 4).

>   A possibly useful upgrade would be a way to specify which argument of an enum element is needed in what order. 
    This would allow to hove multiple generate calls for one enum.

## Macros

The above example already uses another feature.
Macros are defined by '$' followed by an identifier.

Currently the only macro supported is _$SIZE_, which translates to the C-operator _sizeof_. But the way macros are 
implemented makes it pretty easy to add more.

Macros can have any number of arguments as long as they match the C-function they correspond to. 

Macros are just to have function calls as arguments in the enums.

The arguments of an macro are just getting copied to the the output file. So nested macros are not possible.

>   In a later version it will probably be possible for the user to define macros. But for now it is easier to just add 
    new macros manually.

## What's next

There are various ways this language and the generator could be improved. But these features are either too complicated 
or are simply not needed for now.

Thats all for this part. In the next part I will talk about implementing a scanner.

In the next part I will talk about how we can parse the script file into a more manageable format. I will explain the
concept of Tokens and I will show how I implemented a scanner to tokenize the script file.

If something was unclear or you have some (constructive) critisim, feel free to leave a comment or write me on 
[twitter](https://twitter.com/orwell_23).