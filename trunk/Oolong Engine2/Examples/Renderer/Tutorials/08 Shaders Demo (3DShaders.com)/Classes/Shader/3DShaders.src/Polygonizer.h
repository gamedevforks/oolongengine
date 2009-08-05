//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef POLYGONIZER_H
#define POLYGONIZER_H
#include <vector>
#include <map>
#include "Vector.h"
#include "Surface.h"

// Indices of vertices comprising the triangle
struct TTriangle {
    TTriangle(int a, int b, int c) : a(a), b(b), c(c) {}
    int a, b, c;
};

typedef std::vector<vec3> TNormals;
typedef std::vector<vec3> TVertices;
typedef std::vector<TTriangle> TTriangles;
typedef std::map<vec3, int> TVertexIndexMap;

class TPolygonizer {
  public:

    //
    // Setup arguments:
    //   - function we wish to polygonize
    //   - size of the polygonizing cell
    //   - limit to how far to look for components of the implicit surface
    //
    void Setup(const TImplicitSurface* function, float cellSize);

    //
    // Use marching cubes to build the list of triangles and vertices.
    //
    void March(vec3 minv, vec3 maxv);

    // container access methods
    int NumTriangles() const { return (int) triangles.size(); }
    int NumVertices() const { return (int) vertices.size(); }
    int NumNormals() const { return (int) normals.size(); }
    const TTriangle& GetTriangle(int i) const { return triangles[i]; }
    const vec3& GetVertex(int i) const { return vertices[i]; }
    const vec3& GetNormal(int i) const { return normals[i]; }

    // methods useful for OpenGL (eg, glDrawElements)
    const void* GetIndices() const { return (void*) &triangles[0]; }
    const void* GetVertices() const { return (void*) &vertices[0]; }
    const void* GetNormals() const { return (void*) &normals[0]; }

  private:
    TNormals normals;  
    TVertices vertices;
    TTriangles triangles;
    TVertexIndexMap vertexIndexMap;
    const TImplicitSurface* function;
    float cellSize;

    void vMarchCube(const vec3& point);
    float fGetOffset(float fValue1, float fValue2);

    static const float a2fEdgeDirection[12][3];
    static const int a2iEdgeConnection[12][2];
    static const float a2fVertexOffset[8][3];
    static const int aiCubeEdgeFlags[256];
    static const int a2iTriangleConnectionTable[256][16];
};

#endif
