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

#include "Media/Sphere_float.h"
#include "Media/SphereOpt_float.h"
#include "Media/model_texture.h"

CDisplayText * AppDisplayText;
CTexture * Textures;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

/*****************************************************************************
 ** DEFINES
 *****************************************************************************/

#define VIEW_DISTANCE		f2vt(35)

// Times in milliseconds
#define TIME_AUTO_SWITCH	(4000000)
#define TIME_FPS_UPDATE		(10)

// Assuming a 320:480 aspect ratio:
#define CAM_ASPECT	f2vt(((float) 320 / (float) 480))
#define CAM_NEAR	f2vt(0.1f)
#define CAM_FAR		f2vt(1000.0f)

CPODScene		m_sModel;		// Model
CPODScene		m_sModelOpt;	// Triangle optimized model

// not working hangs in TriStripList
//#define ENABLE_LOAD_TIME_STRIP

#ifdef ENABLE_LOAD_TIME_STRIP
unsigned short		*m_pNewIdx;		// Optimized model's indices
#endif

// Model texture
GLuint		m_Texture;

// View and Projection Matrices
MATRIX	m_mView, m_mProj;
VERTTYPE	m_fViewAngle;

// Used to switch mode (not optimized / optimized) after a while
unsigned long	m_uiSwitchTimeDiff;
int				m_nPage;

// Time variables
unsigned long	m_uiLastTime, m_uiTimeDiff;

// FPS variables
unsigned long	m_uiFPSTimeDiff, m_uiFPSFrameCnt;
float			m_fFPS;

#ifdef ENABLE_LOAD_TIME_STRIP
// There is some processing to be done once only; this flags marks whether it has been done.
int				m_nInit;
#endif



/****************************************************************************
 ** Constants
 ****************************************************************************/

// Vectors for calculating the view matrix
VECTOR3 c_vOrigin = { 0, 0 ,0 };
VECTOR3	c_vUp = { 0, 1, 0 };

void CameraGetMatrix();
void ComputeViewMatrix();
void DrawModel( int iOptim );
void CalculateAndDisplayFrameRate();
#ifdef ENABLE_LOAD_TIME_STRIP
void StripMesh();
#endif

bool CShell::InitApplication()
{
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
		printf("Display text textures loaded\n");
	
	Textures = new CTexture;

	
	m_sModel.ReadFromMemory(c_SPHERE_FLOAT_H);
	m_sModelOpt.ReadFromMemory(c_SPHEREOPT_FLOAT_H);
	
#ifdef ENABLE_LOAD_TIME_STRIP
	// Create a stripped version of the mesh at load time
	m_nInit = 2;
#endif
	
	// Init values to defaults
	m_nPage = 0;	
	
	
	/******************************
	 ** Create Textures           **
	 *******************************/
	if(!Textures->LoadTextureFromPointer((void*)model_texture, &m_Texture))
	{
		printf("**ERROR** Failed to load texture for Background.\n");
		return false;
	}
	
	/*********************************
	 ** View and Projection Matrices **
	 *********************************/
	
	/* Get Camera info from POD file */
	CameraGetMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// this is were the projection matrix calculated below in MatrixPerspectiveFovRH() in the CameraGetMatrix() call is applied
	myglMultMatrix(m_mProj.f);
	
	/******************************
	 ** GENERIC RENDER STATES     **
	 ******************************/
	
	/* The Type Of Depth Test To Do */
	glDepthFunc(GL_LEQUAL);
	
	/* Enables Depth Testing */
	glEnable(GL_DEPTH_TEST);
	
	/* Enables Smooth Color Shading */
	glShadeModel(GL_SMOOTH);
	
	/* Define front faces */
	glFrontFace(GL_CW);
	
	/* Sets the clear color */
	myglClearColor(f2vt(0.6f), f2vt(0.8f), f2vt(1.0f), f2vt(1.0f));
	
	/* Reset the model view matrix to position the light */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	/* Setup timing variables */
	struct timeval currTime = {0,0};
	gettimeofday(&currTime, NULL);
	m_uiLastTime = currTime.tv_usec;
	
	m_uiFPSFrameCnt = 0;
	m_fFPS = 0;
	m_fViewAngle = f2vt(0.0f);
	m_uiSwitchTimeDiff = 0;

	return true;
}

