//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef UNIFORMS_H
#define UNIFORMS_H

enum TProgression {
    Linear,
    Exponential,
    Logarithmic
};

union TUniformValue {
    float f[16];
    int i[16];
};

struct TMotion {
    TMotion() : duration(0) {}
    TMotion(const wxString& type);
    TMotion(const wxString& type, const wxString& length);
    TUniformValue start;
    TUniformValue stop;
    int duration;
    TProgression type;
};

typedef std::list<TMotion> TMotionList;

struct TSlider {
    TSlider() { current.i[0] = -1; }
    TSlider(const wxString& type);
    bool IsValid() const { return current.i[0] != -1; }
    TUniformValue start;
    TUniformValue stop;
    TUniformValue current;
    TProgression type;
};

class TUniform {
  public:
    TUniform() : elapsed(0), duration(0) {}
    TUniform(const wxString& name, const wxString& type) : name(name), type(findUniformType(type)), elapsed(0), duration(0) {}
    TUniform(const wxString& name, const wxString& type, GLuint program) : name(name), type(findUniformType(type)), elapsed(0), duration(0) { SetLocation(program); }
    void Update();
    void Reset() { elapsed = 0; }
    void Animate(int period);
    void Load() const;
    bool SetLocation(GLuint program);
    Id GetType() const { return type; }
    int NumComponents() const;
    bool IsInteger() const;
    void SetType(Id type) { this->type = type; }
    wxString GetName() const { return name; }
    TMotion* NewMotion(const wxString& type, const wxString& duration);
    TSlider* NewSlider(const wxString& type);
    void UpdateDuration();
    void UpdateSlider();
    bool HasSlider() const { return slider.IsValid(); }
    void SetSlider(float fraction, int component = 0);
    float GetSlider(int component = 0) const;
    bool IsColor() const { return GetName().Lower().Contains("color"); }
    wxColour GetColor() const;
    void SetColor(wxColour color);
    void Set(const mat2& m);
    void Set(const mat3& m);
    void Set(const mat4& m);
    void Set(float x);
    void Set(float x, float y);
    void Set(float x, float y, float z);
    void Set(float x, float y, float z, float w);
    void Set(int i);
    void Set(int i, int j);
    void Set(int i, int j, int k);
    void Set(int i, int j, int k, int h);
  private:
    TUniformValue value;
    wxString name;
    Id type;
    int location;
    int elapsed;   // current elapsed time in milliseconds
    int duration;  // total length of animation in milliseconds
    TMotionList motions;
    TSlider slider;
};

class TUniformList : public std::list<TUniform> {
  public:
    TUniformList() : ready(false) {}
    bool Load(GLuint program);
    void Animate(int period);
  private:
    bool ready;
};

#endif
