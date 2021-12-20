#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "my_octree.h"

void removeElement(Point *array, int index, int size)
{
    int i;
    for(i = index; i < size - 1; i++) 
        array[i] = array[i + 1];
}

float sqrDist(Point a, Point b)
{
    return pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2);
}

float max(float a, float b)
{
    if (a >= b)
        return a;
    else
        return b;
}

int pointComp(const void* a, const void* b)
{
    const Point *l = (const Point *) a;
    const Point *r = (const Point *) b;
    return sqrDist(p, *l) > sqrDist(p, *r);
}

int octantComp(const void* a, const void * b)
{
    Octant *l = *(Octant **) a;
    Octant *r = *(Octant **) b;
    Point lCenter = l->center;
    Point rCenter = r->center;
    return sqrDist(p, lCenter) > sqrDist(p, rCenter);
}


void initOctree(Octree *octree)
{
    octree->root = NULL;
    octree->points = NULL;
    octree->successors = NULL;
}

void deleteOctree(Octree *octree)
{
    if (octree) {
        if (octree->points) {
            free(octree->points);
            octree->points = NULL;
        }
        if (octree->successors) {
            free(octree->successors);
            octree->successors = NULL;
        }
        if (octree->root) {
            deleteOctant(octree->root);
            octree->root = NULL;
        }
        free(octree);
        octree = NULL;
    }
}

void buildOctree(Octree *octree, Point *pts, int size)
{
    float min[3], max[3], ctr[3];
    float maxext, ext;
    int i = 0;

    clearOctree(octree);
    octree->points = pts;
    octree->successors = malloc(sizeof(int) * size);

    // bounding box
    min[0] = pts[0].x;
    min[1] = pts[0].y;
    min[2] = pts[0].z;
    max[0] = min[0];
    max[1] = min[1];
    max[2] = min[2];

    for (i = 0; i < size; i++) {
        octree->successors[i] = i + 1;
        if (pts[i].x < min[0]) min[0] = pts[i].x;
        if (pts[i].y < min[1]) min[1] = pts[i].y;
        if (pts[i].z < min[2]) min[2] = pts[i].z;
        if (pts[i].x > max[0]) max[0] = pts[i].x;
        if (pts[i].y > max[1]) max[1] = pts[i].y;
        if (pts[i].z > max[2]) max[2] = pts[i].z;
    }

    for (i = 0; i < 3; i++)
        ctr[i] = min[i];
    
    maxext = 0.5f * (max[0] - min[0]);
    ctr[0] += maxext;
    for (i = 1; i < 3; i++) {
        ext = 0.5f * (max[i] - min[i]);
        ctr[i] += ext;
        if (ext > maxext) maxext = ext;
    }

    octree->root = createOctant(octree, size, ctr[0], ctr[1], ctr[2], maxext, 0, size - 1);
}

void clearOctree(Octree *octree)
{
    if (octree->points) {
        free(octree->points);
        octree->points = NULL;
    }
    if (octree->successors) {
        free(octree->successors);
        octree->successors = NULL;
    }
    if (octree->root) {
        deleteOctant(octree->root);
        octree->root = NULL;
    }
}

void initOctant(Octant *octant)
{
    octant->isLeaf = 1;
    octant->center.x = 0.0f;
    octant->center.y = 0.0f;
    octant->center.z = 0.0f;
    octant->extent = 0.0f;
    octant->size = 0;
    octant->begin = 0;
    octant->end = 0;
    memset(&(octant->children), 0, 8 * sizeof(Octant*));
}

void deleteOctant(Octant *octant)
{
    int i = 0;
    if (octant) {
        for (i = 0; i < 8; i++) {
            if (octant->children[i]) { 
                deleteOctant(octant->children[i]);
            }
        }
        free(octant);
        octant = NULL;
    }
}

