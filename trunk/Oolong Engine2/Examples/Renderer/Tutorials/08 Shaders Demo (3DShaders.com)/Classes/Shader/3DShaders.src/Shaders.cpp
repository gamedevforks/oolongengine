//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include <wx/wx.h>
#include <fcntl.h>
#include "Shaders.h"
#include "Frame.h"
#include "Canvas.h"
#include "Xml.h"


BEGIN_EVENT_TABLE(TProgramList, wxListView)
    EVT_LIST_ITEM_SELECTED(Id::ListBox, TProgramList::OnSelect) 
    EVT_SET_FOCUS(TProgramList::OnFocus) 
END_EVENT_TABLE()

bool TProgramList::Create(wxWindow* parent)
{
    Xml::pProgramList = this;
    frame = (TFrame*) parent->GetParent();
    wxListView::Create(parent, Id::ListBox, wxDefaultPosition, wxDefaultSize, wxLC_SINGLE_SEL | wxLC_REPORT | wxLC_NO_HEADER);
    SetBackgroundColour(*wxWHITE);
    parseXml();

    int index = FindItem(0, wxGetApp().StartupProgram());
    if (index < 0)
        index = 0;
    Select(index);

    return true;
}

TProgram* TProgramList::NewShader(const wxString& name, const wxString& vertex, const wxString& fragment,
                                  bool blending, bool reflection, bool depth, Id stress)
{
    programs.push_back(TProgram(name, vertex, fragment, blending, reflection, depth, stress));
    return &programs.back();
}

TUniform* TProgram::NewUniform(const wxString& name, const wxString& type)
{
    uniforms.push_back(TUniform(name, type));
    return &uniforms.back();
}

void TProgramList::OnSelect(wxCommandEvent& event)
{
    wxGetApp().Canvas()->Dirty();
    frame->Update();
    event.Skip();
}

// Return true if a valid program has been loaded.
bool TProgramList::Load()
{
    int selection = GetFirstSelected();
    if (selection < 0)
        selection = lastSelection;

    bool changed = selection != lastSelection;
    lastSelection = selection;

#ifdef FF
    if (!selection) {
        if (changed)
            programs[selection].Validate(frame);
        return false;
    }
#endif

    bool empty = programs[selection].IsEmpty();

    if (!programs[selection].Load()) {
        if (empty)
            SetItemColor(selection, *wxRED);
        return false;
    }

    if (changed || empty)
        programs[selection].Validate(frame);

    return true;
}

TProgram::TProgram(const wxString& name, const wxString& vertexFilename, const wxString& fragmentFilename,
                   bool blending, bool reflection, bool depth, Id stress) :
    defaultModel(0), defaultTexture(0),
    vertexShader(0), fragmentShader(0), program(0),
    vertexFilename(vertexFilename), hasMotion(false),
    fragmentFilename(fragmentFilename), stress(stress),
    ready(false), invalid(false), silhouette(false),
    name(name), models(0), stage(0), lod(0),
    blending(blending), reflection(reflection), depth(depth), mipmap(true)
{
    memset(textures, 0, sizeof(textures));
    tangents = false;
    binormals = false;
}

TProgram::~TProgram()
{
    if (ready)
        glDeleteProgram(program);
}

bool TProgram::compile(GLuint shader, const wxString& filename)
{
    int file, size;
    char* buffer;
    const char* pFilename = filename.c_str();

    wxGetApp().Printf("Compiling '%s'...\n", pFilename);
    file = open(pFilename, O_RDONLY);
    if (file == -1) {
        wxGetApp().Errorf("Unable to open file.\n");
        return false;
    }
    size = lseek(file, 0, SEEK_END);
    buffer = (GLchar*) malloc(size + 1);
    lseek(file, 0, SEEK_SET);
    size = read(file, buffer, size);
    buffer[size] = 0;
    glShaderSource(shader, 1, (const GLchar**) &buffer, 0);
    free(buffer);

    int success;
    glCompileShader(shader);
    wxGetApp().Dump(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return success ? true : false;
}

bool TProgram::Load()
{
    if (!wxGetApp().Canvas()->GL2Support() || !glUseProgram) {
        wxGetApp().SetErrorCode(TApp::NonGL2);
        wxGetApp().Frame()->Close();
        return false;
    }

    if (invalid)
        return false;

    if (ready) {
        glUseProgram(program);

        // It's overkill to reload all uniforms every frame, but 3Dlabs drivers don't resend data.
        return uniforms.Load(program);
    }

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!compile(vertexShader, vertexFilename)) {
        wxGetApp().Errorf("Unable to compile '%s'.\n", vertexFilename.c_str());
        invalid = true;
        return false;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compile(fragmentShader, fragmentFilename)) {
        wxGetApp().Errorf("Unable to compile '%s'.\n", fragmentFilename.c_str());
        invalid = true;
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    int success;
    wxGetApp().Printf("Linking '%s' with '%s'...\n", vertexFilename.c_str(), fragmentFilename.c_str());
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        wxGetApp().Errorf("Failed link.\n");
        wxGetApp().Dump(program);
        invalid = true;
        return false;
    }

    wxGetApp().Printf("Validating...\n");
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        wxGetApp().Errorf("Failed validation.\n");
        wxGetApp().Dump(program);
        invalid = true;
        return false;
    }

    wxGetApp().Dump(program);
    glUseProgram(program);

    if (!uniforms.Load(program)) {
        glUseProgram(0);
        invalid = true;
        return false;
    }

    return ready = true;
}

