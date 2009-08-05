//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include <wx/wx.h>
#include "Model.h"
#include "Surface.h"
#include "Alien.h"
#include "Tables.h"
#include "Frame.h"
#include <cmath>
#include "os.h"
#include "glut_teapot.h"

void TModel::UpdateAttributes(unsigned programHandle)
{
    if (!glGetAttribLocation || !programHandle)
        return;

    if (tangentLoc != glGetAttribLocation(programHandle, "Tangent") ||
        binormalLoc != glGetAttribLocation(programHandle, "Binormal"))
        Reload();
}

void TModel::Load(int tessFactor)
{
    if (id == Id::ModelExternal)
        return;

    unsigned programHandle = wxGetApp().GetCurrentProgram().GetHandle();
    int newTangentLoc = -1;
    int newBinormalLoc = -1;

    if (glGetAttribLocation && programHandle)
    {
        newTangentLoc = glGetAttribLocation(programHandle, "Tangent");
        newBinormalLoc = glGetAttribLocation(programHandle, "Binormal");
    }

    if (tangentLoc !=  newTangentLoc || binormalLoc != newBinormalLoc || slices != tessFactor)
        dirty = true;

    if (!dirty) {
        wxGetApp().UpdateVerts(numVertices);
        return;
    }

    if (tessFactor != -1)
        slices = tessFactor;

    tangentLoc = newTangentLoc;
    binormalLoc = newBinormalLoc;

    wxGetApp().PushMessage("Generating display list...");
    glNewList(glid, GL_COMPILE);
    switch (id) {
        case Id::ModelAlien: numVertices = 0; wxGetApp().Errorf("Alien should be handled in the derived class.\n"); break;
        case Id::ModelRealizm: numVertices = 0; wxGetApp().Errorf("Realizm should be handled in the derived class.\n"); break;
        case Id::ModelSphere: numVertices = TSphere().Draw(slices, tangentLoc, binormalLoc); break;
        case Id::ModelTorus: numVertices = TTorus().Draw(slices, tangentLoc, binormalLoc); break;
        case Id::ModelTrefoil: numVertices = TTrefoil().Draw(slices, tangentLoc, binormalLoc); break;
        case Id::ModelConic: numVertices = TConic().Draw(slices, tangentLoc, binormalLoc); break;
        case Id::ModelKlein: numVertices = TKlein().Draw(slices, tangentLoc, binormalLoc); break;
        case Id::ModelPointCloud: numVertices = drawCloud(); break;
        case Id::ModelBlobbyCloud: numVertices = 0; wxGetApp().Errorf("BlobbyCloud should be handled in the derived class.\n"); break;

        case Id::ModelBigPlane:
            numVertices = TPlane(.001f, 5).Draw(slices, tangentLoc, binormalLoc);
            break;

        case Id::ModelPlane:
            // We actually draw two planes back-to-back.
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            numVertices = TPlane(-.001f).Draw(slices, tangentLoc, binormalLoc);
            numVertices += TPlane(.001f).Draw(slices, tangentLoc, binormalLoc);
            glDisable(GL_CULL_FACE);
            break;

        case Id::ModelTeapot:
            translateTeapot(0, 0, -.5f);
            numVertices = teapot(slices, 1, GL_FILL);
            break;
    }
    glEndList();

    dirty = false;
    wxGetApp().UpdateVerts(numVertices);
    wxGetApp().PopMessage();
}

void TModel::Load(const char* filename)
{
    if (!filename)
        return;

    if (!this->filename || strcmp(this->filename, filename))
        dirty = true;

    if (!dirty) {
        wxGetApp().UpdateVerts(numVertices);
        return;
    }

    delete this->filename;
    this->filename = new char[strlen(filename) + 1];
    strcpy(this->filename, filename);

    numVertices = 0;
    glNewList(glid, GL_COMPILE);
    numVertices = drawFile();
    glEndList();

    dirty = false;
    wxGetApp().UpdateVerts(numVertices);
}

void TModel::Draw() const
{
    glCallList(glid);
}

