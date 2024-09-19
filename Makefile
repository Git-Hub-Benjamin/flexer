flexer:
	clang -g -c ./interpreter/flexer.c -o ./build/flexer.o
	clang -g -c ./hash/hash.c -o ./build/hash.o
	nasm -g -f elf64 -o ./build/preprocess.o preprocess.asm
	clang -no-pie ./build/flexer.o ./build/hash.o ./build/preprocess.o -o flexer.out -lm

test:
	clang -g -c main.c -o ./build/main.o
	nasm -g -f elf64 -o ./build/main2.o main.asm
	clang -no-pie ./build/main.o ./build/main2.o -o main.out -lm