void TProgramList::OnUp()
{
    int i = GetFirstSelected() - 1;
    if (i >= 0)
        Select(i);
}

void TProgramList::OnDown()
{
    int i = GetFirstSelected() + 1;
    if (i < GetItemCount())
        Select(i);
}

void TProgramList::Reload()
{
    int selection = GetFirstSelected();
    programs.clear();
    ClearAll();
    parseXml();

    if (selection >= 0 && selection < GetItemCount())
        Select(selection);
}

void TProgramList::parseXml()
{
    wxListItem item;
    InsertColumn(0, item);

#ifdef FF
    TProgram* ff = NewShader("Fixed function", "", "", false, true, true, Id::StressNone);
    ff->AddModel("Klein");
    ff->AddModel("All");
    ff->SubtractModel("Alien");
    ff->SubtractModel("BlobbyCloud");
    ff->AddTexture("Grid");
    ff->AddTexture("All");
    InsertItem(0, programs[0].GetName(), 0);
#endif

    if (!Xml::Parse(xmlFile)) {
        wxGetApp().Errorf("Unable to open '%s'.\n", xmlFile.GetFullPath().c_str());
        return;
    }

    unsigned start = 0;
#ifdef FF
    start = 1;
#endif

    for (unsigned i = start; i < programs.size(); ++i) {
        wxFileName filename = programs[i].GetName();
        InsertItem(i, filename.GetName(), 0);
        if (programs[i].Invalid())
            SetItemColor(i, *wxRED);
    }
}

// Don't allow the program list to get focus so that keystrokes won't affect the current selection.
void TProgramList::OnFocus(wxFocusEvent& event)
{
    frame->SetFocus();
}

// Add to the set of valid models for this program.
void TProgram::AddModel(const wxString& name)
{
    if (name == "All") {
        if (!models)
            defaultModel = Id::FirstModel;
        models = ~0;
        return;
    }

    int id = findModel(name);
    if (id) {
        if (!models)
            defaultModel = id;
        models |= (1 << (id - Id::FirstModel));
    } else {
        wxGetApp().Errorf("Unrecognized model '%s'.", name.c_str());
    }
}

// Subtract from the set of valid models for this program.
void TProgram::SubtractModel(const wxString& name)
{
    if (name == "All") {
        models = 0;
        return;
    }

    int id = findModel(name);
    if (id)
        models &= ~(1 << (id - Id::FirstModel));
    else
        wxGetApp().Errorf("Unrecognized model '%s'.", name.c_str());
}

// Add to the set of valid textures for this program.
void TProgram::AddTexture(const wxString& file)
{
    if (file == "All") {
        textures[stage] = ~0;
        return;
    }

    int id = findTexture(file);
    if (id) {
        if (!stage && !textures[0])
            defaultTexture = id;
        textures[stage] |= (1 << (id - Id::FirstTexture));
    }
    else
        wxGetApp().Errorf("Unrecognized texture '%s'.", file.c_str());
}

// Subtract from the set of valid textures for this program.
void TProgram::SubtractTexture(const wxString& file)
{
    if (file == "All") {
        textures[stage] = 0;
        return;
    }

    int id = findTexture(file);
    if (id)
        textures[stage] &= ~(1 << (id - Id::FirstTexture));
    else
        wxGetApp().Errorf("Unrecognized texture '%s'.", file.c_str());
}

