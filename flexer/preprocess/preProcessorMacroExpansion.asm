%include "win-compatibility.inc"
default rel

TOK_IDENTIFIER equ 86
TOK_STRING_LITERAL equ 87
TOK_INTEGER_LITERAL equ 88
STRING_TOK_STRUCT_SIZE equ 16
TOK_EOF equ 92
TOK_SKIP  equ 89
TOK_SKIP_IMM  equ 90 ; still skip but expansion needs to know what type of value is here
TOK_SKIP_LIT equ 91 ; same as above but literal

section .bss
    currWorkingToken resq 1 ; to replace
    currWorkingMacro resq 1 ; to replace with

section .text
    global preProcessorMacroExpansion
    extern SrcTokenStream 
    extern SrcTokenStreamTokens ; address of pointer to Tokens
    extern ht_get 
    extern memcpy ; to copy literal structs
    extern free

    ; TokenStream
    ; 0-7   - Tokens
    ; 8-15  - tokenCapacity
    ; 16-23 - tokenIndex

    ; Token
    ; 0-7   - data
    ; 8-11  - TokenType
    ; 12-15 - line
    ; 16-19 - col
    ; 20-23 - PADDING

preProcessorMacroExpansion:
    PROLOGUE

    mov r15, rdi ; save preProcessHt pointer

    xor r12, r12 ; 0 out (r12 is COUNTER)

while_loop_start:
    mov rdi, r12 ; move counter
    imul rdi, 24 ; multiply by 24
    add rdi, [SrcTokenStreamTokens] ; add Tokens pointer 
    mov [currWorkingToken], rdi ; save for future use
    add rdi, 8 ; to tokenType addr
    mov edi, [rdi] ; get TokenType value

    cmp edi, TOK_EOF
    je while_loop_break ; end loop
    cmp edi, TOK_IDENTIFIER ; see if identifer
    jne while_loop_continue

    mov rsi, [currWorkingToken] ; move address of token
    mov rsi, [rsi] ; move key into 2nd arg
    mov rdi, r15 ; CONFIRM THIS IS WITH BRACKETS (1st arg)
    call ht_get

    cmp rax, 0
    je while_loop_continue ; not a macro
    ; rax is now a pointer to a token of tokenType TOK_SKIP_IMM or TOK_SKIP_LIT
    ; we need to replace the current IDENTIFER with the data and update the token type

    mov [currWorkingMacro], rax ; save the address of the macroToken

    ; BUT since we are overwriting this pointer, we need to free the memory here first ;; TODO
    mov rdi, [currWorkingToken]
    mov rdi, [rdi]
    call free

    mov rax, [currWorkingMacro]
    add rax, 8 ; get to tokenType
    mov r9d, dword [rax] ; move the 4 btye tokenType into r9d (4bytes)

    mov rdi, [currWorkingToken]
    mov rax, [currWorkingMacro]
    mov rax, [rax] ; derefrence data 
    mov [rdi], rax ; replace old token.data pointer with IMM or LITERAL pointer
    add rdi, 8 ; get to tokenType
    
    cmp r9d, TOK_SKIP_IMM
    je replaceImm
    cmp r9d, TOK_SKIP_LIT
    je replaceLiteral

    mov rax, 0 ; ERROR this should not happen
    jmp exit_func
replaceImm:
    mov dword [rdi], TOK_INTEGER_LITERAL ; update new tokenType    
    jmp while_loop_continue ; next iteration
replaceLiteral:
    mov dword [rdi], TOK_STRING_LITERAL ; update new tokenType
    jmp while_loop_continue
while_loop_continue:
    INC r12
    jmp while_loop_start
while_loop_break:

exit_func:
    mov rax, 1
    EPILOGUE