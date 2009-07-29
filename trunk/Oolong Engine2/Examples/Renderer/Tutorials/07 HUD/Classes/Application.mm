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
#define VERTEX_ARRAY		0
#define COLOR_ARRAY			1
#define TEXCOORD_ARRAY		1

// Declare the fragment and vertex shaders.
GLuint uiFragShader, uiVertShader;		// Used to hold the fragment and vertex shader handles
GLuint uiProgramObject;					// Used to hold the program handle (made out of the two previous shaders)

GLuint uiFragShader2, uiVertShader2;		// Used to hold the fragment and vertex shader handles
GLuint uiProgramObject2;					// Used to hold the program handle (made out of the two previous shaders)

// This ties in with the shader attribute to link to openGL, see pszVertShader.
const char* pszAttribs[] = { "myVertex", "myColor" };
const char* pszAttribs2[] = { "myVertex", "myTexCoord" };

// Handles for the uniform variables.
int PMVMatrixHandle;
int ColHandle;	

int PMVMatrixHandle2;
int ColHandle2;	
int TextureHandle2;
	
GLuint ui32Vbo = 0;					    // Vertex buffer object handle
GLuint ui32HUD = 0;					    // Vertex buffer object handle

// View Angle for animation
float			m_fAngleY;

CTexture *texture;
GLuint texID_HUD = -1;

GLfloat colors[] = 
{
	1, 0, 0, 1,
	0, 1, 0, 1, 
	0, 0, 1, 1
};

// Matrices for Views, Projections, mViewProjection.
MATRIX mView, mProjection, mViewProjection, mMVP, mModel;
MATRIX mOrthoProj;
MATRIX mIdentity, mScale, mRotate, m_OrbitTranslate, m_OrbitRotate, m_Translate;
Vec3 vTo, vFrom;


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
	texture->LoadTextureFromImageFile( "/crosshair.png", &texID_HUD );
	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000
	if( __OPENGLES_VERSION >= 2 )
	{
		/* Gets the Data Path */	
		if(ShaderLoadFromFile("blank", "/VertexColorFragShader.fsh", GL_FRAGMENT_SHADER, 0, &uiFragShader) == 0)
			printf("Loading the fragment shader fails");
		if(ShaderLoadFromFile("blank", "/VertexColorVertShader.vsh", GL_VERTEX_SHADER, 0, &uiVertShader) == 0)
			printf("Loading the vertex shader fails");
		
		CreateProgram(&uiProgramObject, uiVertShader, uiFragShader, pszAttribs, 2);
		
		// First gets the location of that variable in the shader using its name
		PMVMatrixHandle = glGetUniformLocation(uiProgramObject, "myPMVMatrix");
		ColHandle = glGetUniformLocation(uiProgramObject, "Col");		
		
		/* Gets the Data Path */	
		if(ShaderLoadFromFile("blank", "/TextureFragShader.fsh", GL_FRAGMENT_SHADER, 0, &uiFragShader2) == 0)
			printf("Loading the fragment shader fails");
		if(ShaderLoadFromFile("blank", "/TextureVertShader.vsh", GL_VERTEX_SHADER, 0, &uiVertShader2) == 0)
			printf("Loading the vertex shader fails");
		
		CreateProgram(&uiProgramObject2, uiVertShader2, uiFragShader2, pszAttribs2, 2);
		
		// First gets the location of that variable in the shader using its name
		PMVMatrixHandle2 = glGetUniformLocation(uiProgramObject2, "myPMVMatrix");
		ColHandle2 = glGetUniformLocation(uiProgramObject2, "Col");		
		TextureHandle2 = glGetUniformLocation(uiProgramObject2, "s_texture");
	}
#endif
	
	// We're going to draw a cube, so let's create a vertex buffer first!
	{
		GLfloat verts[] =
		{
			0.0f, 1.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f,
		};
		
		// Generate the vertex buffer object (VBO)
		glGenBuffers(1, &ui32Vbo);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		
		// Set the buffer's data
		// Calculate verts size: (3 vertices * stride (3 GLfloats per each vertex))
		unsigned int uiSize = 3 * (sizeof(GLfloat) * 3);
		glBufferData(GL_ARRAY_BUFFER, uiSize, verts, GL_STATIC_DRAW);
		

	}
	
	{
		// Create Quad 128 x 128 for HUD (Head Up Display)
		GLfloat verts[] = 
		{
			 64.0f, 64.0f, 0.0f,	1.0f, 0.0f,
			-64.0f, 64.0f, 0.0f,	0.0f, 0.0f,
			 64.0f,-64.0f, 0.0f,	1.0f, 1.0f,
			-64.0f,-64.0f, 0.0f,	0.0f, 1.0f
		};
		
		glGenBuffers(1, &ui32HUD);
		glBindBuffer(GL_ARRAY_BUFFER, ui32HUD);
		
		unsigned int uiSize = 4 * (sizeof(GLfloat) * 5);
		glBufferData(GL_ARRAY_BUFFER, uiSize, verts, GL_STATIC_DRAW);
	}
	

	
	
	
	vTo = Vec3(0.0f, 0.5f, 0.0f);
	vFrom = Vec3(0.0f, 0.5f, 3.0f);
	MatrixLookAtRH(mView, vFrom, vTo, Vec3(0,1,0));
	
	MatrixPerspectiveFovRH(mProjection, f2vt(45*PIf/180.0f), f2vt(((float) 320 / (float) 480)), f2vt(0.1f), f2vt(1000.0f), 1);
	MatrixOrthoRH(mOrthoProj, 320, 480, -1, 1, 1);

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
	
	m_fAngleY += 0.02f;

	// Calculate view projection matrix.
	MatrixMultiply(mViewProjection, mView, mProjection);
	
	++frames;
	CFTimeInterval			TimeInterval;
	frameRate = GetFps(frames, TimeInterval);

	AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f Cube: %3.2fms UI: %3.2fms", frameRate, DrawCubeT, DrawUIT);
	
	return true;
}

