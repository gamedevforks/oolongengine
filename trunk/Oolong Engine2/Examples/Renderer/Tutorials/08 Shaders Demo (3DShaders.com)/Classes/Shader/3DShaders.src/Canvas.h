//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef CANVAS_H
#define CANVAS_H
#include "Trackball.h"
#include "Podium.h"
#include <wx/glcanvas.h>
class TFrame;

// GUI widget to contain the GL viewport.
class TCanvas : public wxGLCanvas {
  public:
    TCanvas(wxWindow *parent, int pixelFormat = -1);
    void Draw();
    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event) {}
    void OnMouse(wxMouseEvent& event) { trackball.OnMouse(event); }
    void OnSize(wxSizeEvent& event);
    void NextBackgroundColor();
    void SetBackgroundTexture(Id id) { backgroundTexture = id; }
    void SetBackgroundColor(Id id);
    void SetBlackBackground() { glClearColor(0, 0, 0, 0); }
    void Animate(int period);
    void DisableAA();
    bool IsFrozen() const;
    void Dirty() { dirty = 2; } // used to ensure that at least a couple of draws will occur (needed when switching to glass)
    bool MipmapSupport() const { return mipmapSupport; }
    bool GL2Support() const { return gl2Support; }
    bool AASupport() const { return aaSupport; }
    float GetZoom() const { return zoom; }
    void SetZoom(float z);
    void Reset(bool all = false);
    void Update() { Refresh(FALSE); }
    void Refresh(bool eraseBackground = TRUE, const wxRect* rect = NULL);
    vec3 GetWorldSpace(int x, int y);
    static const float CameraZ;
    static int Attributes[];
    static const int DoubleBufferAttributeIndex;
  private:
    void drawModel() const;
    void drawLogo() const;
    void drawBackground() const;
    void glSetup();
    bool mipmapSupport;
    bool gl2Support;
    bool aaSupport;
    bool glReady;
    bool multisampling;
    bool needsDraw;
    float zoom;
    int width, height;
    float left, right, bottom, top, znear, zfar;
    int currentColor;
    int dirty;
    TPodium podium;
    TTrackball trackball;
    Id backgroundTexture;
    TFrame* frame;
    DECLARE_EVENT_TABLE()
    friend class TApp;
    friend class TCycle;
};

#endif
