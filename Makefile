C_FILES = main.c allocator.c

alloc: $(C_FILES) allocator.h
	gcc -std=gnu99 $(C_FILES) -o alloc -Wall -lm -O0 -Wchar-subscripts

clean:
	rm -f alloc *.o

execv: alloc
	./alloc -m $$((1024*20)) -p 1024 -v

exec: alloc
	./alloc -m $$((1024*20)) -p 1024 # -v
