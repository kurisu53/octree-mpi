#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <stddef.h>
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
    Point testPoint;
    int resultSize = 0, i, j, l;
    int *indsToRemove, *tempInds;
    long nvertices, newvertices, microseconds = 0;
    struct timeval start, stop;
    p_ply ply;
    
    MPI_Status status;
    int mpi_size = 0, mpi_rank = 0, ibeg, iend, tempSize;

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

    // MPI initialization
    checkForSuccess(MPI_Init(&argc, &argv), ERR_INIT);
    checkForSuccess(MPI_Comm_size(MPI_COMM_WORLD, &mpi_size), ERR_COMM_SIZE);
    checkForSuccess(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank), ERR_COMM_RANK);

    // creating a structure for sending Point type

    const int    nitems = 3;
    int          blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
    MPI_Datatype mpi_point_type;
    MPI_Aint     offsets[3];

    offsets[0] = offsetof(Point, x);
    offsets[1] = offsetof(Point, y);
    offsets[2] = offsetof(Point, z);

    checkForSuccess(MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_point_type), ERR_CREATE_STRUCT);
    checkForSuccess(MPI_Type_commit(&mpi_point_type), ERR_COMMIT);

    if(mpi_rank == 0) {
        // master process reads from a file
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
    }
    // master process broadcasts number of points
    checkForSuccess(MPI_Bcast(&nvertices, 1, MPI_LONG, 0, MPI_COMM_WORLD), ERR_BCAST);
    if(mpi_rank != 0)
        testpts = malloc(sizeof(Point) * nvertices);
    // master process broadcasts all points in a cloud
    checkForSuccess(MPI_Bcast(testpts, nvertices, mpi_point_type, 0, MPI_COMM_WORLD), ERR_BCAST);

    // initializing and building an octree from a point cloud
    testOctree = malloc(sizeof(Octree));
    initOctree(testOctree);
    buildOctree(testOctree, testpts, nvertices);

    // calculating indexes for workload division between processes
    ibeg = mpi_rank * nvertices / mpi_size;
    if(mpi_rank == mpi_size - 1)
        iend = nvertices - 1;
    else
        iend = (mpi_rank + 1) * nvertices / mpi_size - 1;
    
    // array of indexes of points to be deleted from a cloud
    indsToRemove = malloc(sizeof(int) * nvertices);
    resultSize = 0;

    if (mpi_rank == 0)
        printf("Starting filtering...\n\n");

    // every process searches for points to be filtered in its own section of a cloud
    gettimeofday(&start, NULL);
    RORfilterParallel(testOctree, k, rad, nvertices, indsToRemove, &resultSize, ibeg, iend);
    if(mpi_rank != 0) {
        // everyone sends calculated indexes to master
        checkForSuccess(MPI_Send(&resultSize, 1, MPI_INT, 0, 1, MPI_COMM_WORLD), ERR_SEND);
        checkForSuccess(MPI_Send(indsToRemove, resultSize, MPI_INT, 0, 1, MPI_COMM_WORLD), ERR_SEND);
    }
    else {
        for(i = 1; i < mpi_size; i++) {
            // master process compiles a complete indsToRemove array
            checkForSuccess(MPI_Recv(&tempSize, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status), ERR_RECV);
            tempInds = malloc(sizeof(int) * tempSize);
            checkForSuccess(MPI_Recv(tempInds, tempSize, MPI_INT, i, 1, MPI_COMM_WORLD, &status), ERR_RECV);
            l = 0;
            for(j = resultSize; j < resultSize + tempSize; j++)
                indsToRemove[j] = tempInds[l++];
            resultSize += tempSize;
            free(tempInds);
        }
        gettimeofday(&stop, NULL);
        microseconds = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
        printf("Points to be filtered found in %f seconds\n", (float)microseconds / 1000000);
        printf("%d points to be removed\n", resultSize);

        // master process handles removing of points from a cloud
        printf("\nFiltering the cloud...\n");
        for(i = 0; i < resultSize; i++)
            removeElement(testpts, indsToRemove[i] - i, nvertices - i);
        newvertices = nvertices - resultSize;
        testpts = realloc(testpts, sizeof(Point) * newvertices);
        testOctree->points = testpts;      
        printf("Finished filtering the cloud! It contains %ld points now\n", newvertices);
    }

    // freeing memory
    deleteOctree(testOctree);
    free(indsToRemove);

    MPI_Finalize();
    return 0;
}