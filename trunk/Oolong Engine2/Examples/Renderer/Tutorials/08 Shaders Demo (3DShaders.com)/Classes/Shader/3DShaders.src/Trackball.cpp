//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Canvas.h"
#include "Frame.h"

const float TTrackball::StartZoom = 0.8f;
const float TTrackball::InertiaThreshold = 1;
const int TTrackball::Delay = 10;

TTrackball::TTrackball() : canvas(0)
{
    Reset();
    inertiaTheta = wxGetApp().Simple() ? 0 : -1;
}

void TTrackball::Reset(bool all)
{
    previous = wxGetElapsedTime(true);
    frames = 0;

    startZoom = StartZoom;
    if (canvas)
        canvas->SetZoom(startZoom);

    Stop();
    vPrev = vec3(0, 0, 0);
    vInc = vec3(0, 0, 0);
    inertiaTheta = all ? (wxGetApp().Simple() ? 0 : -1) : 0;
    inertiaAxis = vec3(0, 1, 0);
    validStart = false;
    xform.identity();
    wxStartTimer();
}

void TTrackball::Stop()
{
    inertiaTheta = 0;
    vInc = vec3(0, 0, 0);
}

void TTrackball::OnMouse(wxMouseEvent& event)
{
    vec3 cursor = canvas->GetWorldSpace(event.GetX(), event.GetY());

    if (event.LeftDown()) {
        if (!event.ControlDown())
            Stop();
        vStart = cursor;
        memcpy((float*) &mStart, (float*) &xform, sizeof(xform));
        startZoom = canvas->GetZoom();
        vPrev = cursor;
        validStart = true;
    } else if (event.LeftUp() && validStart) {
        float theta = 180 * vInc.magnitude();
        if (theta < -InertiaThreshold || theta > InertiaThreshold) {
            inertiaAxis = cross(vStart, cursor);
            if (inertiaAxis.magnitude()) {
                inertiaAxis.unitize();
                inertiaTheta = theta;
            }
        } else if (!event.ControlDown()) {
            Stop();
        }
        validStart = false;
    }
    else if (event.Moving() && event.LeftIsDown()) {
        if (!validStart) {
            vInc = vec3(0, 0, 0);
        } else {
            if (event.ControlDown()) {
                float delta = cursor.y - vStart.y;
                if (delta) {
                    canvas->SetZoom(startZoom + delta);
                    canvas->Update();
                }
            } else {
                float theta = 180 * (cursor - vStart).magnitude();
                if (theta) {
                    vec3 axis = cross(vStart, cursor);
                    axis.unitize();

                    glLoadIdentity();
                    vec3 offset = wxGetApp().Frame()->GetCenter();
                    glTranslate(offset);
                    glRotatef(-theta, axis.x, axis.y, axis.z);
                    glTranslate(-offset);
                    glMultMatrixf((float*) &mStart);
                    glGetFloatv(GL_MODELVIEW_MATRIX, (float*) &xform);
                    canvas->Update();
                }
            }
            vInc = cursor - vPrev;
        }
        vPrev = cursor;
    }

    // Right mouse button zooms.
    else if (event.RightDown()) {
        vStart = cursor;
        startZoom = canvas->GetZoom();
        validStart = true;
    } else if (event.RightUp() && validStart) {
        validStart = false;
    } else if (event.Moving() && event.RightIsDown()) {
        if (validStart) {
            float delta = cursor.y - vStart.y;
            if (delta) {
                canvas->SetZoom(startZoom + delta);
                canvas->Update();
            }
        }
    }

    event.Skip();
}

void TTrackball::OnIdle(wxIdleEvent& event)
{
    long elapsed = wxGetElapsedTime(false);

    // 'elapsed' can be less than 'previous' if the timer gets reset somewhere else.
    if (elapsed < previous) {
        previous = elapsed;
    }

    // Animate the shader if at least one millisecond has elapsed.
    else if (elapsed - previous > 0) {
        canvas->Animate(elapsed - previous);
        previous = elapsed;
    }

    //
    // When not showing frames per second, wait enough between frames to not exceed a maximum frame rate.
    // The proper way of doing this is wait-for-vertical-sync, accessible via the display applet.
    // (Some older 3Dlabs products do not support the wglSwapControl extension.)
    //

    // If wglSwapInterval isn't available, wait for some time to elapse.
    if (wxGetApp().CapFps() && !wglSwapIntervalEXT) {
        if (!inertiaTheta || elapsed < Delay) {
            event.RequestMore();
            event.Skip();
            return;
        }

        // Reset timer.
        wxGetElapsedTime(true);
        previous = 0;
    } else {
        // Recalculate the fps every second.
        if (elapsed > 1000) {
            // Reset timer.
            wxGetElapsedTime(true);
            previous = 0;

            // Update the fps display counter on the status bar.
            wxGetApp().UpdateFps((float) frames * 1000.0f / (float) elapsed);
            frames = 0;
        }
        ++frames;
    }

    // Continue spinning the model.
    glLoadIdentity();
    vec3 offset = wxGetApp().Frame()->GetCenter();
    glTranslate(offset);
    glRotatef(-inertiaTheta, inertiaAxis.x, inertiaAxis.y, inertiaAxis.z);
    glTranslate(-offset);
    glMultMatrixf((float*) &xform);
    glGetFloatv(GL_MODELVIEW_MATRIX, (float*) &xform);
    canvas->Update();
    event.Skip();
}

void TTrackball::LoadMatrix() const
{
    SetMatrix();
    MultMatrix();
}

void TTrackball::SetMatrix() const
{
    glLoadIdentity();

    if (wxGetApp().Frame()->IsChecked(Id::ViewPerspective))
        glTranslatef(0, 0, TCanvas::CameraZ - 1);
    else
        glTranslatef(0, 0, TCanvas::CameraZ);

    glRotatef(20, 1, 0, 0);
}

void TTrackball::MultMatrix() const
{
    glMultMatrixf((float*) &xform);
}

void TTrackball::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
        case WXK_RIGHT: 
        case WXK_NUMPAD_RIGHT: 
            inertiaTheta -= 0.5f;
            inertiaAxis = vec3(0, 1, 0);
            break;

        case WXK_LEFT: 
        case WXK_NUMPAD_LEFT: 
            inertiaTheta += 0.5f;
            inertiaAxis = vec3(0, 1, 0);
            break;

        case WXK_ADD: 
        case WXK_NUMPAD_ADD:
            canvas->SetZoom(canvas->GetZoom() - 0.25F);
            break;

        case WXK_SUBTRACT:
        case WXK_NUMPAD_SUBTRACT:
            canvas->SetZoom(canvas->GetZoom() + 0.25F);
            break;
    }
    canvas->Update();
}

int TTrackball::GetFrameCount()
{
    int f = frames;
    frames = 0;
    return f;
}
