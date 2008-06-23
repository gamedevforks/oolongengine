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
/*
 This example uses art assets from the PowerVR SDK. Imagination Technologies / PowerVR allowed us to use those art assets and we are thankful for this. 
 Having art assets that are optimized for the underlying hardware allows us to show off the capabilties of the graphics chip better.
 */
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>

//#include "Log.h"
#include "App.h"
#include "Mathematics.h"
#include "GraphicsDevice.h"
#include "UI.h"
#include "Macros.h"
#include "Geometry.h"
#include "Memory.h"
#include "Macros.h"

#include <stdio.h>
#include <sys/time.h>

/* 3D data */
#include "Media/sphere_float.h"


/* Textures */
#include "Media/Granite.h"

/*************************
 Defines
 *************************/
#ifndef PI
#define PI 3.14159f
#endif

#define WIDTH 320
#define HEIGHT 480



CDisplayText * AppDisplayText;
CTexture * Textures;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

CPODScene	g_sScene;

VERTTYPE		LightPosition[4];
GLuint			texName;
long			nFrame;
int				nClipPlanes;
bool			bClipPlaneSupported;


void DrawSphere();
void RenderPrimitive(VERTTYPE *pVertex, VERTTYPE *pNormals, VERTTYPE *pUVs, int nNumIndex, unsigned short *pIDX, GLenum mode);
void SetupUserClipPlanes();
void DisableClipPlanes();


//#define GL_OES_VERSION_1_1

bool CShell::InitApplication()
{
	AppDisplayText = new CDisplayText;  
	Textures = new CTexture;
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
		printf("Display text textures loaded\n");

#ifdef GL_OES_VERSION_1_1
#define ClipPlane glClipPlanef
#endif
	
	nFrame = 0L;
	
	LightPosition[0]= f2vt(-1.0f);
	LightPosition[1]= f2vt(1.0f);
	LightPosition[2]= f2vt(1.0f);
	LightPosition[3]= f2vt(0.0f);
	
	g_sScene.ReadFromMemory(c_SPHERE_FLOAT_H);
	
	MATRIX	MyPerspMatrix;
	VERTTYPE	fValue[4];
	
	/* Detects if we are using OpenGL ES 1.1 or if the extension exists */

#ifdef GL_OES_VERSION_1_1
	bClipPlaneSupported = true;
#endif
	
	/* Retrieve max number of clip planes */
	if (bClipPlaneSupported)
	{
		glGetIntegerv(GL_MAX_CLIP_PLANES, &nClipPlanes);
	}
	if (nClipPlanes==0) bClipPlaneSupported = false;
	
	//SPVRTContext Context;
	
	/* Load textures */
	if (!Textures->LoadTextureFromPointer((void*)Granite, &texName)) return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	/* Perspective matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(20.0f*(PI/180.0f)), f2vt((float)WIDTH/(float)HEIGHT), f2vt(10.0f), f2vt(1200.0f), true);
	myglMultMatrix(MyPerspMatrix.f);
	
	/* Modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	/* Setup culling */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	// Setup single light
	glEnable(GL_LIGHTING);
	
	/* Light 0 (White directional light) */
	fValue[0]=f2vt(0.4f); fValue[1]=f2vt(0.4f); fValue[2]=f2vt(0.4f); fValue[3]=f2vt(1.0f);
	myglLightv(GL_LIGHT0, GL_AMBIENT, fValue);
	fValue[0]=f2vt(1.0f); fValue[1]=f2vt(1.0f); fValue[2]=f2vt(1.0f); fValue[3]=f2vt(1.0f);
	myglLightv(GL_LIGHT0, GL_DIFFUSE, fValue);
	fValue[0]=f2vt(1.0f); fValue[1]=f2vt(1.0f); fValue[2]=f2vt(1.0f); fValue[3]=f2vt(1.0f);
	myglLightv(GL_LIGHT0, GL_SPECULAR, fValue);
	myglLightv(GL_LIGHT0, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT0);
	VERTTYPE ambient_light[4] = {f2vt(0.8f), f2vt(0.8f), f2vt(0.8f), f2vt(1.0f)};
	myglLightModelv(GL_LIGHT_MODEL_AMBIENT, ambient_light);
	
	/* Setup all materials */
	VERTTYPE objectMatAmb[] = {f2vt(0.1f), f2vt(0.1f), f2vt(0.1f), f2vt(0.3f)};
	VERTTYPE objectMatDiff[] = {f2vt(0.5f), f2vt(0.5f), f2vt(0.5f), f2vt(0.3f)};
	VERTTYPE objectMatSpec[] = {f2vt(1.0f), f2vt(1.0f), f2vt(1.0f), f2vt(0.3f)};
	myglMaterialv(GL_FRONT_AND_BACK, GL_AMBIENT, objectMatAmb);
	myglMaterialv(GL_FRONT_AND_BACK, GL_DIFFUSE, objectMatDiff);
	myglMaterialv(GL_FRONT_AND_BACK, GL_SPECULAR, objectMatSpec);
	myglMaterial(GL_FRONT_AND_BACK, GL_SHININESS, f2vt(10));	// Nice and shiny so we don't get aliasing from the 1/2 angle
	
	return true;
}

bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;
	
	Textures->ReleaseTexture(texName);
	delete Textures;

	
	g_sScene.Destroy();


	return true;
}

bool CShell::InitView()
{
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		AppDisplayText->DisplayText(0, 10, 0.4f, RGBA(255,255,255,255), "fps: %3.2f", frameRate);
		time = currTime;
		frames = 0;
	}

	return true;
}


