# octree-mpi
Parallel realization of radius outlier filter for point clouds using octree

Uses RPly library for PLY files reading.
Implements building an octree from a point cloud (each process builds a local copy of the tree), finding k nearest neighbors for a given point, radius outlier filtering (each process searches its own section of point cloud for points to be filtered).

__TODO:__
- Adding writing point cloud data to PLY files
- Implementing statistical outlier filter for point clouds
- Adding point cloud visualization
- Overall optimizing & refactoring
