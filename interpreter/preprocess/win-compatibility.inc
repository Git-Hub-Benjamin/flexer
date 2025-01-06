%ifdef __WIN32__ 
    %define WINDOWS 1
%elifdef __WIN64__
    %define WINDOWS 1
%else
    %define WINDOWS 0
%endif

; Prologue macro
%if WINDOWS
%macro PROLOGUE 0
    push rbp
    mov rbp, rsp
    push rdi        ; Windows Callee-saved registers
    push rsi
    push rbx
    push r15
    sub rsp, 32     ; Windows shadow space
    mov r15, rcx
%endmacro
%else
%macro PROLOGUE 0
    push rbp
    mov rbp, rsp
    mov r15, rdi
%endmacro
%endif

; Matching epilogue macro
%if WINDOWS
%macro EPILOGUE 0
    add rsp, 32     ; Remove shadow space
    pop r15
    pop rbx
    pop rsi
    pop rdi
    pop rbp
    ret
%endmacro
%else
%macro EPILOGUE 0
    pop rbp
    ret
%endmacro
%endif