bool CShell::QuitApplication()
{

	m_sModel.Destroy();
	m_sModelOpt.Destroy();
	
#ifdef ENABLE_LOAD_TIME_STRIP
	free(m_pNewIdx);
#endif

	AppDisplayText->DeleteAllWindows();
	AppDisplayText->ReleaseTextures();

	
	delete AppDisplayText;
	
	/* Release all Textures */
	Textures->ReleaseTexture(m_Texture);
	
	delete Textures;
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

	return true;
}


bool CShell::RenderScene()
{
	unsigned long time;
	
	/* Clear the depth and frame buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
#ifdef ENABLE_LOAD_TIME_STRIP
	/*
	 Show a message on-screen then generate the necessary data on the
	 second frame.
	 */
	if(m_nInit)
	{
		--m_nInit;
		
		if(m_nInit)
		{
			AppDisplayText->DisplayDefaultTitle("Optimize Mesh", "", eDisplayTextLogoIMG);
			AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "Generating data...");
			AppDisplayText->Flush();	
			return true;
		}
		
		StripMesh();
	}
#endif
	
	struct timeval currTime = {0,0};

	/*
	 Time
	 */
	//time = PVRShellGetTime();
	gettimeofday(&currTime, NULL);
	time = currTime.tv_usec;
	m_uiTimeDiff = ((time - m_uiLastTime) / 1000.0f);
	m_uiLastTime = time;
	
	// FPS
	m_uiFPSFrameCnt++;
	m_uiFPSTimeDiff += m_uiTimeDiff;
	if(m_uiFPSTimeDiff >= TIME_FPS_UPDATE)
	{
		m_fFPS = (m_uiFPSFrameCnt * 1000.0f)/ (float)m_uiFPSTimeDiff;
		m_uiFPSFrameCnt = 0;
		m_uiFPSTimeDiff = 0;
	}
	
	// Change mode when necessary
	m_uiSwitchTimeDiff += m_uiTimeDiff;
	if ((m_uiSwitchTimeDiff > TIME_AUTO_SWITCH)) // || PVRShellIsKeyPressed(PVRShellKeyNameACTION1))
	{
		m_uiSwitchTimeDiff = 0;
		++m_nPage;
		
#ifdef ENABLE_LOAD_TIME_STRIP
		if(m_nPage > 2)
#else
			if(m_nPage > 1)
#endif
			{
				m_nPage = 0;
			}
	}
	
	/* Calculate the model view matrix turning around the balloon */
	ComputeViewMatrix();
	
	/* Draw the model */
	DrawModel(m_nPage);
	
	/* Calculate the frame rate */
	CalculateAndDisplayFrameRate();

	return true;
}

/*******************************************************************************
 * Function Name  : ComputeViewMatrix
 * Description    : Calculate the view matrix turning around the balloon
 *******************************************************************************/
void ComputeViewMatrix()
{
	VECTOR3 vFrom;
	VERTTYPE factor;
	
	/* Calculate the angle of the camera around the balloon */
	vFrom.x = VERTTYPEMUL(VIEW_DISTANCE, COS(m_fViewAngle));
	vFrom.y = f2vt(0.0f);
	vFrom.z = VERTTYPEMUL(VIEW_DISTANCE, SIN(m_fViewAngle));
	
	// Increase the rotation
	factor = f2vt(0.005f * (float)m_uiTimeDiff);
	m_fViewAngle += factor;
	
	// Ensure it doesn't grow huge and lose accuracy over time
	if(m_fViewAngle > PI)
		m_fViewAngle -= TWOPI;
	
	/* Compute and set the matrix */
	MatrixLookAtRH(m_mView, vFrom, c_vOrigin, c_vUp);
	glMatrixMode(GL_MODELVIEW);
	myglLoadMatrix(m_mView.f);
}

/*******************************************************************************
 * Function Name  : DrawModel
 * Inputs		  : iOptim
 * Description    : Draws the balloon
 *******************************************************************************/
