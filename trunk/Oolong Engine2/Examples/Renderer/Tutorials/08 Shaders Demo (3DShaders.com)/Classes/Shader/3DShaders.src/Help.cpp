//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/wxhtml.h>
#include "App.h"
#include "Frame.h"
#include "Help.h"

wxString aboutText =
        "<html><body>"
        "OpenGL Shading Language Demo " + TApp::Version() + "<br>"
        "Copyright © 2006 3Dlabs.  All rights reserved.<br><br>"
        "For questions and feedback, go to <a href='http://www.3dlabs.com/contact'>http://www.3dlabs.com/contact</a>.<p>"
        "Lead developer: Dave Houlton<br>"
        "Original glsldemo design and development: Philip Rideout<br>"
        "Framebuffer effects: Jon Kennedy<br>"
        "Glew interface: Inderaj Bains<p>"
        "<u>Shader Authors</u><br>"
        "Dave Baldwin<br>"
        "Joshua Doss<br>"
        "Jon Kennedy<br>"
        "John Kessenich<br>"
        "Steve Koren<br>"
        "Ian Nurse<br>"
        "Philip Rideout<br>"
        "Randi Rost<br>"
        "Antonio Tejada<p>"
        "Thanks to the developers of wxWidgets, Glew, and Expat.<br>"
        "Thanks to Jordan Russell for InnoSetup."
        "</body></html>";

wxString keyboardText =
        "a - Animation\n"
        "b - Change background color (textured background must be off)\n"
        "g - Gradient background (textured background must be off)\n"
        "t - Cycle among models\n"
        "z - Cycles among textures\n"
        "p - Toggle podium display\n"
        "w - Wireframe\n"
        "x - Toggle textured background\n"
        "q - Quit\n"
        "<F5> - Refresh\n"
        "<left>, <right> - Rotation about y-axis\n"
        "<up>, <down> - Change program\n"
        "<page down>, <page up> - change tesselation coarseness\n"
        "<+>, <->  - zoom model\n\n";

BEGIN_EVENT_TABLE(THelpAbout, wxDialog)
    EVT_BUTTON(Id::HelpAboutClose, THelpAbout::OnClickClose) 
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(THelpKeyboard, wxDialog)
    EVT_BUTTON(Id::HelpKeyboardClose, THelpKeyboard::OnClickClose) 
END_EVENT_TABLE()

void THelpAbout::Create(wxWindow* parent)
{
    wxDialog::Create(parent, -1, "About", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCAPTION | wxCLIP_CHILDREN);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticBoxSizer* sideSizer = new wxStaticBoxSizer(new wxStaticBox(this, -1, ""), wxHORIZONTAL);
    wxBitmap splash("../textures/splash.png", wxBITMAP_TYPE_PNG);
    wxStaticBitmap* bmp = new wxStaticBitmap(this, -1, splash);
    sideSizer->Add(bmp);
    sideSizer->Add(new wxStaticLine(this, -1, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    wxSize size((int) ((float) splash.GetWidth() * 1.5f), splash.GetHeight());
    html = new TAboutHtml(this, -1, wxDefaultPosition, size);
    html->SetPage(aboutText);
    sideSizer->Add(html);
    topSizer->Add(sideSizer);
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* close = new wxButton(this, Id::HelpAboutClose, "Close"); 
    buttonSizer->Add(close, 0, wxALL, 15);  
    topSizer->Add(buttonSizer, 0, wxALIGN_RIGHT);
    SetSizer(topSizer);
    topSizer->SetSizeHints(this);
    CenterOnScreen();
}

void THelpAbout::OnClickClose(wxCommandEvent& event)
{
    Close();
}

void TAboutHtml::OnLinkClicked(const wxHtmlLinkInfo& link)
{
    SpawnBrowser(link.GetHref());
}

void THelpKeyboard::Create(wxWindow* parent)
{
    wxDialog::Create(parent, -1, "Keyboard Help", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCAPTION | wxCLIP_CHILDREN);
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* text = new wxStaticText(this, -1, keyboardText);
    topSizer->Add(text, 0, wxLEFT | wxTOP | wxRIGHT, 10);
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* close = new wxButton(this, Id::HelpKeyboardClose, "Close"); 
    buttonSizer->Add(close, 0, wxALL, 10);  
    topSizer->Add(buttonSizer, 0, wxALIGN_CENTER);
    SetSizer(topSizer);
    topSizer->SetSizeHints(this);
}

void THelpKeyboard::OnClickClose(wxCommandEvent& event)
{
    Close();
}
