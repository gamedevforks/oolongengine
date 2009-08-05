//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef FRAME_H
#define FRAME_H
#include <wx/wx.h>
#include <wx/laywin.h>
#include "Models.h"
#include "Textures.h"
#include "Shaders.h"
#include "Log.h"
#include "Help.h"
#include "Adjuster.h"
class TCanvas;

class TFrame : public wxFrame {
  public:
    TFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    void OnFileResetAll(wxCommandEvent& event);
    void OnFileExit(wxCommandEvent& event);
    void OnFileLoadXML(wxCommandEvent& event);
    void OnFileCycle(wxCommandEvent& event);
    void OnViewReset(wxCommandEvent& event);
    void OnViewRefresh(wxCommandEvent& event);
    void OnViewFullscreen(wxCommandEvent& event);
    void OnViewAnimate(wxCommandEvent& event) {}
    void OnViewDemo(wxCommandEvent& event) {}
    void OnViewInfoLog(wxCommandEvent& event);
    void OnViewShaderList(wxCommandEvent& event);
    void OnViewAdjuster(wxCommandEvent& event);
    void OnViewWaitVsync(wxCommandEvent& event);
    void OnBackgroundTextures(wxCommandEvent& event);
    void OnBackgroundColors(wxCommandEvent& event);
    void OnModel(wxCommandEvent& event) { LoadModel(event.GetId()); }
    void OnModelExternal(wxCommandEvent& event);
    void OnIncreaseTesselation(wxCommandEvent& event);
    void OnDecreaseTesselation(wxCommandEvent& event);
    void OnTexture(wxCommandEvent& event);
    void OnTextureExternal(wxCommandEvent& event);
    void OnHelpKeyboard(wxCommandEvent& event);
    void OnHelpCommandLine(wxCommandEvent& event);
    void OnHelpAbout(wxCommandEvent& event);
    void OnSashDrag(wxSashEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnRefreshCamera(wxCommandEvent& event);
    void UpdateViewAnimate();
    void Update();
    void SetTexture() { textures.Activate(); }
    void UnsetTexture() { textures.Deactivate(); }
    void SetTexture(Id id) { textures.Activate(id, true); }
    void UnsetTexture(Id id) { textures.Deactivate(id); }
    void LoadTexture(Id id, bool force = false);
    void LoadTexture(Id id, int stage) { textures.Set(id, stage); }
    wxSize TextureSize(Id id) const { return textures.GetSize(id); }
    bool LoadProgram() { return programs.Load(); }
    void InitMenuState();
    void LoadModel(Id id);
    void NextModel();
    void NextTexture();
    void Animate(int period) { programs.Current().Animate(period); }
    void Enable(Id id);
    void Disable(Id id);
    void Check(Id id);
    void Uncheck(Id id);
    void ChooseValid();
    void Toggle(Id id);
    void InitPrograms();
    bool Blending() const { return programs.Current().Blending() || textures.HasAlpha(); }
    bool Depth() const { return programs.Current().Depth(); }
    bool IsFrozen() const { return !IsChecked(Id::ViewAnimate) || !programs.Current().CanAnimate(); }
    void DrawModel() const { models.Current().Draw(); }
    void UpdateModel() { models.Current().Update(); }
    vec3 GetCenter() const;
    TModelList& GetModels() { return models; }
    TTextureList& GetTextures() { return textures; }
    bool IsEnabled(Id id) const;
    bool IsChecked(Id id) const;
    static const int SashWidth = 150;
    static const int SashHeight = 1600;
    static const int StatusBarWidths[];
  private:
    TModelList models;
    TTextureList textures;
    TProgramList programs;
    TAdjuster adjuster;
    TLog log;
    THelpAbout helpAbout;
    THelpKeyboard helpKeyboard;
    wxSashLayoutWindow* sash;
    TCanvas* canvas;
    DECLARE_EVENT_TABLE()
    friend class TApp;
    friend class TCycle;
};

#endif
