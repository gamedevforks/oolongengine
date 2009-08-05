//
// Author:    Jon Kennedy
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#define _USE_MATH_DEFINES
#include "App.h"
#include "BlobbyCloud.h"
#include "Frame.h"
#include "os.h"
#include <algorithm>

// sorting predicate
bool compareBlobs(const TBlobby& b1, const TBlobby& b2)
{
    return b1.distSqr < b2.distSqr;
}

TBlobbyCloud::TBlobbyCloud(unsigned glid, Id id) : TModel(glid, id),
    positions(blobCount), velocities(blobCount), nearestBlobs(blobCount), blobbyPos(maxNearestBlobs)
{
    blobbyCutoff = 100.0;
    limit = vec3(10.0f, 10.0f, 10.0f);
    previousTime = wxGetLocalTimeMillis().GetLo();

    srand(30757);

    // seed the positions
    for(int ii = 0; ii < blobCount; ++ii) {
        positions[ii].x = ((float) rand() / RAND_MAX) * 0.5f;
        positions[ii].y = ((float) rand() / RAND_MAX) * 0.5f;
        positions[ii].z = ((float) rand() / RAND_MAX) * 0.5f;
    }

    // seed the velocities
    for(int ii = 0; ii < blobCount; ++ii) {
        velocities[ii].x = (((float) rand() / RAND_MAX) - 0.5) * 2.0;
        velocities[ii].y = (((float) rand() / RAND_MAX) - 0.5) * 2.0;
        velocities[ii].z = (((float) rand() / RAND_MAX) - 0.5) * 2.0;
    }
}

void TBlobbyCloud::Update()
{
    int elapsedTime = wxGetLocalTimeMillis().GetLo();
    float deltaTime;

    // 'elapsed' can be less than 'previous' if the timer gets reset somewhere else.
    if (elapsedTime < previousTime)
        deltaTime = 0.16f;  // 1/60 of a second
    else
        deltaTime = float(elapsedTime - previousTime) / 1000000.0f;

    previousTime = elapsedTime;

    deltaTime *= 5000.0f;

    for(int ii = 0; ii < (blobCount); ii++) {
        positions[ii] = positions[ii] + velocities[ii] * deltaTime;

        if (positions[ii].x > limit.x) {
            velocities[ii].x = 0 - velocities[ii].x;
            positions[ii].x = limit.x;
        } else if (positions[ii].x < -limit.x) {
            velocities[ii].x = 0 - velocities[ii].x;
            positions[ii].x = -limit.x;
        }
        
        if (positions[ii].y > limit.y) {
            velocities[ii].y = 0 - velocities[ii].y;
            positions[ii].y = limit.y;
        } else if (positions[ii].y < -limit.y) {
            velocities[ii].y = 0 - velocities[ii].y;
            positions[ii].y = -limit.y;
        }
        
        if (positions[ii].z > limit.z) {
            velocities[ii].z = 0 - velocities[ii].z;
            positions[ii].z = limit.z;
        } else if (positions[ii].z < -limit.z) {
            velocities[ii].z = 0 - velocities[ii].z;
            positions[ii].z = -limit.z;
        }
    }
}


void TBlobbyCloud::Load(int tessFactor)
{
    if (tessFactor != -1)
        slices = tessFactor;

    program = wxGetApp().GetCurrentProgram().GetHandle();

    if (!dirty) {
        wxGetApp().UpdateVerts(numVertices * blobCount);
        return;
    }

    previousTime = wxGetLocalTimeMillis().GetLo();
        
     // Set up some default values for the uniforms and send them to the card.
    count = TUniform("BlobbyCount", "int", program);
    count.Set(blobCount);
    count.Load();

    char name[] = "BlobbyPos[xx]";
    for(int ii = 0; ii < maxNearestBlobs; ++ii) {
        sprintf(name, "BlobbyPos[%2d]", ii);
        blobbyPos[ii] = TUniform(name, "vec3", program);
        blobbyPos[ii].Set(0.0f, 0.0f, 0.0f);
        blobbyPos[ii].Load();
    }

    // generate the source sphere display list
    wxGetApp().PushMessage("Generating display list...");
    
    glNewList(glid, GL_COMPILE);
    numVertices = TSphere().Draw(slices);
    glEndList();

    dirty = false;
    wxGetApp().UpdateVerts(numVertices * blobCount);
    wxGetApp().PopMessage();
}

void TBlobbyCloud::Draw() const
{
    for (int ii = 0; ii < blobCount; ++ii) {
        //
        // Find the closest neighbours
        //
        for(int jj = 0; jj < blobCount; ++jj) {
            vec3 distSqr = positions[ii] - positions[jj];
            nearestBlobs[jj].blob = jj;
            nearestBlobs[jj].distSqr = distSqr.mag2();
        }

        std::sort(nearestBlobs.begin(), nearestBlobs.end(), compareBlobs);

        //
        // This is safe as we should always get 1 (ie. the actual blob itself)
        //
        int nearestCount = 0;
        TBlobbies::iterator pBlobs = nearestBlobs.begin();
        do {
            vec3 center = positions[pBlobs->blob];
            blobbyPos[nearestCount].Set(center.x, center.y + 2.5f, center.z);
            blobbyPos[nearestCount].Load();
            ++nearestCount;
            ++pBlobs;
        } while((pBlobs->distSqr < blobbyCutoff) && (nearestCount < maxNearestBlobs));

        //
        // Tell the shader how many blobs to test
        //
        count.Set(nearestCount);
        count.Load();

        //
        // Render it.  
        // The scale factor is to adjust each vertex so it is closer to the desirable radius of influence,
        // hence ensures less travel is required for the verts as they migrate along the normal to find the isosurface.
        // Ideally we would change the size of the original sphere, but this works just as well.
        //
        glPushMatrix();
        glScalef(0.1f, 0.1f, 0.1f);
        glCallList(glid);
        glPopMatrix();
    }
}
