lflexer: compile-c-lexer
	nasm -g -f elf64 -o ./build/hashPreprocessorDirectives.o ./interpreter/preprocess/hashPreprocessorDirectives.asm
	nasm -g -f elf64 -o ./build/preProcessorMacroExpansion.o ./interpreter/preprocess/preProcessorMacroExpansion.asm
	clang -no-pie ./build/flexer.o ./build/hash.o ./build/hashPreprocessorDirectives.o ./build/preProcessorMacroExpansion.o -o flexer.out -lm

wflexer: compile-c-lexer
	nasm -g -f win64 -i ./interpreter/preprocess/ -F cv8 -o ./build/hashPreprocessorDirectives.o ./interpreter/preprocess/hashPreprocessorDirectives.asm
	nasm -g -f win64 -i ./interpreter/preprocess/ -F cv8 -o ./build/preProcessorMacroExpansion.o ./interpreter/preprocess/preProcessorMacroExpansion.asm
	gcc -g ./build/flexer.o ./build/hash.o ./build/hashPreprocessorDirectives.o ./build/preProcessorMacroExpansion.o -o flexer.exe

compile-c-lexer:
	gcc -g -c ./interpreter/flexer.c -o ./build/flexer.o 
	gcc -g -c ./hash/hash.c -o ./build/hash.o
	

test:
	clang -g -c main.c -o ./build/main.o
	nasm -g -f elf64 -o ./build/main2.o main.asm
	clang -no-pie ./build/main.o ./build/main2.o -o main.out -lm

