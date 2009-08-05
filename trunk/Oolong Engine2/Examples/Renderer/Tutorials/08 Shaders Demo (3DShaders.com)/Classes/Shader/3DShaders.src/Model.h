//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef MODEL_H
#define MODEL_H
#include "Tables.h"
#include "Vector.h"

class TModel
{
  public:
    TModel(unsigned glid, Id id) : glid(glid), id(id), filename(0), dirty(true),
                                   tangentLoc(-1), binormalLoc(-1) { ResetTesselation(); }

    virtual ~TModel() { if (filename) delete filename; }
    virtual void Draw() const;
    virtual void Update() {}
    virtual void Refresh() {}
    virtual void Load(int slices = -1);
    void Reload() { dirty = true; Load(slices); }
    void ResetTesselation();
    void SetTesselation(int slices);
    int HighTesselation() const;
    int GetTesselation() const { return slices; }
    void IncreaseTesselation();
    void DecreaseTesselation();
    int MinimumTesselation() const;
    void Load(const char* filename);
    Id GetId() const { return id; }
    vec3 GetCenter() const { return center; }
    void UpdateAttributes(unsigned programHandle);
  protected:
    int drawFile();  // reads in a wavefront file and draws it
    int drawCloud(); // draws a bunch of random points
    vec3 center;     // camera target when the podium is off
    int numVertices; // number of vertices in the display list
    unsigned glid;   // OpenGL-assigned id for the display list
    Id id;           // application's id for this model
    int slices;      // tesselation factor
    char* filename;  // used only when id == Id::ModelExternal
    bool dirty;      // true if the model needs to be re-generated

    // locations of custom vertex attributes.  -1 if unused.
    int tangentLoc;
    int binormalLoc;
};

#endif
