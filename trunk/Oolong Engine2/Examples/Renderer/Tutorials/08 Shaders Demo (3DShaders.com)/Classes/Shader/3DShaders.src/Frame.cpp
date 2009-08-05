//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Frame.h"
#include "Canvas.h"
#include "Tables.h"

const int TFrame::StatusBarWidths[] = {-1, 100, 75};

BEGIN_EVENT_TABLE(TFrame, wxFrame)
    EVT_SIZE(TFrame::OnSize)
    EVT_MENU(Id::FileResetAll, TFrame::OnFileResetAll)
    EVT_MENU(Id::FileLoadXML, TFrame::OnFileLoadXML)
    EVT_MENU(Id::FileCycle, TFrame::OnFileCycle)
    EVT_MENU(Id::FileExit, TFrame::OnFileExit)
    EVT_MENU(Id::ViewRefresh, TFrame::OnViewRefresh)
    EVT_MENU(Id::ViewReset, TFrame::OnViewReset)
    EVT_MENU(Id::ViewFullscreen, TFrame::OnViewFullscreen)
    EVT_MENU(Id::ViewShaderList, TFrame::OnViewShaderList)
    EVT_MENU(Id::ViewInfoLog, TFrame::OnViewInfoLog)
    EVT_MENU(Id::ViewAdjuster, TFrame::OnViewAdjuster)
    EVT_MENU(Id::TextureAbstract, TFrame::OnBackgroundTextures)
    EVT_MENU(Id::TextureWeave, TFrame::OnBackgroundTextures)
    EVT_MENU(Id::BackgroundCyan, TFrame::OnBackgroundColors)
    EVT_MENU(Id::BackgroundBlue, TFrame::OnBackgroundColors)
    EVT_MENU(Id::BackgroundYellow, TFrame::OnBackgroundColors)
    EVT_MENU(Id::BackgroundRed, TFrame::OnBackgroundColors)
    EVT_MENU(Id::BackgroundBlack, TFrame::OnBackgroundColors)
    EVT_MENU(Id::BackgroundWhite, TFrame::OnBackgroundColors)
    EVT_MENU(Id::ViewAnimate, TFrame::OnViewAnimate)
    EVT_MENU(Id::ViewDemo, TFrame::OnViewDemo)
    EVT_MENU(Id::ViewPerspective, TFrame::OnRefreshCamera)
    EVT_MENU(Id::ViewPodium, TFrame::OnRefreshCamera)
    EVT_MENU(Id::ViewWaitVsync, TFrame::OnViewWaitVsync)
    EVT_MENU(Id::ModelKlein, TFrame::OnModel)
    EVT_MENU(Id::ModelTrefoil, TFrame::OnModel)
    EVT_MENU(Id::ModelConic, TFrame::OnModel)
    EVT_MENU(Id::ModelTorus, TFrame::OnModel)
    EVT_MENU(Id::ModelSphere, TFrame::OnModel)
    EVT_MENU(Id::ModelPlane, TFrame::OnModel)
    EVT_MENU(Id::ModelAlien, TFrame::OnModel)
    EVT_MENU(Id::ModelTeapot, TFrame::OnModel)
    EVT_MENU(Id::ModelRealizm, TFrame::OnModel)
    EVT_MENU(Id::ModelPointCloud, TFrame::OnModel)
    EVT_MENU(Id::ModelBlobbyCloud, TFrame::OnModel)
    EVT_MENU(Id::ModelIncreaseTesselation, TFrame::OnIncreaseTesselation)
    EVT_MENU(Id::ModelDecreaseTesselation, TFrame::OnDecreaseTesselation)
    EVT_MENU(Id::ModelExternal, TFrame::OnModelExternal)
    EVT_MENU(Id::TextureDisable, TFrame::OnTexture)
    EVT_MENU(Id::TextureLogo, TFrame::OnTexture)
    EVT_MENU(Id::TextureBaboon, TFrame::OnTexture)
    EVT_MENU(Id::TextureWater, TFrame::OnTexture)
    EVT_MENU(Id::TextureLeopard, TFrame::OnTexture)
    EVT_MENU(Id::TextureLava, TFrame::OnTexture)
    EVT_MENU(Id::TextureGrid, TFrame::OnTexture)
    EVT_MENU(Id::TextureExternal, TFrame::OnTextureExternal)
    EVT_MENU(Id::HelpCommandLine, TFrame::OnHelpCommandLine)
    EVT_MENU(Id::HelpKeyboard, TFrame::OnHelpKeyboard)
    EVT_MENU(Id::HelpAbout, TFrame::OnHelpAbout)
    EVT_SASH_DRAGGED(Id::ListSash, TFrame::OnSashDrag)
