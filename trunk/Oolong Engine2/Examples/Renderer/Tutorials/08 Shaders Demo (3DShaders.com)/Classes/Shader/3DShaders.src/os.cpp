// place all os-specific code in this file
#include <wx/utils.h>
#include "os.h"

#ifdef WIN32

BOOL ChooseMultisamplePixelFormat(HDC hdc, int* pixelFormat)
{
    // Don't use GLEW because it hasn't been initialized yet.
    // Manually obtain the extension pointer.
    PROC proc = wglGetProcAddress("wglChoosePixelFormatARB");
    if (!proc)
        return false;

    PFNWGLCHOOSEPIXELFORMATARBPROC choosePixelFormat = (PFNWGLCHOOSEPIXELFORMATARBPROC) proc;

    unsigned int numFormats;
    float fAttributes[] = {0,0};

    int iAttributes[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB, 24,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_DEPTH_BITS_ARB, 16,
        WGL_STENCIL_BITS_ARB, 1,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, 8,
        0,0
    };

    return choosePixelFormat(hdc, iAttributes, fAttributes, 1, pixelFormat, &numFormats);
}

void SpawnBrowser(const char* htmlFilename)
{
    wxShell("start " + wxString(htmlFilename));
}

void ToggleCursor()
{
    static bool cursor = true;
    cursor = !cursor;
    ShowCursor(cursor);
}

#else // Linux

void SpawnBrowser(const char* htmlFilename) {} // TBD
void ToggleCursor() {} // TVBD
DummySwapInterval* wglSwapIntervalEXT = 0;

#endif

