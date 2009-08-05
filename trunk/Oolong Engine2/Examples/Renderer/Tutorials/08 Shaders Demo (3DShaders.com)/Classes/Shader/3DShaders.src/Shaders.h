//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef SHADERS_H
#define SHADERS_H
#include <vector>
#include <list>
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/filename.h>
#include <expat.h>
#include "Tables.h"
#include "Vector.h"
#include "Textures.h"
#include "Uniforms.h"
class TProgram;
class TFrame;

class TProgram
{
  public:
    TProgram(const wxString& name, const wxString& vertexFilename, const wxString& fragmentFilename, 
             bool reflection, bool blending, bool depth, Id stress);
    ~TProgram();
    TUniform* NewUniform(const wxString& name, const wxString& type);
    wxString GetName() const { return name; }
    bool Load();
    bool IsEmpty() const { return !invalid && !ready; }
    void AddModel(const wxString& name);
    void SubtractModel(const wxString& name);
    void Lod(int lod) { this->lod = lod; }
    void Mipmap(int mipmap) { this->mipmap = mipmap ? true : false; }
    void Validate(TFrame* frame);
    bool Blending() const { return blending; }
    bool Reflection() const { return reflection; }
    bool Depth() const { return depth; }
    void Animate(int period) { uniforms.Animate(period); }
    bool HasSilhouette() const { return silhouette; }
    void SetSilhouette() { silhouette = true; }
    bool Invalid();
    GLuint GetHandle() const { return program; }
    void TextureStage(int stage) { this->stage = stage; }
    void AddTexture(const wxString& file);
    void SubtractTexture(const wxString& file);
    bool CanAnimate() const { return hasMotion; }
    void SetMotion() { hasMotion = true; }
    void SetDefaultTexture(Id id) { defaultTexture = id; }
    void SetDefaultModel(Id id) { defaultModel = id; }
    void RequiresTangents() { tangents = true; }
    void RequiresBinormals() { binormals = true; }
    Id GetStress() const { return stress; }
  private:
    static bool compile(GLuint shader, const wxString& filename);
    bool ready;
    bool invalid;
    bool silhouette;
    bool hasMotion;
    int stage;
    int lod;
    bool mipmap;
    Id stress;
    wxString name;
    wxString vertexFilename;
    wxString fragmentFilename;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint program;
    Id defaultTexture;
    Id defaultModel;
    TUniformList uniforms;
    bool blending;
    bool reflection;
    bool tangents;
    bool binormals;
    bool depth;

    // bitmasks of valid models and textures for this program
    unsigned models;
    unsigned textures[TTextureList::NumStages];
};

typedef std::vector<TProgram> TProgramVector;

class TProgramList : public wxListView {
  public:
    TProgramList();
    bool Create(wxWindow* parent);
    void Autosize();
    void OnUp();
    void OnDown();
    void OnSelect(wxCommandEvent& event);
    void OnFocus(wxFocusEvent& event);
    void SetFolder(const wxString& folder) { this->folder = folder; }
    void Load(const wxFileName& xmlFile);
    bool Next(Id stress, bool ff);
    void First(Id stress, bool ff);
    int GetItemCountStress(Id stress);
    bool Load();
    void Reload();
    void SetItemColor(int selection, const wxColour& color);
    TProgram* NewShader(const wxString& name, const wxString& vertex, const wxString& fragment, bool blending, bool reflection, bool depth, Id stress);
    wxString GetFolder() const { return folder; }
    TProgram& Current() { return GetFirstSelected() >= 0 ? programs[GetFirstSelected()] : programs[lastSelection]; }
    const TProgram& Current() const { return GetFirstSelected() >= 0 ? programs[GetFirstSelected()] : programs[lastSelection]; }
  private:
    void parseXml();
    int lastSelection;
    wxString folder;
    wxFileName xmlFile;
    TProgramVector programs;
    TFrame* frame;
    DECLARE_EVENT_TABLE()
};

#endif
