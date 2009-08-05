// place all os-specific code in this file

void SpawnBrowser(const char* htmlFilename);
void ToggleCursor();

#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/wglew.h>
#include <io.h>
#undef max
#undef min
#define strcasecomp _stricmp

BOOL ChooseMultisamplePixelFormat(HDC hdc, int* pixelFormat);

#else // Linux

#include <GL/gl.h>
#include <unistd.h>
typedef void DummySwapInterval(int);
extern DummySwapInterval* wglSwapIntervalEXT;

#define strcasecomp strcasecmp

#endif
