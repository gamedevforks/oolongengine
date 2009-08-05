//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef LOG_H
#define LOG_H
#include <wx/wx.h>
class TFrame;

// Dialog box to display the information logs produced by shader compilation and linkage.
class TLog : public wxDialog
{
  public:
    void Create(wxWindow* parent);
    void Append(const char* string);
    void OnClose(wxCloseEvent& event);
    void OnClickClose(wxCommandEvent& event);
    void OnClickSave(wxCommandEvent& event);
    void OnClickClear(wxCommandEvent& event);
    void OnActivate(wxActivateEvent& event);
  private:
    wxTextCtrl* text;
    DECLARE_EVENT_TABLE()
    friend class TApp;
};

#endif
