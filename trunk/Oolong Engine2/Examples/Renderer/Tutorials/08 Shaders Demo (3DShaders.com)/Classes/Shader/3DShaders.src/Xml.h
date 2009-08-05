//
// Author:    Philip Rideout
// Copyright: 2002-2006  3Dlabs Inc. Ltd.  All rights reserved.
// License:   see 3Dlabs-license.txt
//

#ifndef XML_H
#define XML_H
#include <stack>
#include <string>
#include <expat.h>
#include <wx/filename.h>

struct TSlider;
struct TMotion;
class TUniform;
class TProgram;
class TProgramList;

#ifndef XMLCALL
#define XMLCALL
#endif

// Use a namespace instead of a class because expat uses C-style callbacks.
namespace Xml {
    bool Parse(const wxFileName& filename);
    const char* findAttr(const char** attrs, int attr);
    void XMLCALL processStartTag(void* data, const char* tag, const char** attrs);
    void XMLCALL processEndTag(void* data, const char* tag);
    void XMLCALL processData(void* user, const XML_Char* data, int length);
    extern std::stack<int> tags;
    extern std::stack<std::string> content;
    extern TSlider* pSlider;
    extern TMotion* pMotion;
    extern TUniform* pUniform;
    extern TProgram* pProgram;
    extern TProgramList* pProgramList;
    extern wxFileName filename;
    extern int slices;
}

#endif