END_EVENT_TABLE()

TFrame::TFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : canvas(0), sash(0), wxFrame(0, -1, title, pos, size, wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN)
{
    wxGetApp().SetFrame(this);
    textures.Set(wxGetApp().Logo() ? Id::TextureLogo : (wxGetApp().Simple() ? Id::TextureDisable : Id::TextureGrid));
    CreateStatusBar(2);
    GetStatusBar()->SetStatusWidths(2, StatusBarWidths);
    wxGetApp().Ready();

    log.Create(this);
    helpAbout.Create(this);
    helpKeyboard.Create(this);
    adjuster.Create(this);
    canvas = new TCanvas(this);

#ifdef WIN32
    //
    // Under win32, SetPixelFormat may not be called more than once on the same window.
    // Moreover we can't call ChoosePixelFormat until we have a valid GL context.
    // The solution is to simply destroy and re-create the canvas.
    //
    int pixelFormat = -1;
    if (!wxGetApp().Simple() && !wxGetApp().Single() &&
        ChooseMultisamplePixelFormat((HDC) canvas->GetHDC(), &pixelFormat) && pixelFormat != -1) {
        delete canvas;
        canvas = new TCanvas(this, pixelFormat);
        glDisable(GL_MULTISAMPLE);
    }
#endif

    const char* vendor = (const char*) glGetString(GL_VENDOR);
    if (!strstr(vendor, "3Dlabs"))
    {
        wxMessageDialog queryAA(this,
            "Do you wish to enable antialiased points and lines?  "
            "On some OpenGL implementations, this feature is detrimental to performance.\n\nIf you are unsure answer 'No'.",
            "Enable AA?", wxNO_DEFAULT | wxYES_NO | wxICON_QUESTION);
        if (queryAA.ShowModal() == wxID_NO)
            canvas->DisableAA();
    }

    wxMenu* menu;
    wxMenuBar* menuBar = new wxMenuBar;

    menu = new wxMenu;
    menu->Append(Id::FileResetAll, "Reset all\tCtrl+Home");
    menu->AppendCheckItem(Id::FileCycle, "Cycle Mode...");
    menu->AppendSeparator();
    menu->Append(Id::FileLoadXML, "Open Program List...\tCtrl+O", "Load an XML file describing a list of shaders.");
    menu->Append(Id::TextureExternal, "Open Texture...\tCtrl+Z", "Load a texture from a JPEG/PNG/BMP file.");
    menu->Append(Id::ModelExternal, "Open Model...\tCtrl+T", "Load a model from an OBJ file.");
    menu->AppendSeparator();
    menu->Append(Id::FileExit, "Exit");
    menuBar->Append(menu, "File");

    wxMenu* multisampleMenu = new wxMenu;
    multisampleMenu->AppendRadioItem(Id::MultisampleAlwaysOff, "Always off");
    multisampleMenu->AppendRadioItem(Id::MultisampleAlwaysOn, "Always on");
    multisampleMenu->AppendRadioItem(Id::MultisampleOnWhenStopped, "On when stopped");

    wxMenu* textureMenu = new wxMenu;
    textureMenu->AppendRadioItem(Id::TextureAbstract, "Abstract");
    textureMenu->AppendRadioItem(Id::TextureWeave, "Weave");

    wxMenu* colorMenu = new wxMenu;
    colorMenu->AppendRadioItem(Id::BackgroundCyan, "Cyan -> Blue");
    colorMenu->AppendRadioItem(Id::BackgroundBlue, "Blue -> Cyan");
    colorMenu->AppendRadioItem(Id::BackgroundYellow, "Yellow -> White");
    colorMenu->AppendRadioItem(Id::BackgroundRed, "Red -> Yellow");
    colorMenu->AppendRadioItem(Id::BackgroundBlack, "Black -> White");
    colorMenu->AppendRadioItem(Id::BackgroundWhite, "White -> Black");

    wxMenu* backgroundMenu = new wxMenu;
    backgroundMenu->AppendRadioItem(Id::BackgroundSolidColor, "Solid color");
    backgroundMenu->AppendRadioItem(Id::BackgroundColorGradient, "Color gradient");
    backgroundMenu->AppendRadioItem(Id::BackgroundScaledTexture, "Stretched texture");
    backgroundMenu->AppendRadioItem(Id::BackgroundTiledTexture, "Tiled texture");
    backgroundMenu->AppendSeparator();
    backgroundMenu->Append(Id::BackgroundTextures, "Textures", textureMenu);
    backgroundMenu->Append(Id::BackgroundColors, "Colors", colorMenu);

    menu = new wxMenu;
    menu->Append(Id::ViewRefresh, "Recompile\tF5");
    menu->Append(Id::ViewReset, "Reset\tHome", "Stop rotation and center the view.");
    menu->AppendCheckItem(Id::ViewFullscreen, "Fullscreen\tCtrl+Enter", "Use the entire display.");
    menu->AppendSeparator();
    menu->AppendCheckItem(Id::ViewPodium, "Podium");
    menu->AppendCheckItem(Id::ViewAnimate, "Animate shader");
    menu->AppendCheckItem(Id::ViewWireframe, "Wireframe");
    menu->AppendCheckItem(Id::ViewCulling, "Cull backfaces");
    menu->AppendCheckItem(Id::ViewPerspective, "Perspective projection");
    menu->Append(Id::ViewBackground, "Background", backgroundMenu);
#ifdef WIN32
    menu->Append(Id::ViewMultisample, "Multisampling", multisampleMenu);
#endif
    menu->AppendSeparator();
    menu->AppendCheckItem(Id::ViewAdjuster, "Uniform adjuster\tAlt+U");
    menu->AppendCheckItem(Id::ViewInfoLog, "Information log\tAlt+I");
    menu->AppendCheckItem(Id::ViewShaderList, "Shader list pane\tAlt+S");
    menuBar->Append(menu, "View");

    menu = new wxMenu;
    menu->AppendRadioItem(Id::ModelKlein, "Klein Bottle");
    menu->AppendRadioItem(Id::ModelTrefoil, "Trefoil Knot");
    menu->AppendRadioItem(Id::ModelConic, "Spiral Conic");
    menu->AppendRadioItem(Id::ModelTorus, "Torus");
    menu->AppendRadioItem(Id::ModelSphere, "Sphere");
    menu->AppendRadioItem(Id::ModelPlane, "Square");
    menu->AppendRadioItem(Id::ModelAlien, "Alien");
    menu->AppendRadioItem(Id::ModelTeapot, "Teapot");
    menu->AppendRadioItem(Id::ModelRealizm  , "Wildcat Realizm Logo");
    menu->AppendRadioItem(Id::ModelPointCloud, "Point Cloud");
    menu->AppendRadioItem(Id::ModelBlobbyCloud, "Blobby Cloud");
	menu->AppendSeparator();
    menu->Append(Id::ModelIncreaseTesselation, "Increase Tesselation\tPgUp");
    menu->Append(Id::ModelDecreaseTesselation, "Decrease Tesselation\tPgDn");
    menuBar->Append(menu, "Model");

    menu = new wxMenu;
    menu->AppendRadioItem(Id::TextureDisable, "Disable");
    menu->AppendRadioItem(Id::TextureGrid, "Grid");
    menu->AppendRadioItem(Id::TextureLogo, "3Dlabs logo");
    menu->AppendRadioItem(Id::TextureBaboon, "Baboon");
    menu->AppendRadioItem(Id::TextureWater, "Water");
    menu->AppendRadioItem(Id::TextureLeopard, "Leopard");
    menu->AppendRadioItem(Id::TextureLava, "Lava");
    menuBar->Append(menu, "Texture");

    menu = new wxMenu;
    menu->Append(Id::HelpKeyboard, "Keyboard...");
    menu->Append(Id::HelpCommandLine, "Command line options...");
    menu->Append(Id::HelpAbout, "About...");
    menuBar->Append(menu, "Help");

    SetMenuBar(menuBar);
    InitMenuState();

    SetIcon(wxIcon("3DLABS_ICON", wxBITMAP_TYPE_ICO_RESOURCE));

    sash = new wxSashLayoutWindow(this, Id::ListSash);
    sash->SetOrientation(wxLAYOUT_VERTICAL);
    sash->SetAlignment(wxLAYOUT_LEFT);
    sash->SetDefaultSize(wxSize(SashWidth, SashHeight));
    sash->SetSashVisible(wxSASH_RIGHT, TRUE);
}

