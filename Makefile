all: octree

octree: main.o my_octree.o rply.o
	gcc -g main.o my_octree.o rply.o -o octree -lm

main.o: main.c
	gcc -g -c main.c -lm

my_octree.o: my_octree.c
	gcc -g -c my_octree.c -lm

rply.o: rply.c
	gcc -g -c rply.c -lm 

clean:
	rm -rf *.o octree