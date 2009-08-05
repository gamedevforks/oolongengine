//
// Author:    Jon Kennedy
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef BLOBBYCLOUD_H
#define BLOBBYCLOUD_H

#include "Shaders.h"
#include "Vector.h"
#include "Surface.h"
#include "Model.h"

// structure used for qsort algorithm.
struct TBlobby {
    float distSqr;
    int   blob;
};

typedef std::vector<vec3> TPositions;
typedef std::vector<vec3> TVelocities;
typedef std::vector<TUniform> TUniforms;
typedef std::vector<TBlobby> TBlobbies;

// vertex shader based blobby cloud
class TBlobbyCloud : public TModel {
  public:
    TBlobbyCloud(unsigned glid, Id id);
    void Draw() const;
    void Update();
    void Reset();
    void Refresh() { dirty = true; Load(-1); }
    void Load(int slices);

  private:

    static const int blobCount = 10;
    static const int maxNearestBlobs = 5;

    GLuint program;
    TPositions positions;
    TVelocities velocities;
    vec3 limit;                         // bounding cube the blobs bounce around within
    int previousTime;                   // used for animation
    mutable TUniform count;             // number of blobs to test against
    mutable TUniforms blobbyPos;
    mutable TBlobbies nearestBlobs;     // list of neighbouring blobby positions to pass to the current blob getting rendered
    float blobbyCutoff;                 // the radius squared distance used to help determine the nearest neighbours
};

#endif