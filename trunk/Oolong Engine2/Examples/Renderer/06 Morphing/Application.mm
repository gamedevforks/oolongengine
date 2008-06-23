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

#include <stdio.h>
#include <sys/time.h>

// 3D Model: Skull Geometry Data
#include "Media/OGLESEvilSkull_low.H"

// Textures
#include "Media/Iris.h"		// Eyes
#include "Media/Metal.h"		// Skull

#include "Media/Fire02.h"		// Background
#include "Media/Fire03.h"		// Background


CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;
CTexture * Textures;

int frames;
float frameRate;

/****************************************************************************
 ** DEFINES                                                                **
 ****************************************************************************/
#ifndef PI
#define PI 3.14159f
#endif

// Geometry Software Processing Defines
#define NMBR_OF_VERTICES	skull5_NumVertex
#define	NMBR_OF_MORPHTRGTS	4

// Animation Define
#define EXPR_TIME			75.0f

#define NUM_TEXTURES		4

#define WIDTH 320
#define HEIGHT 480

/****************************************************************************
 ** STRUCTURES                                                             **
 ****************************************************************************/

/* A structure which holds the object's data */
struct MyObject
{
	int				nNumberOfTriangles;		/* Number of Triangles */
	VERTTYPE		*pVertices;				/* Vertex coordinates */
	VERTTYPE		*pNormals;				/* Vertex normals */
	VERTTYPE		*pUV;				    /* UVs coordinates */
	unsigned short	*pTriangleList;			/* Triangle list */
};
typedef MyObject* lpMyObject;

// vLightPosition
VECTOR4	vLightPosition;

VECTOR3 Eye, At, Up;
MATRIX	MyLookMatrix;

/* Objects */
GLuint		pTexture[NUM_TEXTURES];
MyObject	OGLObject[NUM_MESHES];

/* Software processing buffers */

VERTTYPE	MyMorphedVertices[NMBR_OF_VERTICES*3];
float		fMyAVGVertices[NMBR_OF_VERTICES*3];
float		fMyDiffVertices[NMBR_OF_VERTICES*3*4];

/* Animation Params */

float	fSkull_Weight[5];

float fExprTbl[4][7];
float fJawRotation[7];
float fBackRotation[7];

int nBaseAnim,nTgtAnim;

/* Generic */
int nFrame;

/* Header Object to Lite Conversion */
HeaderStruct_Mesh_Type** Meshes;

/****************************************************************************
 ** Function Definitions
 ****************************************************************************/
void RenderSkull (GLuint pTexture);
void RenderJaw (GLuint pTexture);
void CreateObjectFromHeaderFile	(MyObject *pObject, int nObject);
void CalculateMovement (int nType);
void DrawQuad (float x,float y,float z,float Size, GLuint pTexture);
void DrawDualTexQuad (float x,float y,float z,float Size, GLuint pTexture1, GLuint PTexture2);
void doRenderScene();

