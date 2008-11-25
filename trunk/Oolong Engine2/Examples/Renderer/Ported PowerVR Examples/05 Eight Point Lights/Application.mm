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

/* Textures */
#include "Media/LightTex.h"
#include "Media/GRANITE.h"

/* Geometry */
#include "Media/SimpleGeoTest.H"


CDisplayText * AppDisplayText;
CTexture * Texture;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

// Structure Definitions

//
// Defines the format of a header-object as exported by the MAX
// plugin.
//
typedef struct {
	unsigned int      nNumVertex;
    unsigned int      nNumFaces;
    unsigned int      nNumStrips;
    unsigned int      nFlags;
    unsigned int      nMaterial;
    float             fCenter[3];
    float             *pVertex;
    float             *pUV;
    float             *pNormals;
    float             *pPackedVertex;
    unsigned int      *pVertexColor;
    unsigned int      *pVertexMaterial;
    unsigned short    *pFaces;
    unsigned short    *pStrips;
    unsigned short    *pStripLength;
    struct
    {
        unsigned int  nType;
        unsigned int  nNumPatches;
        unsigned int  nNumVertices;
        unsigned int  nNumSubdivisions;
        float         *pControlPoints;
        float         *pUVs;
    } Patch;
}   HeaderStruct_Mesh;


typedef HeaderStruct_Mesh HeaderStruct_Mesh_Type;

//
// Converts the data exported by MAX to fixed point when used in OpenGL ES common-lit profile == fixed-point
// Expects a pointer to the object structure in the header file
// returns a directly usable geometry in fixed or float format
// 
HeaderStruct_Mesh_Type* LoadHeaderObject(const void *headerObj);

//
// Releases memory allocated by LoadHeaderObject when the geometry is no longer needed
// expects a pointer returned by LoadHeaderObject
// 
void UnloadHeaderObject(HeaderStruct_Mesh_Type* headerObj);


int frames;
float frameRate;

/****************************************************************************
 ** Defines
 ****************************************************************************/
#define NUM_LIGHTS_IN_USE	(8)

/****************************************************************************
 ** Structures
 ****************************************************************************/
struct SLightVars
{
	VECTOR4		position;	// GL_LIGHT_POSITION
	VECTOR4		direction;	// GL_SPOT_DIRECTION
	VECTOR4		ambient;	// GL_AMBIENT
	VECTOR4		diffuse;	// GL_DIFFUSE
	VECTOR4		specular;	// GL_SPECULAR
	
	VECTOR3		vRotationStep;
	VECTOR3		vRotation;
	VECTOR3		vRotationCentre;
	VECTOR3		vPosition;
};


/* Mesh data */
HeaderStruct_Mesh_Type* Meshes[NUM_MESHES];

/* Texture names */
GLuint texNameObject;
GLuint texNameLight;

/* Light properties */
SLightVars m_psLightData[8];
#define WIDTH 320
#define HEIGHT 480

/* Number of frames */
long m_nNumFrames;




/****************************************************************************
 ** Function Definitions
 ****************************************************************************/
void RenderPrimitive(VERTTYPE *pVertex, VERTTYPE *pNormals, VERTTYPE *pUVs, int nNumIndex, unsigned short *pIDX, GLenum mode);
void initLight(SLightVars *pLight);
void stepLight(SLightVars *pLight);
void renderLight(SLightVars *pLight);

//
// Converts the data exported by MAX to fixed point when used in OpenGL ES common-lit profile == fixed-point
// Expects a pointer to the object structure in the header file
// returns a directly usable geometry in fixed or float format
// 
HeaderStruct_Mesh_Type *LoadHeaderObject(const void *headerObj)
{
	HeaderStruct_Mesh_Type *new_mesh = new HeaderStruct_Mesh_Type;
	memcpy (new_mesh,headerObj,sizeof(HeaderStruct_Mesh_Type));
	return (HeaderStruct_Mesh_Type*) new_mesh;
}
//
// Releases memory allocated by LoadHeaderObject when the geometry is no longer needed
// expects a pointer returned by LoadHeaderObject
// 
void UnloadHeaderObject(HeaderStruct_Mesh_Type* headerObj)
{
	delete headerObj;
}