int TModel::drawCloud()
{
    int totalVerts = 0;
    float slices = this->slices * 100;
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glEnable(GL_BLEND);
    glColor4f(1,1,1,0.5f);
    glBegin(GL_POINTS);
    while (slices--) {
        float radius = (float) rand() / RAND_MAX;
        float u = twopi * (float) rand() / RAND_MAX;
        float v = twopi * (float) rand() / RAND_MAX;

        float x = radius * cosf(v) * sinf(u);
        float z = radius * sinf(v) * sinf(u);
        float y = radius * cosf(u);

        float r = (float) rand() / RAND_MAX;
        float g = (float) rand() / RAND_MAX;
        float b = (float) rand() / RAND_MAX;
        float a = (float) rand() / RAND_MAX;
        glColor4f(r, g, b, a);
        glVertex3f(x, y, z);
        ++totalVerts;
    }
    glEnd();
    glDisable(GL_BLEND);
    return totalVerts;
}

int TModel::drawFile()
{
    int totalVerts = 0;
    FILE* file = fopen(filename, "r");
    bool texgen = true;
    char buf[128];
    #define SAFE_STRING "%127s"

    if (!file) {
        wxGetApp().Errorf("Unable to open model file '%s'.\n", filename);
        TPlane(0).Draw(10);
        return 0;
    }

    unsigned numNormals = 0;
    unsigned numTexCoords = 0;
    unsigned numVertices = 0;

    wxGetApp().Statusf("Loading '%s'.  Obtaining size of model...", filename);
    while(fscanf(file, SAFE_STRING, buf) != EOF) {
        if (buf[0] == 'v') {
            switch (buf[1]) {
                case 'n': ++numNormals; break;
                case 't': ++numTexCoords; texgen = false; break;
                default: ++numVertices; break;
            }
        }
        fgets(buf, sizeof(buf), file);
    }
    rewind(file);

    float* vertices = new float[numVertices * 3 + 3];
    float* texCoords = new float[numTexCoords * 2 + 2];
    float* normals = new float[numNormals * 3 + 3];
    float* pVertices = vertices + 3;
    float* pTexCoords = texCoords + 2;
    float* pNormals = normals + 3;
    float minX = 1000, maxX = -1000;
    float minY = 1000, maxY = -1000;
    float minZ = 1000, maxZ = -1000;

    wxGetApp().Statusf("Loading '%s'.  Reading vertex data...", filename);
    while(fscanf(file, SAFE_STRING, buf) != EOF) {
        if (buf[0] != 'v') {
            fgets(buf, sizeof(buf), file);
            continue;
        }

        switch (buf[1]) {
            case 'n':
                fscanf(file, "%f %f %f",  pNormals, pNormals + 1, pNormals + 2);
                pNormals += 3;
                break;

            case 't':
                fscanf(file, "%f %f",  pTexCoords, pTexCoords + 1);
                pTexCoords += 2;
                break;

            default: 
                fscanf(file, "%f %f %f",  pVertices, pVertices + 1, pVertices + 2);

                if (*pVertices > maxX) maxX = *pVertices;
                if (*pVertices < minX) minX = *pVertices;
                ++pVertices;
                if (*pVertices > maxY) maxY = *pVertices;
                if (*pVertices < minY) minY = *pVertices;
                ++pVertices;
                if (*pVertices > maxZ) maxZ = *pVertices;
                if (*pVertices < minZ) minZ = *pVertices;
                ++pVertices;

                break;
        }
    }
    rewind(file);

    float offsetX = -(maxX + minX) / 2;
    float offsetY = -1 - minY / (maxY - minY);
    float offsetZ = -(maxZ + minZ) / 2;
    float scaleX = (maxX - minX);
    float scaleY = (maxY - minY);
    float scaleZ = (maxZ - minZ);
    float scale = std::max(std::max(scaleX, scaleY), scaleZ) / 2;
    center.y = -1 + (maxY - minY) / (2 * scale);
    for (unsigned i = 1; i < numVertices + 1; ++i) {
        vertices[i*3] += offsetX;
        vertices[i*3] /= scale;
        vertices[i*3 + 1] += offsetY;
        vertices[i*3 + 1] /= scale;
        vertices[i*3 + 2] += offsetZ;
        vertices[i*3 + 2] /= scale;
    }

    unsigned v0, t0, n0;
    unsigned v1, t1, n1;
    unsigned v2, t2, n2;

    if (texgen) {
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    }

    wxGetApp().Statusf("Loading '%s'.  Building display list...", filename);
    glBegin(GL_TRIANGLES);
    while(fscanf(file, SAFE_STRING, buf) != EOF) {
        if (buf[0] != 'f') {
            fgets(buf, sizeof(buf), file);
            continue;
        }

        fscanf(file, SAFE_STRING, buf);

        //
        // VN
        //
        if (strstr(buf, "//")) {
            sscanf(buf, "%d//%d", &v0, &n0);
            fscanf(file, "%d//%d", &v1, &n1);
            fscanf(file, "%d//%d", &v2, &n2);

            glNormal3fv(normals + n0 * 3);
            glVertex3fv(vertices + v0 * 3);
            glNormal3fv(normals + n1 * 3);
            glVertex3fv(vertices + v1 * 3);
            glNormal3fv(normals + n2 * 3);
            glVertex3fv(vertices + v2 * 3);
            totalVerts += 3;

            while(fscanf(file, "%d//%d", &v1, &n1) > 0) {
                glNormal3fv(normals + n0 * 3);
                glVertex3fv(vertices + v0 * 3);
                glNormal3fv(normals + n2 * 3);
                glVertex3fv(vertices + v2 * 3);
                glNormal3fv(normals + n1 * 3);
                glVertex3fv(vertices + v1 * 3);
                totalVerts += 3;
                v2 = v1; n2 = n1;
            }
        }

        //
        // VTN
        //
        else if (sscanf(buf, "%d/%d/%d", &v0, &t0, &n0) == 3) {
            fscanf(file, "%d/%d/%d", &v1, &t1, &n1);
            fscanf(file, "%d/%d/%d", &v2, &t2, &n2);

            glTexCoord2fv(texCoords + t0 * 2);
            glNormal3fv(normals + n0 * 3);
            glVertex3fv(vertices + v0 * 3);
            glTexCoord2fv(texCoords + t1 * 2);
            glNormal3fv(normals + n1 * 3);
            glVertex3fv(vertices + v1 * 3);
            glTexCoord2fv(texCoords + t2 * 2);
            glNormal3fv(normals + n2 * 3);
            glVertex3fv(vertices + v2 * 3);
            totalVerts += 3;

            while(fscanf(file, "%d/%d/%d", &v1, &t1, &n1) > 0) {
                glTexCoord2fv(texCoords + t0 * 2);
                glNormal3fv(normals + n0 * 3);
                glVertex3fv(vertices + v0 * 3);
                glTexCoord2fv(texCoords + t2 * 2);
                glNormal3fv(normals + n2 * 3);
                glVertex3fv(vertices + v2 * 3);
                glTexCoord2fv(texCoords + t1 * 2);
                glNormal3fv(normals + n1 * 3);
                glVertex3fv(vertices + v1 * 3);
                totalVerts += 3;
                v2 = v1; t2 = t1; n2 = n1;
            }
        }

        //
        // VT
        //
        else if (sscanf(buf, "%d/%d", &v0, &t0) == 2) {
            fscanf(file, "%d/%d", &v1, &t1);
            fscanf(file, "%d/%d", &v2, &t2);

            glTexCoord3fv(texCoords + t0 * 2);
            glVertex3fv(vertices + v0 * 3);
            glTexCoord3fv(texCoords + t1 * 2);
            glVertex3fv(vertices + v1 * 3);
            glTexCoord3fv(texCoords + t2 * 2);
            glVertex3fv(vertices + v2 * 3);
            totalVerts += 3;

            while(fscanf(file, "%d/%d", &v1, &t1) > 0) {
                glTexCoord3fv(texCoords + t0 * 2);
                glVertex3fv(vertices + v0 * 3);
                glTexCoord3fv(texCoords + t2 * 2);
                glVertex3fv(vertices + v2 * 3);
                glTexCoord3fv(normals + t1 * 2);
                glVertex3fv(vertices + v1 * 3);
                totalVerts += 3;
                v2 = v1; t2 = t1;
            }
        }

        //
        // V
        //
        else {
            sscanf(buf, "%d", &v0);
            fscanf(file, "%d", &v1);
            fscanf(file, "%d", &v2);

            glVertex3fv(vertices + v0 * 3);
            glVertex3fv(vertices + v1 * 3);
            glVertex3fv(vertices + v2 * 3);
            totalVerts += 3;

            while(fscanf(file, "%d", &v1) > 0) {
                glVertex3fv(vertices + v0 * 3);
                glVertex3fv(vertices + v2 * 3);
                glVertex3fv(vertices + v1 * 3);
                totalVerts += 3;
                v2 = v1;
            }
        }
    }
    glEnd();

    wxGetApp().Ready();

    if (texgen) {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    delete vertices;
    delete texCoords;
    delete normals;
    fclose(file);
    return totalVerts;
    #undef SAFE_STRING
}

void TModel::ResetTesselation()
{
    if (wxGetApp().Simple()) {
        switch (id) {
            case Id::ModelRealizm: slices = 10; break;
            case Id::ModelAlien: slices = 10; break;
            case Id::ModelSphere: slices = 20; break;
            case Id::ModelTorus: slices = 20; break;
            case Id::ModelTrefoil: slices = 50; break;
            case Id::ModelConic: slices = 30; break;
            case Id::ModelKlein: slices = 30; break;
            case Id::ModelPointCloud: slices = 30; break;
            case Id::ModelBlobbyCloud: slices = 20; break;
            case Id::ModelPlane: slices = 2; break;
            case Id::ModelTeapot: slices = 5; break;
            default: slices = 10;
        }
    } else {
        switch (id) {
            case Id::ModelRealizm: slices = 30; break;
            case Id::ModelAlien: slices = 30; break;
            case Id::ModelSphere: slices = 40; break;
            case Id::ModelTorus: slices = 40; break;
            case Id::ModelTrefoil: slices = 100; break;
            case Id::ModelConic: slices = 80; break;
            case Id::ModelKlein: slices = 90; break;
            case Id::ModelPointCloud: slices = 80; break;
            case Id::ModelBlobbyCloud: slices = 40; break;
            case Id::ModelPlane: slices = 2; break;
            case Id::ModelTeapot: slices = 9; break;
            default: slices = 10;
        }
    }
    wxGetApp().Frame()->Enable(Id::ModelDecreaseTesselation);
    dirty = true;
}

int TModel::MinimumTesselation() const
{
    switch (id) {
        case Id::ModelRealizm: return 10;
        case Id::ModelAlien: return 30;
        case Id::ModelSphere: return 40;
        case Id::ModelTorus: return 40;
        case Id::ModelTrefoil: return 100;
        case Id::ModelConic: return 80;
        case Id::ModelKlein: return 10;
        case Id::ModelPointCloud: return 80;
        case Id::ModelBlobbyCloud: return 40;
        case Id::ModelPlane: return 1;
        case Id::ModelTeapot: return 1;
    }
    return 10;
}

int TModel::HighTesselation() const
{
    switch (id) {
        case Id::ModelBlobbyCloud: return 40;
        case Id::ModelAlien: return 64;
    }
    return 512;
}

void TModel::IncreaseTesselation()
{
    dirty = true;
    Load(slices + 2);
    wxGetApp().Frame()->Enable(Id::ModelDecreaseTesselation);
}

void TModel::DecreaseTesselation()
{
    dirty = true;
    if (slices - 2 < MinimumTesselation())
        return;
    Load(slices - 2);
    if (slices - 2 < MinimumTesselation())
        wxGetApp().Frame()->Disable(Id::ModelDecreaseTesselation);
}

void TModel::SetTesselation(int tessFactor)
{
    slices = -1;
    dirty = true;
    Load(tessFactor);
    return;
}
