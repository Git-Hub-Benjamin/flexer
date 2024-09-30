TOK_IDENTIFER equ 86
TOK_LITERAL equ 87
TOK_IMMEDIATE equ 88
STRING_TOK_STRUCT_SIZE equ 16

section .bss
    currWorkingToken resq 1

section .text
    global preProcessorMacroExpansion
    extern SrcTokenStream 
    extern SrcTokenStreamTokens ; address of pointer to Tokens
    extern ht_get 
    extern memcpy ; to copy literal structs
    extern preProcessHt

    ; TokenStream
    ; 0-7   - Tokens
    ; 8-15  - tokenCapacity
    ; 16-23 - tokenIndex

    ; Token
    ; 0-7   - data
    ; 8-11  - TokenType
    ; 12-15 - line
    ; 16-19 - col

preProcessorMacroExpansion:
    push rbp
    mov rbp, rsp

    xor r11, r11 ; 0 out (R11 is COUNTER)

while_loop_start:
    mov rdi, r11 ; move counter
    shl rdi, 4 ; mutiply 16
    add rdi, 8 ; to tokenType addr
    add rdi, [SrcTokenStreamTokens] ; add Tokens pointer 
    mov [currWorkingToken], r10 ; save for future use
    mov rdi, [rdi] ; get TokenType value
    cmp rdi, TOK_IDENTIFER ; see if identifer
    jne while_loop_continue

    mov r10, [currWorkingToken] ; restore to r10
    add r10, 4 ; get token num
    shl r10, 4 ; mutiply 16
    add rsi, r10 ; get specific attrToken (2nd arg)

    mov rdi, [preProcessHt] ; CONFIRM THIS IS WITH BRACKETS (1st arg)
    call ht_get

    cmp rax, 0
    je while_loop_continue ; not a macro
    ; rax is now a pointer to an int that is an index into the tokens[?] which then that tokenNum -> attrTokens -> data is the data we need to replace this curr token with
    mov rax, [rax] ; get index
    mov rdi, [SrcTokenStreamTokens] ; get tokens pointer
    shl rax, 4 ; mutiply 16
    add rax, 12 ; get to tokenNum
    add rdi, rax ; tokens[?].tokenNum
    mov rax, [rdi] ; tokenNum
    shl rax, 4 ; mutiply 16
    add rdi, rax ; address of attrTokens[?].data

    mov rsi, [currWorkingToken] ; get address of the pointer to the token
    add rsi, 12 ; get tokenNum address
    mov rsi, [rsi] ; derefrence
    shl rsi, 4 ; mutiply by 16
    add rsi, rax ; get address of pointer to attrToken.data
    mov rsi, [rsi] ; rsi contains the pointer to the data to replace 

    mov r9, rdi ; rdi contains the address of attrTokens[?].data where the data will be taken from 
    add r9, 12
    mov r9, [r9]
    cmp r9, TOK_IMMEDIATE
    je expandImmediate
    cmp r9, TOK_LITERAL
    je expandLiteral

    mov rax, 0 ; ERROR this should not happen
    jmp exit_func
expandImmediate:
    ; right now only support signed ints (4 bytes)
    mov edi, dword [rdi] ; (MIGHT HAVE TO DEREFRENCE AGAIN?)
    mov dword [rsi], edi ; replace num 
    jmp while_loop_continue ; next iteration
expandLiteral:
    mov rdi, [rdi] ; (MIGHT HAVE TO DEREFRENCE AGAIN?)
    mov r9, rdi ; tmp
    mov rdi, rsi ; rdi needs to be dst (data to replace)
    mov rsi, r9  ; rsi need to be src (data to come from)
    mov rcx, STRING_TOK_STRUCT_SIZE ; memcpy size
    call memcpy
    jmp while_loop_continue
while_loop_continue:
    INC r11
    jmp while_loop_start
while_loop_break:

    ; iterate over all tokens until TOK_TOTAL
        ; if IDENTIFER replace attrToken.data with ht_get(HT, IDENTIFER) data

exit_func:

    pop rbp
    ret