bool CShell::InitApplication()
{
//	LOGFUNC("InitApplication()");
	
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
				printf("Display text textures loaded\n");

	/* Load all header objects */
	for(int i = 0; i < NUM_MESHES; i++)
	{
		Meshes[i] = LoadHeaderObject(&Mesh[i]);
	}
	
	MATRIX	MyPerspMatrix;
	int			i;
	
	/* Load textures */
	if (!Texture->LoadTextureFromPointer((void*)GRANITE, &texNameObject))
		return false;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if (!Texture->LoadTextureFromPointer((void*)LightTex, &texNameLight))
		return false;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	/* Setup all materials */
	VERTTYPE objectMatAmb[]		= {f2vt(0.1f), f2vt(0.1f), f2vt(0.1f), f2vt(0.3f)};
	VERTTYPE objectMatDiff[]	= {f2vt(0.5f), f2vt(0.5f), f2vt(0.5f), f2vt(0.3f)};
	VERTTYPE objectMatSpec[]	= {f2vt(1.0f), f2vt(1.0f), f2vt(1.0f), f2vt(0.3f)};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, objectMatAmb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, objectMatDiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, objectMatSpec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, f2vt(10));	// Nice and shiny so we don't get aliasing from the 1/2 angle
	
	/* Initialize all lights */
	srand(0);
	for(i = 0; i < 8; ++i)
	{
		initLight(&m_psLightData[i]);
	}
	
	/* Perspective matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(20.0f*(PIf/180.0f)), f2vt((float)WIDTH / (float)HEIGHT), f2vt(10.0f), f2vt(1200.0f), true);
	glMultMatrixf(MyPerspMatrix.f);
	
	/* Modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(f2vt(0.0f), f2vt(0.0f), f2vt(-500.0f));
	
	/* Setup culling */
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_FRONT);
	
	/* Enable texturing */
	glEnable(GL_TEXTURE_2D);
	

	return true;
}

bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;
	
	for(int i = 0; i < NUM_MESHES; i++)
	{
		UnloadHeaderObject(Meshes[i]);
	}
	
	/* Release textures */
	Texture->ReleaseTexture(texNameObject);
	Texture->ReleaseTexture(texNameLight);

	return true;
}

bool CShell::UpdateScene()
{
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	static CFTimeInterval	startTime = 0;
	CFTimeInterval			TimeInterval;
	
	// calculate our local time
	TimeInterval = CFAbsoluteTimeGetCurrent();
	if(startTime == 0)
		startTime = TimeInterval;
	TimeInterval = TimeInterval - startTime;
	
	frames++;
	if (TimeInterval) 
		frameRate = ((float)frames/(TimeInterval));
	
	AppDisplayText->DisplayText(0, 10, 0.4f, RGBA(255,255,255,255), "fps: %3.2f", frameRate);
	
	return true;
}


bool CShell::RenderScene()
{
	unsigned int i;
	MATRIX		RotationMatrix;
	
	/* Set up viewport */
	glViewport(0, 0, WIDTH, HEIGHT);
	
	/* Clear the buffers */
	glEnable(GL_DEPTH_TEST);
	glClearColor(f2vt(0.0f), f2vt(0.0f), f2vt(0.0f), f2vt(0.0f));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* Lighting */
	
	/* Enable lighting (needs to be specified everyframe as Print3D will turn it off */
	glEnable(GL_LIGHTING);
	
	/* Increase number of frames */
	m_nNumFrames++;
	m_nNumFrames = m_nNumFrames % 3600;
	MatrixRotationY(RotationMatrix, f2vt((-m_nNumFrames*0.1f) * PIf/180.0f));
	
	/* Loop through all lights */
	for(i = 0; i < 8; ++i)
	{
		/* Only process lights that we are actually using */
		if (i < NUM_LIGHTS_IN_USE)
		{
			/* Transform light */
			stepLight(&m_psLightData[i]);
			
			/* Set light properties */
			glLightfv(GL_LIGHT0 + i, GL_POSITION, &m_psLightData[i].position.x);
			glLightfv(GL_LIGHT0 + i, GL_AMBIENT, &m_psLightData[i].ambient.x);
			glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, &m_psLightData[i].diffuse.x);
			glLightfv(GL_LIGHT0 + i, GL_SPECULAR, &m_psLightData[i].specular.x);
			
			/* Enable light */
			glEnable(GL_LIGHT0 + i);
		}
		else
		{
			/* Disable remaining lights */
			glDisable(GL_LIGHT0 + i);
		}
	}
	
	/*************
	 * Begin Scene
	 *************/
	
	/* Set texture and texture environment */
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, texNameObject);
	
	/* Render geometry */
	
	/* Save matrix by pushing it on the stack */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	/* Add a small Y rotation to the model */
	glMultMatrixf(RotationMatrix.f);
	
	/* Loop through all meshes */
	for (i = 0; i < NUM_MESHES; i++)
	{
		/* Render mesh */
		RenderPrimitive(Meshes[i]->pVertex, Meshes[i]->pNormals, Meshes[i]->pUV, Meshes[i]->nNumFaces*3,
						Meshes[i]->pFaces, GL_TRIANGLES);
	}
	
	/* Restore matrix */
	glPopMatrix();
	
	// draw lights
	/* No lighting for lights */
	glDisable(GL_LIGHTING);
	
	/* Disable Z writes */
	glDepthMask(GL_FALSE);
	
	/* Set additive blending */
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	
	/* Render all lights in use */
	for(i = 0; i < NUM_LIGHTS_IN_USE; ++i)
	{
		renderLight(&m_psLightData[i]);
	}
	
	/* Disable blending */
	glDisable(GL_BLEND);
	
	/* Restore Z writes */
	glDepthMask(GL_TRUE);
	
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("Eight Point Lights", "", eDisplayTextLogoIMG);
	
	AppDisplayText->Flush();	
	
	return true;
}

