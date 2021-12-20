#ifndef MY_OCTREE_H
#define MY_OCTREE_H
#define BUCKET_SIZE 32

// point structure

typedef struct Point {
    float x, y, z;
} Point;

Point p;

// utility functions

void removeElement(Point *array, int index, int size);
float sqrDist(Point a, Point b);

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

// initialization and deletion of Octree/Octant

int pointComp(const void*, const void*);
int octantComp(const void*, const void*);


void initOctree(Octree *octree);
void deleteOctree(Octree *octree);

void buildOctree(Octree *octree, Point *pts, int size);
void clearOctree(Octree *octree);

void initOctant(Octant *octant);
void deleteOctant(Octant *octant);

Octant* createOctant(Octree *octree, int sz, float x, float y, float z, float ext, int beginInd, int endInd);

void findKNearest(Octree *octree, int k, float radius, Point **result, int *resultSize);
void findKNearestRecursive(Octree *octree, Octant *octant, int k, float *sqrRadius, Point *result, int *resultSize);
void RORfilter(Octree *octree, int k, float radius, int size, int *result, int *);

int intersects(Octant *, float);


#endif