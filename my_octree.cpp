#include <vector>
#include <stdint.h>
#include <cmath>
#include <limits>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "my_octree.h"

float sqrDist(Point a, Point b) {
    return pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2);
}

bool findElem(const std::vector<int>& vec, int elem) {
    return (std::find(vec.begin(), vec.end(), elem) != vec.end()) ? true : false;
}

void Octree::build(const std::vector<Point>& pts) {
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

    root = createOctant(pts.size(), ctr[0], ctr[1], ctr[2], maxext, 0, pts.size() - 1);

    for (int i = 0; i < 10; i++) {
        int j = i * 1000;
        std::cout << "Successors[" << j << "]: " << successors[j] << '\n';
    }
}

void Octree::clear() {
    delete root;
    root = 0;
    points = 0;
    successors.clear();
}

void Octree::findKNearest(Point p, int k, float radius, std::vector<Point>& result) {
    result.reserve(k);
    float sqrRadius = pow(radius, 2);

    findKNearestRecursive(root, p, sqrRadius, result);
}

void Octree::RORfilter(int k, float radius, std::vector<int>& result) {
    for (int i = 0; i < points->size(); i++) {
        std::vector<Point> currNeighbors;
        currNeighbors.reserve(k);
        findKNearest(points->at(i), k, radius, currNeighbors);
        if (currNeighbors.size() < k) 
            result.push_back(i);
    }
}

Octree::Octant* Octree::createOctant(int sz, float x, float y, float z, float ext, int beginInd, int endInd) {
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
            first = false;
            oct->end = oct->children[i]->end;
        }
    }
    return oct;
}

void Octree::findKNearestRecursive(const Octant* node, Point p, float& sqrRadius, std::vector<Point>& result) {
    const std::vector<Point>& pts = *points;
    if (node->isLeaf) {
        int index = node->begin;
        for (int i = 0; i < node->size; i++) {
            const Point& currPoint = pts[index];
            float dist = sqrDist(p, currPoint);
            if (dist < sqrRadius && dist > 0) {
                if (result.size() == result.capacity())
                    result.pop_back();
                result.push_back(currPoint);

                struct comp {
                    Point p_comp;
                    comp(Point p) { p_comp = p; }
                    bool operator()(Point a, Point b) const { 
                        return sqrDist(p_comp, a) < sqrDist(p_comp, b);
                    }
                };
                std::sort(result.begin(), result.end(), comp(p));

                if (result.size() == result.capacity())
                    sqrRadius = sqrDist(p, result.back());
            }
            index = successors[index];
        }
    }
    else {
        std::vector<Octant*> currChildren;
        currChildren.reserve(8);
        for (int i = 0; i < 8; i++) {
            if (node->children[i] != 0)
                currChildren.push_back(node->children[i]);
        }

        struct comp {
            Point p_comp;
            comp(Point p) { p_comp = p; }
            bool operator()(Octant* a, Octant* b) const {
                Point aCenter = { a->centerX, a->centerY, a->centerZ };
                Point bCenter = { b->centerX, b->centerY, b->centerZ };
                return sqrDist(p_comp, aCenter) < sqrDist(p_comp, bCenter);
            }
        };
        std::sort(currChildren.begin(), currChildren.end(), comp(p));
        for (int i = 0; i < currChildren.size(); i++) {
            if (intersects(currChildren[i], p, sqrRadius)) 
                findKNearestRecursive(currChildren[i], p, sqrRadius, result);
        }
    }
}

bool Octree::intersects(const Octant* oct, Point p, float sqrRadius) {
    float x = abs(p.x - oct->centerX);
    float y = abs(p.y - oct->centerY);
    float z = abs(p.z - oct->centerZ);
    float maxdist = sqrt(sqrRadius) + oct->extent;

    if (x > maxdist || y > maxdist || z > maxdist)
        return false;

    int check = (x < oct->extent) + (y < oct->extent) + (z < oct->extent);
    if (check > 1)
        return true;

    x = std::max(x - oct->extent, 0.0f);
    y = std::max(y - oct->extent, 0.0f);
    z = std::max(z - oct->extent, 0.0f);
    return (pow(x, 2) + pow(y, 2) + pow(z, 2) < sqrRadius);
}