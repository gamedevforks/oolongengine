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
#import <OpenGLES/ES1/gl.h>

//#include "Log.h"
#include "App.h"
#include "Mathematics.h"
#include "GraphicsDevice.h"
#include "UI.h"
#include "Macros.h"

#include <stdio.h>
#include <sys/time.h>

CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

bool CShell::InitApplication()
{
//	LOGFUNC("InitApplication()");
	
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
//		LOG("Display text textures loaded", Logger::LOG_DATA);
				printf("Display text textures loaded\n");


	return true;
}

bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;

	return true;
}

bool CShell::InitView()
{
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    glDisable(GL_CULL_FACE);
	
//	UpdatePolarCamera();

	//Set the OpenGL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	MATRIX	MyPerspMatrix;
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(70), f2vt(((float) 320 / (float) 480)), f2vt(0.1f), f2vt(1000.0f), 0);
	myglMultMatrix(MyPerspMatrix.f);

//	glOrthof(-40 / 2, 40 / 2, -60 / 2, 60 / 2, -1, 1);
	
	static CFTimeInterval	startTime = 0;
	CFTimeInterval			time;
	
	//Calculate our local time
	time = CFAbsoluteTimeGetCurrent();
	if(startTime == 0)
	startTime = time;
	time = time - startTime;
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, - 10.0f);
	glRotatef(50.0f * fmod(time, 360.0), 0.0, 1.0, 1.0);

	return true;
}

bool CShell::ReleaseView()
{
	return true;
}

bool CShell::UpdateScene()
{
 	static struct timeval time = {0,0};
	struct timeval currTime = {0,0};
 
 	frames++;
	gettimeofday(&currTime, NULL); // gets the current time passed since the last frame in seconds
	
	if (currTime.tv_usec - time.tv_usec) 
	{
		frameRate = ((float)frames/((currTime.tv_usec - time.tv_usec) / 1000000.0f));
		AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f", frameRate);
		time = currTime;
		frames = 0;
	}

	return true;
}


bool CShell::RenderScene()
{
    const float verts[] =
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

    glEnableClientState(GL_VERTEX_ARRAY);
    
    glColor4f(0, 1, 0, 1);
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glColor4f(1, 0, 1, 1);
    glVertexPointer(3, GL_FLOAT, 0, verts + 12);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glColor4f(0, 0, 1, 1);
    glVertexPointer(3, GL_FLOAT, 0, verts + 24);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glColor4f(1, 1, 0, 1);
    glVertexPointer(3, GL_FLOAT, 0, verts + 36);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glColor4f(1, 0, 0, 1);
    glVertexPointer(3, GL_FLOAT, 0, verts + 48);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glColor4f(0, 1, 1, 1);
    glVertexPointer(3, GL_FLOAT, 0, verts + 60);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("Basic Skeleton", "", eDisplayTextLogoIMG);
	
	AppDisplayText->Flush();	
	
	return true;
}

