#ifndef MY_OCTREE_H
#define MY_OCTREE_H

#define BUCKET_SIZE 32 // max number of points in a leaf octant

// error codes for MPI functions

#define ERR_INIT 1
#define ERR_COMM_SIZE 2
#define ERR_COMM_RANK 3
#define ERR_CREATE_STRUCT 4
#define ERR_COMMIT 5
#define ERR_BCAST 6
#define ERR_SEND 7
#define ERR_RECV 8

// point structure

typedef struct Point {
    float x, y, z;
} Point;

Point *testpts;
Point p;

// utility functions

void checkForSuccess(int, int);
void removeElement(Point *, int, int);
float sqrDist(Point, Point);
float max(float, float);

// Octree and Octant structures

typedef struct Octant {
    int isLeaf;
    struct Octant* children[8];
    int size;
    int begin;
    int end;
    Point center;
    float extent;
} Octant;

typedef struct Octree {
    Octant* root;
    Point* points;
    int* successors;
} Octree;

// comparators for sorting neighbors and octants

int pointComp(const void*, const void*);
int octantComp(const void*, const void*);

// initialization and deletion of Octree/Octant

void initOctree(Octree *);
void deleteOctree(Octree *);

void initOctant(Octant *);
void deleteOctant(Octant *);

// building/clearing Octree, creating octants

void buildOctree(Octree *, Point *, int);
void clearOctree(Octree *);

Octant* createOctant(Octree *, int, float, float, float, float, int, int);

// k nearest neighbors search and filtering

void findKNearest(Octree *, int, float, Point **, int *);
void findKNearestRecursive(Octree *, Octant *, int, float *, Point *, int *);
void RORfilterParallel(Octree *, int, float, int, int *, int *, int, int);
int intersects(Octant *, float);

#endif