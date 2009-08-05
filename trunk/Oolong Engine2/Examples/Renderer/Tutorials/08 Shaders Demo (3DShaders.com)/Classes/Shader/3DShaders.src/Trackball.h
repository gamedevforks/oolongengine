//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef TRACKBALL_H
#define TRACKBALL_H
#include "Vector.h"
#include <wx/timer.h>

class TCanvas;

// Processes mouse motion and keyboard input that affect camera orientation and position.
class TTrackball
{
  public:
    TTrackball();
    void OnIdle(wxIdleEvent& event);
    void OnMouse(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void LoadMatrix() const;
    void SetMatrix() const;
    void MultMatrix() const;
    bool IsFrozen() const { return inertiaTheta == 0 && !validStart; }
    void Reset(bool all = false);
    void Stop();
    int GetFrameCount();
    void SetCanvas(TCanvas* canvas) { this->canvas = canvas; }
    static const float StartZoom;
    static const float InertiaThreshold;
    static const int Delay;
  private:
    long previous;
    int frames;
    TCanvas* canvas;
    bool validStart;
    vec3 vStart;
    vec3 vPrev;
    vec3 vInc;
    vec3 inertiaAxis;
    float inertiaTheta;
    float startZoom;
    mat4 mStart;
    mat4 xform;
};

#endif
