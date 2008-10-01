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
#include "Geometry.h"
#include "UI.h"
#include "App.h"
#include "Memory.h"
#include "Macros.h"
#include "Timer.h"

#include <stdio.h>
#include <sys/time.h>

#define CAM_ASPECT	(1.333333333f)
#define CAM_NEAR	(3000.0f)
#define CAM_FAR		(4000.0f)

// The mallet's texture
#include "mallet.h"

#define SCENE_FILE @"model_float"

// Texture handle
GLuint			m_ui32MalletTexture;

// 3D Model
CPODScene	* m_Scene;

// Projection and Model View matrices
MATRIX		m_mProjection, m_mView;

// Array to lookup the textures for each material in the scene
GLuint*			m_puiTextures;

// Variables to handle the animation in a time-based manner
int				m_iTimePrev;
VERTTYPE		m_fFrame;
float       m_AvgFramerate;

#define DEMO_FRAME_RATE	(1.0f / 30.0f)

// allocate in heap
CDisplayText * AppDisplayText;
CTexture * Textures;

// function definitions
void CameraGetMatrix();
void LoadMaterial(int index);
void DrawModel();



bool CShell::InitApplication()
{
   AppDisplayText = new CDisplayText;    
	Textures = new CTexture;
	m_Scene = new CPODScene;
   
   // Get the resource path
   NSString *path = [[NSBundle mainBundle] pathForResource:SCENE_FILE ofType:@"pod" inDirectory:@"/"];

   // Load the POD file
   bool bRes = m_Scene->ReadFromFile([path UTF8String]);
	if(!bRes)
	{
      fprintf(stderr, "**ERROR** Failed to open file:\n%s.\n", [path UTF8String]);
		return false;
	}

	// Initialize variables used for the animation
	m_fFrame = 0;
	m_iTimePrev = 0;
   m_AvgFramerate = 0;
   
   // the PowerVR example's InitView() starts here.
   
   // loads the texture
   if(!Textures->LoadTextureFromPointer((void*)mallet, &m_ui32MalletTexture))
	{
		fprintf(stderr, "**ERROR** Failed to load texture for Mallet.\n");
      return false;
	}
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Init DisplayText
	if(!AppDisplayText->SetTextures(WindowHeight, WindowWidth))
   {
		fprintf(stderr, "ERROR: Cannot initialise Print3D\n");
      return false;
   }

	/* Model View Matrix */
	CameraGetMatrix();

	/* Projection Matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	myglMultMatrix(m_mProjection.f);
   
   
   /* GENERIC RENDER STATES */

	/* Enables Depth Testing */
	glEnable(GL_DEPTH_TEST);

	/* Enables Smooth Colour Shading */
	glShadeModel(GL_SMOOTH);

	/* Enable texturing */
	glEnable(GL_TEXTURE_2D);

	/* Define front faces */
	glFrontFace(GL_CW);

	/* Enables texture clamping */
	myglTexParameter( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	myglTexParameter( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   
   
   /* Reset the model view matrix to position the light */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

   /* Setup ambiant light */
   glEnable(GL_LIGHTING);
   VERTTYPE lightGlobalAmbient[] = {f2vt(1.0f), f2vt(1.0f), f2vt(1.0f), f2vt(1.0f)};
   myglLightModelv(GL_LIGHT_MODEL_AMBIENT, lightGlobalAmbient);

   /* Setup a directional light source */
   VERTTYPE lightPosition[] = {f2vt(-0.7f), f2vt(-1.0f), f2vt(+0.2f), f2vt(0.0f)};
   VERTTYPE lightAmbient[]  = {f2vt(1.0f), f2vt(1.0f), f2vt(1.0f), f2vt(1.0f)};
   VERTTYPE lightDiffuse[]  = {f2vt(1.0f), f2vt(1.0f), f2vt(1.0f), f2vt(1.0f)};
   VERTTYPE lightSpecular[] = {f2vt(0.2f), f2vt(0.2f), f2vt(0.2f), f2vt(1.0f)};

   glEnable(GL_LIGHT0);
   myglLightv(GL_LIGHT0, GL_POSITION, lightPosition);
   myglLightv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
   myglLightv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
   myglLightv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
   
   if(!m_iTimePrev)
      m_iTimePrev = GetTimeInMsSince1970();
   
	return true;
}

bool CShell::QuitApplication()
{
	// Frees the texture
	Textures->ReleaseTexture(m_ui32MalletTexture);

	// Frees the memory allocated for the scene
	m_Scene->Destroy();
	
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;
	delete Textures;
	delete m_Scene;
	
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

bool CShell::InitView()
{
   // Sets the clear color
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // TODO: try backface culling
   glDisable(GL_CULL_FACE);
	
	//UpdatePolarCamera();
	
	return true;
}

bool CShell::RenderScene()
{
	/*
		Calculates the frame number to animate in a time-based manner.
	*/
	int iTime = GetTimeInMsSince1970();
   //int iTime = GetTimeInMs();

	int iDeltaTime = iTime - m_iTimePrev;
	m_iTimePrev	= iTime;

	m_fFrame	+= iDeltaTime * 0.03f; // * DEMO_FRAME_RATE;

	while(m_fFrame > m_Scene->nNumFrame-1)
		m_fFrame -= m_Scene->nNumFrame-1;
   
   /* Clear the depth and frame buffer */
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
	/* Set Z compare properties */
	glEnable(GL_DEPTH_TEST);

	/* Disable Blending*/
	glDisable(GL_BLEND);

	/* Calculate the model view matrix */
	glMatrixMode(GL_MODELVIEW);
	myglLoadMatrix(m_mView.f);

   // Draw the model
   DrawModel();
	
   // calc framerate, avg over 100 frames.
   float N = 20;
   if(!m_AvgFramerate)
      m_AvgFramerate = 30.0f;
   m_AvgFramerate = ((m_AvgFramerate * (N - 1)) + (1000.0f / (float)iDeltaTime)) / N;
   
   // show text on the display
	AppDisplayText->DisplayDefaultTitle("POD Scene", "", eDisplayTextLogoIMG);
	AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "framerate: %3.1f", m_AvgFramerate);
	
	AppDisplayText->Flush();	
	
	return true;
}

// MARK: -
/*******************************************************************************
 * Function Name  : CameraGetMatrix
 * Global Used    :
 * Description    : Function to setup camera position
 *
 *******************************************************************************/
void CameraGetMatrix()
{
	VECTOR3	vFrom, vTo, vUp;
	VERTTYPE	fFOV;

	//Set the Up Vector
	vUp.x = f2vt(0.0f);
	vUp.y = f2vt(1.0f);
	vUp.z = f2vt(0.0f);

	//If the scene contains a camera then...
	if(m_Scene->nNumCamera)
	{
		//.. get the Camera's position, direction and FOV.
		fFOV = m_Scene->GetCameraPos(vFrom, vTo, 0);
		/*
		Convert the camera's field of view from horizontal to vertical
		(the 0.75 assumes a 4:3 aspect ratio).
		*/
		fFOV = VERTTYPEMUL(fFOV, WindowHeight/WindowWidth);
	}
	else
	{
		fFOV = VERTTYPEMUL(M_PI, f2vt(0.16667f));
	}

	/* Set up the view matrix */
	MatrixLookAtRH(m_mView, vFrom, vTo, vUp);

	/* Set up the projection matrix */
   MatrixPerspectiveFovRH(m_mProjection, fFOV, f2vt(WindowHeight/WindowWidth), f2vt(CAM_NEAR), f2vt(CAM_FAR), true);
}

/*******************************************************************************
 * Function Name  : DrawModel
 * Description    : Draws the model
 *******************************************************************************/
void DrawModel()
{
   int err;

	//Set the frame number
	m_Scene->SetFrame(m_fFrame);

	//Iterate through all the mesh nodes in the scene
	for(int iNode = 0; iNode < (int)m_Scene->nNumMeshNode; ++iNode)
	{
		//Get the mesh node.
		SPODNode* pNode = &m_Scene->pNode[iNode];

		//Get the mesh that the mesh node uses.
		SPODMesh* pMesh = &m_Scene->pMesh[pNode->nIdx];

		//Load the material that belongs to the mesh node.
		LoadMaterial(pNode->nIdxMaterial);

		//If the mesh has bone weight data then we must be skinning.
		bool bSkinning = pMesh->sBoneWeight.pData != 0;

		// If the mesh is used for skining then set up the matrix palettes.
		if(bSkinning)
		{
			//Enable the matrix palette extension
			glEnable(GL_MATRIX_PALETTE_OES);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

			/*
				Enables the matrix palette stack extension, and apply subsequent
				matrix operations to the matrix palette stack.
			*/
			glMatrixMode(GL_MATRIX_PALETTE_OES);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

			MATRIX	mBoneWorld;
			int			i32NodeID;

			/*
				Iterate through all the bones in the batch
			*/
			for(int j = 0; j < pMesh->sBoneBatches.pnBatchBoneCnt[0]; ++j)
			{
				/*
					Set the current matrix palette that we wish to change. An error
					will be returned if the index (j) is not between 0 and
					GL_MAX_PALETTE_MATRICES_OES. The value of GL_MAX_PALETTE_MATRICES_OES
					can be retrieved using glGetIntegerv, the initial value is 9.

					GL_MAX_PALETTE_MATRICES_OES does not mean you need to limit
					your character to 9 bones as you can overcome this limitation
					by using bone batching which splits the mesh up into sub-meshes
					which use only a subset of the bones.
				*/

            glCurrentPaletteMatrixOES(j);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

				// Generates the world matrix for the given bone in this batch.
				i32NodeID = pMesh->sBoneBatches.pnBatches[j];
				m_Scene->GetBoneWorldMatrix(mBoneWorld, *pNode, m_Scene->pNode[i32NodeID]);

				// Multiply the bone's world matrix by the view matrix to put it in view space
				MatrixMultiply(mBoneWorld, mBoneWorld, m_mView);

				// Load the bone matrix into the current palette matrix.
				myglLoadMatrix(mBoneWorld.f);
			}
		}
		else
		{
			//If we're not skinning then disable the matrix palette.
			glDisable(GL_MATRIX_PALETTE_OES);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		}

		//Switch to the modelview matrix.
		glMatrixMode(GL_MODELVIEW);
		//Push the modelview matrix
		glPushMatrix();

		//Get the world matrix for the mesh and transform the model view matrix by it.
		MATRIX worldMatrix;
		m_Scene->GetWorldMatrix(worldMatrix, *pNode);
		myglMultMatrix(worldMatrix.f);

		/* Modulate with vertex color */
		myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		/* Enable lighting */
		glEnable(GL_LIGHTING);

		/* Enable back face culling */
		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		/* Enable States */
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		// If the mesh has uv coordinates then enable the texture coord array state
		if (pMesh->psUVW)
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		if(bSkinning)
		{
			//If we are skinning then enable the relevant states.
			glEnableClientState(GL_MATRIX_INDEX_ARRAY_OES);
			glEnableClientState(GL_WEIGHT_ARRAY_OES);
                  if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		}

		/* Set Data Pointers */
		// Used to display non interleaved geometry
		glVertexPointer(pMesh->sVertex.n, VERTTYPEENUM, pMesh->sVertex.nStride, pMesh->sVertex.pData);
		glNormalPointer(VERTTYPEENUM, pMesh->sNormals.nStride, pMesh->sNormals.pData);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		if (pMesh->psUVW)
			glTexCoordPointer(2, VERTTYPEENUM, pMesh->psUVW->nStride, pMesh->psUVW->pData);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		if(bSkinning)
		{
			//Set up the indexes into the matrix palette.
			glMatrixIndexPointerOES(pMesh->sBoneIdx.n, GL_UNSIGNED_BYTE, pMesh->sBoneIdx.nStride, pMesh->sBoneIdx.pData);
			glWeightPointerOES(pMesh->sBoneWeight.n, VERTTYPEENUM, pMesh->sBoneWeight.nStride, pMesh->sBoneWeight.pData);
                  if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		}

		/* Draw */
		if(!pMesh->nNumStrips)
		{
			if(pMesh->sFaces.pData)
			{
				// Indexed Triangle list
				glDrawElements(GL_TRIANGLES, pMesh->nNumFaces*3, GL_UNSIGNED_SHORT, pMesh->sFaces.pData);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

			}
			else
			{
				// Non-Indexed Triangle list
				glDrawArrays(GL_TRIANGLES, 0, pMesh->nNumFaces*3);
                     if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

			}
		}
		else
		{
			if(pMesh->sFaces.pData)
			{
				// Indexed Triangle strips
				int offset = 0;
				for(int i = 0; i < (int)pMesh->nNumStrips; i++)
				{
					glDrawElements(GL_TRIANGLE_STRIP, pMesh->pnStripLength[i]+2, GL_UNSIGNED_SHORT, pMesh->sFaces.pData + offset*2);
					offset += pMesh->pnStripLength[i]+2;
                        if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

				}
			}
			else
			{
				// Non-Indexed Triangle strips
				int offset = 0;
				for(int i = 0; i < (int)pMesh->nNumStrips; i++)
				{
					glDrawArrays(GL_TRIANGLE_STRIP, offset, pMesh->pnStripLength[i]+2);
					offset += pMesh->pnStripLength[i]+2;
                        if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

				}
			}
		}

		/* Disable States */
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		if(bSkinning)
		{
			glDisableClientState(GL_MATRIX_INDEX_ARRAY_OES);
			glDisableClientState(GL_WEIGHT_ARRAY_OES);
         if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

		}

		//Reset the modelview matrix back to what it was before we transformed by the mesh node.
		glPopMatrix();
               if((err = glGetError()) != GL_NO_ERROR) printf("gl error at %s : %i\n", __FILE__, __LINE__);

	}

	//We are finished with the matrix pallete so disable it.
	glDisable(GL_MATRIX_PALETTE_OES);
}

/*******************************************************************************
 * Function Name  : LoadMaterial
 * Input		  : index into the material list
 * Description    : Loads the material index
 *******************************************************************************/
void LoadMaterial(int index)
{
	/*
		Load the model's material
	*/
	SPODMaterial* mat = &m_Scene->pMaterial[index];

	glBindTexture(GL_TEXTURE_2D, m_ui32MalletTexture);

	VERTTYPE prop[4];
	int i;
	prop[3] = f2vt(1.0f);

	for (i=0; i<3; ++i)
		prop[i] = mat->pfMatAmbient[i];

	myglMaterialv(GL_FRONT_AND_BACK, GL_AMBIENT, prop);

	for (i=0; i<3; ++i)
		prop[i] = mat->pfMatDiffuse[i];

	myglMaterialv(GL_FRONT_AND_BACK, GL_DIFFUSE, prop);

	for (i=0; i<3; ++i)
		prop[i] = mat->pfMatSpecular[i];

	myglMaterialv(GL_FRONT_AND_BACK, GL_SPECULAR, prop);
	myglMaterial(GL_FRONT_AND_BACK, GL_SHININESS, mat->fMatShininess);
}