void TFrame::InitPrograms()
{
    programs.Create(sash);
    programs.SetSize(0, 0, sash->GetClientSize().GetWidth(), SashHeight);

    // Work around a problem causing the sash to be undraggable at startup.
    sash->SetDefaultSize(wxSize(0, SashHeight));
    wxLayoutAlgorithm().LayoutFrame(this, canvas);
    sash->SetDefaultSize(wxSize(SashWidth, SashHeight));
    wxLayoutAlgorithm().LayoutFrame(this, canvas);

    programs.SetColumnWidth(0, programs.GetClientSize().GetWidth());
}

void TFrame::InitMenuState()
{
    if (wxGetApp().Simple()) {
        Uncheck(Id::ViewPodium);
        Check(Id::BackgroundSolidColor);
        Check(Id::MultisampleAlwaysOff);
    } else {
        Check(Id::ViewPodium);
        Check(Id::BackgroundScaledTexture);
        Check(wxGetApp().Fsaa());
    }

    Check(Id::ViewPerspective);
    Check(Id::ViewShaderList);
    Check(Id::ViewAnimate);
    Uncheck(Id::ViewWireframe);
    Uncheck(Id::ViewInfoLog);

    if (wxGetApp().CapFps())
        Check(Id::ViewWaitVsync);
    else
        Uncheck(Id::ViewWaitVsync);
}

