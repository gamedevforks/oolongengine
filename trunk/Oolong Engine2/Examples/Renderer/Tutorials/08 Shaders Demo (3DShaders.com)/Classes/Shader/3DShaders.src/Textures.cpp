//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Canvas.h"
#include <wx/wx.h>
#include <wx/image.h>
#include "os.h"
#include <GL/glew.h>
#include "Textures.h"
#include <png.h>

void CreateNoise3D();
const wxString folder = "../textures/";
const bool mirror = true;

TTextureList::TTextureList()
{
    wxImage::AddHandler(new wxPNGHandler);
    wxImage::AddHandler(new wxJPEGHandler);
    memset(textures, 0, sizeof(textures));
    memset(current, 0, sizeof(current));
    useMipmap = true;
}

void TTextureList::Load(const char* filename)
{
    int index = Id::TextureExternal - Id::FirstTexture;
    TTexture& texture = textures[index];

    if (!texture.glid)
        glGenTextures(1, &texture.glid);

    wxImage image(filename);
    if (!image.Ok()) {
        wxGetApp().Errorf("Unable to load a valid image from '%s'.\n", filename);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, texture.glid);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image.GetWidth(), image.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.GetData());
    if (wxGetApp().Canvas()->MipmapSupport()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

void TTextureList::Deactivate(Id id)
{
    if (id == Id::Texture3DNoise)
        glDisable(GL_TEXTURE_3D);
    else
        glDisable(GL_TEXTURE_2D);
}

void TTextureList::Activate(Id id, bool forceMipmap)
{
    if (!id || id == Id::TextureDisable)
        return;

    int index = id - Id::FirstTexture;
    TTexture& texture = textures[index];
    wxString textureString = Textures[index];

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (id == Id::Texture3DNoise) {
        glEnable(GL_TEXTURE_3D);

        if (texture.glid) {
            glBindTexture(GL_TEXTURE_3D, texture.glid);
            SetFilter(forceMipmap);
            return;
        }

        glGenTextures(1, &texture.glid);
        glBindTexture(GL_TEXTURE_3D, texture.glid);
        SetFilter(forceMipmap);
        texture.alpha = false;
        CreateNoise3D();
        return;
    }

    glEnable(GL_TEXTURE_2D);

    //
    // Grab the frame buffer to enable cool effects like glass and haze.
    // Create a 2048x2048 texture for it, since that's a reasonable power-of-two.
    // Note this fails to grab everything for viewports bigger than 2048x2048.
    //
    if (id == Id::TextureFramebuffer) {
        const int size = 2048;
        if (!texture.glid) {
            glGenTextures(1, &texture.glid);
            glBindTexture(GL_TEXTURE_2D, texture.glid);
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, size, size, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            glBindTexture(GL_TEXTURE_2D, texture.glid);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            wxSize s = wxGetApp().GetCanvasSize();
            int w = (int) s.GetWidth();
            int h = (int) s.GetHeight();
            w = w < size ? w : (int) size;
            h = h < size ? h : (int) size;
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
        }
        return;
    }

    if (texture.glid) {
        glBindTexture(GL_TEXTURE_2D, texture.glid);
        SetFilter(forceMipmap);
        return;
    }

    glGenTextures(1, &texture.glid);
    glBindTexture(GL_TEXTURE_2D, texture.glid);

    SetFilter(forceMipmap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // PNG files require special handling for alpha.
    if (strstr(textureString, ".png")) {
        loadPng(texture, folder + textureString);
        return;
    }

    // Otherwise use the wxWindows API to load in the image data.
    texture.alpha = false;
    wxImage image(folder + textureString);
    if (!image.Ok()) {
        wxGetApp().Errorf("Unable to load a valid image from '%s'.\n", textureString.c_str());
        return;
    }

    texture.width = image.GetWidth();
    texture.height = image.GetHeight();

    if (mirror)
        image = image.Mirror(false);

    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, image.GetWidth(), image.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.GetData());
}

void TTextureList::SetFilter(bool forceMipmap) const
{
    if (wxGetApp().Canvas()->MipmapSupport() && (useMipmap || forceMipmap)) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

void TTextureList::loadPng(TTexture& texture, const char* filename)
{
    png_struct_def* png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    png_info_struct* info = png_create_info_struct(png);
    png_info_struct* end = png_create_info_struct(png);

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        wxGetApp().Errorf("Unable to open texture file '%s'.\n", filename);
        return;
    }
    png_init_io(png, fp);
    png_read_info(png, info);
    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    if (colorType != PNG_COLOR_TYPE_RGB_ALPHA && colorType != PNG_COLOR_TYPE_RGB) {
        wxGetApp().Errorf("Texture file '%s' uses an unsupported format.  For PNG files only RGB and RGBA are supported.\n", filename);
        return;
    }
    
    bool alpha = colorType == PNG_COLOR_TYPE_RGB_ALPHA;
    int components = alpha ? 4 : 3;

    if (png_get_bit_depth(png, info) != 8) {
        wxGetApp().Errorf("Texture file '%s' uses an unsupported bit depth.  For PNG files only 8 bpp is supported.\n", filename);
        return;
    }

    png_bytepp rows = new png_bytep[height];
    png_bytep image = new png_byte[height * width * components];

    if (mirror) {
        for (int i = 0; i < height; i++)
            rows[i] = image + (height - i - 1) * width * components;
    } else {
        for (int i = 0; i < height; i++)
            rows[i] = image + i * width * components;
    }

    png_set_rows(png, info, rows);
    png_read_image(png, rows);

    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0, alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, image);

    png_destroy_read_struct(&png, &info, &end);
    fclose(fp);
    delete[] image;
    delete[] rows;

    texture.alpha = alpha;
    texture.width = width;
    texture.height = height;
}

void TTextureList::Activate()
{
    if (!glActiveTexture) {
        Activate(current[0]);
        return;
    }

    for (int stage = 0; stage < NumStages; ++stage) {
        if (current[stage]) {
            glActiveTexture(GL_TEXTURE0 + stage);
            Activate(current[stage]);
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

void TTextureList::Deactivate()
{
    if (!glActiveTexture) {
        Deactivate(current[0]);
        return;
    }

    for (int stage = 0; stage < NumStages; ++stage) {
        if (current[stage]) {
            glActiveTexture(GL_TEXTURE0 + stage);
            Deactivate(current[stage]);
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

void TTextureList::Set(Id id, int stage)
{
    if (stage == -1) {
        memset(current, 0, sizeof(current));
        current[0] = id;
        return;
    }

    current[stage] = id;
}

wxSize TTextureList::GetSize(Id id) const
{
    int index = id - Id::FirstTexture;
    const TTexture& texture = textures[index];
    return wxSize(texture.width, texture.height);
}