/*******************************************************************************
 * Function Name  : initLight
 * Inputs		  : *pLight
 * Description    : Initialize light structure
 *******************************************************************************/
void initLight(SLightVars *pLight)
{
	/* Light ambient colour */
	pLight->ambient.x = f2vt(0.0);
	pLight->ambient.y = f2vt(0.0);
	pLight->ambient.z = f2vt(0.0);
	pLight->ambient.w = f2vt(1.0);
	
	/* Light Diffuse colour */
	double difFac = 0.4;
	pLight->diffuse.x = f2vt((float)( difFac * (rand()/(double)RAND_MAX) ) * 2.0f); //1.0;
	pLight->diffuse.y = f2vt((float)( difFac * (rand()/(double)RAND_MAX) ) * 2.0f); //1.0;
	pLight->diffuse.z = f2vt((float)( difFac * (rand()/(double)RAND_MAX) ) * 2.0f); //1.0;
	pLight->diffuse.w = f2vt((float)( 1.0 ));
	
	/* Light Specular colour */
	double specFac = 0.1;
	pLight->specular.x = f2vt((float)( specFac * (rand()/(double)RAND_MAX) ) * 2.0f); //1.0;
	pLight->specular.y = f2vt((float)( specFac * (rand()/(double)RAND_MAX) ) * 2.0f); //1.0;
	pLight->specular.z = f2vt((float)( specFac * (rand()/(double)RAND_MAX) ) * 2.0f); //1.0;
	pLight->specular.w = f2vt((float)( 1.0 ));
	
	/* Randomize some of the other parameters */
	float lightDist = 80.0f;
	pLight->vPosition.x = f2vt((float)((rand()/(double)RAND_MAX) * lightDist/2.0f ) + lightDist/2.0f);
	pLight->vPosition.y = f2vt((float)((rand()/(double)RAND_MAX) * lightDist/2.0f ) + lightDist/2.0f);
	pLight->vPosition.z = f2vt((float)((rand()/(double)RAND_MAX) * lightDist/2.0f ) + lightDist/2.0f);
	
	float rStep = 2;
	pLight->vRotationStep.x = f2vt((float)( rStep/2.0 - (rand()/(double)RAND_MAX) * rStep ));
	pLight->vRotationStep.y = f2vt((float)( rStep/2.0 - (rand()/(double)RAND_MAX) * rStep ));
	pLight->vRotationStep.z = f2vt((float)( rStep/2.0 - (rand()/(double)RAND_MAX) * rStep ));
	
	pLight->vRotation.x = f2vt(0.0f);
	pLight->vRotation.y = f2vt(0.0f);
	pLight->vRotation.z = f2vt(0.0f);
	
	pLight->vRotationCentre.x = f2vt(0.0f);
	pLight->vRotationCentre.y = f2vt(0.0f);
	pLight->vRotationCentre.z = f2vt(0.0f);
}

/*******************************************************************************
 * Function Name  : stepLight
 * Inputs		  : *pLight
 * Description    : Advance one step in the light rotation.
 *******************************************************************************/
