//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef TEXTURES_H
#define TEXTURES_H
#include "Tables.h"

struct TTexture {
    unsigned glid;
    bool alpha;
    unsigned width;
    unsigned height;
};

class TTextureList {
  public:
    TTextureList();
    void Init();
    void Load(const char* filename);
    void Activate(Id id, bool forceMipmap = false);
    void Deactivate(Id id);
    void SetFilter(bool forceMipmap) const;
    bool HasAlpha(Id id) const { return textures[id - Id::FirstTexture].alpha; }
    void Activate();
    void Deactivate();
    bool HasAlpha() const { return HasAlpha(current[0]); }
    void Set(Id id, int stage = -1);
    Id GetId(int stage = 0) const { return current[stage]; }
    wxSize GetSize(Id id) const;
    void DisableMipmaps() { useMipmap = false; }
    void EnableMipmaps() { useMipmap = true; }
    static const int NumStages = 8;
  private:
    static void loadPng(TTexture& texture, const char* filename);
    bool useMipmap;
    TTexture textures[Id::NumTextures];
    Id current[NumStages];
};

#endif
