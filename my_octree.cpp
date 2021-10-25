#include <vector>

struct OctPoint {
	float x;
	float y;
	float z;
	int code;

	OctPoint(float px, float py, float pz) : x(px), y(py), z(pz), code(0) { }
	OctPoint(float px, float py, float pz, int pcode) : x(px), y(py), z(pz), code(pcode) { }

	OctPoint operator+(OctPoint p) {
		return OctPoint(this->x + p.x, this->y + p.y, this->z + p.z);
	}

	OctPoint operator-(OctPoint p) {
		return OctPoint(this->x - p.x, this->y - p.y, this->z - p.z);
	}

	OctPoint operator*(float f) {
		return OctPoint(this->x * f, this->y * f, this->z * f);
	}
};

class Octree
{
private:

	std::vector<Octree> children[8];
	std::vector<OctPoint*> points;
	int size;
	OctPoint boxCenter;
	float boxRadius;

public:

	Octree() :
		points, size(0), boxCenter(0.0, 0.0, 0.0, 0), boxRadius(0.0) {
		memset(children, 0, sizeof(children));
	}

	void findBox(std::vector<OctPoint*> pts, unsigned int sz)
	{
		Point min = *pts[0], max = *pts[0];
		for (int i = 1; i < sz; i++) {
			if (pts[i]->x < min.x)
				min.x = pts[i]->x;
			if (pts[i]->y < min.y)
				min.y = pts[i]->y;
			if (pts[i]->z < min.z)
				min.z = pts[i]->z;
			if (pts[i]->x > max.x)
				max.x = pts[i]->x;
			if (pts[i]->y > max.y)
				max.y = pts[i]->y;
			if (pts[i]->z > max.z)
				max.z = pts[i]->z;
		}
		Point radius = max - min;
		boxCenter = min + radius * 0.5;
		if (radius.x >= radius.y && radius.x >= radius.z)
			boxRadius = radius.x;
		else if (radius.y >= radius.x && radius.y >= radius.z)
			boxRadius = radius.y;
		else
			boxRadius = radius.z;
	}

	void build(Point** pts, int sz, int maxSz, int depth, int maxDepth, Point bCenter, float bRadius)
	{
		if (depth >= maxDepth || sz <= maxSz) {
			size = sz;
			points = new Point * [sz];
			memcpy(points, pts, sizeof(Point*) * sz);
			return;
		}

		int childSizes[8];

		for (int i = 0; i < sz; i++) {
			pts[i]->code = 0;
			if (pts[i]->x > bCenter.x)
				pts[i]->code |= 1;
			if (pts[i]->y > bCenter.y)
				pts[i]->code |= 1;
			if (pts[i]->z > bCenter.z)
				pts[i]->code |= 1;
			childSizes[pts[i]->code]++;
		}

		for (int i = 0; i < 8; i++) {
			if (!childSizes[i])
				continue;
			children[i] = new Octree;
			Point** pointList = new Point * [childSizes[i]];
			Point** ptr = pointList;
			for (int j = 0; j < sz; j++) {
				if (pts[j]->code == i) {
					*ptr = pts[j];
					ptr++;
				}
			}

			Point offsets[8] = {
				Point(-0.5, -0.5, -0.5),
				Point(0.5, -0.5, -0.5),
				Point(-0.5, 0.5, -0.5),
				Point(0.5, 0.5, -0.5),
				Point(-0.5, -0.5, 0.5),
				Point(0.5, -0.5, 0.5),
				Point(-0.5, 0.5, 0.5),
				Point(0.5, 0.5, 0.5)
			};
			Point offset = offsets[i] * bRadius;
			children[i]->build(pointList, sz, maxSz, depth + 1, 
				maxDepth, bCenter + offset, bRadius * 0.5);
			delete[] pointList;
		}
	}
};