//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#include "App.h"
#include "Tables.h"
#include "Xml.h"
#include "Shaders.h"
#include <expat.h>
#include <cstring>

wxFileName Xml::filename;
std::stack<int> Xml::tags;
std::stack<std::string> Xml::content;
TSlider* Xml::pSlider = 0;
TMotion* Xml::pMotion = 0;
TUniform* Xml::pUniform = 0;
TProgram* Xml::pProgram = 0;
TProgramList* Xml::pProgramList = 0;

const char* Xml::findAttr(const char** attrs, int attr)
{
    while (attrs && *attrs) {
        if (!strcmp(XmlAttrs[attr - Id::FirstXmlAttr], *attrs))
            return *(attrs + 1);
        attrs += 2;
    }
    return 0;
}

void XMLCALL Xml::processStartTag(void* data, const char* tag, const char** attrs)
{
    tags.push(findXmlTag(tag));

    switch (tags.top()) {
        case Id::XmlTagDemo:
            pProgramList->SetFolder(filename.GetPath(wxPATH_GET_SEPARATOR) + wxString(findAttr(attrs, Id::XmlAttrFolder)));
            break;

        case Id::XmlTagProgram:
        {
            // Retreive the relevent XML attributes.
            wxString name = findAttr(attrs, Id::XmlAttrName);
            wxString vertex = findAttr(attrs, Id::XmlAttrVertex);
            wxString fragment = findAttr(attrs, Id::XmlAttrFragment);
            wxString blending = findAttr(attrs, Id::XmlAttrBlending);
            wxString reflection = findAttr(attrs, Id::XmlAttrReflection);
            wxString depth = findAttr(attrs, Id::XmlAttrDepth);
            wxString stressText = findAttr(attrs, Id::XmlAttrStress);

            // Set up defaults.
            if (vertex.empty())
                vertex = name;
            if (fragment.empty())
                fragment = name;
            if (blending.empty())
                blending = "off";
            if (reflection.empty())
                reflection = "on";
            if (depth.empty())
                depth = "on";

            Id stress;
            if (stressText.empty())
                stress = Id::StressNone;
            else
                stress = stressText == "vertex" ? Id::StressVertex : Id::StressFragment;

            // Form full path strings.
            vertex = pProgramList->GetFolder() + "/" + vertex + ".vert";
            fragment = pProgramList->GetFolder() + "/" + fragment + ".frag";

            // Create the program object.
            pProgram = pProgramList->NewShader(name, vertex, fragment, blending == "on", reflection == "on", depth == "on", stress);
            break;
        }

        case Id::XmlTagUniform:
        {
            if (!pProgram) {
                wxGetApp().Errorf("XML error: <uniform> outside <shader>.\n");
                break;
            }

            const char* name = findAttr(attrs, Id::XmlAttrName);
            const char* type = findAttr(attrs, Id::XmlAttrType);
            pUniform = pProgram->NewUniform(name, type);
            break;
        }

        case Id::XmlTagTangents:
        {
            if (!pProgram) {
                wxGetApp().Errorf("XML error: <tangents> outside <shader>.\n");
                break;
            }

            pProgram->RequiresTangents();
            break;
        }

        case Id::XmlTagBinormals:
        {
            if (!pProgram) {
                wxGetApp().Errorf("XML error: <binormals> outside <shader>.\n");
                break;
            }

            pProgram->RequiresBinormals();
            break;
        }

        case Id::XmlTagMotion:
        {
            if (!pUniform) {
                wxGetApp().Errorf("XML error: <linear> outside <uniform>.\n");
                break;
            }

            if (pUniform->GetType() == 0) {
                wxGetApp().Errorf("XML error: <motion> specified in <uniform> with unknown 'type'.\n");
                break;
            }

            const char* type = findAttr(attrs, Id::XmlAttrType);
            const char* length = findAttr(attrs, Id::XmlAttrLength);

            pMotion = length ? pUniform->NewMotion(type, length) : 0;
            pProgram->SetMotion();

            break;
        }

        case Id::XmlTagSlider:
        {
            if (!pUniform) {
                wxGetApp().Errorf("XML error: <linear> outside <uniform>.\n");
                break;
            }

            if (pUniform->GetType() == 0) {
                wxGetApp().Errorf("XML error: <slider> specified in <uniform> with unknown 'type'.\n");
                break;
            }

            const char* type = findAttr(attrs, Id::XmlAttrType);
            pSlider = pUniform->NewSlider(type);
            break;
        }

        case Id::XmlTagSilhouette:
        {
            if (!pProgram) {
                wxGetApp().Errorf("XML error: <silhouette> outside <program>.\n");
                break;
            }
            pProgram->SetSilhouette();
            break;
        }

        case Id::XmlTagTextures:
        {
            const char* stage = findAttr(attrs, Id::XmlAttrStage);
            if (stage)
                pProgram->TextureStage(atoi(stage));

            const char* mipmap = findAttr(attrs, Id::XmlAttrMipmap);
            if (mipmap)
                pProgram->Mipmap(atoi(mipmap));
            break;
        }

        case Id::XmlTagModels:
        {
            const char* lod = findAttr(attrs, Id::XmlAttrLod);
            if (lod)
                pProgram->Lod(atoi(lod));
            break;
        }
    }
    content.push(std::string());
}

