#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "rply.h"

#include "my_octree.h"

// callback function for PLY file reading
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
    // declaring variables
    Octree *testOctree;
    Point *result = NULL;
    int resultSize = 0, i;
    int *indsToRemove;
    long nvertices, newvertices, microseconds;
    struct timeval start, stop;
    p_ply ply;

    // command line arguments
    char *filename; // PLY source file name
    int k; // min number of neighbors every point should have
    float rad; // search radius for neighbors

    if (argc != 4) {
        fprintf(stderr, "3 command line arguments must be passed: filename,\n min number of neighbors every point should have, search radius\n");
        exit(EXIT_FAILURE);
    }
    filename = argv[1];
    k = atoi(argv[2]);
    rad = atof(argv[3]);
   
    // reading from a PLY file using RPly library
    ply = ply_open(filename, NULL, 0, NULL);
    if (!ply) {
        fprintf(stderr, "File pointer is null\n");
        exit(EXIT_FAILURE);
    }
    if (!ply_read_header(ply)) {
        fprintf(stderr, "Failed to read PLY file header\n");
        exit(EXIT_FAILURE);
    }
    nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 1);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 2);
    printf("File contains %ld points\n", nvertices);
    testpts = malloc(sizeof(Point) * nvertices);
    if (!ply_read(ply)) {
        fprintf(stderr, "Failed to read from PLY file\n");
        exit(EXIT_FAILURE);
    }
    ply_close(ply);

    // initializing and building an octree from a point cloud
    testOctree = malloc(sizeof(Octree));
    initOctree(testOctree);
    buildOctree(testOctree, testpts, nvertices);
    
    // array of indexes of points to be deleted from a cloud
    indsToRemove = malloc(sizeof(int) * nvertices);
    resultSize = 0;
    
    printf("Starting filtering...\n\n");
    // timed radius outlier filtering
    gettimeofday(&start, NULL);
    RORfilter(testOctree, k, rad, nvertices, indsToRemove, &resultSize);
    gettimeofday(&stop, NULL);
    microseconds = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    printf("Points to be filtered found in %f seconds\n", (float)microseconds / 1000000);
    printf("%d points to be removed\n", resultSize);

    printf("\nFiltering the cloud...\n");
    for(i = 0; i < resultSize; i++)
        removeElement(testpts, indsToRemove[i] - i, nvertices - i);
    newvertices = nvertices - resultSize;
    testpts = realloc(testpts, sizeof(Point) * newvertices);
    testOctree->points = testpts;
    printf("Finished filtering the cloud! It contains %ld points now\n", newvertices);

    // freeing memory
    deleteOctree(testOctree);
    free(result);
    free(indsToRemove);

    return 0;
}