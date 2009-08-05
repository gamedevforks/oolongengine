//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Canvas.h"
#include "Frame.h"

BEGIN_EVENT_TABLE(TCanvas, wxGLCanvas)
    EVT_SIZE(TCanvas::OnSize)
    EVT_PAINT(TCanvas::OnPaint)
    EVT_ERASE_BACKGROUND(TCanvas::OnEraseBackground)
    EVT_MOUSE_EVENTS(TCanvas::OnMouse)
END_EVENT_TABLE()

int TCanvas::Attributes[] = 
{
    WX_GL_RGBA,
    WX_GL_DEPTH_SIZE, 16,
    WX_GL_STENCIL_SIZE, 1,
    WX_GL_DOUBLEBUFFER,
    0
};

const int TCanvas::DoubleBufferAttributeIndex = 5;
const float TCanvas::CameraZ = -5;

//
// wxGLCanvas has been hacked so that it can set up an extended pixel format.  Use -1 for the default pixel format.
//
#ifdef WIN32
TCanvas::TCanvas(wxWindow *parent, int pixelFormat) : wxGLCanvas(parent, (wxGLCanvas*) 0, -1, wxDefaultPosition, wxDefaultSize, 0, "TCanvas", Attributes, pixelFormat)
#else
TCanvas::TCanvas(wxWindow *parent, int pixelFormat) : wxGLCanvas(parent, (wxGLCanvas*) 0, -1, wxDefaultPosition, wxDefaultSize, 0, "TCanvas", Attributes)
#endif
{
    trackball.SetCanvas(this);
    glReady = false;
    mipmapSupport = false;
    gl2Support = false;
    aaSupport = true;
    frame = (TFrame*) parent;
    zoom = TTrackball::StartZoom;
    currentColor = wxGetApp().Simple() ? 3 : 0;
    multisampling = pixelFormat != -1;
    backgroundTexture = Id::TextureAbstract;
    needsDraw = true;
    dirty = 1;
}

void TCanvas::Reset(bool all)
{
    if (all)
        currentColor = wxGetApp().Simple() ? 3 : 0;

    SetZoom(GetZoom());
    trackball.Reset(all);
    dirty = 2;
}

void TCanvas::OnSize(wxSizeEvent& event)
{
    wxGLCanvas::OnSize(event);

    if (GetContext() && glReady)
        glSetup();
}

void TCanvas::glSetup()
{
    GetClientSize(&width, &height);
    float aspect = (float) width / (float) height;
    float vp = 0.8f;
    left = -vp;
    right = vp;
    bottom = -vp / aspect;
    top = vp / aspect;
    znear = 2;
    zfar = 10;

    SetCurrent();
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (frame->IsChecked(Id::ViewPerspective))
        glFrustum(left * zoom, right * zoom, bottom * zoom, top * zoom, znear, zfar);
    else
        glOrtho(left * zoom * 3, right * zoom * 3, bottom * zoom * 3, top * zoom * 3, znear, zfar);

    // If the podium is turned off, aim the camera at the center of the model rather than (0,0,0).
    if (!frame->IsChecked(Id::ViewPodium))
        glTranslate(-frame->GetCenter());

    glMatrixMode(GL_MODELVIEW);

    if (glReady)
        return;

    if (glewInit() != GLEW_OK)
        wxGetApp().Errorf("Unable to initialize GLEW.\n");

    // Wait for vsync.
    if (wglSwapIntervalEXT) {
        if (wxGetApp().CapFps())
            wglSwapIntervalEXT(1);
        else
            wglSwapIntervalEXT(0);
    }

    // Automatic mipmap generation was introduced in OpenGL 1.4
    int major, minor;
    const GLubyte* string = glGetString(GL_VERSION);
    sscanf(reinterpret_cast<const char*>(string), "%d.%d", &major, &minor);
    mipmapSupport = major > 1 || (major == 1 && minor >= 4);
    gl2Support = major > 1;

    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glPointSize(4);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (aaSupport)
    {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POINT_SMOOTH);
    }

    float dcolor[] = {0.7f, 0.75f, 0.75f, 1.0f};
    float scolor[] = {0.125f, 0.125f, 0.125f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dcolor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, scolor);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 125);

    --currentColor;
    NextBackgroundColor();

    frame->InitPrograms();
    frame->LoadModel(wxGetApp().Logo() ? Id::ModelPlane : Id::ModelKlein);

    glReady = true;
}

