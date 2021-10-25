#include <vector>
#include <stdint.h>
#include <cmath>
#include <limits>

struct Point {
	float x, y, z;
};

class Octree
{
public:
    Octree() : root(0), points(0) {}
    ~Octree() {
        delete root;
    }

    void build(const std::vector<Point>& pts) {
        clear();
        points = &pts;
        successors = std::vector<int>(pts.size());

        //bounding box
        float min[3], max[3];
        min[0] = pts[0].x;
        min[1] = pts[0].y;
        min[2] = pts[0].z;
        max[0] = min[0];
        max[1] = min[1];
        max[2] = min[2];

        for (int i = 0; i < pts.size(); i++) {
            successors[i] = i + 1;
            const Point& p = pts[i];

            if (p.x < min[0]) min[0] = p.x;
            if (p.y < min[1]) min[1] = p.y;
            if (p.z < min[2]) min[2] = p.z;
            if (p.x > max[0]) max[0] = p.x;
            if (p.y > max[1]) max[1] = p.y;
            if (p.z > max[2]) max[2] = p.z;
        }

        float ctr[3] = { min[0], min[1], min[2] };

        float maxext = 0.5f * (max[0] - min[0]);
        ctr[0] += maxext;
        for (int i = 1; i < 3; i++)
        {
            float ext = 0.5f * (max[i] - min[i]);
            ctr[i] += ext;
            if (ext > maxext) maxext = ext;
        }

        root = createOctant(ctr[0], ctr[1], ctr[2], maxext, 0, pts.size() - 1, pts.size());
    }

    void clear() {
        delete root;
        root = 0;
        points = 0;
        successors.clear();
    }

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
            for (int i = 0; i < 8; ++i) 
                delete children[i];
        }
    };

    Octant* root;
    const std::vector<Point>* points;
    std::vector<int> successors;

    Octant* createOctant(int sz, float x, float y, float z, float ext, int beginInd, int endInd) {
        Octant* oct = new Octant;
        oct->isLeaf = true;
        oct->centerX = x;
        oct->centerY = y;
        oct->centerZ = z;
        oct->extent = ext;
        oct->begin = beginInd;
        oct->end = endInd;
        oct->size = sz;

        static const float factor[] = { -0.5f, 0.5f };

        if (sz > 32 && ext > 0) {
            oct->isLeaf = false;
            const std::vector<Point>& pts = *points;
            std::vector<int> childrenBegins(8, 0);
            std::vector<int> childrenEnds(8, 0);
            std::vector<int> childrenSizes(8, 0);
            int index = beginInd;

            for (int i = 0; i < sz; i++) {
                const Point& p = pts[index];
                int code = 0;
                if (p.x > x) code |= 1;
                if (p.y > y) code |= 2;
                if (p.z > z) code |= 4;

                if (childrenSizes[code] == 0)
                    childrenBegins[code] = index;
                else
                    successors[childrenEnds[code]] = index;
                childrenSizes[code] += 1;

                childrenEnds[code] = index;
                index = successors[index];
            }

            float childExt = 0.5f * ext;
            bool first = true;
            int lastChildInd = 0;
            for (int i = 0; i < 8; i++) {
                if (childrenSizes[i] == 0) 
                    continue;

                float childX = x + factor[(i & 1) > 0] * ext;
                float childY = y + factor[(i & 2) > 0] * ext;
                float childZ = z + factor[(i & 4) > 0] * ext;

                oct->children[i] = createOctant(childrenSizes[i], childX, childY, childZ, childExt, childrenBegins[i], childrenEnds[i]);

                if (first)
                    oct->begin = oct->children[i]->begin;
                else
                    successors[oct->children[lastChildInd]->end] = oct->children[i]->begin;  

                lastChildInd = i;
                oct->end = oct->children[i]->end;
                first = false;
            }
        }
        return oct;
    }
};