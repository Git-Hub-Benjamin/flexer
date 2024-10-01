section .data
    noMemoryStr db 'No memory...', 0

TOK_DEFINE equ 33
TOK_INCLUDE equ 34
TOK_HASH_A equ 84
TOK_IDENTIFER equ 86
TOK_LITERAL equ 87
TOK_IMMEDIATE equ 88
TOK_TOTAL equ 92
TOK_SKIP  equ 89
TOK_SKIP_IMM  equ 90 ; still skip but expansion needs to know what type of value is here
TOK_SKIP_LIT equ 91 ; same as above but literal

section .bss
    SrcTokenStreamTokens resq 1 ; pointer of tokens[?]
    SrcTokenStreamTokens2 resq 1 ; debug
    currPreProcessToken resq 1 ; points to the curr #define ID IMM / LITERAL <- (the imm / literal token) 
    

section .text
    global hashPreprocessorDirectives
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
    ; 20-23 - (PADDING)


hashPreprocessorDirectives:
    push rbp
    mov rbp, rsp

    mov r15, rdi ; save the htTable address

    mov rbx, [SrcTokenStream]
    mov qword [SrcTokenStreamTokens], rbx 
    add rbx, 24
    mov [SrcTokenStreamTokens2], rbx

    ; iterate over tokens, find a # with an define keyword following it , store the id and val following the #define into the preProcess hash table

    mov rcx, 0 ; first Token in tokenStream

    ; Tokens[rcx].TokenType != TOK_TOTAL

token_while_loop:

    mov rdi, rcx ; get index 
    imul rdi, 24 ; correct the size to 16
    add rdi, [SrcTokenStreamTokens] ; get Tokens pointer
    add rdi, 8 ; get tokentype
    mov eax, [rdi] ; move actual val into rax
    cmp eax, TOK_TOTAL ; cmp TOT_TOTAL to current token.tokentype 
    je token_while_loop_end ; break;

    cmp rax, TOK_HASH_A ; see if hash
    jne token_while_loop_next_iter ; continue 

    add rdi, 24 ; get next index
    mov eax, [rdi] ; mov next index token type into rax
    cmp eax, TOK_DEFINE ; see if token define
    je include_define
    cmp eax, TOK_INCLUDE ; see if token include
    je include_define

    ; inncorrect usage of #
    mov rax, 0 ; fail
    jmp exit_func

include_define:
    add rdi, 24 ; get next index
    mov eax, [rdi] ; mov next index token type into rax
    cmp eax, TOK_IDENTIFER ; see if next token is identifer
    je include_define_identifer 
    ; inncorrect usage of #define / #include 
    mov rax, 0 ; fail
    jmp exit_func
include_define_identifer:
    add rdi, 24 ; get next index
    mov eax, [rdi] ; mov next index token type into rax
    cmp eax, TOK_IMMEDIATE ; see if next token is immediate
    je immediate_or_literal
    cmp eax, TOK_LITERAL ; see if token is literal
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
    pop rbp
    ret    

null_check_2:

    mov rdi, [currPreProcessToken] ; restore this address

    ; 3rd arg void* val
    mov rdx, rdi ; move the address of the token

    ; 2nd arg char* key
    sub rdi, 24
    mov rsi, [rdi]

    ; 1st arg ht_table
    mov rdi, r15 ; pointer to ht 1st arg for ht_set 
    call ht_set ; set this #define IDENTIFER LITERAL into hash table

    ; set #, define, id, val -> all to tokenType = TOK_SKIP
    xor rcx, rcx ; 0 out (we saved rcx above)
    mov rdx, [currPreProcessToken] ; get address of this back
    add rdx, 8 ; get to tokenType part
set_token_skip:
    cmp rcx, 4 ; amount to do (0 , 1 , 2 , 3 (4 times))
    mov eax, [rdx] ; move the tokentype into eax
    je set_token_skip_out
    cmp eax, TOK_IMMEDIATE
    je set_tok_imm
    cmp eax, TOK_LITERAL
    je set_tok_lit
    jmp set_tok_skip
set_tok_lit:
    mov eax, TOK_SKIP_LIT
    jmp set_token
set_tok_imm:
    mov eax, TOK_SKIP_IMM
    jmp set_token
set_tok_skip:
    mov eax, TOK_SKIP ; thing to fill
set_token:
    mov [rdx], eax ; move TOK_SKIP into curr token.tokentype
    sub rdx, 24 ; move to before token
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
    pop rbp
    ret