void DrawModel( int iOptim )
{
	SPODMesh		*mesh;
	unsigned short	*indices;
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	MATRIX worldMatrix;
	m_sModel.GetWorldMatrix(worldMatrix, m_sModel.pNode[0]);
	myglMultMatrix(worldMatrix.f);
	
	/* Enable back face culling */
	//	glEnable(GL_CULL_FACE);
	//	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	/* Enable States */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	/* Set Data Pointers */
	switch(iOptim)
	{
		default:
			mesh	= m_sModel.pMesh;
			indices	= (unsigned short*) mesh->sFaces.pData;
			break;
		case 1:
			mesh	= m_sModelOpt.pMesh;
			indices	= (unsigned short*) mesh->sFaces.pData;
			break;
#ifdef ENABLE_LOAD_TIME_STRIP
		case 2:
			mesh	= m_sModel.pMesh;
			indices	= m_pNewIdx;
			break;
#endif
	}
	
	// Used to display interleaved geometry
	glVertexPointer(3, VERTTYPEENUM, mesh->sVertex.nStride, mesh->pInterleaved + (long)mesh->sVertex.pData);
	glTexCoordPointer(2, VERTTYPEENUM, mesh->psUVW->nStride, mesh->pInterleaved + (long)mesh->psUVW->pData);
	
	/* Draw */
	glDrawElements(GL_TRIANGLES, mesh->nNumFaces*3, GL_UNSIGNED_SHORT, indices);
	
	/* Disable States */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glPopMatrix();
}


/*******************************************************************************
 * Function Name  : CameraGetMatrix
 * Description    : Function to setup camera position
 *******************************************************************************/
void CameraGetMatrix()
{
	VECTOR3	vFrom, vTo, vUp;
	VERTTYPE		fFOV;
	
	vUp.x = f2vt(0.0f);	vUp.y = f2vt(1.0f);	vUp.z = f2vt(0.0f);
	
	if(m_sModel.nNumCamera)
	{
		/* Get Camera data from POD Geometry File */
		fFOV = m_sModel.GetCameraPos(vFrom, vTo, 0);
		fFOV = VERTTYPEMUL(fFOV, CAM_ASPECT);		// Convert from horizontal FOV to vertical FOV (0.75 assumes a 4:3 aspect ratio)
	}
	else
	{
		fFOV = f2vt(PIf / 6);
	}
	
	/* View */
	MatrixLookAtRH(m_mView, vFrom, vTo, vUp);
	
	/* Projection */
	MatrixPerspectiveFovRH(m_mProj, f2vt(fFOV), CAM_ASPECT, CAM_NEAR, CAM_FAR, true);
}

/*******************************************************************************
 * Function Name  : CalculateAndDisplayFrameRate
 * Description    : Computes and displays the on screen information
 *******************************************************************************/
void CalculateAndDisplayFrameRate()
{
//	char	pTitle[512];
	char	*pDesc;
	
	//sprintf(pTitle, "Optimize Mesh");
	
	/* Print text on screen */
	switch(m_nPage)
	{
		default:
			pDesc = "Indexed Triangle List: Unoptimized";
			break;
		case 1:
			pDesc = "Indexed Triangle List: Optimized (at export time)";
			break;
#ifdef ENABLE_LOAD_TIME_STRIP
		case 2:
			pDesc = "Indexed Triangle List: Optimized (at load time)";
			break;
#endif
	}

	
	AppDisplayText->DisplayDefaultTitle("Optimize Mesh", "", eDisplayTextLogoIMG);
	AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f", m_fFPS);
	AppDisplayText->DisplayText(0, 10, 0.4f, RGBA(255,255,255,255), "%s", pDesc);

	AppDisplayText->Flush();	
}

#ifdef ENABLE_LOAD_TIME_STRIP
/*******************************************************************************
 * Function Name  : StripMesh
 * Description    : Generates the stripped-at-load-time list.
 *******************************************************************************/
void StripMesh()
{
	// Make a copy of the indices (we can't edit the constants in the header)
	m_pNewIdx = (unsigned short*)malloc(sizeof(unsigned short)*m_sModel.pMesh->nNumFaces*3);
	memcpy(m_pNewIdx, m_sModel.pMesh->sFaces.pData, sizeof(unsigned short)*m_sModel.pMesh->nNumFaces*3);
	
	TriStripList(m_pNewIdx, m_sModel.pMesh->nNumFaces);
}
#endif


