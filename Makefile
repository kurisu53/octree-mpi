all: octree

octree: main.o my_octree.o rply.o
	mpicc -g main.o my_octree.o rply.o -o octree -lm

main.o: main.c
	mpicc -g -c main.c -lm

my_octree.o: my_octree.c
	mpicc -g -c my_octree.c -lm

rply.o: rply.c
	mpicc -g -c rply.c -lm 

clean:
	rm -rf *.o octree