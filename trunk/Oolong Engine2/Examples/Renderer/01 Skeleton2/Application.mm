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
#import <OpenGLES/ES2/gl.h>

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

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0

CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

structTimer DrawCubeTimer;
float DrawCubeT;

structTimer DrawUITimer;
float DrawUIT;

// Matrix used for projection model view (PMVMatrix)
float pfIdentity[] =
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

// Declare the fragment and vertex shaders.
GLuint uiFragShader, uiVertShader;		// Used to hold the fragment and vertex shader handles
GLuint uiProgramObject;					// Used to hold the program handle (made out of the two previous shaders)
GLuint ui32Vbo = 0;					    // Vertex buffer object handle

// This ties in with the shader attribute to link to openGL, see pszVertShader.
const char* pszAttribs[] = { "myVertex"};

// Handles for the uniform variables.
int PMVMatrixHandle;
int ColHandle;

// View Angle for animation
float			m_fAngleY;

// Matrices for Views, Projections, mViewProjection.
MATRIX mView, mProjection, mViewProjection, mMVP, mModel, mRotate, mRotateY, mRotateX;
Vec3 vTo, vFrom;

// Fragment and vertex shaders.
/*const char* pszFragShader = "\
uniform mediump vec4 Col;\
void main (void)\
{\
gl_FragColor = Col;\
}";*/

/*const char* pszVertShader = "\
attribute highp vec4	myVertex;\
uniform mediump mat4	myPMVMatrix;\
void main(void)\
{\
gl_Position = myPMVMatrix * myVertex;\
}";*/

bool CShell::InitApplication()
{
	//AppDisplayText = new CDisplayText;  
	
	//if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
	//	printf("Display text textures loaded\n");
	
	StartTimer(&DrawCubeTimer);
	StartTimer(&DrawUITimer);
	
	//ShaderLoadSourceFromMemory(pszFragShader, GL_FRAGMENT_SHADER, &uiFragShader);
	//ShaderLoadSourceFromMemory(pszVertShader, GL_VERTEX_SHADER, &uiVertShader);

	char *buffer = new char[2048];
	GetResourcePathASCII(buffer, 2048);
	
	CPVRTResourceFile::SetReadPath(buffer);
	
	/* Gets the Data Path */
	
	if(ShaderLoadFromFile("blank", "/FragShader.fsh", GL_FRAGMENT_SHADER, 0, &uiFragShader) == 0)
		printf("Loading the fragment shader fails");
	if(ShaderLoadFromFile("blank", "/VertShader.vsh", GL_VERTEX_SHADER, 0, &uiVertShader) == 0)
		printf("Loading the vertex shader fails");
	
	//printf(filename);

	delete buffer;
	
	CreateProgram(&uiProgramObject, uiVertShader, uiFragShader, pszAttribs, 1);

	// We're going to draw a cube, so let's create a vertex buffer first!
	{
		GLfloat verts[] =
		{
			1.0f, 1.0f,-1.0f,	
			-1.0f, 1.0f,-1.0f,	
			-1.0f, 1.0f, 1.0f,	
			1.0f, 1.0f, 1.0f,	
			
			1.0f,-1.0f, 1.0f,	
			-1.0f,-1.0f, 1.0f,	
			-1.0f,-1.0f,-1.0f,	
			1.0f,-1.0f,-1.0f,	
			
			1.0f, 1.0f, 1.0f,	
			-1.0f, 1.0f, 1.0f,	
			-1.0f,-1.0f, 1.0f,	
			1.0f,-1.0f, 1.0f,	
			
			1.0f,-1.0f,-1.0f,	
			-1.0f,-1.0f,-1.0f,	
			-1.0f, 1.0f,-1.0f,	
			1.0f, 1.0f,-1.0f,	
			
			1.0f, 1.0f,-1.0f,	
			1.0f, 1.0f, 1.0f,	
			1.0f,-1.0f, 1.0f,	
			1.0f,-1.0f,-1.0f,
			
			-1.0f, 1.0f, 1.0f,	
			-1.0f, 1.0f,-1.0f,	
			-1.0f,-1.0f,-1.0f,	
			-1.0f,-1.0f, 1.0f
		};
		
		// Generate the vertex buffer object (VBO)
		glGenBuffers(1, &ui32Vbo);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		
		// Set the buffer's data
		// Calculate verts size: (3 vertices * stride (3 GLfloats per each vertex))
		unsigned int uiSize = 24 * (sizeof(GLfloat) * 3);
		glBufferData(GL_ARRAY_BUFFER, uiSize, verts, GL_STATIC_DRAW);
	}
	
	// First gets the location of that variable in the shader using its name
	PMVMatrixHandle = glGetUniformLocation(uiProgramObject, "myPMVMatrix");
	ColHandle = glGetUniformLocation(uiProgramObject, "Col");
	
	vTo = Vec3(0.0f, 0.0f, 150.0f);
	vFrom = Vec3(0.0f, 0.0f, 0.0f);
	MatrixLookAtRH(mView, vFrom, vTo, Vec3(0,1,0));
	
	MatrixPerspectiveFovRH(mProjection, f2vt(70), f2vt(((float) 320 / (float) 480)), f2vt(0.1f), f2vt(1000.0f), 1);

	m_fAngleY = 0.0;	
	
	return true;
}

