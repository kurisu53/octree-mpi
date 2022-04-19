#ifndef MY_OCTREE_H
#define MY_OCTREE_H
#define BUCKET_SIZE 32 // max number of points in a leaf octant
#define ROR_FILTER 0
#define SOR_FILTER 1

// point structure

typedef struct Point {
    float x, y, z;
} Point;

Point *inputpts, *resultpts;
Point p;

// utility functions

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
int floatComp(const void*, const void*);
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

void findKNearest(Octree *, int, float, Point **, int *, int, float **);
void findKNearestRecursive(Octree *, Octant *, int, float *, Point *, int *, float *);
void RORfilter(Octree *, int, float, int, int *, long *);
void SORfilter(Octree *, int, int, float, int *, long *);

int intersects(Octant *, float);

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.1415926536

double AWGN_generator()
{
    /* Generates additive white Gaussian Noise samples with zero mean and a standard deviation of 1. */

    double temp1;
    double temp2;
    double result;
    int p;

    p = 1;

    while( p > 0 )
    {
    temp2 = ( rand() / ( (double)RAND_MAX ) ); /*  rand() function generates an
                                                        integer between 0 and  RAND_MAX,
                                                        which is defined in stdlib.h.
                                                    */

    if ( temp2 == 0 )
    {// temp2 is >= (RAND_MAX / 2)
        p = 1;
    }// end if
    else
    {// temp2 is < (RAND_MAX / 2)
        p = -1;
    }// end else

    }// end while()

    temp1 = cos( ( 2.0 * (double)PI ) * rand() / ( (double)RAND_MAX ) );
    result = sqrt( -2.0 * log( temp2 ) ) * temp1;

    return result;	
    // return the generated random sample to the caller

}// end AWGN_generator()

#endif