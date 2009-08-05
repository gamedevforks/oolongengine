//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef REALIZM_H
#define REALIZM_H
#include <list>
#include "Model.h"
#include "Surface.h"
#include "Polygonizer.h"

typedef std::list<TImplicitSurface*> TComponents;

class TBlob : public TImplicitSurface {
  public:
    TBlob() : threshold(0.1f) {}
    ~TBlob();
    int Draw() const;
    void Insert(TImplicitSurface* component) { components.push_back(component); }
    void Polygonize(int slices);
    vec3 Center() const { return components.back()->Center(); }
  private:
    static const bool exploitSymmetry = false;
    float eval(const vec3& v) const;
    float threshold;
    TComponents components;
    TPolygonizer triangles;
};

class TRealizm : public TModel {
  public:
    TRealizm(unsigned glid, Id id);
    void Draw() const;
    void Load(int slices);
  private:
    int draw() const;
    TBlob blob;
    static void transform(float& x, float& y, float& z);
    static void transform(float& radius);
    static int data[];
    static float scale;
    static float offset;
};

#endif
