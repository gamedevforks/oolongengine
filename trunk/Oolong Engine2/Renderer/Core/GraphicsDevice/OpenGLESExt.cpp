/*
Oolong Engine for the iPhone / iPod touch
Copyright (c) 2007-2008 Wolfgang Engel  http://code.google.com/p/oolongengine/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#include <string.h>

#include "OpenGLESExt.h"


void OpenGLESExt::Init()
{
	const GLubyte *pszGLExtensions;

	/* Retrieve GL extension string */
    pszGLExtensions = glGetString(GL_EXTENSIONS);

	/* GL_OES_matrix_palette */
	if (strstr((char *)pszGLExtensions, "GL_OES_matrix_palette"))
	{
		glCurrentPaletteMatrixOES			= (PFNGLCURRENTPALETTEMATRIXOES)eaglGetProcAddress("glCurrentPaletteMatrixOES");
		glLoadPaletteFromModelViewMatrixOES	= (PFNGLLOADPALETTEFROMMODELVIEWMATRIXOES)eglGetProcAddress("glLoadPaletteFromModelViewMatrixOES");
		glMatrixIndexPointerOES				= (PFNGLMATRIXINDEXPOINTEROES)eglGetProcAddress("glMatrixIndexPointerOES");
		glWeightPointerOES					= (PFNGLWEIGHTPOINTEROES)eglGetProcAddress("glWeightPointerOES");
	}

	/* GL_IMG_user_clip_plane */
	if (strstr((char *)pszGLExtensions, "GL_IMG_user_clip_plane"))
	{
		/* glClipPlanexIMG and glClipPlanefIMG */
		glClipPlanexIMG = (PFNGLCLIPPLANEXIMG)eglGetProcAddress("glClipPlanexIMG");
		glClipPlanefIMG = (PFNGLCLIPPLANEFIMG)eglGetProcAddress("glClipPlanefIMG");
	}

	/* GL_IMG_vertex_program */
	if (strstr((char *)pszGLExtensions, "GL_IMG_vertex_program"))
	{
		/* glVertexAttribPointerARB */
		glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARB)eglGetProcAddress("glVertexAttribPointerARB");

		/* glEnableVertexAttribArrayARB */
		glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARB)eglGetProcAddress("glEnableVertexAttribArrayARB");

		/* glDisableVertexAttribArrayARB */
		glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARB)eglGetProcAddress("glDisableVertexAttribArrayARB");

		/* glProgramStringARB */
		glProgramStringARB = (PFNGLPROGRAMSTRINGARB)eglGetProcAddress("glProgramStringARB");

		/* glBindProgramARB */
		glBindProgramARB = (PFNGLBINDPROGRAMARB)eglGetProcAddress("glBindProgramARB");

		/* glDeleteProgramsARB */
		glDeleteProgramsARB = (PFNGLDELETEPROGRAMSARB)eglGetProcAddress("glDeleteProgramsARB");

		/* glIsProgramARB */
		glIsProgramARB = (PFNGLISPROGRAMARB)eglGetProcAddress("glIsProgramARB");

		/* glGenProgramsARB */
		glGenProgramsARB = (PFNGLGENPROGRAMSARB)eglGetProcAddress("glGenProgramsARB");

		/* glVertexAttrib4fvARB */
		glVertexAttrib4fvARB = (PFNGLVERTEXATTRIB4FVARB)eglGetProcAddress("glVertexAttrib4fvARB");

		/* glVertexAttrib4xvIMG */
		glVertexAttrib4xvIMG = (PFNGLVERTEXATTRIB4XVIMG)eglGetProcAddress("glVertexAttrib4xvIMG");

		/* glProgramLocalParameter4xIMG */
		glProgramLocalParameter4xIMG = (PFNGLPROGRAMLOCALPARAMETER4XIMG)eglGetProcAddress("glProgramLocalParameter4xIMG");

		/* glProgramLocalParameter4xvIMG */
		glProgramLocalParameter4xvIMG = (PFNGLPROGRAMLOCALPARAMETER4XVIMG)eglGetProcAddress("glProgramLocalParameter4xvIMG");

		/* glProgramEnvParameter4xIMG */
		glProgramEnvParameter4xIMG = (PFNGLPROGRAMENVPARAMETER4XIMG)eglGetProcAddress("glProgramEnvParameter4xIMG");

		/* glProgramEnvParameter4xvIMG */
		glProgramEnvParameter4xvIMG = (PFNGLPROGRAMENVPARAMETER4XVIMG)eglGetProcAddress("glProgramEnvParameter4xvIMG");

		/* glProgramEnvParameter4fARB */
		glProgramEnvParameter4fARB = (PFNGLPROGRAMENVPARAMETER4FARB)eglGetProcAddress("glProgramEnvParameter4fARB");

		/* glProgramEnvParameter4fvARB */
		glProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARB)eglGetProcAddress("glProgramEnvParameter4fvARB");

		/* glProgramLocalParameter4fARB */
		glProgramLocalParameter4fARB = (PFNGLPROGRAMLOCALPARAMETER4FARB)eglGetProcAddress("glProgramLocalParameter4fARB");

		/* glProgGL_OES_draw_textureramLocalParameter4fvARB */
		glProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARB)eglGetProcAddress("glProgramLocalParameter4fvARB");
	}

	/* GL_OES_draw_texture */
	if (strstr((char *)pszGLExtensions, "GL_OES_draw_texture"))
	{
		/* glDrawTexiOES */
		glDrawTexiOES = (PFNGLDRAWTEXIOES)eglGetProcAddress("glDrawTexiOES");
		/* glDrawTexivOES */
		glDrawTexivOES = (PFNGLDRAWTEXIVOES)eglGetProcAddress("glDrawTexivOES");
		/* glDrawTexsOES */
		glDrawTexsOES = (PFNGLDRAWTEXSOES)eglGetProcAddress("glDrawTexsOES");
		/* glDrawTexsvOES */
		glDrawTexsvOES = (PFNGLDRAWTEXSVOES)eglGetProcAddress("glDrawTexsvOES");
		/* glDrawTexxOES */
		glDrawTexxOES = (PFNGLDRAWTEXXOES)eglGetProcAddress("glDrawTexxOES");
		/* glDrawTexxvOES */
		glDrawTexxvOES = (PFNGLDRAWTEXXVOES)eglGetProcAddress("glDrawTexxvOES");
		/* glDrawTexfOES */
		glDrawTexfOES = (PFNGLDRAWTEXFOES)eglGetProcAddress("glDrawTexfOES");
		/* glDrawTexfvOES */
		glDrawTexfvOES = (PFNGLDRAWTEXFVOES)eglGetProcAddress("glDrawTexfvOES");
	}

	/* IMG_texture_stream */
	if (strstr((char *)pszGLExtensions, "GL_IMG_texture_stream"))
	{
		glGetTexStreamDeviceAttribivIMG = (PFNGLGETTEXSTREAMDEVICEATTRIBIVIMG)eglGetProcAddress("glGetTexStreamDeviceAttribivIMG");
		glTexBindStreamIMG = (PFNGLTEXBINDSTREAMIMG)eglGetProcAddress("glTexBindStreamIMG");
		glGetTexStreamDeviceNameIMG = (PFNGLGETTEXSTREAMDEVICENAMEIMG)eglGetProcAddress("glGetTexStreamDeviceNameIMG");
	}

	/* GL_EXT_multi_draw_arrays */
	if (strstr((char *)pszGLExtensions, "GL_EXT_multi_draw_arrays"))
	{
		glMultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTS)eglGetProcAddress("glMultiDrawElementsEXT");
	}

}

// The recommended technique for querying OpenGL extensions;
// from http://opengl.org/resources/features/OGLextensions/
bool OpenGLESExt::IsGLExtensionSupported(const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    /* Extension names should not have spaces. */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    extensions = glGetString(GL_EXTENSIONS);

    /* It takes a bit of care to be fool-proof about parsing the
    OpenGL extensions string. Don't be fooled by sub-strings, etc. */
    start = extensions;
    for (;;) {
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return true;
        start = terminator;
    }
    return false;
}