void TFrame::OnHelpAbout(wxCommandEvent& event)
{
    helpAbout.Show();
}

void TFrame::OnHelpKeyboard(wxCommandEvent& event)
{
    helpKeyboard.Show();
}

void TFrame::OnHelpCommandLine(wxCommandEvent& event)
{
    wxGetApp().GetCommandLineParser()->Usage();
}

void TFrame::LoadTexture(Id id, bool force)
{
    if (!force && !IsEnabled(id)) {
        Check(id);
        for (id = Id::FirstSelectableTexture; id <= Id::LastSelectableTexture; ++id) {
            wxMenuItem* item = GetMenuBar()->FindItem(id);
            if (item && item->IsEnabled())
                break;
        }
        if (id > Id::LastSelectableTexture)
            id = 0;
    }

    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (item)
        item->Check(true);
    else
        Check(Id::TextureDisable);

    textures.Set(id);
    if (programs.GetItemCount())
        programs.Current().SetDefaultTexture(id);
}

void TFrame::LoadModel(Id model)
{
    if (model == Id::ModelBigPlane && wxGetApp().IsBenching()) {
        models.Load(model);
        return;
    }

    Id current = models.CurrentId();
    if (!IsEnabled(model)) {
        Id id;
        Uncheck(model);

        // LastModel is a non-fixed model (externally loaded) so don't try it.
        for (id = current + 1; id < Id::LastModel; ++id) {
            wxMenuItem* item = GetMenuBar()->FindItem(id);
            if (item && item->IsEnabled())
                break;
        }
        if (id >= Id::LastModel) {
            for (id = Id::FirstModel; id <= current; ++id) {
                wxMenuItem* item = GetMenuBar()->FindItem(id);
                if (item && item->IsEnabled())
                    break;
            }
            if (id > current)
                id = Id::LastModel;
        }
        model = (id >= Id::LastModel) ? Id(0) : id;
    }

    wxMenuItem* item = GetMenuBar()->FindItem(model);
    if (item)
        item->Check(true);

    models.Load(model);

    if (!model || model == Id::ModelExternal) {
        Disable(Id::ModelIncreaseTesselation);
        Disable(Id::ModelDecreaseTesselation);
    } else {
        Enable(Id::ModelIncreaseTesselation);
        Enable(Id::ModelDecreaseTesselation);
    }

    // If the podium is turned off, aim the camera at the center of the model rather than (0,0,0).
    if (!IsChecked(Id::ViewPodium) && !wxGetApp().IsBenching())
        canvas->Reset();

    canvas->Update();
}

