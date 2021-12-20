# octree-mpi
Parallel realization of radius outlier filter for point clouds using octree

Uses RPly library for PLY files reading.

Implements building an octree from a point cloud, finding k nearest neighbors for a given point, parallelized radius outlier filtering.

## Usage:

1. Compile using **make**
2. Run using **mpirun -np X ./octree filename k radius**, where X is number of processes, filename is source PLY file name, k is min number of neighbors every point should have, radius is search radius (float, for example 1.5f)

## TODO:

- Adding writing point cloud data to PLY files
- Implementing statistical outlier filter for point clouds
- Adding point cloud visualization
- Overall optimizing & refactoring
