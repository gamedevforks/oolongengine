/*
GameKit .blend file reader for Oolong Engine
Copyright (c) 2009 Erwin Coumans http://gamekit.googlecode.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

///
/// 
///

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>

#include "OolongReadBlend.h"
#include "btBulletDynamicsCommon.h"

//#include "Log.h"
#include "App.h"
#include "Mathematics.h"
#include "GraphicsDevice.h"
#include "MemoryManager.h"
#include "UI.h"
#include "Macros.h"
#include "Timing.h"

#include <stdio.h>
#include <sys/time.h>



class OolongBulletBlendReader* blendReader = 0;
class btDiscreteDynamicsWorld* dynamicsWorld = 0;


CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

structTimer DrawCubeTimer;
float DrawCubeT;

structTimer DrawUITimer;
float DrawUIT;

GLuint texID_rock=-1;



///AppleGetBundleDirectory is needed to access the .blend file from the Bundle file
#define MAXPATHLEN 512
char* AppleGetBundleDirectory(void) {
	CFURLRef bundleURL;
	CFStringRef pathStr;
	static char path[MAXPATHLEN];
	memset(path,MAXPATHLEN,0);
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	
	bundleURL = CFBundleCopyBundleURL(mainBundle);
	pathStr = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);
	CFStringGetCString(pathStr, path, MAXPATHLEN, kCFStringEncodingASCII);
	CFRelease(pathStr);
	CFRelease(bundleURL);	
	return path;
}


bool CShell::InitApplication()
{
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
		printf("Display text textures loaded\n");
	
	StartTimer(&DrawCubeTimer);
	StartTimer(&DrawUITimer);

	
	///create dynamicsWorld
	btCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btBroadphaseInterface* pairCache = new btDbvtBroadphase();
	btDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
	btConstraintSolver* constraintSolver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,pairCache,constraintSolver,collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0,0,-10));
	
	//create OolongBulletBlendReader
	blendReader = new OolongBulletBlendReader(dynamicsWorld);
	
	FILE* file = 0;
	
	
	
	char* name="PhysicsAnimationBakingDemo.blend";
	//char* name="Ragdoll.blend";

	char fullName[512];
	
	sprintf(fullName,"%s/%s",AppleGetBundleDirectory(),name);
	file = fopen(fullName,"rb");

	if (!file)
		exit(0);
	
	int fileLen;
	char*memoryBuffer =  0;
	
	{
		long currentpos = ftell(file); /* save current cursor position */
		long newpos;
		int bytesRead = 0;
		
		fseek(file, 0, SEEK_END); /* seek to end */
		newpos = ftell(file); /* find position of end -- this is the length */
		fseek(file, currentpos, SEEK_SET); /* restore previous cursor position */
		
		fileLen = newpos;
		
		memoryBuffer = (char*)malloc(fileLen);
		bytesRead = fread(memoryBuffer,fileLen,1,file);
		
	}
	
	fclose(file);
	
	
	if (file)
	{
		bool verboseDumpAllTypes = false;
		if (blendReader->readFile(memoryBuffer,fileLen, verboseDumpAllTypes))
		{
			blendReader->convertAllObjects(verboseDumpAllTypes);
		} else {
			printf("Error: invalid/unreadable file: %s\n",fullName);
		}

	} else {
		printf("Error: cannot open file: %s\n",fullName);
	}

	
	///some testing to create textures programmatically/from memory buffer
	GLubyte*	image=(GLubyte*)malloc(256*256*4*sizeof(int));
	for(int y=0;y<256;++y)
	{
		const int	t=y>>4;
		GLubyte*	pi=image+y*256*3;
		for(int x=0;x<256;++x)
		{
			const int		s=x>>4;
			const GLubyte	b=180;					
			GLubyte			c=b+((s+t&1)&1)*(255-b);
			pi[0]=pi[1]=pi[2]=c;pi+=3;
		}
	}
	
	texID_rock = -1;
	glGenTextures(1,(GLuint*)&texID_rock);
	glBindTexture(GL_TEXTURE_2D,texID_rock);
	
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,256,256,0,GL_RGB,GL_UNSIGNED_BYTE,image);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D,texID_rock);
	
	glDisable(GL_COLOR_MATERIAL);
	
	return true;
}



bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;

	return true;
}

bool CShell::UpdateScene()
{
	ResetTimer(&DrawCubeTimer);

	glBindTexture(GL_TEXTURE_2D,texID_rock);
	glEnable(GL_TEXTURE_2D);
	
	
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Set the OpenGL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	MATRIX	MyPerspMatrix;
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(70), f2vt((WindowHeight / WindowWidth)), f2vt(0.1f), f2vt(1000.0f), 0);
	glMultMatrixf(MyPerspMatrix.f);

	++frames;

	CFTimeInterval			TimeInterval;

	frameRate = GetFps(frames, TimeInterval);

	AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f Cube: %3.2fms UI: %3.2fms", frameRate, DrawCubeT, DrawUIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(-90.0, 0.0, 0.0, 1.0);
	
	float m[16];
	blendReader->m_cameraTrans.inverse().getOpenGLMatrix(m);
	glMultMatrixf(m);
	
	return true;
}


bool CShell::RenderScene()
{
	
	for (int i=0;i<blendReader->m_graphicsObjects.size();i++)
	{
		blendReader->m_graphicsObjects[i].render();
	}
   
	dynamicsWorld->stepSimulation(1./60.);
	
	DrawCubeT = GetAverageTimeValueInMS(&DrawCubeTimer);
	
	ResetTimer(&DrawUITimer);
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("BlendParse", "", eDisplayTextLogoIMG);
	
	AppDisplayText->Flush();	
	
	DrawUIT = GetAverageTimeValueInMS(&DrawUITimer);

	return true;
}

