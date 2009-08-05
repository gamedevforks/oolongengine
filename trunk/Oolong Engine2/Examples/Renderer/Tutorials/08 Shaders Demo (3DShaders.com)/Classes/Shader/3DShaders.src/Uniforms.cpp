//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include <wx/wx.h>
#include "Shaders.h"

TMotion::TMotion(const wxString& type)
{
    // In case of an error, default to linear growth across one second.
    duration = 0;
    this->type = Linear;

    if (type == "" || type == "linear")
        this->type = Linear;
    else if (type == "exponential")
        this->type = Exponential;
    else if (type == "logarthmic")
        this->type = Logarithmic;
    else
        wxGetApp().Errorf("Unrecognized motion type '%s'.\n", type.c_str());
}

TMotion::TMotion(const wxString& type, const wxString& length)
{
    // In case of an error, default to linear growth across one second.
    duration = 1000;
    this->type = Linear;

    if (type == "" || type == "linear")
        this->type = Linear;
    else if (type == "exponential")
        this->type = Exponential;
    else if (type == "logarthmic")
        this->type = Logarithmic;
    else
        wxGetApp().Errorf("Unrecognized motion type '%s'.\n", type.c_str());

    if (length == "") {
        wxGetApp().Errorf("Motion length not specified.\n");
    } else if (length.Contains("ms")) { // milliseconds
        if (!length.SubString(0, length.Length() - 3).ToULong((unsigned long*) &duration))
            wxGetApp().Errorf("Invalid number: '%s'.\n", length.c_str());
    } else if (length.Contains("s")) { // seconds
        if (!length.SubString(0, length.Length() - 2).ToULong((unsigned long*) &duration))
            wxGetApp().Errorf("Invalid number: '%s'.\n", length.c_str());
        else
            duration *= 1000;
    } else if (length.Contains("min")) { // minutes
        if (!length.SubString(0, length.Length() - 4).ToULong((unsigned long*) &duration))
            wxGetApp().Errorf("Invalid number: '%s'.\n", length.c_str());
        else
            duration *= 1000 * 60;
    } else {
        wxGetApp().Errorf("Motion length must be end in one of : {s, ms, min}.\n");
    }
}

TSlider::TSlider(const wxString& type)
{
    memset(&current, 0, sizeof(TUniformValue));
    this->type = Linear;

    if (type == "" || type == "linear")
        this->type = Linear;
    else if (type == "exponential")
        this->type = Exponential;
    else if (type == "logarthmic")
        this->type = Logarithmic;
    else
        wxGetApp().Errorf("Unrecognized slider type '%s'.\n", type.c_str());
}

// Update the values of special-purpose uniforms.
void TUniform::Update()
{
    if (name == "FrameWidth") {
        int res[4];
        glGetIntegerv(GL_VIEWPORT, res); 
        if(type.value == Id::UniformTypeFloat)
            value.f[0] = (float) res[2];
    } else if (name == "FrameHeight") {
        int res[4];
        glGetIntegerv(GL_VIEWPORT, res); 
        if(type.value == Id::UniformTypeFloat)
            value.f[0] = res[3];
    }
}

