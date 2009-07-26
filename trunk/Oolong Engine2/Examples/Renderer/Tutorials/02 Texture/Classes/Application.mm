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

#import <OpenGLES/EAGL.h>

//#include "Log.h"
#include <TargetConditionals.h>
#include <Availability.h>
#include "App.h"
#include "Mathematics.h"
#include "GraphicsDevice.h"
#include "MemoryManager.h"
#include "UI.h"
#include "Macros.h"
#include "Timing.h"
#include "Pathes.h"
#include "ResourceFile.h"


#include <stdio.h>
#include <sys/time.h>

CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

structTimer DrawCubeTimer;
float DrawCubeT;

structTimer DrawUITimer;
float DrawUIT;

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0
#define TEXCOORD_ARRAY	1

// Declare the fragment and vertex shaders.
GLuint uiFragShader, uiVertShader;		// Used to hold the fragment and vertex shader handles
GLuint uiProgramObject;					// Used to hold the program handle (made out of the two previous shaders)

// This ties in with the shader attribute to link to openGL, see pszVertShader.
const char* pszAttribs[] = { "myVertex", "myTexCoord" };

// Handles for the uniform variables.
int PMVMatrixHandle;
int ColHandle;	
int TextureHandle;	
	
GLuint ui32Vbo = 0;					    // Vertex buffer object handle

// View Angle for animation
float			m_fAngleY;

// Matrices for Views, Projections, mViewProjection.
MATRIX mView, mProjection, mViewProjection, mMVP, mModel, mRotate, mRotateY, mRotateX;
Vec3 vTo, vFrom;

CTexture *texture;
GLuint texID_rock = -1;




bool CShell::InitApplication()
{
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
		printf("Display text textures loaded\n");
	
	
	StartTimer(&DrawCubeTimer);
	StartTimer(&DrawUITimer);

	char buffer[2048];
	GetResourcePathASCII(buffer, 2048);
	
	CPVRTResourceFile::SetReadPath(buffer);
	
	texture = new CTexture();
	texture->LoadTextureFromPVR( "/rock_mipmap_4.pvr", &texID_rock );
	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000
	if( __OPENGLES_VERSION >= 2 )
	{
		/* Gets the Data Path */	
		if(ShaderLoadFromFile("blank", "/TextureFragShader.fsh", GL_FRAGMENT_SHADER, 0, &uiFragShader) == 0)
			printf("Loading the fragment shader fails");
		if(ShaderLoadFromFile("blank", "/TextureVertShader.vsh", GL_VERTEX_SHADER, 0, &uiVertShader) == 0)
			printf("Loading the vertex shader fails");
		
		CreateProgram(&uiProgramObject, uiVertShader, uiFragShader, pszAttribs, 2);
		
		// First gets the location of that variable in the shader using its name
		PMVMatrixHandle = glGetUniformLocation(uiProgramObject, "myPMVMatrix");
		ColHandle = glGetUniformLocation(uiProgramObject, "Col");		
		TextureHandle = glGetUniformLocation(uiProgramObject, "s_texture");		
	}
#endif
	
	// We're going to draw a cube, so let's create a vertex buffer first!
	{
		GLfloat verts[] =
		{
			// Top - facing to +Y		// Texture Coord
			 1.0f, 1.0f,-1.0f,			1.0f, 0.0f,	
			-1.0f, 1.0f,-1.0f,			0.0f, 0.0f,	
			-1.0f, 1.0f, 1.0f,			0.0f, 1.0f,
			 1.0f, 1.0f, 1.0f,			1.0f, 1.0f,
			
			// Bottom - facing to -Y
			 1.0f,-1.0f, 1.0f,			1.0f, 0.0f,
			-1.0f,-1.0f, 1.0f,			0.0f, 0.0f,
			-1.0f,-1.0f,-1.0f,			0.0f, 1.0f,
			 1.0f,-1.0f,-1.0f,			1.0f, 1.0f,
			
			// Front - facing to +Z
			 1.0f, 1.0f, 1.0f,			1.0f, 0.0f,
			-1.0f, 1.0f, 1.0f,			0.0f, 0.0f,
			-1.0f,-1.0f, 1.0f,			0.0f, 1.0f,
			 1.0f,-1.0f, 1.0f,			1.0f, 1.0f,
			
			// Back - facing to -Z
			 1.0f,-1.0f,-1.0f,			1.0f, 0.0f,
			-1.0f,-1.0f,-1.0f,			0.0f, 0.0f,
			-1.0f, 1.0f,-1.0f,			0.0f, 1.0f,
			 1.0f, 1.0f,-1.0f,			1.0f, 1.0f,
			
			// Right - facing to +X
			 1.0f, 1.0f,-1.0f,			1.0f, 0.0f,
			 1.0f, 1.0f, 1.0f,			0.0f, 0.0f,
			 1.0f,-1.0f, 1.0f,			0.0f, 1.0f,
			 1.0f,-1.0f,-1.0f,			1.0f, 1.0f,
			
			// Left - facing to -X
			-1.0f, 1.0f, 1.0f,			1.0f, 0.0f,
			-1.0f, 1.0f,-1.0f,			0.0f, 0.0f,
			-1.0f,-1.0f,-1.0f,			0.0f, 1.0f,
			-1.0f,-1.0f, 1.0f,			1.0f, 1.0f
		};
		
		// Generate the vertex buffer object (VBO)
		glGenBuffers(1, &ui32Vbo);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		
		// Set the buffer's data
		// Calculate verts size: (3 vertices * stride (3 GLfloats per each vertex))
		unsigned int uiSize = 24 * (sizeof(GLfloat) * 5);
		glBufferData(GL_ARRAY_BUFFER, uiSize, verts, GL_STATIC_DRAW);
	}
	
	
	vTo = Vec3(0.0f, 0.0f, 150.0f);
	vFrom = Vec3(0.0f, 0.0f, 0.0f);
	MatrixLookAtRH(mView, vFrom, vTo, Vec3(0,1,0));
	
	MatrixPerspectiveFovRH(mProjection, f2vt(30*PIf/180.0f), f2vt(((float) 320 / (float) 480)), f2vt(0.1f), f2vt(1000.0f), 1);

	m_fAngleY = 0.0;	
	
	return true;
}

