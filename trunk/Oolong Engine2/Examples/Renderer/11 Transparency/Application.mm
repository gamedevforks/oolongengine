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

/* Textures */
#include "Media/BACK.h"
#include "Media/Flora.h"
#include "Media/REFLECT.h"

/* Geometry */
#include "Media/Vase.h"

#define WIDTH 320
#define HEIGHT 480


CDisplayText * AppDisplayText;
CTexture * Textures;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

/* Mesh pointers */
HeaderStruct_Mesh_Type *glassMesh;
HeaderStruct_Mesh_Type *silverMesh;

/* Texture names */
GLuint					backTexName;
GLuint					floraTexName;
GLuint					reflectTexName;

/* Rotation variables */
VERTTYPE				fXAng, fYAng;

/****************************************************************************
 ** Function Definitions
 ****************************************************************************/
void RenderObject(HeaderStruct_Mesh_Type *mesh);
void RenderReflectiveObject(HeaderStruct_Mesh_Type *mesh, MATRIX *pNormalTx);
void RenderBackground();

bool CShell::InitApplication()
{
//	LOGFUNC("InitApplication()");
	
	AppDisplayText = new CDisplayText;  
	Textures = new CTexture;
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
//		LOG("Display text textures loaded", Logger::LOG_DATA);
				printf("Display text textures loaded\n");

	/* Init values to defaults */
	fXAng = 0;
	fYAng = 0;
	
	/* Allocate header data */
	glassMesh	= LoadHeaderObject(&Mesh[M_GLASS]);
	silverMesh	= LoadHeaderObject(&Mesh[M_SILVER]);
	
	//bool			err;
	//SPVRTContext	Context;
	MATRIX		MyPerspMatrix;
	
	
	/* Load textures */
	if (!Textures->LoadTextureFromPointer((void*)BACK,&backTexName))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if (!Textures->LoadTextureFromPointer((void*)Flora,&floraTexName))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if (!Textures->LoadTextureFromPointer((void*)REFLECT,&reflectTexName))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	/* Calculate projection matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(35.0f*(3.14f/180.0f)), f2vt((float)WIDTH/(float)HEIGHT), f2vt(10.0f), f2vt(1200.0f), true);
	myglMultMatrix(MyPerspMatrix.f);
	
	/* Enable texturing */
	glEnable(GL_TEXTURE_2D);
	
	return true;
}

bool CShell::QuitApplication()
{
    /* Release header data */
	UnloadHeaderObject(glassMesh);
	UnloadHeaderObject(silverMesh);
	
	/* Release textures */
	Textures->ReleaseTexture(backTexName);
	Textures->ReleaseTexture(floraTexName);
	Textures->ReleaseTexture(reflectTexName);
	
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;
	delete Textures;

	return true;
}

bool CShell::InitView()
{
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	return true;
};

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
	MATRIX RotationMatrix, TmpX, TmpY;
	
	/* Set up viewport */
	glViewport(0, 0, WIDTH, HEIGHT);
	
	/* Increase rotation angles */
	fXAng += VERTTYPEDIV(PI, f2vt(100.0f));
	fYAng += VERTTYPEDIV(PI, f2vt(150.0f));
	
	if(fXAng >= PI)
		fXAng -= TWOPI;
	
	if(fYAng >= PI)
		fYAng -= TWOPI;
	
	/* Clear the buffers */
	glEnable(GL_DEPTH_TEST);
	myglClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* Draw the scene */
	
	/***************
	 ** Background **
	 ***************/
	/* No need for Z test */
	glDisable(GL_DEPTH_TEST);
	
	/* Set projection matrix to identity (with or without display rotation) for background */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
//	if(PVRShellGet(prefIsRotated) && PVRShellGet(prefFullScreen))
//		myglRotate(f2vt(90), f2vt(0), f2vt(0), f2vt(1));
	
	/* Set modelview matrix to identity for background */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	/* Set background texture */
	glBindTexture(GL_TEXTURE_2D, backTexName);
	
	/* Set texture environment mode: use texture directly */
	myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	/* Render background */
	RenderBackground();
	
	
	/**********************
	 ** Reflective silver **
	 **********************/
	
	/* Calculate rotation matrix */
	MatrixRotationX(TmpX, fXAng);
	MatrixRotationY(TmpY, fYAng);
	MatrixMultiply(RotationMatrix, TmpX, TmpY);
	
	/* Modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	myglTranslate(f2vt(0.0f), f2vt(0.0f), f2vt(-200.0f));
	myglMultMatrix(RotationMatrix.f);
	
	/* Projection matrix */
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	
	/* Enable depth test */
	glEnable(GL_DEPTH_TEST);
	
	/* Render front faces only (model has reverse winding) */
	glCullFace(GL_FRONT);
	
	/* Set texture and texture env mode */
	glBindTexture(GL_TEXTURE_2D,reflectTexName);
	myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	/* Render reflective object */
	RenderReflectiveObject(silverMesh, &RotationMatrix);
	
	
	/*******************
	 ** See-thru glass **
	 *******************/
	
	/* Don't update Z-buffer */
	glDepthMask(GL_FALSE);
	
	/* Enable alpha blending */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	/* Set texture and texture env mode */
	glBindTexture(GL_TEXTURE_2D, floraTexName);
	myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	/* Pass 1: only render back faces (model has reverse winding) */
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	RenderObject(glassMesh);
	
	/* Pass 2: only render front faces (model has reverse winding) */
	glCullFace(GL_FRONT);
	RenderObject(glassMesh);
	
	/* Restore Z-writes */
	glDepthMask(GL_TRUE);
	
	/* Disable alpha blending */
	glDisable(GL_BLEND);
	
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("Vase", "Translucency and reflections", eDisplayTextLogoIMG);
	
	AppDisplayText->Flush();	
	
	return true;
}


