#pragma once
#include <vector>
#include <cmath>

struct Point {
	float x, y, z;
};

float sqrDist(Point a, Point b);
bool findElem(const std::vector<int>& vec, int elem);

class Octree {
public:
	Octree() : root(0), points(0) {}
	~Octree() {
		delete root;
	}

	void build(const std::vector<Point>& pts);
	void clear();
	void findKNearest(Point p, int k, float radius, std::vector<Point>& result);
	void RORfilter(int k, float radius, std::vector<int>& result);

protected:
	class Octant {
	public:
		Octant* children[8];
		int size;
		bool isLeaf;
		float centerX, centerY, centerZ;
		float extent;
		int begin, end;

		Octant() : isLeaf(true), centerX(0.0f), centerY(0.0f), centerZ(0.0f), extent(0.0f), begin(0), end(0), size(0) {
			memset(&children, 0, 8 * sizeof(Octant*));
		}
		~Octant() {
			for (int i = 0; i < 8; i++)
				delete children[i];
		}
	};

	Octant* root;
	const std::vector<Point>* points;
	std::vector<int> successors;

	Octant* createOctant(int sz, float x, float y, float z, float ext, int beginInd, int endInd);
	void findKNearestRecursive(const Octant* node, Point p, float& sqrRadius, std::vector<Point>& result);
	bool intersects(const Octant* oct, Point p, float sqrRadius);
};