//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "Realizm.h"
#include "App.h"
#include "Vector.h"
#include "Surface.h"

float TRealizm::scale = 1.0f / 250.0f;
float TRealizm::offset = -400;

// Create the blob shape by reading in the table at the bottom of this file.
TRealizm::TRealizm(unsigned glid, Id id) : TModel(glid, id)
{
    center = vec3(0, -0.5f, 0);
    int* datum = data;
    int numComponents = *datum++;
    for (int c = 0; c < numComponents; ++c) {
        char type = (char) *datum++;
        switch (type) {

            // Spherical component
            case 's':
            {
                float x = (float) *datum++;
                float y = (float) *datum++;
                float z = (float) *datum++;
                float radius = (float) *datum++;
                float strength = (float) *datum++;

                transform(x, y, z);
                transform(radius);

                TMetaBall* sphere = new TMetaBall(vec3(x, y, z), radius, strength);
                blob.Insert(sphere);
                break;
            }

            // Cylindrical component (tube)
            case 't':
            {
                float x1 = (float) *datum++;
                float y1 = (float) *datum++;
                float z1 = (float) *datum++;
                float x2 = (float) *datum++;
                float y2 = (float) *datum++;
                float z2 = (float) *datum++;
                float radius = (float) *datum++;
                float strength = (float) *datum++;

                transform(x1, y1, z1);
                transform(x2, y2, z2);
                transform(radius);

                TTube* tube = new TTube(vec3(x1, y1, z1), vec3(x2, y2, z2), radius, strength);
                blob.Insert(tube);
                break;
            }
        }
    }
}

void TRealizm::Draw() const
{
    glCallList(glid);
}

void TRealizm::Load(int tessFactor)
{
    if (tessFactor != -1)
        slices = tessFactor;

    if (!dirty) {
        wxGetApp().UpdateVerts(numVertices);
        return;
    }

    wxGetApp().Statusf("Tesselating blob shapes...");
    ::wxStartTimer();
    blob.Polygonize(slices);
    long length = ::wxGetElapsedTime();
    wxGetApp().Statusf("Tesselation took %d milliseconds.", length);

    glNewList(glid, GL_COMPILE);
    numVertices = draw();
    glEndList();

    dirty = false;
    wxGetApp().UpdateVerts(numVertices);
}

int TRealizm::draw() const
{
    return blob.Draw();
}

float TBlob::eval(const vec3& v) const
{
    float f = 0;
    for (TComponents::const_iterator c = components.begin(); c != components.end(); ++c) {
        const TImplicitSurface& component = **c;
        f += component(v);
    }
    f -= threshold;
    return f;
}

void TBlob::Polygonize(int slices)
{
    float cellSize = 0.04f - slices * 0.01f / 30;
    triangles.Setup(this, cellSize);
    vec3 minv(-1.6f, -0.8f, -0.1f);
    vec3 maxv(+1.6f, -0.2f, exploitSymmetry ? 0 : 0.1f);
    triangles.March(minv, maxv);
}

TBlob::~TBlob()
{
    for (TComponents::iterator c = components.begin(); c != components.end(); ++c)
        delete *c;
}

int TBlob::Draw() const
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, triangles.GetVertices()); 
    glNormalPointer(GL_FLOAT, 0, triangles.GetNormals()); 
    glTexCoordPointer(2, GL_FLOAT, 3 * sizeof(float), triangles.GetVertices());  // texture coordinate = vertex position

    if (exploitSymmetry) {
        glPushMatrix();
        glDrawElements(GL_TRIANGLES, 3 * triangles.NumTriangles(), GL_UNSIGNED_INT, triangles.GetIndices());
        glScalef(1,1,-1);
        glDrawElements(GL_TRIANGLES, 3 * triangles.NumTriangles(), GL_UNSIGNED_INT, triangles.GetIndices());
        glPopMatrix();
        return 2 * triangles.NumVertices();
    }

    glDrawElements(GL_TRIANGLES, 3 * triangles.NumTriangles(), GL_UNSIGNED_INT, triangles.GetIndices());
    return triangles.NumVertices();
}

void TRealizm::transform(float& x, float& y, float& z)
{
    x += offset;
    y += offset;
    x *= scale;
    y *= scale;
    y -= 0.5f;
}

void TRealizm::transform(float& radius)
{
    radius *= scale;
}

// define a table with the locations and strengths of all blob components in the Realizm logo
int TRealizm::data[] = {
    45,
    't', 38, 415, 0, 38, 345, 0, 75, 60,
    't', 30, 452, 0, 82, 451, 0, 75, 50,
    's', 91, 451, 0, 75, 70,
    's', 97, 447, 0, 75, 70,
    's', 107, 441, 0, 75, 70,
    's', 112, 430, 0, 75, 70,
    's', 112, 418, 0, 75, 70,
    's', 105, 407, 0, 75, 90,
    's', 90, 400, 0, 75, 70,
    's', 82, 397, 0, 75, 60,
    's', 75, 395, 0, 75, 70,
    't', 75, 395, 0, 110, 341, 0, 75, 50,
    's', 75, 395, 0, 75, -70,
    't', 156, 450, 0, 215, 450, 0, 75, 60,
    't', 155, 450, 0, 155, 347, 0, 75, 60,
    't', 155, 345, 0, 276, 345, 0, 75, 60,
    't', 155, 400, 0, 217, 400, 0, 75, 60,
    's', 155, 450, 0, 75, -60,
    's', 155, 345, 0, 75, -60,
    's', 155, 400, 0, 75, -60,
    't', 297, 455, 0, 350, 342, 0, 75, 60,
    't', 297, 455, 0, 263, 381, 0, 75, 60,
    't', 263, 380, 0, 330, 380, 0, 75, 60,
    's', 297, 455, 0, 75, -60,
    's', 263, 381, 0, 75, -60,
    's', 330, 380, 0, 75, -60,
    't', 387, 449, 0, 388, 344, 0, 75, 60,
    't', 388, 344, 0, 440, 344, 0, 75, 60,
    's', 388, 344, 0, 75, -60,
    's', 481, 451, 0, 85, 47,
    't', 480, 405, 0, 480, 345, 0, 75, 60,
    't', 524, 453, 0, 597, 453, 0, 75, 60,
    't', 597, 453, 0, 524, 343, 0, 75, 60,
    't', 524, 343, 0, 601, 343, 0, 75, 60,
    's', 524, 343, 0, 75, -60,
    's', 597, 453, 0, 75, -60,
    't', 650, 345, 0, 650, 449, 0, 75, 60,
    't', 650, 449, 0, 668, 449, 0, 75, 60,
    't', 668, 449, 0, 710, 349, 0, 75, 60,
    't', 710, 349, 0, 751, 449, 0, 75, 60,
    't', 751, 449, 0, 772, 449, 0, 75, 60,
    't', 772, 449, 0, 772, 344, 0, 75, 60,
    's', 710, 349, 0, 75, -60,
    's', 650, 449, 0, 75, -70,
    's', 772, 449, 0, 75, -70,
};
