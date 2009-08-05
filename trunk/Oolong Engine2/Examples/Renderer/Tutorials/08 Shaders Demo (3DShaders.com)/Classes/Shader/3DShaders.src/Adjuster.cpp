//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include <wx/wx.h>
#include <wx/colordlg.h>
#include "Frame.h"
#include "Adjuster.h"
#include "Tables.h"


const int MaxSliderValue = 1000;

IMPLEMENT_DYNAMIC_CLASS(TAdjuster, wxDialog)

BEGIN_EVENT_TABLE(TAdjuster, wxDialog)
    EVT_CLOSE(TAdjuster::OnClose)
    EVT_SCROLL(TAdjuster::OnSlide)
    EVT_COMMAND_RANGE(-65535,  65535, wxEVT_COMMAND_BUTTON_CLICKED, TAdjuster::OnClick) 
END_EVENT_TABLE()

void TAdjuster::Create(wxWindow* parent)
{
    wxDialog::Create(parent, -1, "Adjuster", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN);
}

void TAdjuster::OnClose(wxCloseEvent& event)
{
    wxGetApp().Frame()->Uncheck(Id::ViewAdjuster);
    wxGetApp().Frame()->UpdateViewAnimate();
    event.Skip();
}

void TAdjuster::Set(const TUniformList& uniforms)
{
    bool shown = IsShown();
    if (shown)
        Hide();

    // Remove the old controls.
    for (wxWindowListNode* node = GetChildren().GetFirst(); node;) {
        wxWindow* current = node->GetData();
        node = node->GetNext();
        delete current;
    }
    GetChildren().Clear();
    sliders.clear();

    // Work around a problem with Fit() by resizing to bigger than need be.
    SetSize(500, 500);

    // Create a new vertical layout and iterate through each adjustustable uniform.
    wxBoxSizer* vertical = new wxBoxSizer(wxVERTICAL);
    bool empty = true;
    for (TUniformList::const_iterator u = uniforms.begin(); u != uniforms.end(); ++u) {
        if (u->IsColor()) {
            empty = false;
            wxBoxSizer* horizontal = new wxBoxSizer(wxHORIZONTAL);
            wxButton* button = new wxButton(this, -1, "", wxDefaultPosition);
            button->SetBackgroundColour(u->GetColor());
            wxStaticText* label = new wxStaticText(this, -1, u->GetName(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
            horizontal->Add(label, 0, wxALIGN_CENTRE_VERTICAL, 5);
            horizontal->Add(button, 1, wxALIGN_CENTRE_VERTICAL | wxEXPAND | wxALL, 5);
            vertical->Add(horizontal, 1, wxEXPAND | wxALL, 10);

            // Add this to the id-uniform map.
            pickers[button->GetId()] = const_cast<TUniform*>(&*u);
            continue;
        }

        if (!u->HasSlider())
            continue;

        // Insert a label-slider pair using a horizontal layout.
        empty = false;
        wxBoxSizer* horizontal = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* label = new wxStaticText(this, -1, u->GetName(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
        horizontal->Add(label, 0, wxALIGN_CENTRE_VERTICAL, 5);

        for (int i = 0; i < u->NumComponents(); ++i) {
            float value = u->GetSlider(i) * MaxSliderValue;
            wxSlider* slider = new wxSlider(this, -1, (int) value, 1, MaxSliderValue);
            horizontal->Add(slider, 1, wxALIGN_CENTRE_VERTICAL | wxEXPAND | wxALL, 5);
            sliders[slider->GetId()] = TUniformComponent(const_cast<TUniform*>(&*u), i);
        }

        vertical->Add(horizontal, 1, wxEXPAND | wxALL, 10);
    }

    if (empty) {
        wxStaticText* label = new wxStaticText(this, -1, "The current shader does not have any adjustable uniforms.", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
        vertical->Add(label, 1, wxEXPAND | wxALL, 10);
    }

    SetSizerAndFit(vertical, true);

    if (shown)
        Show();
}

void TAdjuster::OnSlide(wxScrollEvent& event)
{
    int value = event.GetInt();
    TUniform* uniform = sliders[event.GetId()].first;
    int component = sliders[event.GetId()].second;
    uniform->SetSlider((float) value / MaxSliderValue, component);
    uniform->UpdateSlider();
    event.Skip();
}

void TAdjuster::OnClick(wxCommandEvent& event)
{
    if (pickers.find(event.GetId()) != pickers.end()) {
        TUniform* uniform = pickers[event.GetId()];
        wxColour color = uniform->GetColor();
        color = wxGetColourFromUser(this, color);
        uniform->SetColor(color);
        FindWindowById(event.GetId())->SetBackgroundColour(color);
    }

    event.Skip();
}
