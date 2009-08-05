//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef HELP_H
#define HELP_H
#include <wx/wx.h>
#include <wx/wxhtml.h>

class TAboutHtml : public wxHtmlWindow
{
  public:
      TAboutHtml(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
        wxHtmlWindow(parent, id, pos, size) {}
      void OnLinkClicked(const wxHtmlLinkInfo& link);
};

class THelpAbout : public wxDialog
{
  public:
    void Create(wxWindow* parent);
    void OnClickClose(wxCommandEvent& event);
  private:
    TAboutHtml* html;
    DECLARE_EVENT_TABLE()
    friend class TApp;
};

class THelpKeyboard : public wxDialog
{
  public:
    void Create(wxWindow* parent);
    void OnClickClose(wxCommandEvent& event);
  private:
    DECLARE_EVENT_TABLE()
};

#endif