void TFrame::NextModel()
{
    Id model = models.CurrentId();
    if (model < Id::LastModel - 1)
        LoadModel(model + 1);
    else
        LoadModel(Id::FirstModel);
}

void TFrame::NextTexture()
{
    Id texture = textures.GetId();
    if (texture < Id::LastSelectableTexture)
        LoadTexture(texture + 1);
    else if (texture == Id::LastSelectableTexture)
        LoadTexture(Id::FirstSelectableTexture);
}

void TFrame::OnIncreaseTesselation(wxCommandEvent& event)
{
    models.Current().IncreaseTesselation();
}

void TFrame::OnDecreaseTesselation(wxCommandEvent& event)
{
    models.Current().DecreaseTesselation();
}

void TFrame::OnBackgroundColors(wxCommandEvent& event)
{
    canvas->SetBackgroundColor(event.GetId());
}

void TFrame::OnBackgroundTextures(wxCommandEvent& event)
{
    canvas->SetBackgroundTexture(event.GetId());
}

void TFrame::OnTexture(wxCommandEvent& event)
{
    LoadTexture(event.GetId());
}

void TFrame::OnViewReset(wxCommandEvent& event)
{
    wxGetApp().Ready();
    canvas->Reset();
}

void TFrame::OnViewFullscreen(wxCommandEvent& event)
{
    ShowFullScreen(IsChecked(Id::ViewFullscreen));
}

void TFrame::OnFileResetAll(wxCommandEvent& event)
{
    programs.Select(0);
    programs.Load();
    models.ResetTesselation();
    LoadTexture(Id::TextureGrid);
    LoadModel(Id::ModelKlein);
    wxGetApp().Ready();
    canvas->Reset(true);
    InitMenuState();
    OnViewShaderList(event);
}

void TFrame::OnModelExternal(wxCommandEvent& event)
{
    wxString filename = wxFileSelector("Open Model", "..", "", "", "Wavefront model (*.obj)|*.obj");
    if (!filename.empty())
        models.Load(filename);
}

void TFrame::OnTextureExternal(wxCommandEvent& event)
{
    wxString filename = wxFileSelector("Open Texture", "..", "", "", "BMP files (*.bmp)|*.bmp|PNG files (*.png)|*.png|JPEG files (*.jpg, *.jpeg)|*.jpg;*.jpeg");
    if (!filename.empty()) {
        textures.Set(Id::TextureExternal);
        textures.Load(filename);
    }
}

void TFrame::OnSashDrag(wxSashEvent& event)
{
    if (event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE) {
        sash->SetDefaultSize(wxSize(0, SashHeight));
        Uncheck(Id::ViewShaderList);
    } else if (event.GetDragStatus() == wxSASH_STATUS_OK && event.GetDragRect().width) {
        sash->SetDefaultSize(wxSize(event.GetDragRect().width, SashHeight));
    } else {
        sash->SetDefaultSize(wxSize(SashWidth, SashHeight));
    }

    wxLayoutAlgorithm().LayoutFrame(this, canvas);
    programs.Autosize();
}

void TFrame::OnSize(wxSizeEvent& event)
{
    wxLayoutAlgorithm().LayoutFrame(this, canvas);
    if (sash)
        programs.Autosize();
    canvas->Update();
    event.Skip();
}

void TFrame::OnViewShaderList(wxCommandEvent& event)
{
    int width = IsChecked(Id::ViewShaderList) ? SashWidth : 0;
    sash->SetDefaultSize(wxSize(width, SashHeight));
    wxLayoutAlgorithm().LayoutFrame(this, canvas);
    programs.Autosize();
    event.Skip();
}

void TFrame::OnViewRefresh(wxCommandEvent& event)
{
    wxGetApp().Ready();
    programs.Reload();
    programs.Autosize();
}

void TFrame::OnViewInfoLog(wxCommandEvent& event)
{
    if (IsChecked(Id::ViewInfoLog)) {
        log.Show();
        SetFocus();
    } else {
        log.Hide();
    }
    event.Skip();
}

