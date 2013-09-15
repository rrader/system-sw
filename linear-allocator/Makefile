C_FILES = main.c allocator.c

alloc: $(C_FILES) allocator.h
	gcc -std=c99 $(C_FILES) -o alloc -Wall

clean:
	rm -f alloc *.o