Octant* createOctant(Octree *octree, int sz, float x, float y, float z, float ext, int beginInd, int endInd)
{
    int i = 0, index, code, first, lastChildInd;
    int childrenBegins[8];
    int childrenEnds[8];
    int childrenSizes[8];
    float childExt, childX, childY, childZ;
    static const float factor[] = { -0.5f, 0.5f };
    Point *pts = NULL;

    Octant *oct = malloc(sizeof(Octant));
    initOctant(oct);
    oct->size = sz;
    oct->center.x = x;
    oct->center.y = y;
    oct->center.z = z;
    oct->extent = ext;
    oct->begin = beginInd;
    oct->end = endInd;

    if (sz > BUCKET_SIZE && ext > 0) {
        oct->isLeaf = 0;
        pts = octree->points;

        for (i = 0; i < 8; i++) {
            childrenBegins[i] = 0;
            childrenEnds[i] = 0;
            childrenSizes[i] = 0;
        }
        index = beginInd;

        for (i = 0; i < sz; i++) {
            code = 0;
            if (pts[index].x > x) code |= 1;
            if (pts[index].y > y) code |= 2;
            if (pts[index].z > z) code |= 4;

            if (childrenSizes[code] == 0) {
                childrenBegins[code] = index;
            }
            else {
                octree->successors[childrenEnds[code]] = index;
            }

            childrenSizes[code] += 1;

            childrenEnds[code] = index;
            index = octree->successors[index];
        }

        childExt = 0.5f * ext;
        first = 1;
        lastChildInd = 0;

        for (i = 0; i < 8; i++) {
            if (childrenSizes[i] == 0) {
                continue;
            }

            childX = x + factor[(i & 1) > 0] * ext;
            childY = y + factor[(i & 2) > 0] * ext;
            childZ = z + factor[(i & 4) > 0] * ext;

            oct->children[i] = createOctant(octree, childrenSizes[i], childX, childY, childZ, childExt, childrenBegins[i], childrenEnds[i]);

            if (first) {
                oct->begin = oct->children[i]->begin;
            }
            else {
                octree->successors[oct->children[lastChildInd]->end] = oct->children[i]->begin;
            }

            lastChildInd = i;
            first = 0;
            oct->end = oct->children[i]->end;
        }
    }
    return oct;
}

void findKNearest(Octree *octree, int k, float radius, Point **result, int *resultSize)
{
    float sqrRadius;

    *result = malloc(sizeof(Point) * k);
    sqrRadius = pow(radius, 2);
    findKNearestRecursive(octree, octree->root, k, &sqrRadius, *result, resultSize);
}

void findKNearestRecursive(Octree *octree, Octant *octant, int k, float *sqrRadius, Point *result, int *resultSize)
{
    int index, i = 0, currChildrenSize = 0;
    float dist;

    Point *pts = octree->points;
    Point currPoint;
    Octant* currChildren[8];

    if (octant->isLeaf) {
        index = octant->begin;
        for (i = 0; i < octant->size; i++) {
            currPoint = pts[index];
            dist = sqrDist(p, currPoint);
            if (dist < *sqrRadius && dist > 0) {
                if (*resultSize == k)
                    (*resultSize)--;
                (*resultSize)++;
                result[(*resultSize)-1] = currPoint;

                qsort(result, *resultSize, sizeof(Point), pointComp);

                if(*resultSize == k)
                    *sqrRadius = sqrDist(p, result[(*resultSize)-1]);
            }
            index = octree->successors[index];
        }
    }
    else {
        for (i = 0; i < 8; i++) {
            if (octant->children[i] != 0) {
                currChildrenSize++;
                currChildren[currChildrenSize-1] = octant->children[i];
            }
        }
        qsort(currChildren, currChildrenSize, sizeof(Octant*), octantComp);

        for (i = 0; i < currChildrenSize; i++) {
            if (intersects(currChildren[i], *sqrRadius))
                findKNearestRecursive(octree, currChildren[i], k, sqrRadius, result, resultSize);
        }
    }
}

void RORfilter(Octree *octree, int k, float radius, int size, int *result, int *resultSize) 
{
    int i, j = 0, innerResultSize;
    Point *currNeighbors = NULL;
    for (i = 0; i < size; i++) 
    {
        innerResultSize = 0;
        p = octree->points[i];
        findKNearest(octree, k, radius, &currNeighbors, &innerResultSize);
        if (innerResultSize < k) 
        {
            (*resultSize)++;
            result[(*resultSize)-1] = i;
        }
        free(currNeighbors);
        currNeighbors = NULL;        
    }
}

int intersects(Octant *oct, float sqrRadius)
{
    float x = abs(p.x - oct->center.x);
    float y = abs(p.y - oct->center.y);
    float z = abs(p.z - oct->center.z);
    float maxdist = sqrt(sqrRadius) + oct->extent;
    int check;

    if (x > maxdist || y > maxdist || z > maxdist)
        return 0;
    
    check = (x < oct->extent) + (y < oct->extent) + (z < oct->extent);
    if (check > 1)
        return 1;
    
    x = max(x - oct->extent, 0.0f); 
    y = max(y - oct->extent, 0.0f);
    z = max(z - oct->extent, 0.0f);
    return (pow(x, 2) + pow(y, 2) + pow(z, 2) < sqrRadius);
}