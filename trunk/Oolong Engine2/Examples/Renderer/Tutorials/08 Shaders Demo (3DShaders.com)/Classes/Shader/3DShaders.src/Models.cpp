//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Models.h"
#include "os.h"
#include "Alien.h"
#include "Realizm.h"
#include "BlobbyCloud.h"

TModelList::TModelList()
{
    memset(models, 0, sizeof(models));
    init = false;
    current = 0;
    slices = -1;
    ResetTesselation();
}

void TModelList::ResetTesselation()
{
    for (Id i = 0; i < Id::NumModels; ++i)
        if (models[i])
            models[i]->ResetTesselation();
}

void TModelList::SetTesselation(int slices)
{
    for (Id i = 0; i < Id::NumModels; ++i)
        if (models[i])
            models[i]->SetTesselation(slices);
}

void TModelList::glSetup()
{
    unsigned glid = glGenLists(Id::NumModels);
    for (Id i = 0; i < Id::NumModels; ++i) {
        if (i == Id::ModelAlien - Id::FirstModel)
            models[i] = new TAlien(glid + i, Id::ModelAlien);
        else if (i == Id::ModelRealizm - Id::FirstModel)
            models[i] = new TRealizm(glid + i, Id::ModelRealizm);
        else if (i == Id::ModelBlobbyCloud - Id::FirstModel)
            models[i] = new TBlobbyCloud(glid + i, Id::ModelBlobbyCloud);
        else
            models[i] = new TModel(glid + i, Id::FirstModel + i);
    }

    init = true;
}

void TModelList::PushTesselation(int slices)
{
    previous = current;
    this->slices = current->GetTesselation();
    current->Load(slices);
}

void TModelList::PopTesselation()
{
    if (slices != -1) {
        previous->Load(slices);
        current->Load();
    }
    slices = -1;
}

void TModelList::Load(Id id)
{
    if (!init)
        glSetup();

    assert(id >= Id::FirstModel && id <= Id::LastModel);
    current = models[id - Id::FirstModel];
    current->Load();
}

void TModelList::Load(const char* filename)
{
    if (!init)
        glSetup();

    current = models[Id::ModelExternal - Id::FirstModel];
    current->Load(filename);
}

TModelList::~TModelList()
{
    for (int i = 0; i < Id::NumModels; ++i)
        delete models[i];
}

TModel& TModelList::Current()
{
    return *current;
}

const TModel& TModelList::Current() const
{
    return *current;
}
