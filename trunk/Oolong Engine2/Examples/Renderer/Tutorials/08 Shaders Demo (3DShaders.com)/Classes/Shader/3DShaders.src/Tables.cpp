//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include <cstring>
#include "Tables.h"
#include "os.h"

const char* Textures[Id::NumTextures] =
{
    // Selectable textures
    "Disable",
    "Grid.png",
    "3Dlabs.png",
    "Baboon.jpg",
    "Water.jpg",
    "Leopard.jpg",
    "Lava.jpg",

    // Unselectable textures
    "3DNoise",
    "Framebuffer",
    "RgbRand.bmp",
    "GlyphMosaic.jpg",
    "House.jpg",
    "Hannah.jpg",
    "HannahBlurry.jpg",
    "3DlabsNormal.png",
    "Day.jpg",
    "Night.jpg",
    "Clouds.jpg",
    "Rust.bmp",
    "PointSprite.png",
    "Weave.jpg",
    "Rainbow.jpg",
    "Abstract.jpg",

    // Arbitrary texture
    "External",
};


const char* Models[Id::NumModels] =
{
    "Klein",
    "Trefoil",
    "Conic",
    "Torus",
    "Sphere",
    "Plane",
    "BigPlane",
    "Realizm",
    "Alien",
    "Teapot",
    "PointCloud",
    "BlobbyCloud",
    "External",
};

const char* XmlTags[Id::NumXmlTags] =
{
    "demo",
    "program",
    "models",
    "textures",
    "uniform",
    "slider",
    "silhouette",
    "tangents",
    "binormals",
    "motion",
};

const char* XmlAttrs[Id::NumXmlAttrs] =
{
    "name",
    "type",
    "stress",
    "vertex",
    "fragment",
    "blending",
    "reflection",
    "depth",
    "folder",
    "stage",
    "lod",
    "mipmap",
    "length",
};

const char* UniformTypes[Id::NumUniformTypes] =
{
    "int",
    "bool",
    "vec2",
    "vec3",
    "vec4",
    "ivec2",
    "ivec3",
    "ivec4",
    "mat2",
    "mat3",
    "mat4",
    "float",
};

//
// Declare some nice preset color gradients for the background.
// Order: bottom left, bottom right, top right, top left
//
const vec3 BackgroundColors[Id::NumBackgroundColors][4] =
{
    vec3(.4f,1,1), vec3(.4f,1,1), vec3(0,0,.3f), vec3(0,0,.3f),
    vec3(0,0,.5f), vec3(0,0,.5f), vec3(0,.8f,.8f), vec3(0,.8f,.8f),
    vec3(.8F,.8F,0), vec3(.8F,.8F,0), vec3(0.7f,0.7f,0.35f), vec3(.9F,.9F,.9F),
    vec3(.8f,0,0), vec3(.8f,0,0), vec3(.8f,.8f,0), vec3(.8f,.8f,0),
    vec3(0,0,0), vec3(0,0,0), vec3(.8f,.8f,.8f), vec3(.8f,.8f,.8f),
    vec3(.8f,.8f,.8f),  vec3(.8f,.8f,.8f), vec3(0,0,0), vec3(0,0,0),
};

//
// Note that using std::hash_map would be more efficient for table lookups.
// For this application, we prefer simplicity and portability over performance.
//
int findUniformType(const char* type)
{
    for (int i = 0; i < Id::NumUniformTypes; ++i)
        if (!strcasecomp(UniformTypes[i], type))
            return i + Id::FirstUniformType;
    return 0;
}

int findXmlTag(const char* tag)
{
    for (int i = 0; i < Id::NumXmlTags; ++i)
        if (!strcasecomp(XmlTags[i], tag))
            return i + Id::FirstXmlTag;
    return 0;
}

int findTexture(const char* file)
{
    for (int i = 0; i < Id::NumTextures; ++i) {
        if (!strcasecomp(Textures[i], file))
            return i + Id::FirstTexture;
        if (strstr(Textures[i], file))
            return i + Id::FirstTexture;
    }
    return 0;
}

int findModel(const char* name)
{
    for (int i = 0; i < Id::NumModels; ++i)
        if (!strcasecomp(Models[i], name))
            return i + Id::FirstModel;
    return 0;
}