void TCanvas::NextBackgroundColor()
{
    if (++currentColor >= Id::NumBackgroundColors)
        currentColor = 0;

    vec3 color = BackgroundColors[currentColor][0];
    glClearColor(color.x, color.y, color.z, 0);
}

void TCanvas::SetBackgroundColor(Id id)
{
    currentColor = id - Id::FirstBackgroundColor;
    vec3 color = BackgroundColors[currentColor][0];
    glClearColor(color.x, color.y, color.z, 0);
}

vec3 TCanvas::GetWorldSpace(int x, int y)
{
    vec3 v;
    v.x = (float) x / (float) width;
    v.y = 1 - (float) y / (float) height;
    v.x *= (right - left);
    v.y *= (top - bottom);
    v.x += left;
    v.y += bottom;
    v.z = CameraZ;
    return v;
}

void TCanvas::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    Dirty();
}

void TCanvas::Draw()
{
    if (glReady && !IsFrozen())
        needsDraw = true;

    if (!GetContext() || !needsDraw)
        return;

    SetCurrent();
    if (!glReady)
        glSetup();

    if (multisampling && !wxGetApp().IsBenching() &&
        !frame->IsChecked(Id::MultisampleAlwaysOff) &&
        (frame->IsChecked(Id::MultisampleAlwaysOn) || IsFrozen()))
        glEnable(GL_MULTISAMPLE);
    else
        glDisable(GL_MULTISAMPLE);

    //
    // Draw the background or do a clear.
    //
    drawBackground();

    //
    //   1) Draw the podium top in pure black.  Also write to the stencil buffer.
    //   2) Draw a reflection of the model and test against the stencil buffer.
    //   3) Draw the podium again using blending and texturing.
    //
    if (frame->IsChecked(Id::ViewPodium)) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
        trackball.LoadMatrix();
        glColor4f(0, 0, 0, 0);
        podium.DrawTop();
        glEnable(GL_DEPTH_TEST);

        if (wxGetApp().GetCurrentProgram().Reflection()) {
            glStencilFunc(GL_EQUAL, 1, 0xffffffff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glFrontFace(GL_CW);

            {
                trackball.LoadMatrix();
                glTranslatef(0, -2, 0);
                glScalef(1, -1, 1);
                drawModel();
            }

            glFrontFace(GL_CCW);
        }

        glDisable(GL_STENCIL_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        trackball.LoadMatrix();
        frame->SetTexture(Id::TextureRust);
        podium.Draw();
        frame->UnsetTexture(Id::TextureRust);
    }

    // Draw the model(s).
    {
        trackball.LoadMatrix();
        drawModel();
        drawLogo();
    }

    // Finish the scene.
    glFlush();
    if (!wxGetApp().Single())
        SwapBuffers();

    if (IsFrozen())
        needsDraw = false;

    if (dirty)
        --dirty;
}

void TCanvas::Refresh(bool eraseBackground, const wxRect* rect)
{
    needsDraw = true;
    return wxGLCanvas::Refresh(eraseBackground, rect);
}

void TCanvas::drawLogo() const
{
    if (!wxGetApp().CapFps() || wxGetApp().Simple())
        return;

    static float alpha = 0;
    if (alpha < 0.75f)
        alpha += 0.01f;

    // Set up some GL state: load the texture, turn on mipmapping, etc.
    glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
    frame->SetTexture(Id::TextureLogo);
    if (MipmapSupport()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glColor4f(1, 1, 1, alpha);

    // Quick constants for canvas width/height, logo width/height, and x/y padding.
    const float cw = width;
    const float ch = height;
    const float tw = 64;
    const float th = 64;
    const float xp = 3;
    const float yp = 3;

    // Set the view such that texels map one-to-one with pixels.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, cw, 0, ch, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw a textured quad in the upper-right corner.
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(cw - tw - xp, ch - th - yp);
    glTexCoord2f(1, 0);
    glVertex2f(cw - xp, ch - th - yp);
    glTexCoord2f(1, 1);
    glVertex2f(cw - xp, ch - yp);
    glTexCoord2f(0, 1);
    glVertex2f(cw - tw - xp, ch - yp);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}

//
// Draw the background and clear the framebuffer.
//
void TCanvas::drawBackground() const
{
    if (frame->IsChecked(Id::BackgroundScaledTexture) || frame->IsChecked(Id::BackgroundTiledTexture)) {
        glColor4f(1,1,1,1);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        frame->SetTexture(backgroundTexture);
        wxSize c = wxGetApp().GetCanvasSize();
        wxSize t = frame->TextureSize(backgroundTexture);

        float xScale = 1;
        float yScale = 1;
        if (frame->IsChecked(Id::BackgroundTiledTexture)) {
            xScale = (float) c.GetWidth() / (float) t.GetWidth();
            yScale = (float) c.GetHeight() / (float) t.GetHeight();
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glBegin(GL_QUADS);
        glTexCoord2f(0, yScale);
        glVertex2f(-1, -1);
        glTexCoord2f(xScale, yScale);
        glVertex2f(1, -1);
        glTexCoord2f(xScale, 0);
        glVertex2f(1, 1);
        glTexCoord2f(0, 0);
        glVertex2f(-1, 1);
        glEnd();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
        frame->UnsetTexture(backgroundTexture);
    } else if (frame->IsChecked(Id::BackgroundColorGradient)) {
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        if (currentColor == 1)
            glScalef(3,3,1);
        glBegin(GL_QUADS);
        glColor(BackgroundColors[currentColor][0]);
        glVertex2f(-1,-1);
        glColor(BackgroundColors[currentColor][1]);
        glVertex2f(1,-1);
        glColor(BackgroundColors[currentColor][2]);
        glVertex2f(1,1);
        glColor(BackgroundColors[currentColor][3]);
        glVertex2f(-1,1);
        glEnd();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
}

void TCanvas::drawModel() const
{
    frame->UpdateModel();

    if (frame->IsChecked(Id::ViewCulling))
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    // Draw a white wireframe if requested.  Turn on blending because line smooth is on.
    if (frame->IsChecked(Id::ViewWireframe)) {
        glColor4f(1,1,1,1);
        glEnable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1);
        frame->DrawModel();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);
        return;
    }

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    bool fixedFunction = !frame->LoadProgram();
    frame->SetTexture();

    if (frame->Blending())
        glEnable(GL_BLEND);
    if (!frame->Depth() || wxGetApp().IsBenching())
        glDisable(GL_DEPTH_TEST);

    if (fixedFunction || !glUseProgram) {
        if (glUseProgram)
            glUseProgram(0);
        glEnable(GL_LIGHTING);
        frame->DrawModel();
        glDisable(GL_LIGHTING);
    } else {
        if (wxGetApp().GetCurrentProgram().HasSilhouette()) {
            glEnable(GL_CULL_FACE);
            glDepthFunc(GL_LESS);
            glCullFace(GL_BACK);
            frame->DrawModel();
            glEnable(GL_BLEND);
            glLineWidth(2.0);
            if (glUseProgram)
                glUseProgram(0);
            glPolygonMode(GL_BACK, GL_LINE);
            frame->UnsetTexture();
            glDepthFunc(GL_LEQUAL);
            glCullFace(GL_FRONT);
            glColor4f(0,0,0,1);
            frame->DrawModel();
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            frame->DrawModel();
            glUseProgram(0);
        }
    }

    glColor4f(1,1,1,1);
    frame->UnsetTexture();
    glPopAttrib();
}

bool TCanvas::IsFrozen() const
{
    return !dirty && trackball.IsFrozen() && frame->IsFrozen();
}

void TCanvas::SetZoom(float z)
{
    zoom = z;
    if (glReady)
        glSetup();
}

void TCanvas::Animate(int period)
{
    if (frame->IsChecked(Id::ViewAnimate))
    {
        frame->Animate(period);
        Update();
    }
}


void TCanvas::DisableAA()
{
    aaSupport = false;
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);
}
