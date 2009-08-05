//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef TABLES_H
#define TABLES_H
#include "Vector.h"

struct Id {
    enum EId {
        FileResetAll = 1,
        FileCycle,
        FileLoadXML,
        FileExit,

        ViewRefresh,
        ViewReset,
        ViewFullscreen,
        ViewWireframe,
        ViewCulling,
        ViewWaitVsync,
        ViewPerspective,
        ViewBackground,
        ViewMultisample,
        ViewPodium,
        ViewAnimate,
        ViewDemo,
        ViewInfoLog,
        ViewAdjuster,
        ViewShaderList,

        BackgroundSolidColor,
        BackgroundColorGradient,
        BackgroundScaledTexture,
        BackgroundTiledTexture,
        BackgroundTextures,
        BackgroundColors,

        BackgroundCyan,
        BackgroundBlue,
        BackgroundYellow,
        BackgroundRed,
        BackgroundBlack,
        BackgroundWhite,

        MultisampleAlwaysOff,
        MultisampleAlwaysOn,
        MultisampleOnWhenStopped,

        ModelKlein,
        ModelTrefoil,
        ModelConic,
        ModelTorus,
        ModelSphere,
        ModelPlane,
        ModelBigPlane,
        ModelRealizm,
        ModelAlien,
        ModelTeapot,
        ModelPointCloud,
        ModelBlobbyCloud,
        ModelExternal,
        ModelIncreaseTesselation,
        ModelDecreaseTesselation,
        ModelExternalPlaceholder,

        TextureDisable,
        TextureGrid,
        TextureLogo,
        TextureBaboon,
        TextureWater,
        TextureLeopard,
        TextureLava,

        Texture3DNoise,
        TextureFramebuffer,
        TextureRgbRand,
        TextureGlyphMosaic,
        TextureHouse,
        TextureHannah,
        TextureHannahBlurry,
        Texture3DlabsNormal,
        TextureDay,
        TextureNight,
        TextureClouds,
        TextureRust,
        TexturePointSprite,
        TextureWeave,
        TextureRainbow,
        TextureAbstract,
        TextureExternal,

        HelpCommandLine,
        HelpKeyboard,
        HelpAbout,
        HelpAboutClose,
        HelpKeyboardClose,

        ListSash,
        ListBox,
        LogClose,
        LogClear,
        LogSave,

        XmlTagDemo,
        XmlTagProgram,
        XmlTagModels,
        XmlTagTextures,
        XmlTagUniform,
        XmlTagSlider,
        XmlTagSilhouette,
        XmlTagTangents,
        XmlTagBinormals,
        XmlTagMotion,

        XmlAttrName,
        XmlAttrType,
        XmlAttrStress,
        XmlAttrVertex,
        XmlAttrFragment,
        XmlAttrBlending,
        XmlAttrReflection,
        XmlAttrDepth,
        XmlAttrFolder,
        XmlAttrStage,
        XmlAttrLod,
        XmlAttrMipmap,
        XmlAttrLength,

        UniformTypeInt,
        UniformTypeBool,
        UniformTypeVec2,
        UniformTypeVec3,
        UniformTypeVec4,
        UniformTypeIvec2,
        UniformTypeIvec3,
        UniformTypeIvec4,
        UniformTypeMat2,
        UniformTypeMat3,
        UniformTypeMat4,
        UniformTypeFloat,

        StressNone,
        StressVertex,
        StressFragment,


        FirstModel = ModelKlein,
        LastModel = ModelExternal,
        NumModels = LastModel - FirstModel + 1,

        FirstBackgroundColor = BackgroundCyan,
        LastBackgroundColor = BackgroundWhite,
        NumBackgroundColors = LastBackgroundColor - FirstBackgroundColor + 1,

        FirstTexture = TextureDisable,
        LastTexture = TextureExternal,
        FirstSelectableTexture = TextureGrid,
        LastSelectableTexture = TextureLava,
        NumTextures = LastTexture - FirstTexture + 1,

        FirstXmlTag = XmlTagDemo,
        LastXmlTag = XmlTagMotion,
        NumXmlTags = LastXmlTag - FirstXmlTag + 1,

        FirstXmlAttr = XmlAttrName,
        LastXmlAttr = XmlAttrLength,
        NumXmlAttrs = LastXmlAttr - FirstXmlAttr + 1,

        FirstUniformType = UniformTypeInt,
        LastUniformType = UniformTypeFloat,
        NumUniformTypes = LastUniformType - FirstUniformType + 1,
    };

    union {
        EId name;
        int value;
    };

    Id(EId name) : name(name) {}
    Id(int value) : value(value) {}
    Id() : value(0) {}
    operator EId() const { return name; }
    void operator++() { ++value; }
    void operator++(int) { value++; }
};

extern const char* Textures[Id::NumTextures];
extern const char* Models[Id::NumModels];
extern const char* XmlTags[Id::NumXmlTags];
extern const char* XmlAttrs[Id::NumXmlAttrs];
extern const char* UniformTypes[Id::NumUniformTypes];
extern const vec3 BackgroundColors[Id::NumBackgroundColors][4];


int findUniformType(const char* type);
int findXmlTag(const char* tag);
int findTexture(const char* file);
int findModel(const char* name);

#endif