bool CShell::InitApplication()
{
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
		printf("Display text textures loaded\n");
	
	Textures = new CTexture;

	/* Setup base constants in contructor */
	
	/* Camera and Light details */
	
	vLightPosition.x=f2vt(-1.0f); vLightPosition.y=f2vt(1.0f); vLightPosition.z=f2vt(1.0f); vLightPosition.w=f2vt(0.0f);
	
	Eye.x = f2vt(0.0f);			Eye.y = f2vt(0.0f);			Eye.z = f2vt(300.0f);
	At.x  = f2vt(0.0f);			At.y  = f2vt(-30.0f);		At.z  = f2vt(0.0f);
	Up.x  = f2vt(0.0f);			Up.y  = f2vt(1.0f);			Up.z  = f2vt(0.0f);
	
	/* Animation Table */
	
	fSkull_Weight[0] = 0.0f;
	fSkull_Weight[1] = 1.0f;
	fSkull_Weight[2] = 0.0f;
	fSkull_Weight[3] = 0.0f;
	fSkull_Weight[4] = 0.0f;
	
	fExprTbl[0][0]=1.0f;	fExprTbl[1][0]=1.0f;	fExprTbl[2][0]=1.0f;	fExprTbl[3][0]=1.0f;
	fExprTbl[0][1]=0.0f;	fExprTbl[1][1]=0.0f;	fExprTbl[2][1]=0.0f;	fExprTbl[3][1]=1.0f;
	fExprTbl[0][2]=0.0f;	fExprTbl[1][2]=0.0f;	fExprTbl[2][2]=1.0f;	fExprTbl[3][2]=1.0f;
	fExprTbl[0][3]=1.0f;	fExprTbl[1][3]=0.0f;	fExprTbl[2][3]=1.0f;	fExprTbl[3][3]=0.0f;
	fExprTbl[0][4]=-1.0f;	fExprTbl[1][4]=0.0f;	fExprTbl[2][4]=0.0f;	fExprTbl[3][4]=0.0f;
	fExprTbl[0][5]=0.0f;	fExprTbl[1][5]=0.0f;	fExprTbl[2][5]=-1.0f;	fExprTbl[3][5]=0.0f;
	fExprTbl[0][6]=0.0f;	fExprTbl[1][6]=0.0f;	fExprTbl[2][6]=0.0f;	fExprTbl[3][6]=-1.0f;
	
	fJawRotation[0]=45.0f;
	fJawRotation[1]=25.0f;
	fJawRotation[2]=40.0f;
	fJawRotation[3]=20.0f;
	fJawRotation[4]=45.0f;
	fJawRotation[5]=25.0f;
	fJawRotation[6]=30.0f;
	
	fBackRotation[0]=0.0f;
	fBackRotation[1]=25.0f;
	fBackRotation[2]=40.0f;
	fBackRotation[3]=90.0f;
	fBackRotation[4]=125.0f;
	fBackRotation[5]=80.0f;
	fBackRotation[6]=30.0f;
	
	nBaseAnim = 0;
	nTgtAnim  = 1;
	
	/* Some start values */
	nFrame = 0;
	
	int i;
	
	Meshes = new HeaderStruct_Mesh_Type*[NUM_MESHES];
	for(i = 0; i < NUM_MESHES; i++)
		Meshes[i] = LoadHeaderObject(&Mesh[i]);
	
	/* Initialise Meshes */
	for (i=0; i<NUM_MESHES; i++)
	{
		CreateObjectFromHeaderFile(&OGLObject[i], i);
	}
	
	VERTTYPE fVal[4];
	int j;
	MATRIX		MyPerspMatrix;
	
	/***********************
	 ** LOAD TEXTURES     **
	 ***********************/
	if(!Textures->LoadTextureFromPointer((void*)Iris, &pTexture[0]))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if(!Textures->LoadTextureFromPointer((void*)Metal, &pTexture[1]))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if(!Textures->LoadTextureFromPointer((void*)Fire02, &pTexture[2]))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if(!Textures->LoadTextureFromPointer((void*)Fire03, &pTexture[3]))
		return false;
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	/******************************
	 ** GENERIC RENDER STATES     **
	 *******************************/
	
	// The Type Of Depth Test To Do
	glDepthFunc(GL_LEQUAL);
	
	// Enables Depth Testing
	glEnable(GL_DEPTH_TEST);
	
	// Enables Smooth Color Shading
	glShadeModel(GL_SMOOTH);
	
	// Blending mode
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	/* Create perspective matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	/* Culling */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(70.0f*(3.14f/180.0f)), f2vt((float)WIDTH/(float)HEIGHT), f2vt(10.0f), f2vt(10000.0f), true);
	myglMultMatrix(MyPerspMatrix.f);
	
	/* Create viewing matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	MatrixLookAtRH(MyLookMatrix, Eye, At, Up);
	myglMultMatrix(MyLookMatrix.f);
	
	/* Enable texturing */
	glEnable(GL_TEXTURE_2D);
	
	/* Lights (only one side lighting) */
	glEnable(GL_LIGHTING);
	
	/* Light 0 (White directional light) */
	fVal[0]=f2vt(0.2f); fVal[1]=f2vt(0.2f); fVal[2]=f2vt(0.2f); fVal[3]=f2vt(1.0f);
	myglLightv(GL_LIGHT0, GL_AMBIENT, fVal);
	
	fVal[0]=f2vt(1.0f); fVal[1]=f2vt(1.0f); fVal[2]=f2vt(1.0f); fVal[3]=f2vt(1.0f);
	myglLightv(GL_LIGHT0, GL_DIFFUSE, fVal);
	
	fVal[0]=f2vt(1.0f); fVal[1]=f2vt(1.0f); fVal[2]=f2vt(1.0f); fVal[3]=f2vt(1.0f);
	myglLightv(GL_LIGHT0, GL_SPECULAR, fVal);
	
	myglLightv(GL_LIGHT0, GL_POSITION, &vLightPosition.x);
	
	glEnable(GL_LIGHT0);
	
	glDisable(GL_LIGHTING);
	
	/* Calculate AVG Model for Morphing */
	for (i=0; i<NMBR_OF_VERTICES*3;i++)
	{
		fMyAVGVertices[i]=0;
		
		for (j=0; j<NMBR_OF_MORPHTRGTS;j++)
		{
			fMyAVGVertices[i]+=Mesh[j].pVertex[i]*0.25f; // Use Header Data Directly because it has to stay float
		}
	}
	
	/* Calculate Differences for Morphing */
	for (i=0; i<NMBR_OF_VERTICES*3;i++)
	{
		fMyDiffVertices[i*4+0]=fMyAVGVertices[i]-Mesh[0].pVertex[i];
		fMyDiffVertices[i*4+1]=fMyAVGVertices[i]-Mesh[1].pVertex[i];
		fMyDiffVertices[i*4+2]=fMyAVGVertices[i]-Mesh[2].pVertex[i];
		fMyDiffVertices[i*4+3]=fMyAVGVertices[i]-Mesh[3].pVertex[i];
	}
	
	return true;
}

bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;
	
	int i;
	
	/* Release Geometry */
	/* free allocated memory */
	for(i=0; i<NUM_MESHES; i++)
	{
		UnloadHeaderObject(Meshes[i]);
	}
	delete [] Meshes;
	
	//int i;
	
	/* release all textures */
	for(i = 0; i < NUM_TEXTURES; i++)
	{
		Textures->ReleaseTexture(pTexture[i]);
	}
	delete Textures;
	
	return true;
}

bool CShell::InitView()
{
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	static CFTimeInterval	startTime = 0;
	CFTimeInterval			time;
	
	//Calculate our local time
	time = CFAbsoluteTimeGetCurrent();
	if(startTime == 0)
	startTime = time;
	time = time - startTime;

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
	/* View Port */
	glViewport(0,0, WIDTH, HEIGHT);
	
	/* Actual Render */
	doRenderScene();
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("Morphing", "", eDisplayTextLogoIMG);
	
	AppDisplayText->Flush();	
	
	return true;
}

/*******************************************************************************
 * Function Name  : doRenderScene
 * Returns		  : None
 * Description    : Actual Rendering
 *******************************************************************************/
void doRenderScene()
{
	register int i;
	float fCurrentfJawRotation,fCurrentfBackRotation;
	float fFactor,fInvFactor;
	
	/* Update Skull Weights and Rotations using Animation Info */
	if (nFrame>EXPR_TIME)
	{
		nFrame=0;
		nBaseAnim=nTgtAnim;
		
		nTgtAnim++;
		
		if (nTgtAnim>6)
		{
			nTgtAnim=0;
		}
	}
	
	fFactor=float(nFrame)/EXPR_TIME;
	fInvFactor=1.0f-fFactor;
	
	fSkull_Weight[0] = (fExprTbl[0][nBaseAnim]*fInvFactor)+(fExprTbl[0][nTgtAnim]*fFactor);
	fSkull_Weight[1] = (fExprTbl[1][nBaseAnim]*fInvFactor)+(fExprTbl[1][nTgtAnim]*fFactor);
	fSkull_Weight[2] = (fExprTbl[2][nBaseAnim]*fInvFactor)+(fExprTbl[2][nTgtAnim]*fFactor);
	fSkull_Weight[3] = (fExprTbl[3][nBaseAnim]*fInvFactor)+(fExprTbl[3][nTgtAnim]*fFactor);
	
	fCurrentfJawRotation = fJawRotation[nBaseAnim]*fInvFactor+(fJawRotation[nTgtAnim]*fFactor);
	fCurrentfBackRotation = fBackRotation[nBaseAnim]*fInvFactor+(fBackRotation[nTgtAnim]*fFactor);
	
	/* Update Base Animation Value - FrameBased Animation for now */
	nFrame++;
	
	/* Update Skull Vertex Data using Animation Params */
	for (i=0; i<NMBR_OF_VERTICES*3;i++)
	{
		MyMorphedVertices[i]=f2vt(fMyAVGVertices[i] + (fMyDiffVertices[i*4+0] * fSkull_Weight[0]) \
								  + (fMyDiffVertices[i*4+1] * fSkull_Weight[1]) \
								  + (fMyDiffVertices[i*4+2] * fSkull_Weight[2]) \
								  + (fMyDiffVertices[i*4+3] * fSkull_Weight[3]) );
	}
	
	/* Buffer Clear */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* Render Skull and Jaw Opaque with Lighting */
	glDisable(GL_BLEND);		// Opaque = No Blending
	glEnable(GL_LIGHTING);		// Lighting On
	
	/* Render Animated Jaw - Rotation Only */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	glLoadIdentity();
	
	myglMultMatrix(MyLookMatrix.f);
	
	myglTranslate(f2vt(0),f2vt(-50.0f),f2vt(-50.0f));
	
	myglRotate(f2vt(-fCurrentfJawRotation), f2vt(1.0f), f2vt(0.0f), f2vt(0.0f));
	myglRotate(f2vt(fCurrentfJawRotation) - f2vt(30.0f), f2vt(0), f2vt(1.0f), f2vt(-1.0f));
	
	RenderJaw (pTexture[1]);
	
	glPopMatrix();
	
	/* Render Morphed Skull */
	
	glPushMatrix();
	
	myglRotate(f2vt(fCurrentfJawRotation) - f2vt(30.0f), f2vt(0), f2vt(1.0f), f2vt(-1.0f));
	
	RenderSkull (pTexture[1]);
	
	/* Render Eyes and Background with Alpha Blending and No Lighting*/
	
	glEnable(GL_BLEND);			// Enable Alpha Blending
	glDisable(GL_LIGHTING);		// Disable Lighting
	
	/* Render Eyes using Skull Model Matrix */
	DrawQuad (-30.0f ,0.0f ,50.0f ,20.0f , pTexture[0]);
	DrawQuad ( 33.0f ,0.0f ,50.0f ,20.0f , pTexture[0]);
	glPopMatrix();
	
	/* Render Dual Texture Background with different base color, rotation, and texture rotation */
	
	glPushMatrix();
	
	glDisable(GL_BLEND);			// Disable Alpha Blending
	
	myglColor4(f2vt(0.7f+0.3f*((fSkull_Weight[0]))), f2vt(0.7f), f2vt(0.7f), f2vt(1.0f));	// Animated Base Color
	myglTranslate(f2vt(10.0f), f2vt(-50.0f), f2vt(0.0f));
	myglRotate(f2vt(fCurrentfBackRotation*4.0f),f2vt(0),f2vt(0),f2vt(-1.0f));	// Rotation of Quad
	
	/* Animated Texture Matrix */
	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	myglTranslate(f2vt(-0.5f), f2vt(-0.5f), f2vt(0.0f));
	myglRotate(f2vt(fCurrentfBackRotation*-8.0f), f2vt(0), f2vt(0), f2vt(-1.0f));
	myglTranslate(f2vt(-0.5f), f2vt(-0.5f), f2vt(0.0f));
	
	/* Draw Geometry */
	DrawDualTexQuad (0.0f, 0.0f, -50.0f, 480.0f, pTexture[3], pTexture[2]);
	
	/* Disable Animated Texture Matrix */
	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	/* Reset Color */
	myglColor4(f2vt(1.0f), f2vt(1.0f), f2vt(1.0f), f2vt(1.0f));
}

