//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Cycle.h"
#include "Frame.h"
#include "Canvas.h"

void TCycle::Notify()
{
    TFrame* frame = wxGetApp().frame;
    TProgramList& programs = frame->programs;
    int program = rand() % programs.GetItemCount();
    Id model = Id::FirstModel + rand() % (Id::NumModels - 1);
    programs.Select(program);
    frame->LoadModel(model);
}

void TCycle::Start()
{
    wxTextEntryDialog dialog(0, "Delay in seconds?", "Please enter an integer.", "3", wxOK | wxCENTRE);
    dialog.ShowModal();

    long seconds;
    if (dialog.GetValue().ToLong(&seconds))
        wxTimer::Start(seconds * 1000);
    else
        wxTimer::Start(3000);

    Notify();
}

void TCycle::Stop()
{
    TFrame* frame = wxGetApp().frame;
    wxTimer::Stop();
    frame->Uncheck(Id::FileCycle);
}
