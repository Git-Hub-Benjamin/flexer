# Detect OS
ifeq ($(OS),Windows_NT)
    # Windows detection that works in PowerShell
    WIN_VER := $(shell systeminfo | findstr /B /C:"OS Version:")
    ifneq (,$(findstring 22H2,$(WIN_VER)))
        # Windows 11
        CC = gcc
        NASM_FLAGS = -g -f win64 -i ./interpreter/preprocess/ -F cv8
        OBJ_EXT = .o
        LINKER = gcc -g
        EXE_EXT = .exe
    else
        # Windows 10
        CC = clang 
        NASM_FLAGS = -g -f win64 -i ./interpreter/preprocess/ -F cv8
        OBJ_EXT = .obj
        LINKER = clang -m64 -g -gcodeview -fuse-ld=lld
        EXE_EXT = .exe
    endif
else
    # Linux
    CC = clang
    NASM_FLAGS = -g -f elf64
    OBJ_EXT = .o
    LINKER = clang -no-pie
    EXE_EXT = .out
    EXTRA_LIBS = -lm
endif

# Common variables
BUILD_DIR = build
SRC_DIR = interpreter
HASH_DIR = hash
TARGET = flexer$(EXE_EXT)

# Common compiler flags
CFLAGS = -g -gcodeview -Z7 -D_CRT_SECURE_NO_WARNINGS
    
# Build rules
all: $(TARGET)

$(TARGET): compile-c-lexer
	nasm $(NASM_FLAGS) -o ./$(BUILD_DIR)/hashPreprocessorDirectives$(OBJ_EXT) ./$(SRC_DIR)/preprocess/hashPreprocessorDirectives.asm
	nasm $(NASM_FLAGS) -o ./$(BUILD_DIR)/preProcessorMacroExpansion$(OBJ_EXT) ./$(SRC_DIR)/preprocess/preProcessorMacroExpansion.asm
	$(LINKER) ./$(BUILD_DIR)/flexer$(OBJ_EXT) ./$(BUILD_DIR)/hash$(OBJ_EXT) ./$(BUILD_DIR)/hashPreprocessorDirectives$(OBJ_EXT) ./$(BUILD_DIR)/preProcessorMacroExpansion$(OBJ_EXT) -o $(TARGET) $(EXTRA_LIBS)

compile-c-lexer:
	$(CC) $(CFLAGS) -c ./$(SRC_DIR)/flexer.c -o ./$(BUILD_DIR)/flexer$(OBJ_EXT)
	$(CC) $(CFLAGS) -c ./$(HASH_DIR)/hash.c -o ./$(BUILD_DIR)/hash$(OBJ_EXT)

clean:
	powershell Remove-Item -Recurse -Force $(BUILD_DIR)
	mkdir $(BUILD_DIR)

info:
	@echo "Detected configuration:"
	@echo "Compiler: $(CC)"
	@echo "NASM flags: $(NASM_FLAGS)"
	@echo "Object extension: $(OBJ_EXT)"
	@echo "Executable extension: $(EXE_EXT)"
	@echo "Windows version: $(WIN_VER)"

.PHONY: all clean compile-c-lexer info