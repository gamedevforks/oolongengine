//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Frame.h"
#include "Canvas.h"
#include <wx/cmdline.h>

BEGIN_EVENT_TABLE(TApp, wxApp)
    EVT_KEY_UP(TApp::OnKeyUp)
    EVT_KEY_DOWN(TApp::OnKeyDown)
    EVT_IDLE(TApp::OnIdle)
END_EVENT_TABLE()

const wxCmdLineEntryDesc TApp::commandLineDescription[] =
{
    { wxCMD_LINE_SWITCH, "s", "simple", "start with solid background, no podium, no texture, no FSAA, no logo, no initial spin"},
    { wxCMD_LINE_SWITCH, "b", "single", "use single-buffering"},
    { wxCMD_LINE_SWITCH, "l", "logo", "start with the 3Dlabs logo using the plane model"},
    { wxCMD_LINE_SWITCH, "i", "immediate",  "use immediate mode (Alien only)"},
    { wxCMD_LINE_OPTION, "p", "program",  "start with the specified program"},
    { wxCMD_LINE_NONE }
};

extern MSG s_currentMsg;

// The override of the "game loop" isn't much different from the base class version.
// We purposely render here rather than within in an OnPaint handler.
// This method returns the wParam of the WM_QUIT message.
int TApp::MainLoop()
{
    m_keepGoing = TRUE;

    while (m_keepGoing)
    {
        frame->canvas->Draw();
        while (!Pending() && ProcessIdle())
            ;

        DoMessage();
    }

    return s_currentMsg.wParam;
}

// returns errorlevel
int TApp::OnExit()
{
    delete parser;

    if (errorCode == NonGL2)
    {
        wxMessageBox(
            "You must have OpenGL 2.0 compliant drivers to run glsldemo!",
            "OpenGL 2.0 Driver Not Found", wxOK | wxICON_EXCLAMATION, 0
        );
        return 1;
    }

    return 0;
}

bool TApp::OnInit()
{
    errorCode = Success;
    parser = new wxCmdLineParser(argc, argv);
    parser->SetDesc(commandLineDescription);

    wxString description = "start with the specified multisampling mode:";
    description += "\n\t0\talways off";
    description += "\n\t1\talways on";
    description += "\n\t2\ton when stopped";
    parser->AddOption("f", "fsaa", description, wxCMD_LINE_VAL_NUMBER);


    parser->Parse();
    simple = parser->Found("s");
    logo = parser->Found("l");
    waitVsync = true;
    parser->Found("p", &program);

    long fsaa;
    if (parser->Found("f", &fsaa)) {
        switch (fsaa) {
            case 0: this->fsaa = Id::MultisampleAlwaysOff; break;
            case 1: this->fsaa = Id::MultisampleAlwaysOn; break;
            case 2: this->fsaa = Id::MultisampleOnWhenStopped; break;
        }
    } else {
        this->fsaa = Id::MultisampleOnWhenStopped;
    }

    if (single = parser->Found("b"))
        TCanvas::Attributes[TCanvas::DoubleBufferAttributeIndex] = 0;

    frame = 0;
    Renew();


    return true;
} 

void TApp::OnKeyUp(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
        case WXK_SPACE: frame->canvas->trackball.Stop(); break;
        case 'T': frame->NextModel(); break;
        case 'Z': frame->NextTexture(); break;
        case 'Q': frame->Close(); break;
        case 'X': frame->Toggle(Id::BackgroundScaledTexture); break;
        case 'P': frame->Toggle(Id::ViewPodium); break;
        case 'B': frame->canvas->NextBackgroundColor(); break;
        case 'A': frame->Toggle(Id::ViewAnimate); break;
        case 'W': frame->Toggle(Id::ViewWireframe); break;
        case 'G': frame->Toggle(Id::BackgroundColorGradient); break;
        case WXK_CONTROL: frame->canvas->SetCursor(wxCURSOR_ARROW); break;
    }
    frame->canvas->Update();
    event.Skip();
}

void TApp::OnKeyDown(wxKeyEvent& event)
{
    // Determine if the event originates from a dialog box.
    bool dialog = false;
    if (event.GetEventObject()->IsKindOf(CLASSINFO(wxWindow))) {
        wxWindow* window = (wxWindow*) event.GetEventObject();
        while (window->GetParent() && !window->IsKindOf(CLASSINFO(wxDialog)))
            window = window->GetParent();
        dialog = window->IsKindOf(CLASSINFO(wxDialog));
    }

    if (!dialog) {
        frame->canvas->trackball.OnKeyDown(event);
        switch (event.GetKeyCode()) {
            case WXK_DOWN: frame->programs.OnDown(); break;
            case WXK_UP: frame->programs.OnUp(); break;
        }
    }

    switch (event.GetKeyCode()) {
        case WXK_ESCAPE:
            frame->log.Close();
            frame->adjuster.Close();
            if (frame->IsChecked(Id::ViewFullscreen)) {
                frame->Uncheck(Id::ViewFullscreen);
                frame->ShowFullScreen(false);
            }
            break;

        // Preempt some keystrokes without calling Skip() to prevent them from being processed by controls.
        case WXK_HOME:
            Ready();
            frame->canvas->Reset();

        case WXK_END:
            return;
    }
    frame->canvas->Update();
    event.Skip();
}

