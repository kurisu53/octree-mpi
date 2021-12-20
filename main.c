#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "rply.h"

#include "my_octree.h"

Point *testpts;

static int vertex_cb(p_ply_argument argument) 
{
    static int i = 0;
    long flag;
    ply_get_argument_user_data(argument, NULL, &flag);
    switch (flag)
    {
        case 0:
            testpts[i].x = ply_get_argument_value(argument);
            break;
        case 1:
            testpts[i].y = ply_get_argument_value(argument);
            break;
        case 2:
            testpts[i++].z = ply_get_argument_value(argument);
            break;
        default:
            break;
    }
    return 1;
}

int main(int argc, char* argv[])
{
    Octree *testOctree;
    Point *result = NULL;
    int resultSize = 0, i, *indsToRemove;
    long nvertices, newvertices, microseconds;
    struct timeval start, stop;
   
    p_ply ply = ply_open("skull.ply", NULL, 0, NULL);
    if (!ply) return 1;
    if (!ply_read_header(ply)) return 1;
    nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 1);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 2);
    printf("%ld\n %ld\n", nvertices, nvertices * sizeof(Point));
    testpts = malloc(sizeof(Point) * nvertices);
    if (!ply_read(ply)) return 1;
    ply_close(ply);

    testOctree = malloc(sizeof(Octree));
    initOctree(testOctree);
    buildOctree(testOctree, testpts, nvertices);

    printf("\n\nFinding k nearest neighbors for point %f %f %f\n", testpts[1].x,  testpts[1].y, testpts[1].z);
    p = testpts[1];
    findKNearest(testOctree, 5, 20.0f, &result, &resultSize);

    for (i = 0; i < resultSize; i++) 
        printf("%f %f %f %f\n", result[i].x, result[i].y, result[i].z, sqrt(sqrDist(p, result[i])));
    
    indsToRemove = malloc(sizeof(int) * nvertices);
    resultSize = 0;
    
    gettimeofday(&start, NULL);
    RORfilter(testOctree, 10, 2.5f, nvertices, indsToRemove, &resultSize);
    gettimeofday(&stop, NULL);
    microseconds = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    printf("Time spent = %lu mircoseconds\nTime spent = %f miliseconds\nTime spent = %f seconds\n",  
            microseconds, 
            (float)microseconds / 1000, 
            (float)microseconds / 1000000);

    printf("%d points to remove\n", resultSize);

    for(i = 0; i < resultSize; i++)
        removeElement(testpts, indsToRemove[i] - i, nvertices - i);
    newvertices = nvertices - resultSize;
    testpts = realloc(testpts, sizeof(Point) * newvertices);
    testOctree->points = testpts;
    deleteOctree(testOctree);
    free(result);
    free(indsToRemove);

    return 0;
}