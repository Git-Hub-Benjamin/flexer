section .data
    noMemoryStr db 'No memory...', 0

TOK_DEFINE equ 33
TOK_INCLUDE equ 34
TOK_HASH equ 84
TOK_IDENTIFER equ 86
TOK_LITERAL equ 87
TOK_IMMEDIATE equ 88
TOK_TOTAL equ 89
TOK_SKIP  equ 90

section .bss
    preProcessHt resq 1 
    SrcTokenStreamTokens resq 1 ; pointer of tokens[?]
    currPreProcessToken resq 1 ; points to the curr #define ID IMM / LITERAL <- (the imm / literal token) 
    

section .text
    global hashPreprocessorDirectives
    global preProcessHt ; so expander can access
    global SrcTokenStreamTokens ; why not

    extern malloc ; amount of bytes
    extern printf ; print error
    extern ht_set ; pointer to hash table, pointer to char, pointer to val
    extern ht_get ; pointer to hash table, pointer to string
    extern ht_create ; default entries to initalize with -1 will set a defautl value
    extern keywordTokenConverter
    extern SrcTokenStream 

    ; TokenStream
    ; 0-7   - Tokens
    ; 8-15  - tokenCapacity
    ; 16-23 - tokenIndex

    ; Token
    ; 0-7   - data
    ; 8-11  - TokenType
    ; 12-15 - line
    ; 16-19 - col


hashPreprocessorDirectives:
    push rbp
    mov rbp, rsp

    push rbx ; callee saved register

    mov rbx, [SrcTokenStream] ; get SrcTokenStream.tokens
    mov qword [SrcTokenStreamTokens], SrcTokenStream 

    ; iterate over tokens, find a # with an define keyword following it , store the id and val following the #define into the preProcess hash table
    mov rdi, -1 ; default size for intial value
    call ht_create
    cmp rax, 0
    jne null_check_1

    ; null returned
    mov rdi, noMemoryStr
    call printf
    mov rax, 0 ; error val
    pop rbx
    pop rbp
    ret

null_check_1:

    mov [preProcessHt], rax ; move the preProcess hash table into this pointer
    mov rcx, 0 ; first Token in tokenStream

    ; Tokens[rcx].TokenType != TOK_TOTAL

token_while_loop:

    mov rdi, rcx ; get index 
    shl rdi, 4 ; correct the size to 16
    add rdi, [SrcTokenStreamTokens] ; get Tokens pointer
    add rdi, 8 ; get tokentype
    mov rax, [rdi] ; move actual val into rax
    cmp rax, TOK_TOTAL ; cmp TOT_TOTAL to current token.tokentype 
    je token_while_loop_end ; break;

    cmp rax, TOK_HASH ; see if hash
    jne token_while_loop_next_iter ; continue 

    add rdi, 16 ; get next index
    mov rax, [rdi] ; mov next index token type into rax
    cmp rax, TOK_DEFINE ; see if token define
    je include_define
    cmp rax, TOK_INCLUDE ; see if token include
    je include_define

    ; inncorrect usage of #
    mov rax, 0 ; fail
    jmp exit_func

include_define:
    add rdi, 16 ; get next index
    mov rax, [rdi] ; mov next index token type into rax
    cmp rax, TOK_IDENTIFER ; see if next token is identifer
    je include_define_identifer 
    ; inncorrect usage of #define / #include 
    mov rax, 0 ; fail
    jmp exit_func
include_define_identifer:
    add rdi, 16 ; get next index
    mov rax, [rdi] ; mov next index token type into rax
    cmp rax, TOK_IMMEDIATE ; see if next token is immediate
    je immediate_or_literal
    cmp rax, TOK_LITERAL ; see if token is literal
    je immediate_or_literal
    ; inncorrect usage of #define / #include identifer 
    mov rax, 0 ; fail
    jmp exit_func
immediate_or_literal:
    ; found valid #define IDENTIFER IMM or #define IDENTIFER LITERAL
    ; store in table then set all these tokens to TOK_SKIP

    sub rdi, 8 ; setup pointer
    mov [currPreProcessToken], rdi ; store address in here

    push rcx ; save rcx (caller saved register)

    mov rdi, 4 ; allocate 4 bytes 
    call malloc
    cmp rax, 0 ; see if null
    jne null_check_2

    ; null returned
    mov rdi, noMemoryStr
    call printf
    mov rax, 0 ; error val
    pop rcx
    pop rbx
    pop rbp
    ret    

null_check_2:

    mov rdi, [currPreProcessToken] ; restore this address

    ; 3rd arg void* val
    mov rcx, rdi

    ; 2nd arg char* key
    sub rdi, 16
    mov rsi, rdi

    ; 1st arg ht_table
    mov rdi, [preProcessHt] ; pointer to ht 1st arg for ht_set 

    call ht_set ; set this #define IDENTIFER LITERAL into hash table

    ; set #, define, id, val -> all to tokenType = TOK_SKIP
    xor rcx, rcx ; 0 out (we saved rcx above)
    mov rdx, [currPreProcessToken] ; get address of this back
    add rdx, 12 ; get to tokenType part
    mov rax, TOK_SKIP ; thing to fill
set_token_skip:
    cmp rcx, 3 ; amount to do (0 , 1 , 2 , 3 (4 times))
    je set_token_skip_out
    mov [rdx], rax ; move TOK_SKIP into curr token.tokentype
    sub rdx, 16 ; move to before token
    INC rcx ; rcx++
    jmp set_token_skip
set_token_skip_out:

    pop rcx ; doing this to offset push and pops, we are about to set it manually
    add rcx, 3 ; only 3 not 4 because we are about to increment anyways
token_while_loop_next_iter:
    inc rcx ; increment to next index
    jmp token_while_loop

token_while_loop_end:
    mov rax, 1 ; All went good

exit_func: ; only should be jmped to if stack is cleaned up and return code is set
    pop rbx ; callee saved
    pop rbp
    ret