void TUniform::Animate(int period)
{
    if (!duration)
        return;

    elapsed += period;
    while (elapsed > duration)
        elapsed -= duration;
    
    int total = 0;
    for (TMotionList::iterator i = motions.begin(); i != motions.end(); ++i) {
        if (elapsed < total + i->duration) {
            int numFloatComponents = 0;
            int numIntComponents = 0;

            if (IsInteger())
                numIntComponents = NumComponents();
            else
                numFloatComponents = NumComponents();

            //
            // All three progressions satisfy these conditions:
            //
            //   f(0) = start
            //   f(length) = stop
            //
            //      Linear: f(t) = start + t * (stop - start) / length;
            // Exponential: f(t) = TBD
            // Logarithmic: f(t) = TBD
            //

            if (i->type == Linear) {
                for (int c = 0; c < numFloatComponents; ++c) {
                    float start = i->start.f[c];
                    float stop = i->stop.f[c];
                    value.f[c] = start + (float) (elapsed - total) * (stop - start) / i->duration;
                }

                for (int c = 0; c < numIntComponents; ++c) {
                    int start = i->start.i[c];
                    int stop = i->stop.i[c];
                    float f = (float) (elapsed - total) * (float) (stop - start) / i->duration;
                    value.i[c] = start + (int) f;
                }
            } else if (i->type == Exponential) {
                wxGetApp().Errorf("Exponential motion is not yet implemented.");
            } else if (i->type == Logarithmic) {
                wxGetApp().Errorf("Logarthmic motion is not yet implemented.");
            } else {
                wxGetApp().Errorf("Unknown motion type.");
            }

            break;
        }
        total += i->duration;
    }
}

void TUniform::Load() const
{
    switch (type.value) {
        case Id::UniformTypeBool:
        case Id::UniformTypeInt: glUniform1i(location, value.i[0]); break;
        case Id::UniformTypeIvec2: glUniform2iv(location, 1, (int*) value.i); break;
        case Id::UniformTypeIvec3: glUniform3iv(location, 1, (int*) value.i); break;
        case Id::UniformTypeIvec4: glUniform4iv(location, 1, (int*) value.i); break;
        case Id::UniformTypeFloat: glUniform1f(location, value.f[0]); break;
        case Id::UniformTypeVec2: glUniform2fv(location, 1, (float*) value.f); break;
        case Id::UniformTypeVec3: glUniform3fv(location, 1, (float*) value.f); break;
        case Id::UniformTypeVec4: glUniform4fv(location, 1, (float*) value.f); break;
        case Id::UniformTypeMat2: glUniformMatrix2fv(location, 1, GL_FALSE, (float*) value.f); break;
        case Id::UniformTypeMat3: glUniformMatrix3fv(location, 1, GL_FALSE, (float*) value.f); break;
        case Id::UniformTypeMat4: glUniformMatrix4fv(location, 1, GL_FALSE, (float*) value.f); break;
    }
}

int TUniform::NumComponents() const
{
    switch (type.value) {
        case Id::UniformTypeFloat: return 1;
        case Id::UniformTypeVec2: return 2;
        case Id::UniformTypeVec3: return 3;
        case Id::UniformTypeVec4: return 4;
        case Id::UniformTypeInt: return 1;
        case Id::UniformTypeBool: return 1;
        case Id::UniformTypeIvec2: return 2;
        case Id::UniformTypeIvec3: return 3;
        case Id::UniformTypeIvec4: return 4;
        case Id::UniformTypeMat2: return 4;
        case Id::UniformTypeMat3: return 9;
        case Id::UniformTypeMat4: return 16;
    }
    return 0;
}

bool TUniform::IsInteger() const
{
    switch (type.value) {
        case Id::UniformTypeBool:
        case Id::UniformTypeInt:
        case Id::UniformTypeIvec2:
        case Id::UniformTypeIvec3:
        case Id::UniformTypeIvec4:
            return true;
    }
    return false;
}

bool TUniform::SetLocation(GLuint program)
{
    location = glGetUniformLocation(program, name);
    return location != -1;
}

TMotion* TUniform::NewMotion(const wxString& type, const wxString& length)
{
    motions.push_back(TMotion(type, length));
    return &motions.back();
}

TSlider* TUniform::NewSlider(const wxString& type)
{
    slider = TSlider(type);
    return &slider;
}

void TUniform::UpdateDuration()
{
    duration = 0;
    for (TMotionList::iterator i = motions.begin(); i != motions.end(); ++i)
        duration += i->duration;
}

void TUniform::UpdateSlider()
{
    if (!slider.IsValid())
        return;

    if (IsInteger()) {
        for (int c = 0; c < NumComponents(); ++c)
            value.i[c] = slider.current.i[c];
    } else {
        for (int c = 0; c < NumComponents(); ++c)
            value.f[c] = slider.current.f[c];
    }
}


