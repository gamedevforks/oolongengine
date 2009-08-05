//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef ADJUSTER_H
#define ADJUSTER_H
#include <map>
#include <wx/wx.h>
class TFrame;
class TUniformList;

typedef std::pair<TUniform*, int> TUniformComponent;
typedef std::map<int, TUniformComponent> TIdUniformComponentMap;
typedef std::map<int, TUniform*> TIdUniformMap;

// Dialog box with sliders and color pickers for interacting with uniforms.
class TAdjuster : public wxDialog {
  DECLARE_DYNAMIC_CLASS(TAdjuster)
  public:
    void Create(wxWindow* parent);
    void OnClose(wxCloseEvent& event);
    void OnSlide(wxScrollEvent& event);
    void OnClick(wxCommandEvent& event);
    void Set(const TUniformList& uniforms);
  private:
    TIdUniformComponentMap sliders;
    TIdUniformMap pickers;
    DECLARE_EVENT_TABLE()
    friend class TApp;
};

#endif