/*******************************************************************************
 * Function Name  : RenderReflectiveObject
 * Inputs		  : *mesh, *pNormalTx
 * Description    : Code to render the reflective parts of the object
 *******************************************************************************/
void RenderReflectiveObject(HeaderStruct_Mesh_Type *mesh, MATRIX *pNormalTx)
{
	VERTTYPE		*uv_transformed = new VERTTYPE[2 * mesh->nNumVertex];
	VERTTYPE		EnvMapMatrix[16];
	unsigned int	i;
	
	/* Calculate matrix for environment mapping: simple multiply by 0.5 */
	for (i=0; i<16; i++)
	{
		/* Convert matrix to fixed point */
		EnvMapMatrix[i] = VERTTYPEMUL(pNormalTx->f[i], f2vt(0.5f));
	}
	
	/* Calculate UVs for environment mapping */
	for (i = 0; i < mesh->nNumVertex; i++)
	{
		uv_transformed[2*i] =	VERTTYPEMUL(mesh->pNormals[3*i+0], EnvMapMatrix[0]) +
		VERTTYPEMUL(mesh->pNormals[3*i+1], EnvMapMatrix[4]) +
		VERTTYPEMUL(mesh->pNormals[3*i+2], EnvMapMatrix[8]) +
		f2vt(0.5f);
		
		uv_transformed[2*i+1] =	VERTTYPEMUL(mesh->pNormals[3*i+0], EnvMapMatrix[1]) +
		VERTTYPEMUL(mesh->pNormals[3*i+1], EnvMapMatrix[5]) +
		VERTTYPEMUL(mesh->pNormals[3*i+2], EnvMapMatrix[9]) +
		f2vt(0.5f);
	}
	
	/* Set vertex pointer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, VERTTYPEENUM, 0, mesh->pVertex);
	
	/* Set texcoord pointer */
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, VERTTYPEENUM, 0, uv_transformed);
	
	/* Draw object */
	glDrawElements(GL_TRIANGLES, mesh->nNumFaces*3, GL_UNSIGNED_SHORT, mesh->pFaces);
	
	/* Restore client states */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	/* Delete memory */
	delete[] uv_transformed;
}


/*******************************************************************************
 * Function Name  : RenderObject
 * Inputs		  : *mesh
 * Description    : Code to render an object
 *******************************************************************************/
void RenderObject(HeaderStruct_Mesh_Type *mesh)
{
	/* Set vertex pointer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, VERTTYPEENUM, 0, mesh->pVertex);
	
	/* Set texcoord pointer */
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, VERTTYPEENUM, 0, mesh->pUV);
	
	/* Draw object */
	glDrawElements(GL_TRIANGLES, mesh->nNumFaces*3, GL_UNSIGNED_SHORT, mesh->pFaces);
	
	/* Restore client states */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


/*******************************************************************************
 * Function Name  : RenderBackground
 * Description    : Code to render a quad background
 *******************************************************************************/
void RenderBackground()
{
	VERTTYPE BkndVerts[] = {f2vt(-1),f2vt(-1),f2vt(1),
		f2vt(1),f2vt(-1),f2vt(1),
		f2vt(-1),f2vt(1),f2vt(1),
	f2vt(1),f2vt(1),f2vt(1)};
	VERTTYPE BkndUV[] =	   {f2vt(0.5f/256.0f),		f2vt(0.5f/256.0f),
		f2vt(1-0.5f/256.0f),	f2vt(0.5f/256.0f),
		f2vt(0.5f/256.0f),		f2vt(1-0.5f/256.0f),
	f2vt(1-0.5f/256.0f),	f2vt(1-0.5f/256.0f)};
	
	/* Set vertex pointer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, VERTTYPEENUM, 0, BkndVerts);
	
	/* Set texcoord pointer */
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, VERTTYPEENUM, 0, BkndUV);
	
	/* Draw object */
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	/* Restore client states */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

