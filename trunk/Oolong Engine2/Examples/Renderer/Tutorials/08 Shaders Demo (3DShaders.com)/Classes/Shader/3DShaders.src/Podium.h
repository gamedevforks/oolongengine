//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef PODIUM_H
#define PODIUM_H

// Represents the circular reflective podium.
class TPodium
{
  public:
    TPodium();
    void Draw() const;
    void DrawTop(bool outline = false) const { drawCircle(y, outline, false); }
    void DrawBottom(bool outline = false) const { drawCircle(y - thickness, outline, true); }
    void DrawSide() const;
  private:
    void drawCircle(float y, bool outline, bool clockwise) const;
    void drawCircle(float y, bool clockwise) const;
    float delta;
    float xrad;
    float yrad;
    float trad;
    float thickness;
    float y;
};

#endif