void TFrame::OnFileExit(wxCommandEvent& event)
{
    Close();
    event.Skip();
}

void TFrame::Check(Id id)
{
    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (!item)
        return;

    item->Check(true);
}

void TFrame::Uncheck(Id id)
{
    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (!item)
        return;

    item->Check(false);
}

void TFrame::Enable(Id id)
{
    if (id == Id::ModelExternal && wxGetApp().IsBenching())
        return;

    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (!item)
        return;

    if (id >= Id::FirstModel && id <= Id::LastModel) {
        item->Enable(true);
        if (!models.CurrentId())
            LoadModel(id);
    } else if (id >= Id::FirstTexture && id <= Id::LastSelectableTexture) {
        item->Enable(true);
        if (!textures.GetId())
            LoadTexture(id);
    } else {
        item->Enable(true);
    }
}

bool TFrame::IsEnabled(Id id) const
{
    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (!item)
        return false;
    return item->IsEnabled();
}


bool TFrame::IsChecked(Id id) const
{
    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (!item)
        return false;
    return item->IsChecked() && item->IsEnabled();
}


void TFrame::Disable(Id id)
{
    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (!item)
        return;

    item->Enable(false);
}

void TFrame::ChooseValid()
{
    for (Id id = Id::FirstModel; id <= Id::LastModel; ++id) {
        wxMenuItem* item = GetMenuBar()->FindItem(id);
        if (item && item->IsChecked() && !item->IsEnabled())
            LoadModel(Id::FirstModel);
    }

    for (Id id = Id::FirstTexture; id <= Id::LastSelectableTexture; ++id) {
        wxMenuItem* item = GetMenuBar()->FindItem(id);
        if (item && item->IsChecked() && !item->IsEnabled())
            LoadTexture(Id::FirstTexture);
    }
}

void TFrame::Toggle(Id id)
{
    switch (id.value) {
        case Id::BackgroundScaledTexture:
            if (IsChecked(id))
                Check(Id::BackgroundColorGradient);
            else
                Check(id);
            return;

        case Id::BackgroundColorGradient:
            if (IsChecked(id))
                Check(Id::BackgroundSolidColor);
            else
                Check(id);
            return;
    }

    wxMenuItem* item = GetMenuBar()->FindItem(id);
    if (item)
        item->Check(!item->IsChecked());

    wxCommandEvent nop;
    switch (id.value) {
        case Id::ViewPodium: OnRefreshCamera(nop); break;
        case Id::ViewWaitVsync: wxGetApp().ToggleWaitVsync(); break;
    }
}

void TFrame::OnViewWaitVsync(wxCommandEvent& event)
{
    wxGetApp().ToggleWaitVsync();
    event.Skip();
}

void TFrame::OnRefreshCamera(wxCommandEvent& event)
{
    canvas->SetZoom(canvas->GetZoom());
}

void TFrame::OnViewAdjuster(wxCommandEvent& event)
{
    UpdateViewAnimate();
    if (IsChecked(Id::ViewAdjuster)) {
        adjuster.Show();
        SetFocus();
    } else {
        adjuster.Hide();
    }
    event.Skip();
}

void TFrame::UpdateViewAnimate()
{
    if (IsChecked(Id::ViewAdjuster) || !programs.Current().CanAnimate())
        Disable(Id::ViewAnimate);
    else
        Enable(Id::ViewAnimate);
}

void TFrame::OnFileLoadXML(wxCommandEvent& event)
{
    wxString filename = wxFileSelector("Open Program List", "../shaders", "", "", "XML (*.xml)|*.xml");
    if (!filename.empty()) {
        programs.Load(filename);
        programs.Autosize();
    }
}

vec3 TFrame::GetCenter() const
{
    vec3 center = !models.CurrentId() ? vec3() : models.Current().GetCenter();
    return center;
}

void TFrame::Update()
{
    wxFrame::Update();
    canvas->Update();
}

void TFrame::OnFileCycle(wxCommandEvent& event)
{
    if (IsChecked(Id::FileCycle))
        wxGetApp().StartCycle();
    else
        wxGetApp().StopCycle();

    event.Skip();
}