void TApp::OnIdle(wxIdleEvent& event)
{
    frame->canvas->trackball.OnIdle(event);
}

void TApp::Errorf(const char* format, ...)
{
    wxString message;

    va_list marker;
    va_start(marker, format);
    message.PrintfV(format, marker);
    frame->log.text->SetDefaultStyle(wxTextAttr(*wxRED));
    frame->log.Append(message);
    if (frame->GetMenuBar() && !frame->IsChecked(Id::ViewInfoLog))
        message += " (info log might have more details)";
    frame->SetStatusText(message, 0);
}

void TApp::Printf(const char* format, ...)
{
    wxString message;

    va_list marker;
    va_start(marker, format);
    message.PrintfV(format, marker);
    frame->log.text->SetDefaultStyle(wxTextAttr(*wxWHITE));
    frame->log.Append(message);
}

void TApp::Statusf(const char* format, ...)
{
    wxString message;

    va_list marker;
    va_start(marker, format);
    message.PrintfV(format, marker);
    frame->SetStatusText(message, 0);
    frame->GetStatusBar()->Update();
}

void TApp::PushMessage(const char* format, ...)
{
    wxString message;

    va_list marker;
    va_start(marker, format);
    message.PrintfV(format, marker);
    messageStack = frame->GetStatusBar()->GetStatusText(0);
    frame->SetStatusText(message, 0);
    frame->GetStatusBar()->Update();
}

void TApp::PopMessage()
{
    frame->SetStatusText(messageStack, 0);
}

void TApp::Dump(const char* buffer)
{
    frame->log.text->SetDefaultStyle(wxTextAttr(*wxLIGHT_GREY));
    frame->log.Append(buffer);
    frame->log.Append("\n");
}

void TApp::Dump(GLuint handle)
{
    char* buffer = 0;
    int length = 0;

    // reset the error code
    glGetError();

    // this function accepts both shaders AND programs, so do a type check.
    if (glIsProgram(handle))
    {
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);
        if (glGetError() != GL_NO_ERROR)
            return;
        buffer = new char[length];
        glGetProgramInfoLog(handle, length, 0, buffer);
        if (glGetError() != GL_NO_ERROR)
            return;
    }
    else if (glIsShader(handle))
    {
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);
        if (glGetError() != GL_NO_ERROR)
            return;
        buffer = new char[length];
        glGetShaderInfoLog(handle, length, 0, buffer);
        if (glGetError() != GL_NO_ERROR)
            return;
    }

    if (buffer)
    {
        Dump(buffer);
        delete buffer;
    }
}

void TApp::Ready()
{
    frame->SetStatusText("Ready.", 0);
}

void TApp::UpdateVerts(int count)
{
    char buffer[32];
    sprintf(buffer, "%d vertices", count);
    frame->SetStatusText(buffer, 1);
}

void TApp::UpdateFps(float fps)
{
    if (waitVsync)
        return;

    this->fps = fps;
    char buffer[16];
    sprintf(buffer, "%3.1f fps", fps);
    if (!IsBenching())
        frame->SetStatusText(buffer, 2);
}

void TApp::ToggleWaitVsync()
{
    wxStatusBar* status = frame->GetStatusBar();
    waitVsync = !waitVsync;
    if (!waitVsync)
        status->SetFieldsCount(3, TFrame::StatusBarWidths);
    else
        status->SetFieldsCount(2, TFrame::StatusBarWidths);

    if (wglSwapIntervalEXT) {
        if (CapFps())
            wglSwapIntervalEXT(1);
        else
            wglSwapIntervalEXT(0);
    }
}

const TProgram& TApp::GetCurrentProgram() const
{
    return frame->programs.Current();
}

TAdjuster& TApp::Adjuster() const
{
    return frame->adjuster;
}

bool TApp::IsBenching() const
{
    bool retval = false;
    return retval;
}

wxSize TApp::GetCanvasSize() const
{
    return wxSize(frame->canvas->width, frame->canvas->height);
}

TCanvas* TApp::Canvas() const
{
    return frame->canvas;
}

void TApp::Renew()
{
    TFrame* prev = frame;
    new TFrame("OpenGL Shading Language Demo", wxDefaultPosition, wxSize(800,650));
    frame->Show(TRUE);
    SetTopWindow(frame);
    frame->canvas->glSetup();
    delete prev;
}


IMPLEMENT_APP(TApp)