void TUniform::SetSlider(float fraction, int component)
{
    if (slider.type == Linear) {
        if (IsInteger()) {
            int start = slider.start.i[component];
            int stop = slider.stop.i[component];
            slider.current.i[component] = (int) (start + fraction * (stop - start));
        } else {
            float start = slider.start.f[component];
            float stop = slider.stop.f[component];
            slider.current.f[component] = start + fraction * (stop - start);
        }
    } else if (slider.type == Exponential) {
        wxGetApp().Errorf("Exponential sliders are not yet implemented.");
    } else if (slider.type == Logarithmic) {
        wxGetApp().Errorf("Logarthmic sliders are not yet implemented.");
    } else {
        wxGetApp().Errorf("Unknown slider type.");
    }
}

float TUniform::GetSlider(int component) const
{
    if (slider.type == Linear) {
        if (IsInteger()) {
            int start = slider.start.i[component];
            int stop = slider.stop.i[component];
            return (value.i[component] - start) / (float) (stop - start);
        } else {
            float start = slider.start.f[component];
            float stop = slider.stop.f[component];
            return (value.f[component] - start) / (stop - start);
        }
    } else if (slider.type == Exponential) {
        wxGetApp().Errorf("Exponential sliders are not yet implemented.");
    } else if (slider.type == Logarithmic) {
        wxGetApp().Errorf("Logarthmic sliders are not yet implemented.");
    } else {
        wxGetApp().Errorf("Unknown slider type.");
    }
    return 0;
}

wxColour TUniform::GetColor() const
{
    float r = value.f[0] * 255;
    float g = value.f[1] * 255;
    float b = value.f[2] * 255;
    return wxColour((unsigned char) r, (unsigned char) g, (unsigned char) b);
}

void TUniform::SetColor(wxColour color)
{
    value.f[0] = (float) color.Red() / 255;
    value.f[1] = (float) color.Green() / 255;
    value.f[2] = (float) color.Blue() / 255;
}

void TUniform::Set(float x)
{
    value.f[0] = x;
}

void TUniform::Set(float x, float y)
{
    value.f[0] = x;
    value.f[1] = y;
}

void TUniform::Set(float x, float y, float z)
{
    value.f[0] = x;
    value.f[1] = y;
    value.f[2] = z;
}

void TUniform::Set(float x, float y, float z, float w)
{
    value.f[0] = x;
    value.f[1] = y;
    value.f[2] = z;
    value.f[3] = w;
}

void TUniform::Set(const mat2& m)
{
    memcpy(value.f, m.data, 4 * sizeof(float));
}

void TUniform::Set(const mat3& m)
{
    memcpy(value.f, m.data, 9 * sizeof(float));
}

void TUniform::Set(const mat4& m)
{
    memcpy(value.f, m.data, 16 * sizeof(float));
}

void TUniform::Set(int i)
{
    value.i[0] = i;
}

void TUniform::Set(int i, int j)
{
    value.i[0] = i;
    value.i[1] = j;
}

void TUniform::Set(int i, int j, int k)
{
    value.i[0] = i;
    value.i[1] = j;
    value.i[2] = k;
}

void TUniform::Set(int i, int j, int k, int h)
{
    value.i[0] = i;
    value.i[1] = j;
    value.i[2] = k;
    value.i[3] = h;
}

bool TUniformList::Load(GLuint program)
{
    if (!ready) {
        for (iterator i = begin(); i != end(); ++i) {
            if (!i->SetLocation(program))
                wxGetApp().Errorf("Unable to load uniform '%s'.\n", i->GetName().c_str());
        }
        ready = true;
    }

    for (iterator i = begin(); i != end(); ++i) {
        i->Update();
        i->Load();
    }

    return true;
}

void TUniformList::Animate(int period)
{
    for (iterator i = begin(); i != end(); ++i)
        i->Animate(period);
}
