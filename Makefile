# Detect OS
ifeq ($(OS),Windows_NT)
    # Windows detection that works in PowerShell
    WIN_VER := $(shell systeminfo | findstr /B /C:"OS Version:")
    ifneq (,$(findstring 22H2,$(WIN_VER)))
        # Windows 11
        CC = gcc
        NASM_FLAGS = -g -f win64 -i ./flexer/preprocess/ -F cv8
        OBJ_EXT = .o
        LINKER = gcc -g
        EXE_EXT = .exe
        CFLAGS = -g -gcodeview -Z7 -D_CRT_SECURE_NO_WARNINGS
    else
        # Windows 10
        CC = clang 
        NASM_FLAGS = -g -f win64 -i ./flexer/preprocess/ -F cv8
        OBJ_EXT = .obj
        LINKER = clang -m64 -g -gcodeview -fuse-ld=lld
        EXE_EXT = .exe
        CFLAGS = -g -gcodeview -Z7 -D_CRT_SECURE_NO_WARNINGS
    endif
else
    # Linux
    CC = clang
    NASM_FLAGS = -W --gstabs -f elf64 -i ./flexer/preprocess/
    OBJ_EXT = .o
    LINKER = clang -no-pie
    EXE_EXT = .out
    EXTRA_LIBS = -lm
    CFLAGS = -g -D_CRT_SECURE_NO_WARNINGS
endif

# Directory structure
BUILD_DIR = build
SRC_DIR = flexer
HASH_DIR = hash
PREPROCESS_DIR = $(SRC_DIR)/preprocess

# Source files
C_SRCS = $(SRC_DIR)/flexer.c \
         $(HASH_DIR)/hash.c \
         $(SRC_DIR)/fparse.c \

ASM_SRCS = $(PREPROCESS_DIR)/hashPreprocessorDirectives.asm \
           $(PREPROCESS_DIR)/preProcessorMacroExpansion.asm

# Object files
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%$(OBJ_EXT),$(notdir $(C_SRCS)))
ASM_OBJS = $(patsubst %.asm,$(BUILD_DIR)/%$(OBJ_EXT),$(notdir $(ASM_SRCS)))

# Target executable
TARGET = flexer$(EXE_EXT)
    
# Build rules
all: $(TARGET)

$(TARGET): $(C_OBJS) $(ASM_OBJS)
	$(LINKER) $^ -o $@ $(EXTRA_LIBS)

# Pattern rule for C files
$(BUILD_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%$(OBJ_EXT): $(HASH_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for ASM files
$(BUILD_DIR)/%$(OBJ_EXT): $(PREPROCESS_DIR)/%.asm
	nasm $(NASM_FLAGS) $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	powershell Remove-Item -Recurse -Force $(BUILD_DIR)
	mkdir $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)
endif

info:
	@echo "Detected configuration:"
	@echo "Compiler: $(CC)"
	@echo "NASM flags: $(NASM_FLAGS)"
	@echo "Object extension: $(OBJ_EXT)"
	@echo "Executable extension: $(EXE_EXT)"
	@echo "Windows version: $(WIN_VER)"
	@echo "C sources: $(C_SRCS)"
	@echo "ASM sources: $(ASM_SRCS)"
	@echo "Object files: $(C_OBJS) $(ASM_OBJS)"

.PHONY: all clean info