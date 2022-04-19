# octree-mpi
Parallel realization of radius outlier filter for point clouds using octree

Uses RPly library for PLY files reading.

Implements building an octree from a point cloud, finding k nearest neighbors for a given point, adding gaussian noise to the cloud, parallelized radius outlier filtering, parallelized statistical outlier filtering.

## Usage:

1. Compile using **make**
2. Run using **mpirun -np X ./octree filename k radius filter_type add_noise noise_density**, where filename is source PLY file name, k is min number of neighbors every point should have (or mean k for SOR filter), radius is search radius for ROR / multiplier for SOR (float, for example 1.5f), filter_type is R for ROR and S for SOR, add_noise is Y/N, noise_density is a float indicating which percent of the points will be noised.

## TODO:

- Overall optimizing & refactoring