/*******************************************************************************
 * Function Name  : CreateObjectFromHeaderFile
 * Input/Output	  :
 * Global Used    :
 * Description    : Function to initialise object from a .h file
 *******************************************************************************/

/* This is done so that we can modify the data (which we couldn't do
 if it was actually the static constant array from the header */

void CreateObjectFromHeaderFile (MyObject *pObj, int nObject)
{
	/* Get model info */
	pObj->nNumberOfTriangles	= Meshes[nObject]->nNumFaces;
	
	/* Vertices */
	pObj->pVertices=(VERTTYPE*)Meshes[nObject]->pVertex;
	
	/* Normals */
	pObj->pNormals=(VERTTYPE*)Meshes[nObject]->pNormals;
	
	/* Get triangle list data */
	pObj->pTriangleList=(unsigned short *)Meshes[nObject]->pFaces;
	
	/* UVs */
	pObj->pUV = Meshes[nObject]->pUV;
	
}

/*******************************************************************************
 * Function Name  : RenderSkull
 * Input		  : Texture Pntr and Filter Mode
 * Returns        :
 * Global Used    :
 * Description    : Renders the Skull data using the Morphed Data Set.
 *******************************************************************************/
void RenderSkull (GLuint pTexture)
{
	/* Enable texturing */
	glBindTexture(GL_TEXTURE_2D, pTexture);
	
	/* Enable and set vertices, normals and index data */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer		(3, VERTTYPEENUM, 0, MyMorphedVertices);
	
	if(OGLObject[1].pNormals)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer	(VERTTYPEENUM, 0, OGLObject[1].pNormals);
	}
	
	if(OGLObject[1].pUV)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer	(2, VERTTYPEENUM, 0, OGLObject[1].pUV);
	}
	
	/* Draw mesh */
	glDrawElements(GL_TRIANGLES, OGLObject[1].nNumberOfTriangles*3, GL_UNSIGNED_SHORT, OGLObject[2].pTriangleList);
	
	/* Make sure to disable the arrays */
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	
}