bool CShell::RenderScene()
{
	// Set Vieweport size
	glViewport(0, 0, WIDTH, HEIGHT);
	
	// Clear the buffers
	glEnable(GL_DEPTH_TEST);
	
	myglClearColor(f2vt(0.0f), f2vt(0.0f), f2vt(0.0f), f2vt(0.0f));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	// Texturing
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texName);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_BLEND);
	myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	// Transformations
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	myglTranslate(f2vt(0.0f), f2vt(0.0f), f2vt(-500.0f));
	myglRotate(f2vt((float)nFrame/5.0f),f2vt(0),f2vt(1),f2vt(0));
	
	// Draw sphere with user clip planes
	SetupUserClipPlanes();
	glDisable(GL_CULL_FACE);
	DrawSphere();
	glDisable(GL_TEXTURE_2D);
	DisableClipPlanes();
	
	/* Increase frame number */
	nFrame++;
	
	/* Display info text */
	if (bClipPlaneSupported)
	{
		AppDisplayText->DisplayDefaultTitle("User Clip Planes", "User defined clip planes", eDisplayTextLogoIMG);
	}
	else
	{
		AppDisplayText->DisplayDefaultTitle("User Clip Planes", "User clip planes are not available", eDisplayTextLogoIMG);
	}
	
	AppDisplayText->Flush();	
	
	return true;
}

	/*!****************************************************************************
	 @Function		DrawSphere
	 @Description	Draw the rotating sphere
	 ******************************************************************************/
	void DrawSphere()
	{
		/* Enable States */
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		/* Set Data Pointers */
		SPODMesh* mesh = g_sScene.pMesh;
		
		// Used to display non interleaved geometry
		glVertexPointer(3, VERTTYPEENUM, mesh->sVertex.nStride, mesh->sVertex.pData);
		glNormalPointer(VERTTYPEENUM, mesh->sNormals.nStride, mesh->sNormals.pData);
		glTexCoordPointer(2, VERTTYPEENUM, mesh->psUVW->nStride, mesh->psUVW->pData);
		
		/* Draw */
		glDrawElements(GL_TRIANGLES, mesh->nNumFaces*3, GL_UNSIGNED_SHORT, mesh->sFaces.pData);
		
		/* Disable States */
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	
	/*!****************************************************************************
	 @Function		RenderPrimitive
	 @Input			pVertex pNormals pUVs nFirst nStripLength mode
	 @Modified		None.
	 @Output		None.
	 @Return		None.
	 @Description	Render the central object
	 ******************************************************************************/
	void RenderPrimitive(VERTTYPE *pVertex, VERTTYPE *pNormals, VERTTYPE *pUVs, int nNumIndex, unsigned short *pIDX, GLenum mode)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,VERTTYPEENUM,0,pVertex);
		
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(VERTTYPEENUM,0,pNormals);
		
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2,VERTTYPEENUM,0,pUVs);
		
		glDrawElements(mode,nNumIndex,GL_UNSIGNED_SHORT,pIDX);
		
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	
	/*!****************************************************************************
	 @Function		SetupUserClipPlanes
	 @Description	Setup the user clip planes
	 ******************************************************************************/
	void SetupUserClipPlanes()
	{
		if (!bClipPlaneSupported) return;
		VERTTYPE ofs = f2vt(((float)sin(-nFrame / 50.0f) * 10));
		
		if (nClipPlanes < 1) return;
		VERTTYPE equation0[] = {f2vt(1), 0, f2vt(-1), f2vt(65)+ofs};
		ClipPlane( GL_CLIP_PLANE0, equation0 );
		glEnable( GL_CLIP_PLANE0 );
		
		if (nClipPlanes < 2) return;
		VERTTYPE equation1[] = {f2vt(-1), 0, f2vt(-1), f2vt(65)+ofs};
		ClipPlane( GL_CLIP_PLANE1, equation1 );
		glEnable( GL_CLIP_PLANE1 );
		
		if (nClipPlanes < 3) return;
		VERTTYPE equation2[] = {f2vt(-1), 0, f2vt(1), f2vt(65)+ofs};
		ClipPlane( GL_CLIP_PLANE2, equation2 );
		glEnable( GL_CLIP_PLANE2 );
		
		if (nClipPlanes < 4) return;
		VERTTYPE equation3[] = {f2vt(1), 0, f2vt(1), f2vt(65)+ofs};
		ClipPlane( GL_CLIP_PLANE3, equation3 );
		glEnable( GL_CLIP_PLANE3 );
		
		if (nClipPlanes < 5) return;
		VERTTYPE equation4[] = {0, f2vt(1), 0, f2vt(40)+ofs};
		ClipPlane( GL_CLIP_PLANE4, equation4 );
		glEnable( GL_CLIP_PLANE4 );
		
		if (nClipPlanes < 6) return;
		VERTTYPE equation5[] = {0, f2vt(-1), 0, f2vt(40)+ofs};
		ClipPlane( GL_CLIP_PLANE5, equation5 );
		glEnable( GL_CLIP_PLANE5 );
	}
	
	/*!****************************************************************************
	 @Function		DisableClipPlanes
	 @Description	Disable all the user clip planes
	 ******************************************************************************/
	void DisableClipPlanes()
	{
		if (!bClipPlaneSupported) return;
		glDisable( GL_CLIP_PLANE0 );
		glDisable( GL_CLIP_PLANE1 );
		glDisable( GL_CLIP_PLANE2 );
		glDisable( GL_CLIP_PLANE3 );
		glDisable( GL_CLIP_PLANE4 );
		glDisable( GL_CLIP_PLANE5 );
	}
	