bool CShell::QuitApplication()
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 )
	{
		// Frees the OpenGL handles for the program and the 2 shaders
		glDeleteProgram(uiProgramObject);
		glDeleteShader(uiFragShader);
		glDeleteShader(uiVertShader);
	}
#endif
	
	// Delete the VBO as it is no longer needed
	glDeleteBuffers(1, &ui32Vbo);
	
	AppDisplayText->ReleaseTextures();
	delete AppDisplayText;
	
	return true;
}

bool CShell::UpdateScene()
{		
	ResetTimer(&DrawCubeTimer);
	
	// Sets the clear color.
	// The colours are passed per channel (red,green,blue,alpha) as float values from 0.0 to 1.0
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Rotate the model matrix
	MatrixIdentity(mModel);
	MatrixIdentity(mRotate);
	MatrixIdentity(mRotateY);
	MatrixIdentity(mRotateX);
	// Set the model 10 units back into the screen.
	MatrixTranslation(mModel, 0.0f, 0.0f, 10.0f);
	// Rotate the model by a small amount.
	MatrixRotationY(mRotateY, m_fAngleY);
	MatrixRotationX(mRotateX, m_fAngleY);
	MatrixMultiply(mRotate, mRotateY, mRotateX);
	MatrixMultiply(mModel, mRotate, mModel);
	
	m_fAngleY += 0.02f;

	// Calculate view projection matrix.
	MatrixMultiply(mViewProjection, mView, mProjection);
	
	// Calculate model view projection matrix
	MatrixMultiply(mMVP, mModel, mViewProjection);

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000
	if( __OPENGLES_VERSION >= 2 ) {
		glUseProgram(uiProgramObject);
		// Then passes the matrix to that variable
		glUniformMatrix4fv( PMVMatrixHandle, 1, GL_FALSE, mMVP.f);
	}
	else 
#endif
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(mProjection.f);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(mView.f);
		glMultMatrixf(mModel.f);
	}
	
	++frames;
	CFTimeInterval			TimeInterval;
	frameRate = GetFps(frames, TimeInterval);

	AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f Cube: %3.2fms UI: %3.2fms", frameRate, DrawCubeT, DrawUIT);
	
	return true;
}

bool CShell::RenderScene()
{
	glActiveTexture( GL_TEXTURE0 );
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texID_rock);
	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 ) 
	{
		glUseProgram(uiProgramObject);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		
		glEnableVertexAttribArray(VERTEX_ARRAY);
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 20, 0);	// Stride = 20 bytes
		
		glEnableVertexAttribArray(TEXCOORD_ARRAY);
		glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
		
		glUniform1i( TextureHandle, 0 );
		
		// First gets the location of the color variable in the shader using its name
		// Then passes the proper color to that variable.
		glUniform4f(ColHandle, 1.0, 1.0, 1.0, 1.0);
		glDrawArrays(GL_TRIANGLE_FAN,  0, 4);
		glDrawArrays(GL_TRIANGLE_FAN,  4, 4);
		glDrawArrays(GL_TRIANGLE_FAN,  8, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 20, 4);
	}
	else
#endif
	{
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 20, 0);	// Stride = 20 bytes
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		glTexCoordPointer(2, GL_FLOAT, 20, (void *)12);
		
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
		glDrawArrays(GL_TRIANGLE_FAN, 20, 4);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	DrawCubeT = GetAverageTimeValueInMS(&DrawCubeTimer);
	
	ResetTimer(&DrawUITimer);
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("Texture", "", eDisplayTextLogoIMG);
	AppDisplayText->Flush();	
	
	DrawUIT = GetAverageTimeValueInMS(&DrawUITimer);

	return true;
}
