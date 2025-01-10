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

## Build (Compiles on Linux only, as of now... Working on making it Windows compatible but Windows Assembly is weird)
1. mkdir build
2. make
3. cd build

## Usage
./flexer.out main.c 

## Sample Output
```c

#define BENJAMIN 10

int main() {
  int x = BENJAMIN;
  return 0;
}
```

### Before preprocessing

│ 2: 1 ❬ OP→ #❭  0
│ 2: 2 ❬ KW→ define❭  1
│ 2: 9 ❬ ID→ identifer 「BENJAMIN 」❭  2
│ 2:18 ❬ INT→ immediate 「10 」❭  3
│ 4: 1 ❬ KW→ int❭  4
│ 4: 5 ❬ ID→ identifer 「main 」❭  5
│ 4: 9 ❬ OP→ (❭  6
│ 4:10 ❬ OP→ )❭  7
│ 4:12 ❬ OP→ {❭  8
│ 5: 3 ❬ KW→ int❭  9
│ 5: 7 ❬ ID→ identifer 「x 」❭ 10
│ 5: 9 ❬ OP→ =❭ 11
│ 5:11 ❬ ID→ identifer 「BENJAMIN 」❭ 12
│ 5:19 ❬ OP→ ;❭ 13
│ 6: 3 ❬ KW→ return❭ 14
│ 6:10 ❬ INT→ immediate 「0 」❭ 15
│ 6:11 ❬ OP→ ;❭ 16
│ 7: 1 ❬ OP→ }❭ 17

### After preprocessing

│ 2: 1 ❬ OP→ TOK_SKIP❭  0
│ 2: 2 ❬ OP→ TOK_SKIP❭  1
│ 2: 9 ❬ OP→ TOK_SKIP❭  2
│ 2:18 ❬ OP→ TOK_SKIP_IMM❭  3
│ 4: 1 ❬ KW→ int❭  4
│ 4: 5 ❬ ID→ identifer 「main 」❭  5
│ 4: 9 ❬ OP→ (❭  6
│ 4:10 ❬ OP→ )❭  7
│ 4:12 ❬ OP→ {❭  8
│ 5: 3 ❬ KW→ int❭  9
│ 5: 7 ❬ ID→ identifer 「x 」❭ 10
│ 5: 9 ❬ OP→ =❭ 11
│ 5:11 ❬ INT→ immediate 「10 」❭ 12
│ 5:19 ❬ OP→ ;❭ 13
│ 6: 3 ❬ KW→ return❭ 14
│ 6:10 ❬ INT→ immediate 「0 」❭ 15
│ 6:11 ❬ OP→ ;❭ 16
│ 7: 1 ❬ OP→ }❭ 17

# Parsing

Abstract Syntax Tree:
├── Node: 【Program 】
│   ├── Node: 【Function 】 ➜ main 📝int
│   │   ├── Node: 【Block 】
│   │   │   ├── Node: 【Variable 】 ➜ x 📝int
│   │   │   │   ├── Node: 【Immediate 】 ➜ 10
│   │   │   ├── Node: 【Return 】
│   │   │   │   ├── Node: 【Immediate 】 ➜ 0