/*******************************************************************************
 * Function Name  : RenderJaw
 * Input		  : Texture Pntr and Filter Mode
 * Returns        :
 * Global Used    :
 * Description    : Renders the Skull Jaw - uses direct data no morphing
 *******************************************************************************/
void RenderJaw (GLuint pTexture)
{
	/* Bind correct texture */
	glBindTexture(GL_TEXTURE_2D, pTexture);
	
	/* Enable and set vertices, normals and index data */
	if(OGLObject[M_JAW].pVertices)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer		(3, VERTTYPEENUM, 0, OGLObject[4].pVertices);
	}
	
	if(OGLObject[M_JAW].pNormals)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer	(VERTTYPEENUM, 0, OGLObject[4].pNormals);
	}
	
	if(OGLObject[M_JAW].pUV)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer	(2, VERTTYPEENUM, 0, OGLObject[4].pUV);
	}
	
	/* Draw mesh */
	glDrawElements(GL_TRIANGLES, OGLObject[4].nNumberOfTriangles*3, GL_UNSIGNED_SHORT, OGLObject[4].pTriangleList);
	
	/* Make sure to disable the arrays */
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	
}

/*******************************************************************************
 * Function Name  : DrawQuad
 * Input		  : Size, (x,y,z) and texture pntr
 * Returns        :
 * Global Used    :
 * Description    : Basic Draw Quad with Size in Location X, Y, Z.
 *******************************************************************************/
void DrawQuad (float x,float y,float z,float Size, GLuint pTexture)
{
	/* Bind correct texture */
	glBindTexture(GL_TEXTURE_2D, pTexture);
	
	/* Vertex Data */
	VERTTYPE verts[] =		{	f2vt(x+Size), f2vt(y-Size), f2vt(z),
		f2vt(x+Size), f2vt(y+Size), f2vt(z),
		f2vt(x-Size), f2vt(y-Size), f2vt(z),
		f2vt(x-Size), f2vt(y+Size), f2vt(z)
	};
	
	VERTTYPE texcoords[] =	{	f2vt(0.0f), f2vt(1.0f),
		f2vt(0.0f), f2vt(0.0f),
		f2vt(1.0f), f2vt(1.0f),
		f2vt(1.0f), f2vt(0.0f)
	};
	
	/* Set Arrays - Only need Vertex Array and Tex Coord Array*/
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,VERTTYPEENUM,0,verts);
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2,VERTTYPEENUM,0,texcoords);
	
	/* Draw Strip */
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	
	/* Disable Arrays */
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

/*******************************************************************************
 * Function Name  : DrawDualTexQuad
 * Input		  : Size, (x,y,z) and texture pntr
 * Returns        :
 * Global Used    :
 * Description    : Basic Draw Dual Textured Quad with Size in Location X, Y, Z.
 *******************************************************************************/
void DrawDualTexQuad (float x,float y,float z,float Size, GLuint pTexture1, GLuint pTexture2)
{
	/* Set Texture and Texture Options */
	glBindTexture(GL_TEXTURE_2D, pTexture1);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pTexture2);
	glEnable(GL_TEXTURE_2D);
	
	/* Vertex Data */
	VERTTYPE verts[] =		{	f2vt(x+Size), f2vt(y-Size), f2vt(z),
		f2vt(x+Size), f2vt(y+Size), f2vt(z),
		f2vt(x-Size), f2vt(y-Size), f2vt(z),
		f2vt(x-Size), f2vt(y+Size), f2vt(z)
	};
	
	VERTTYPE texcoords[] =	{	f2vt(0.0f), f2vt(1.0f),
		f2vt(0.0f), f2vt(0.0f),
		f2vt(1.0f), f2vt(1.0f),
		f2vt(1.0f), f2vt(0.0f)
	};
	
	/* Set Arrays - Only need Vertex Array and Tex Coord Arrays*/
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,VERTTYPEENUM,0,verts);
	
    glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2,VERTTYPEENUM,0,texcoords);
	
	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2,VERTTYPEENUM,0,texcoords);
	
	/* Draw Strip */
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	
	/* Disable Arrays */
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
	glActiveTexture(GL_TEXTURE0);
	
}