void stepLight(SLightVars *pLight)
{
	MATRIX RotationMatrix, RotationMatrixX, RotationMatrixY, RotationMatrixZ;
	
	/* Increase rotation angles */
	pLight->vRotation.x += pLight->vRotationStep.x;
	pLight->vRotation.y += pLight->vRotationStep.y;
	pLight->vRotation.z += pLight->vRotationStep.z;
	
	while(pLight->vRotation.x > f2vt(360.0f)) pLight->vRotation.x -= f2vt(360.0f);
	while(pLight->vRotation.y > f2vt(360.0f)) pLight->vRotation.y -= f2vt(360.0f);
	while(pLight->vRotation.z > f2vt(360.0f)) pLight->vRotation.z -= f2vt(360.0f);
	
	/* Create three rotations from rotation angles */
	MatrixRotationX(RotationMatrixX, VERTTYPEMUL(pLight->vRotation.x, f2vt(PIf/180.0f)));
	MatrixRotationY(RotationMatrixY, VERTTYPEMUL(pLight->vRotation.y, f2vt(PIf/180.0f)));
	MatrixRotationZ(RotationMatrixZ, VERTTYPEMUL(pLight->vRotation.z, f2vt(PIf/180.0f)));
	
	/* Build transformation matrix by concatenating all rotations */
	MatrixMultiply(RotationMatrix, RotationMatrixY, RotationMatrixZ);
	MatrixMultiply(RotationMatrix, RotationMatrixX, RotationMatrix);
	
	/* Transform light with transformation matrix, setting w to 1 to indicate point light */
	TransTransformArray((VECTOR3*)&pLight->position, &pLight->vPosition, 1, &RotationMatrix);
	pLight->position.w = f2vt(1.0f);
}

/*******************************************************************************
 * Function Name  : renderLight
 * Inputs		  : *pLight
 * Description    : Draw every light as a quad.
 *******************************************************************************/
void renderLight(SLightVars *pLight)
{
	VERTTYPE	quad_verts[4*5];
	VERTTYPE	quad_uvs[2*5];
	VERTTYPE	fLightSize = f2vt(5.0f);
	
	/* Set quad vertices */
	quad_verts[0]  = pLight->position.x - fLightSize;
	quad_verts[1]  = pLight->position.y - fLightSize;
	quad_verts[2]  = pLight->position.z;
	
	quad_verts[3]  = pLight->position.x + fLightSize;
	quad_verts[4]  = pLight->position.y - fLightSize;
	quad_verts[5]  = pLight->position.z;
	
	quad_verts[6]  = pLight->position.x - fLightSize;
	quad_verts[7]  = pLight->position.y + fLightSize;
	quad_verts[8]  = pLight->position.z;
	
	quad_verts[9]  = pLight->position.x + fLightSize;
	quad_verts[10] = pLight->position.y + fLightSize;
	quad_verts[11] = pLight->position.z;
	
	/* Set Quad UVs */
	quad_uvs[0]   = f2vt(0);
	quad_uvs[1]   = f2vt(0);
	quad_uvs[2]   = f2vt(1);
	quad_uvs[3]   = f2vt(0);
	quad_uvs[4]   = f2vt(0);
	quad_uvs[5]   = f2vt(1);
	quad_uvs[6]   = f2vt(1);
	quad_uvs[7]   = f2vt(1);
	
	/* Set texture and texture environment */
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, texNameLight);
	
	/* Set vertex data */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, VERTTYPEENUM, 0, quad_verts);
	
	/* Set texture coordinates data */
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, VERTTYPEENUM, 0, quad_uvs);
	
	/* Set light colour 2x overbright for more contrast (will be modulated with texture) */
	glColor4f(VERTTYPEMUL(pLight->diffuse.x,f2vt(2.0f)), VERTTYPEMUL(pLight->diffuse.y,f2vt(2.0f)), VERTTYPEMUL(pLight->diffuse.z,f2vt(2.0f)), f2vt(1));
	
	/* Draw quad */
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	/* Disable client states */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*******************************************************************************
 * Function Name  : RenderPrimitive
 * Inputs		  : pVertex, pNormals, pUVs, nFirst, nStripLength, mode
 * Description    : Code to render an object
 *******************************************************************************/
void RenderPrimitive(VERTTYPE *pVertex, VERTTYPE *pNormals, VERTTYPE *pUVs,
							  int nNumIndex, unsigned short *pIDX, GLenum mode)
{
	/* Set vertex data */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,VERTTYPEENUM,0,pVertex);
	
	/* Set normal data */
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(VERTTYPEENUM,0,pNormals);
	
	/* Set texture coordinates data */
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2,VERTTYPEENUM,0,pUVs);
	
	/* Draw as indexed data */
	glDrawElements(mode, nNumIndex, GL_UNSIGNED_SHORT, pIDX);
	
	/* Disable client states */
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