void DrawHUD()
{	

	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 ) 
	{
		glUseProgram(uiProgramObject2);
		
		glUniformMatrix4fv( PMVMatrixHandle2, 1, GL_FALSE, mOrthoProj.f);
		
		glBindBuffer(GL_ARRAY_BUFFER, ui32HUD);
		glEnableVertexAttribArray(VERTEX_ARRAY);
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 20, 0);	// Stride = 20 bytes
		
		glEnableVertexAttribArray(TEXCOORD_ARRAY);
		glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glActiveTexture( GL_TEXTURE0 );
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID_HUD);
		glUniform1i( TextureHandle2, 0 );
		
		glUniform4f(ColHandle2, 1.0, 1.0, 1.0, 1.0);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLE_STRIP,  0, 4);
	}
	else
#endif
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(mOrthoProj.f);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glActiveTexture( GL_TEXTURE0 );
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID_HUD);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32HUD);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 20, 0);	// Stride = 20 bytes
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		glTexCoordPointer(2, GL_FLOAT, 20, (void *)12);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glDisableClientState(GL_COLOR_ARRAY);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		glDrawArrays(GL_TRIANGLE_STRIP,  0, 4);
	}	
	glActiveTexture( GL_TEXTURE0 );
	glDisable(GL_TEXTURE_2D);
}

void DrawTriangle()
{	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 ) 
	{
		glUseProgram(uiProgramObject);
		
		MatrixMultiply(mMVP, mModel, mViewProjection);
		glUniformMatrix4fv( PMVMatrixHandle, 1, GL_FALSE, mMVP.f);
		
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		glEnableVertexAttribArray(VERTEX_ARRAY);
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glEnableVertexAttribArray(COLOR_ARRAY);
		glVertexAttribPointer(COLOR_ARRAY, 4, GL_FLOAT, GL_FALSE, 0, colors);
		
		glDrawArrays(GL_TRIANGLES,  0, 3);
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
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, colors);
		
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}	
}

bool CShell::RenderScene()
{
	/*
	 Just drew a non-indexed triangle array from the pointers previously given.
	 This function allows the use of other primitive types : triangle strips, lines, ...
	 For indexed geometry, use the function glDrawElements() with an index list.
	 */
	
	MatrixIdentity( mModel );
	MatrixRotationY( mModel, m_fAngleY );
	DrawTriangle();

	MatrixIdentity( mIdentity );
	MatrixScaling( mScale, 0.5, 0.5, 0.5 );
	MatrixRotationY( mRotate, m_fAngleY * 16 );
	MatrixTranslation( m_OrbitTranslate, -1.0, 0, 0 );
	MatrixRotationY( m_OrbitRotate, m_fAngleY );
	MatrixTranslation( m_Translate, 0, 0.25, 0 );
	MatrixMultiply( mModel, mScale, mRotate );
	MatrixMultiply( mModel, mModel, m_OrbitTranslate );
	MatrixMultiply( mModel, mModel, m_OrbitRotate );
	MatrixMultiply( mModel, mModel, m_Translate );
	DrawTriangle();
	
	MatrixIdentity( mIdentity );
	MatrixTranslation( m_OrbitTranslate, 1.0, 0, 0 );
	MatrixMultiply( mModel, mScale, mRotate );
	MatrixMultiply( mModel, mModel, m_OrbitTranslate );
	MatrixMultiply( mModel, mModel, m_OrbitRotate );
	MatrixMultiply( mModel, mModel, m_Translate );
	DrawTriangle();
	 
	
	DrawHUD();
	
	DrawCubeT = GetAverageTimeValueInMS(&DrawCubeTimer);
	
	ResetTimer(&DrawUITimer);
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("HUD", "", eDisplayTextLogoIMG);
	AppDisplayText->Flush();	
	
	DrawUIT = GetAverageTimeValueInMS(&DrawUITimer);

	return true;
}
