#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <stddef.h>
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
    Point testPoint;
    MPI_Status status;
    struct timeval start, stop;
    long microseconds = 0;
    int resultSize = 0, i, j, k, *indsToRemove, mpi_size = 0, mpi_rank = 0, ibeg, iend, tempSize, *tempInds;
    long nvertices, newvertices;
    p_ply ply;

    checkForSuccess(MPI_Init(&argc, &argv), 1);
    checkForSuccess(MPI_Comm_size(MPI_COMM_WORLD, &mpi_size), 2);
    checkForSuccess(MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank), 3);

    const int    nitems = 3;
    int          blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
    MPI_Datatype mpi_point_type;
    MPI_Aint     offsets[3];

    offsets[0] = offsetof(Point, x);
    offsets[1] = offsetof(Point, y);
    offsets[2] = offsetof(Point, z);

    checkForSuccess(MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_point_type), 20);
    checkForSuccess(MPI_Type_commit(&mpi_point_type), 21);

    if(mpi_rank == 0)
    {
        ply = ply_open("skull.ply", NULL, 0, NULL);
        if (!ply) return 1;
        if (!ply_read_header(ply)) return 1;
        nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
        ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 1);
        ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 2);
        printf("%ld\n %ld\n", nvertices, nvertices * sizeof(Point));
        testpts = malloc(sizeof(Point) * nvertices);
        if (!ply_read(ply)) return 1;
        ply_close(ply);
    }
    checkForSuccess(MPI_Bcast(&nvertices, 1, MPI_LONG, 0, MPI_COMM_WORLD), 9);
    if(mpi_rank != 0)
        testpts = malloc(sizeof(Point) * nvertices);
    checkForSuccess(MPI_Bcast(testpts, nvertices, mpi_point_type, 0, MPI_COMM_WORLD), 9);

    testOctree = malloc(sizeof(Octree));

    initOctree(testOctree);
    buildOctree(testOctree, testpts, nvertices);

    ibeg = mpi_rank * nvertices / mpi_size;
    if(mpi_rank == mpi_size - 1)
        iend = nvertices - 1;
    else
        iend = (mpi_rank + 1) * nvertices / mpi_size - 1;
    
    indsToRemove = malloc(sizeof(int) * nvertices);
    resultSize = 0;

    gettimeofday(&start, NULL);
    RORfilterParallel(testOctree, 10, 2.5f, nvertices, indsToRemove, &resultSize, ibeg, iend);
    if(mpi_rank != 0)
    {
        checkForSuccess(MPI_Send(&resultSize, 1, MPI_INT, 0, 1, MPI_COMM_WORLD), 4);
        checkForSuccess(MPI_Send(indsToRemove, resultSize, MPI_INT, 0, 1, MPI_COMM_WORLD), 4);
    }
    else
    {
        for(i = 1; i < mpi_size; i++)
        {
            checkForSuccess(MPI_Recv(&tempSize, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status), 5);
            tempInds = malloc(sizeof(int) * tempSize);
            checkForSuccess(MPI_Recv(tempInds, tempSize, MPI_INT, i, 1, MPI_COMM_WORLD, &status), 5);
            k = 0;
            for(j = resultSize; j < resultSize + tempSize; j++)
                indsToRemove[j] = tempInds[k++];
            resultSize += tempSize;
            free(tempInds);
        }
        gettimeofday(&stop, NULL);
        microseconds = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
        printf("R%d: Time spent = %lu mircoseconds\nTime spent = %f miliseconds\nTime spent = %f seconds\n", 
                mpi_rank, 
                microseconds, 
                (float)microseconds / 1000, 
                (float)microseconds / 1000000);

        for(i = 0; i < resultSize; i++)
            removeElement(testpts, indsToRemove[i] - i, nvertices - i);
        newvertices = nvertices - resultSize;
        testpts = realloc(testpts, sizeof(Point) * newvertices);
        testOctree->points = testpts;      
    }

    deleteOctree(testOctree);
    free(indsToRemove);

    MPI_Finalize();
    return 0;
}