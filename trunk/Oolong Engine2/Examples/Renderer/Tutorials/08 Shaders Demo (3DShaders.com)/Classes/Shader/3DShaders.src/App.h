//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef APP_H
#define APP_H
#include <GL/glew.h>
#include <wx/wx.h>
#include <cstdarg>
#include <wx/cmdline.h>
#include "os.h"
#include "Cycle.h"
#include "Tables.h"

class TFrame;
class TCanvas;
class TAdjuster;
class TProgram;

// Application class.  Globally accessible via wxGetApp().
class TApp : public wxApp {
  public:

    static wxString Version() { return "v3.8"; }
    int MainLoop();

    // Initialization stuff.
    bool OnInit();
    int OnExit();
    void SetFrame(TFrame* frame) { this->frame = frame; }

    // Event Handlers.
    void OnKeyUp(wxKeyEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnIdle(wxIdleEvent& event);

    // C-style output methods.  Callers of Errorf should include a period and a linefeed.
    void Errorf(const char* format, ...);
    void Printf(const char* format, ...);
    void Statusf(const char* format, ...);

    // Methods for dumping GL infologs.  They automatically append a linefeed.
    void Dump(const char* buffer);
    void Dump(GLuint handle);

    // Methods for updating the status bar.
    void Ready();
    void UpdateVerts(int count);
    void UpdateFps(float fps);
    void ToggleWaitVsync();
    void PushMessage(const char* format, ...);
    void PopMessage();

    // Various query methods.
    Id Fsaa() const { return fsaa; }
    bool Simple() const { return simple; }
    bool Single() const { return single; }
    bool Logo() const { return logo; }
    bool CapFps() const { return !IsBenching() && waitVsync; }
    wxString StartupProgram() const { return program; }
    TFrame* Frame() const { return frame; }
    TCanvas* Canvas() const;
    TAdjuster& Adjuster() const;
    const TProgram& GetCurrentProgram() const;

    // Miscellaneous.
    wxSize GetCanvasSize() const;
    void Renew();
    void StartCycle() { cycle.Start(); }
    void StopCycle() { cycle.Stop(); }
    bool IsCycling() const { return cycle.IsRunning(); }
    bool IsBenching() const;
    wxCmdLineParser* GetCommandLineParser() { return parser; }

    enum TError {Success, NonGL2};
    void SetErrorCode(TError e) { errorCode = e; }

  private:

    wxCmdLineParser* parser;
    TCycle cycle;
    TFrame* frame;
    bool waitVsync;
    float fps;
    TError errorCode;
    wxString messageStack;

    // Switches
    bool simple;
    bool single;
    bool logo;

    // Options
    wxString program;
    int fsaa;

    static const wxCmdLineEntryDesc commandLineDescription[];

    friend class TCycle;
    DECLARE_EVENT_TABLE()
};

DECLARE_APP(TApp)

#endif
