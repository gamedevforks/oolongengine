//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include <wx/wx.h>
#include <wx/statline.h>
#include "Frame.h"
#include "Log.h"
#include "Tables.h"

BEGIN_EVENT_TABLE(TLog, wxDialog)
    EVT_CLOSE(TLog::OnClose)
    EVT_ACTIVATE(TLog::OnActivate)
    EVT_BUTTON(Id::LogClose, TLog::OnClickClose) 
    EVT_BUTTON(Id::LogSave, TLog::OnClickSave) 
    EVT_BUTTON(Id::LogClear, TLog::OnClickClear) 
END_EVENT_TABLE()

void TLog::Create(wxWindow* parent)
{
    wxDialog::Create(parent, -1, "Information Log", wxDefaultPosition, wxSize(425, 600), wxDEFAULT_DIALOG_STYLE | wxCAPTION | wxRESIZE_BORDER | wxCLIP_CHILDREN);

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    text = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_RICH | wxTE_RICH2 | wxTE_READONLY | wxTE_MULTILINE);
    text->SetBackgroundColour(*wxBLACK);
    
    text->SetDefaultStyle(wxTextAttr(*wxGREEN, *wxBLACK));
    text->AppendText("Ready.\n");
    topSizer->Add(text, 1, wxEXPAND | wxALL, 10);

    topSizer->Add(new wxStaticLine(this, -1), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, Id::LogSave, "Save"), 0, wxALL, 15);
    buttonSizer->Add(new wxButton(this, Id::LogClear, "Clear"), 0, wxALL, 15);
    wxButton* close = new wxButton(this, Id::LogClose, "Close"); 
    buttonSizer->Add(close, 0, wxALL, 15);  
    close->SetDefault();
    close->SetFocus();

    topSizer->Add(buttonSizer, 0, wxALIGN_RIGHT);
    SetAutoLayout(true);
    SetSizer(topSizer);
}

void TLog::OnClose(wxCloseEvent& event)
{
    wxGetApp().Frame()->Uncheck(Id::ViewInfoLog);
    event.Skip();
}

void TLog::OnClickClose(wxCommandEvent& event)
{
    Close();
}

void TLog::OnClickSave(wxCommandEvent& event)
{
    wxString filename = wxFileSelector("Save File As", "..", "info.log", "", "Log file (*.log)|*.log", wxSAVE);
    if (!filename.empty())
        text->SaveFile(filename);
}

void TLog::OnClickClear(wxCommandEvent& event)
{
    text->Clear();
}

void TLog::OnActivate(wxActivateEvent& event)
{
    // Work around a wx bug by scrolling to invalidate the text box.
    text->ScrollLines(-1);

    event.Skip();
}

void TLog::Append(const char* string)
{
    // Work around a wx bug by appending in chunks of 128 characters.
    while (strlen(string) > 128) {
        wxString segment(string, 128);
        string += 128;
        text->AppendText(segment);
    }
    
    if (string && *string)
        text->AppendText(string);

    // Work around a wx bug by scrolling to invalidate the text box.
    text->ScrollLines(-1);
}