bool CShell::QuitApplication()
{
	// Frees the OpenGL handles for the program and the 2 shaders
	glDeleteProgram(uiProgramObject);
	glDeleteShader(uiFragShader);
	glDeleteShader(uiVertShader);
	
	// Delete the VBO as it is no longer needed
	glDeleteBuffers(1, &ui32Vbo);
	
	//AppDisplayText->ReleaseTextures();
	
	//delete AppDisplayText;

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

	// Then passes the matrix to that variable
	glUniformMatrix4fv( PMVMatrixHandle, 1, GL_FALSE, mMVP.f);

	/*
		Enable the custom vertex attribute at index VERTEX_ARRAY.
		We previously just bound that index to the variable in our shader "vec4 MyVertex;"
	*/
	glEnableVertexAttribArray(VERTEX_ARRAY);

	// Sets the vertex data to this attribute index
	glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 0, 0);
	
	++frames;
	CFTimeInterval			TimeInterval;
	frameRate = GetFps(frames, TimeInterval);

	//AppDisplayText->DisplayText(0, 6, 1.0f, RGBA(255,255,255,255), "fps: %3.2f Cube: %3.2fms UI: %3.2fms", frameRate, DrawCubeT, DrawUIT);
	
	return true;
}

bool CShell::RenderScene()
{
	/*
	 Just drew a non-indexed triangle array from the pointers previously given.
	 This function allows the use of other primitive types : triangle strips, lines, ...
	 For indexed geometry, use the function glDrawElements() with an index list.
	 */
	// First gets the location of the color variable in the shader using its name
	// Then passes the proper color to that variable.
	glUniform4f(ColHandle, 1.0, 0.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLE_FAN,  0, 4);
	glUniform4f(ColHandle, 0.0, 1.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLE_FAN,  4, 4);
	glUniform4f(ColHandle, 0.0, 0.0, 1.0, 1.0);
	glDrawArrays(GL_TRIANGLE_FAN,  8, 4);
	glUniform4f(ColHandle, 1.0, 0.0, 1.0, 1.0);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glUniform4f(ColHandle, 1.0, 1.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
	glUniform4f(ColHandle, 0.0, 1.0, 1.0, 1.0);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	DrawCubeT = GetAverageTimeValueInMS(&DrawCubeTimer);
	
	ResetTimer(&DrawUITimer);
	
	// show text on the display
	//AppDisplayText->DisplayDefaultTitle("Basic Skeleton", "", eDisplayTextLogoIMG);
	
	//AppDisplayText->Flush();	
	
	DrawUIT = GetAverageTimeValueInMS(&DrawUITimer);

	return true;
}
