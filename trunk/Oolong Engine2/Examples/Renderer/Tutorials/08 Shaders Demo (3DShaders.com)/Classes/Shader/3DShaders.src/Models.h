//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef MODELS_H
#define MODELS_H
#include "Tables.h"
#include "Model.h"

class TModelList
{
  public:
    TModelList();
    ~TModelList();
    void Load(Id id);
    void Load(const char* filename);
    void ResetTesselation();
    void PushTesselation(int slices);
    void PopTesselation();
    void SetTesselation(int slices);
    TModel& Current();
    const TModel& Current() const;
    Id CurrentId() const { return (current == 0) ? Id(0) : current->GetId(); }
  private:
    void glSetup();
    TModel* models[Id::NumModels];
    TModel* current;
    bool init;
    int slices;
    TModel* previous;
};

#endif