// This is called when the currently selected program changes.
void TProgram::Validate(TFrame* frame)
{
    frame->UpdateViewAnimate();

    for (Id id = 0; id < Id::NumModels; ++id)
        if (models & (1 << id))
            frame->Enable(id + Id::FirstModel);

    for (Id id = 0; id < Id::NumModels; ++id)
        if (!(models & (1 << id)))
            frame->Disable(id + Id::FirstModel);

    // Always at least enable the "disable texture" option.
    if (!textures[0])
        textures[0] |= 1;

    for (Id id = 0; id <= Id::LastSelectableTexture - Id::FirstTexture; ++id)
        if (textures[0] & (1 << id))
            frame->Enable(id + Id::FirstTexture);

    for (Id id = 0; id <= Id::LastSelectableTexture - Id::FirstTexture; ++id)
        if (!(textures[0] & (1 << id)))
            frame->Disable(id + Id::FirstTexture);

    // Load the default texture/model if one exists.
    if (defaultTexture) {
        frame->Enable(Id::TextureDisable);
        frame->LoadTexture(defaultTexture, true);
    }

    //
    // Activate any textures beyond stage 0.
    //
    for (stage = 1; stage < TTextureList::NumStages; ++stage) {
        for (Id id = 0; id < Id::NumTextures; ++id)
            if (textures[stage] & (1 << id))
                frame->LoadTexture(id + Id::FirstTexture, stage);
    }

    if (!lod) {
        frame->Enable(Id::ModelIncreaseTesselation);
        frame->Enable(Id::ModelDecreaseTesselation);
        frame->GetModels().PopTesselation();
    }


    if (defaultModel)
        frame->LoadModel(defaultModel);

    // If we've disabled the current model/texture, then change the current model/texture.
    frame->ChooseValid();

    // Set the tesselation factor if the XML specified a LOD (level of detail).
    if (lod && !wxGetApp().IsBenching()) {
        frame->Disable(Id::ModelIncreaseTesselation);
        frame->Disable(Id::ModelDecreaseTesselation);
        frame->GetModels().PushTesselation(lod);
    }

    // Tell the texture list if we want mipmapping or not.
    if (!mipmap)
        frame->GetTextures().DisableMipmaps();
    else
        frame->GetTextures().EnableMipmaps();

    // Tell the model whether or not to include custom attributes.
    frame->GetModels().Current().UpdateAttributes(GetHandle());

    // Refresh the model in case it depends on the shader (eg, alien and skinning)
    frame->GetModels().Current().Refresh();

    // Set up the sliders.
    wxGetApp().Adjuster().Set(uniforms);
}

void TProgramList::Load(const wxFileName& xmlFile)
{
    this->xmlFile = xmlFile;
    Select(0);
    Reload();
}

bool TProgram::Invalid()
{
    if (!models) {
        wxGetApp().Errorf("XML error: at least one model must be specified for every program.\n");
        return invalid = true;
    }

    return invalid;
}

void TProgramList::SetItemColor(int selection, const wxColour& color)
{
    wxListItem info;
    info.SetMask(wxLIST_MASK_FORMAT);
    info.SetId(selection);
    info.SetTextColour(color);
    SetItem(info);
}

void TProgramList::First(Id stress, bool ff)
{
    if (ff) {
        Select(0);
        return;
    }

    int i = 0;

    if (stress != Id::StressNone)
        while (i < GetItemCount() && programs[i].GetStress() != stress)
            ++i;

    if (i < GetItemCount())
        Select(i);
}

bool TProgramList::Next(Id stress, bool ff)
{
    if (ff)
        return false;

    int i = GetFirstSelected() + 1;

    if (stress != Id::StressNone)
        while (i < GetItemCount() && programs[i].GetStress() != stress)
            ++i;

    if (i < GetItemCount()) {
        Select(i);
        return true;
    }

    return false;
}

int TProgramList::GetItemCountStress(Id stress)
{
    if (stress == Id::StressNone)
        return GetItemCount();

    int count = 0;
    for (int i = 0; i < GetItemCount(); ++i)
        if (programs[i].GetStress() == stress)
            ++count;

    return count;
}

void TProgramList::Autosize()
{
    if (Xml::pProgramList)
        SetColumnWidth(0, GetClientSize().GetWidth());
}

TProgramList::TProgramList() : xmlFile("../shaders/default.xml"), folder("../shaders"), lastSelection(-1)
{
    Xml::pProgramList = 0;
}