void XMLCALL Xml::processEndTag(void* data, const char* tag)
{
    const char* pData = content.top().c_str();

    switch (tags.top()) {
        case Id::XmlTagUniform:
        {
            if (!pUniform) {
                wxGetApp().Errorf("XML error: <uniform> end tag without corresponding start tag.\n");
                break;
            }

            if (pSlider)
                pUniform->UpdateSlider();

            float x, y, z, w;
            int i, j, k, h;

            if (!pUniform->GetType()) {
                if (strchr(pData, '.') || strchr(pData, 'e')) {
                    int components = sscanf(pData, "%f %f %f %f", &x, &y, &z, &w);
                    switch (components)
                    {
                        case 1: pUniform->SetType(Id::UniformTypeFloat); break;
                        case 2: pUniform->SetType(Id::UniformTypeVec2); break;
                        case 3: pUniform->SetType(Id::UniformTypeVec3); break;
                        case 4: pUniform->SetType(Id::UniformTypeVec4); break;
                    }
                } else {
                    int components = sscanf(pData, "%d %d %d %d", &i, &j, &k, &h);
                    switch (components) {
                        case 1: pUniform->SetType(Id::UniformTypeInt); break;
                        case 2: pUniform->SetType(Id::UniformTypeIvec2); break;
                        case 3: pUniform->SetType(Id::UniformTypeIvec3); break;
                        case 4: pUniform->SetType(Id::UniformTypeIvec4); break;
                    }
                }
            }

            switch (pUniform->GetType()) {
                case Id::UniformTypeFloat: if (sscanf(pData, "%f", &x) > 0) pUniform->Set(x); break;
                case Id::UniformTypeVec2: if (sscanf(pData, "%f %f", &x, &y) > 0) pUniform->Set(x, y); break;
                case Id::UniformTypeVec3: if (sscanf(pData, "%f %f %f", &x, &y, &z) > 0) pUniform->Set(x, y, z); break;
                case Id::UniformTypeVec4: if (sscanf(pData, "%f %f %f %f", &x, &y, &z, &w) > 0) pUniform->Set(x, y, z, w); break;

                case Id::UniformTypeInt: if (sscanf(pData, "%d", &i) > 0) pUniform->Set(i); break;
                case Id::UniformTypeIvec2: if (sscanf(pData, "%d %d", &i, &j) > 0) pUniform->Set(i, j); break;
                case Id::UniformTypeIvec3: if (sscanf(pData, "%d %d %d", &i, &j, &k) > 0) pUniform->Set(i, j, k); break;
                case Id::UniformTypeIvec4: if (sscanf(pData, "%d %d %d %d", &i, &j, &k, &h) > 0) pUniform->Set(i, j, k, h); break;

                case Id::UniformTypeMat2:
                {
                    mat2 m;
                    sscanf(pData, "%f %f", &x, &y); m.data[0] = x; m.data[1] = y;
                    sscanf(pData, "%f %f", &x, &y); m.data[2] = x; m.data[3] = y;
                    pUniform->Set(m);
                }
                
                case Id::UniformTypeMat3:
                {
                    mat3 m;
                    sscanf(pData, "%f %f %f", &x, &y, &z); m.data[0] = x; m.data[1] = y; m.data[2] = z;
                    sscanf(pData, "%f %f %f", &x, &y, &z); m.data[3] = x; m.data[4] = y; m.data[5] = z;
                    sscanf(pData, "%f %f %f", &x, &y, &z); m.data[6] = x; m.data[7] = y; m.data[8] = z;
                    pUniform->Set(m);
                }

                case Id::UniformTypeMat4:
                {
                    mat4 m;
                    sscanf(pData, "%f %f %f %f", &x, &y, &z, &w); m.data[0] = x; m.data[1] = y; m.data[2] = z; m.data[3] = w;
                    sscanf(pData, "%f %f %f %f", &x, &y, &z, &w); m.data[4] = x; m.data[5] = y; m.data[6] = z; m.data[7] = w;
                    sscanf(pData, "%f %f %f %f", &x, &y, &z, &w); m.data[8] = x; m.data[9] = y; m.data[10] = z; m.data[11] = w;
                    sscanf(pData, "%f %f %f %f", &x, &y, &z, &w); m.data[12] = x; m.data[13] = y; m.data[14] = z; m.data[15] = w;
                    pUniform->Set(m);
                }
            }

            pUniform->UpdateDuration();

            if (pSlider) {
                for (int c = 0; c < pUniform->NumComponents(); ++c)
                    pUniform->SetSlider(pUniform->GetSlider(c), c);
                pSlider = 0;
            }

            break;
        }

        case Id::XmlTagMotion:
        {
            if (!pMotion)
                break;

            TUniformValue& start = pMotion->start;
            TUniformValue& stop = pMotion->stop;

            switch (pUniform->GetType()) {
                case Id::UniformTypeFloat:
                    sscanf(pData, "%f %f", &start.f[0], &stop.f[0]);
                    break;

                case Id::UniformTypeVec2:
                    sscanf(pData, "%f %f %f %f", &start.f[0], &start.f[1], &stop.f[0], &stop.f[1]);
                    break;

                case Id::UniformTypeVec3:
                    sscanf(pData, "%f %f %f %f %f %f", &start.f[0], &start.f[1], &start.f[2],
                        &stop.f[0], &stop.f[1], &stop.f[2]);
                    break;

                case Id::UniformTypeVec4:
                    sscanf(pData, "%f %f %f %f %f %f %f %f", &start.f[0], &start.f[1],
                        &start.f[2], &start.f[3], &stop.f[0], &stop.f[1], &stop.f[2], &stop.f[3]);
                    break;

                case Id::UniformTypeInt:
                    sscanf(pData, "%d %d", &start.i[0], &stop.i[0]);
                    break;

                case Id::UniformTypeIvec2:
                    sscanf(pData, "%d %d %d %d", &start.i[0], &start.i[1], &stop.i[0], &stop.i[1]);
                    break;

                case Id::UniformTypeIvec3:
                    sscanf(pData, "%d %d %d %d %d %d", &start.i[0], &start.i[1], &start.i[2],
                        &stop.i[0], &stop.i[1], &stop.i[2]);
                    break;

                case Id::UniformTypeIvec4:
                    sscanf(pData, "%d %d %d %d %d %d %d %d", &start.i[0], &start.i[1], &start.i[2],
                        &start.i[3], &stop.i[0], &stop.i[1], &stop.i[2], &stop.i[3]);
                    break;

                case Id::UniformTypeMat2:
                case Id::UniformTypeMat3:
                case Id::UniformTypeMat4:
                    wxGetApp().Errorf("XML error: matrices not yet implemented.\n");
                    break;
            }
            break;
        }

        case Id::XmlTagSlider:
        {
            if (!pSlider) {
                wxGetApp().Errorf("XML error: invalid slider.\n");
                break;
            }

            TUniformValue& current = pSlider->current;
            TUniformValue& start = pSlider->start;
            TUniformValue& stop = pSlider->stop;

            switch (pUniform->GetType()) {
                case Id::UniformTypeFloat:
                    sscanf(pData, "%f %f", &start.f[0], &stop.f[0]);
                    break;

                case Id::UniformTypeVec2:
                    sscanf(pData, "%f %f %f %f", &start.f[0], &start.f[1], &stop.f[0], &stop.f[1]);
                    break;

                case Id::UniformTypeVec3:
                    sscanf(pData, "%f %f %f %f %f %f", &start.f[0], &start.f[1], &start.f[2],
                        &stop.f[0], &stop.f[1], &stop.f[2]);
                    break;

                case Id::UniformTypeVec4:
                    sscanf(pData, "%f %f %f %f %f %f %f %f", &start.f[0], &start.f[1],
                        &start.f[2], &start.f[3], &stop.f[0], &stop.f[1], &stop.f[2], &stop.f[3]);
                    break;

                case Id::UniformTypeInt:
                    sscanf(pData, "%d %d", &start.i[0], &stop.i[0]);
                    break;

                case Id::UniformTypeIvec2:
                    sscanf(pData, "%d %d %d %d", &start.i[0], &start.i[1], &stop.i[0], &stop.i[1]);
                    break;

                case Id::UniformTypeIvec3:
                    sscanf(pData, "%d %d %d %d %d %d", &start.i[0], &start.i[1], &start.i[2],
                        &stop.i[0], &stop.i[1], &stop.i[2]);
                    break;

                case Id::UniformTypeIvec4:
                    sscanf(pData, "%d %d %d %d %d %d %d %d", &start.i[0], &start.i[1], &start.i[2],
                        &start.i[3], &stop.i[0], &stop.i[1], &stop.i[2], &stop.i[3]);
                    break;

                case Id::UniformTypeMat2:
                case Id::UniformTypeMat3:
                case Id::UniformTypeMat4:
                    wxGetApp().Errorf("XML error: matrices not yet implemented.\n");
                    break;
            }

            if (pUniform->IsInteger()) {
                for (int c = 0; c < pUniform->NumComponents(); ++c)
                    current.i[c] = (start.i[c] + stop.i[c]) / 2;
            } else {
                for (int c = 0; c < pUniform->NumComponents(); ++c)
                    current.f[c] = (start.f[c] + stop.f[c]) / 2;
            }

            break;
        }

        case Id::XmlTagModels:
        {
            if (!pProgram)
                return;

            // Parse the list of models.  Each model is seperated by whitespace.
            while (pData && *pData) {
                char* whitespace = strpbrk(pData, " \t\n");
                if (whitespace)
                    *whitespace = 0;
                int length = strlen(pData);
                if (!length)
                    break;
                if (*pData == '-')
                    pProgram->SubtractModel(pData + 1);
                else
                    pProgram->AddModel(pData);
                if (!whitespace)
                    break;
                pData = whitespace + 1;
                while (*pData && pData == strpbrk(pData, " \t\n"))
                    ++pData;
            }
            break;
        }

        case Id::XmlTagTextures:
        {
            if (!pProgram)
                return;

            // Parse the list of textures.  Each texture is seperated by whitespace.
            while (pData && *pData) {
                char* whitespace = strpbrk(pData, " \t\n");
                if (whitespace)
                    *whitespace = 0;
                int length = strlen(pData);
                if (!length)
                    break;
                if (*pData == '-')
                    pProgram->SubtractTexture(pData + 1);
                else
                    pProgram->AddTexture(pData);
                if (!whitespace)
                    break;
                pData = whitespace + 1;
                while (*pData && pData == strpbrk(pData, " \t\n"))
                    ++pData;
            }
            break;
        }
    }
    tags.pop();
    content.pop();
}

void XMLCALL Xml::processData(void* user, const XML_Char* data, int length)
{
    if (!tags.empty())
        content.top().insert(content.top().length(), data, length);
}

bool Xml::Parse(const wxFileName& filename)
{
    Xml::filename = filename;
    XML_Parser parser = XML_ParserCreate(0);
    XML_SetElementHandler(parser, processStartTag, processEndTag);
    XML_SetCharacterDataHandler(parser, processData);

    FILE* xml = fopen(filename.GetFullPath(), "r");
    if (!xml)
        return false;

    char buffer[1024];
    int done;
    do {
        fgets(buffer, sizeof(buffer), xml);
        done = feof(xml);
        if (!done && XML_Parse(parser, buffer, strlen(buffer), 0) == XML_STATUS_ERROR)
            wxGetApp().Errorf("XML parse error at line %d:\n%s\n",
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
    } while (!done);
    fclose(xml);
    XML_ParserFree(parser);
    return true;
}
