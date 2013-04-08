C_FILES = main.c allocator.c

alloc: $(C_FILES) allocator.h
	gcc -std=gnu99 $(C_FILES) -o alloc -Wall -lm -O0

clean:
	rm -f alloc *.o

exec: alloc
	./alloc -m $$((1024*10)) -p 1024 -v
