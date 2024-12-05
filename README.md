# C Lexer & Preprocessor 

A custom lexer and preprocessor written in C++ that handles tokenization and preprocessing of C source code.

## Features
- Token classification (identifiers, keywords, operators, literals)
- Macro expansion and definition (#define)
- Conditional compilation (#ifdef, #ifndef)
- Line continuation handling (by just mallocing the whole file lol)
- Comment removal (single-line and multi-line)
- Source location tracking
- Error reporting with line/column information

## Build (Compiles on Linux only)
1. mkdir build
2. make
3. cd build

## Usage
./flexer.out main.c 
                ^
              source file

## Sample Output
```c

#define BENJAMIN 10

int main() {
  int x = BENJAMIN;
  return 0;
}

```
(add it lol)

