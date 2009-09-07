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
#include "Geometry.h"
#include "MemoryManager.h"
#include "UI.h"
#include "Macros.h"
#include "Timing.h"
#include "Pathes.h"
#include "ResourceFile.h"


#include <stdio.h>
#include <sys/time.h>

/******************************************************************************
 Defines
 ******************************************************************************/

// Return value defines.
#define SUCCESS 1
#define FAIL 0

// Camera constants. Used for making the projection matrix.
// Assuming a 4:3 aspect ratio.
#define WIDTH 320
#define HEIGHT 480
#define CAM_ASPECT	((float)WIDTH / (float) HEIGHT)
#define CAM_NEAR	(4.0f)
#define CAM_FAR		(1000.0f)

#define SKYBOX_ZOOM			150.0f
#define SKYBOX_ADJUSTUVS	true

// First, shader semantics recognized by this program.
enum EUniformSemantic
{
	eUsUnknown,
	eUsPOSITION,
	eUsNORMAL,
	eUsUV,
	eUsWORLDVIEWPROJECTION,
	eUsWORLDVIEW,
	eUsWORLDVIEWIT,
	eUsVIEWIT,
	eUsLIGHTDIRECTION,
	eUsTEXTURE
};

/****************************************************************************
 ** Constants
 ****************************************************************************/
const static SPFXUniformSemantic psUniformSemantics[] =
{
	{ "POSITION",				eUsPOSITION },
	{ "NORMAL",					eUsNORMAL },
	{ "UV",						eUsUV },
	{ "WORLDVIEWPROJECTION",	eUsWORLDVIEWPROJECTION },
	{ "WORLDVIEW",				eUsWORLDVIEW },
	{ "WORLDVIEWIT",			eUsWORLDVIEWIT },
	{ "VIEWIT",					eUsVIEWIT},
	{ "LIGHTDIRECTION",			eUsLIGHTDIRECTION },
	{ "TEXTURE",				eUsTEXTURE }
};

struct SEffect
{
	CPFXEffect	*pEffect;
	SPFXUniform	*psUniforms;
	unsigned int ui32UniformCount;
	
	SEffect()
	{
		pEffect = 0;
		psUniforms = 0;
		ui32UniformCount = 0;
	}
};

const float g_fFrameRate = 1.0f / 30.0f;
const unsigned int g_ui32NoOfEffects = 8;
const unsigned int g_ui32TexNo = 5;

const bool g_bBlendShader[g_ui32NoOfEffects] = {
false,
false,
false,
false,
true,
false,
false,
true
};

/******************************************************************************
 Content file names
 ******************************************************************************/

// POD scene files
const char c_szSceneFile[] = "/Scene.pod";

// Textures
const char * g_aszTextureNames[g_ui32TexNo] = {
"Balloon.pvr",
"Balloon_pvr.pvr",
"Noise.pvr",
"Skybox.pvr",
"SkyboxMidnight.pvr"
};

// PFX file
const char * g_pszEffectFileName = "/effects.pfx";

/******************************************************************************
 Class Variables
 ******************************************************************************/

// Print3D class used to display text
CDisplayText * AppDisplayText;

// Allocate texture interface.
CTexture * Textures;

// IDs for the various textures
GLuint m_ui32TextureIDs[g_ui32TexNo];

// 3D Model
CPVRTModelPOD m_Scene;

// Projection and Model View matrices
MATRIX m_mProjection, m_mView;

// Variables to handle the animation in a time-based manner
int m_iTimePrev;
float m_fFrame;

// The effect currently being displayed
int m_i32Effect;

// The Vertex buffer object handle array.
GLuint			*m_aiVboID;
GLuint			m_iSkyVboID;

/* View Variables */
VERTTYPE fViewAngle;
VERTTYPE fViewDistance, fViewAmplitude, fViewAmplitudeAngle;
VERTTYPE fViewUpDownAmplitude, fViewUpDownAngle;

/* Vectors for calculating the view matrix and saving the camera position*/
VECTOR3 vTo, vUp, vCameraPosition;

/* Skybox */
VERTTYPE* g_SkyboxVertices;
VERTTYPE* g_SkyboxUVs;

//animation
float fBurnAnim;

bool bPause;
float fDemoFrame;

// The variables required for the effects
CPFXParser *m_pEffectParser;
SEffect *m_pEffects;

// Variables needed for timing.
int frames;
float frameRate;

structTimer DrawSkyboxTimer;
float DrawSkyboxT;

structTimer DrawUITimer;
float DrawUIT;

structTimer SwitchShaderTimer;
float DrawShaderT;

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0

// Declare the fragment and vertex shaders.
GLuint uiFragShader, uiVertShader;		// Used to hold the fragment and vertex shader handles
GLuint uiProgramObject;					// Used to hold the program handle (made out of the two previous shaders)

// This ties in with the shader attribute to link to openGL, see pszVertShader.
const char* pszAttribs[] = { "myVertex"};

// Handles for the uniform variables.
int PMVMatrixHandle;
int ColHandle;

GLuint ui32Vbo = 0;					    // Vertex buffer object handle

/******************************************************************************
 Helper Function Declarations
 ******************************************************************************/

void CameraGetFieldOfView();
void DrawMesh(SPODMesh* mesh);
void ComputeViewMatrix();
void DrawSkybox();
bool LoadEffect(SEffect *pSEffect, const char * pszEffectName, const char *pszFileName);
bool LoadTextures(string* const pErrorStr);
bool DestroyEffect(SEffect *pSEffect);
void SetVertex(VERTTYPE** Vertices, int index, VERTTYPE x, VERTTYPE y, VERTTYPE z);
void SetUV(VERTTYPE** UVs, int index, VERTTYPE u, VERTTYPE v);
void ChangeSkyboxTo(SEffect *pSEffect, GLuint ui32NewSkybox);
void CreateSkybox(float scale, bool adjustUV, int textureSize, VERTTYPE** Vertices, VERTTYPE** UVs);
void DestroySkybox(VERTTYPE* Vertices, VERTTYPE* UVs);


/******************************************************************************
 Begin Code for Actual Application
 ******************************************************************************/

bool CShell::InitApplication()
{
	/* Init values to defaults */
	fViewAngle = PIOVERTWO;
	
	fViewDistance = f2vt(100.0f);
	fViewAmplitude = f2vt(60.0f);
	fViewAmplitudeAngle = f2vt(0.0f);
	
	fViewUpDownAmplitude = f2vt(50.0f);
	fViewUpDownAngle = f2vt(0.0f);
	
	vTo.x = f2vt(0);
	vTo.y = f2vt(0);
	vTo.z = f2vt(0);
	
	vUp.x = f2vt(0);
	vUp.y = f2vt(1);
	vUp.z = f2vt(0);	
	
	//AppDisplayText = new CDisplayText;
	Textures = new CTexture;
	
	//if(AppDisplayText->SetTextures(WindowHeight, WindowWidth) != SUCCESS)
	//	printf("Coult not properly load assets for displaying text!\n");
	
	/**********************/
	/* Create the Effects */
	/**********************/

	char buffer[2048];
	GetResourcePathASCII(buffer, 2048);
	
	CPVRTResourceFile::SetReadPath(buffer);
	
	string *ErrorStr = new string();
	
	// Step 1. Load the scene.
	if (m_Scene.ReadFromFile(c_szSceneFile) != SUCCESS)
	{
		printf("Could not properly load scene!\n");
		return false;
	}
		
	// Step 2. Load the textures.
	if(LoadTextures(ErrorStr) != SUCCESS)
	{
		printf("Could not properly load the textures!\n");
		return false;
	}
	
	// Step 3. Create the skybox.
	CreateSkybox( 500.0f, true, 512, &g_SkyboxVertices, &g_SkyboxUVs);
	
	// Step 4.  Begin effect creation by parsing effect file.
	m_pEffectParser = new CPFXParser();

	if(m_pEffectParser->ParseFromFile(g_pszEffectFileName, ErrorStr) != SUCCESS)
	{
		delete m_pEffectParser;
		printf("Could not properly parse file.\n");
		return false;
	}

	m_pEffects = new SEffect[m_pEffectParser->m_nNumEffects];

	// Step 5. Create skybox effect in shader.
	if(LoadEffect(&m_pEffects[0], "skybox_effect", g_pszEffectFileName) != SUCCESS)
	{
		printf("Failed to create skybox effect!");
		delete m_pEffectParser;
		delete[] m_pEffects;
		return false;
	}
	
	// Step 6. Initialize the Balloon Shaders
	if(!LoadEffect(&m_pEffects[1], "balloon_effect1", g_pszEffectFileName) ||
	   !LoadEffect(&m_pEffects[2], "balloon_effect2", g_pszEffectFileName) ||
	   !LoadEffect(&m_pEffects[3], "balloon_effect3", g_pszEffectFileName) ||
	   !LoadEffect(&m_pEffects[4], "balloon_effect4", g_pszEffectFileName) ||
	   !LoadEffect(&m_pEffects[5], "balloon_effect5", g_pszEffectFileName) ||
	   !LoadEffect(&m_pEffects[6], "balloon_effect6", g_pszEffectFileName) ||
	   !LoadEffect(&m_pEffects[7], "balloon_effect7", g_pszEffectFileName))
	{
		printf("Failed to load balloon effects.");
		delete m_pEffectParser;
		delete[] m_pEffects;
		return false;
	}

	// Step 7. Create Geometry Buffer Objects.
	m_aiVboID = new GLuint[m_Scene.nNumMeshNode];
	glGenBuffers(m_Scene.nNumMeshNode, m_aiVboID);
	
	for(unsigned int i = 0; i < m_Scene.nNumMeshNode ; ++i)
	{
		SPODNode* pNode = &m_Scene.pNode[i];
		
		// Gets pMesh referenced by the pNode
		SPODMesh* pMesh = &m_Scene.pMesh[pNode->nIdx];
		
		// Genereta a vertex buffer and set the interleaved vertex datas.
		glBindBuffer(GL_ARRAY_BUFFER, m_aiVboID[i]);
		glBufferData(GL_ARRAY_BUFFER, pMesh->sVertex.nStride*pMesh->nNumVertex, pMesh->pInterleaved, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
	}
	
	glGenBuffers(1, &m_iSkyVboID);
	glBindBuffer(GL_ARRAY_BUFFER, m_iSkyVboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VERTTYPE)*3 * 8, &g_SkyboxVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	//Step 9. Prepare the openGL rendering states. Enables depth test using the z-buffer.
	glEnable(GL_DEPTH_TEST);
	
	//Proper screen color.  Was originally in initView() from PowerVR.
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
	
	// Step 10. Initialize Projection Matrix.
	CameraGetFieldOfView();
	
	/* Init Values */
	bPause = false;
	fDemoFrame = 0.0;
	fBurnAnim = 0.0f;
	
	m_i32Effect = 1;
	
	delete ErrorStr;
	
	// Begin the timing!
	StartTimer(&DrawSkyboxTimer);
	StartTimer(&DrawUITimer);
	StartTimer(&SwitchShaderTimer);
	
	//delete buffer;
	
	return true;
}

bool CShell::QuitApplication()
{
	
	// Free the textures
	unsigned int i;
	
	for(i = 0; i < g_ui32TexNo; ++i)
	{
		glDeleteTextures(1, &(m_ui32TextureIDs[i]));
	}
	
	// Release Vertex buffer objects.
	glDeleteBuffers(m_Scene.nNumMeshNode, m_aiVboID);
	glDeleteBuffers(1, &m_iSkyVboID);
	delete[] m_aiVboID;
	
	// Destroy the Skybox
	DestroySkybox( g_SkyboxVertices, g_SkyboxUVs );
	
	for(i = 0; i < m_pEffectParser->m_nNumEffects; ++i)
		DestroyEffect(&m_pEffects[i]);
	
	delete[] m_pEffects;
	delete m_pEffectParser;
	
	// Destroy all information related to the scene.
	m_Scene.Destroy();
	
	// Clear up display text information.
	//AppDisplayText->ReleaseTextures();
	delete AppDisplayText;

	return true;
}

bool CShell::UpdateScene()
{		
	ResetTimer(&DrawSkyboxTimer);
	
	// Clears the colour and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if(!bPause)
	{
		m_fFrame   += 1.0f;
		fDemoFrame += 1.0f;
		fBurnAnim  += 0.0125f;
		
		if(fBurnAnim >= 1.0f)
			fBurnAnim = 1.0f;
	}
	
	// Automatic Shader Change Over Time.
	if ((fDemoFrame > 500) || (m_i32Effect == 2 && fDemoFrame > 80))
	{
		if(++m_i32Effect >= (int) g_ui32NoOfEffects)
		{
			m_i32Effect = 1;
			m_fFrame = 0.0f;
		}
		fDemoFrame = 0.0f;
		fBurnAnim = 0.0f;

		//printf("Time for a shader change.\n");
		//printf("Now entering shader %d.\n", m_i32Effect);
		//printf("Frame count (cyclical) is now: %f.\n", m_fFrame);
	}
	
	/* Setup Shader and Shader Constants */
	
	/* Calculate the model view matrix turning around the balloon */
	ComputeViewMatrix();
	
	++frames;
	CFTimeInterval			TimeInterval;
	frameRate = GetFps(frames, TimeInterval);

	//AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), 
	//							"fps: %3.2f Skybox: %3.2fms UI: %3.2fms", 
	//							frameRate, DrawSkyboxT, DrawUIT);
	
	return true;
}

bool CShell::RenderScene()
{
	unsigned int i, j;	
	int location;
	
    //glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	DrawSkybox();

	m_pEffects[m_i32Effect].pEffect->Activate();
	
	for(i = 0; i < m_Scene.nNumMeshNode; i++)
	{
		SPODNode* pNode = &m_Scene.pNode[i];
		
		// Gets pMesh referenced by the pNode
		SPODMesh* pMesh = &m_Scene.pMesh[pNode->nIdx];
		
		// Gets the node model matrix
		MATRIX mWorld, mWORLDVIEW;
		mWorld = m_Scene.GetWorldMatrix(*pNode);
		
		MatrixMultiply(mWORLDVIEW, mWorld, m_mView);
		
#ifdef DISPLAY_MATRIXES
		printf("Balloon mWorld matrix\n");
		printf("%f %f %f %f\n", mWorld.f[0], mWorld.f[1], mWorld.f[2], mWorld.f[3]);
		printf("%f %f %f %f\n", mWorld.f[4], mWorld.f[5], mWorld.f[6], mWorld.f[7]);
		printf("%f %f %f %f\n", mWorld.f[8], mWorld.f[9], mWorld.f[10], mWorld.f[11]);
		printf("%f %f %f %f\n", mWorld.f[12], mWorld.f[13], mWorld.f[14], mWorld.f[15]);
#endif

		glBindBuffer(GL_ARRAY_BUFFER, m_aiVboID[i]);
		
		for(j = 0; j < m_pEffects[m_i32Effect].ui32UniformCount; ++j)
		{
			switch(m_pEffects[m_i32Effect].psUniforms[j].nSemantic)
			{
				case eUsPOSITION:
				{
					glVertexAttribPointer(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 3, GL_FLOAT, GL_FALSE, pMesh->sVertex.nStride, pMesh->sVertex.pData);
					glEnableVertexAttribArray(m_pEffects[m_i32Effect].psUniforms[j].nLocation);
				}
					break;
				case eUsNORMAL:
				{
					glVertexAttribPointer(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 3, GL_FLOAT, GL_FALSE, pMesh->sNormals.nStride, pMesh->sNormals.pData);
					glEnableVertexAttribArray(m_pEffects[m_i32Effect].psUniforms[j].nLocation);
				}
					break;
				case eUsUV:
				{
					glVertexAttribPointer(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 2, GL_FLOAT, GL_FALSE, pMesh->psUVW[0].nStride, pMesh->psUVW[0].pData);
					glEnableVertexAttribArray(m_pEffects[m_i32Effect].psUniforms[j].nLocation);
				}
					break;
				case eUsWORLDVIEWPROJECTION:
				{
					MATRIX mMVP;
#ifdef DISPLAY_MATRIXES
					printf("Balloon mWorldView matrix\n");
					printf("%f %f %f %f\n", mWORLDVIEW.f[0], mWORLDVIEW.f[1], mWORLDVIEW.f[2], mWORLDVIEW.f[3]);
					printf("%f %f %f %f\n", mWORLDVIEW.f[4], mWORLDVIEW.f[5], mWORLDVIEW.f[6], mWORLDVIEW.f[7]);
					printf("%f %f %f %f\n", mWORLDVIEW.f[8], mWORLDVIEW.f[9], mWORLDVIEW.f[10], mWORLDVIEW.f[11]);
					printf("%f %f %f %f\n", mWORLDVIEW.f[12], mWORLDVIEW.f[13], mWORLDVIEW.f[14], mWORLDVIEW.f[15]);
					printf("Balloon m_mProjection matrix\n");
					printf("%f %f %f %f\n", m_mProjection.f[0], m_mProjection.f[1], m_mProjection.f[2], m_mProjection.f[3]);
					printf("%f %f %f %f\n", m_mProjection.f[4], m_mProjection.f[5], m_mProjection.f[6], m_mProjection.f[7]);
					printf("%f %f %f %f\n", m_mProjection.f[8], m_mProjection.f[9], m_mProjection.f[10], m_mProjection.f[11]);
					printf("%f %f %f %f\n", m_mProjection.f[12], m_mProjection.f[13], m_mProjection.f[14], m_mProjection.f[15]);
#endif
					// Passes the model-view-projection matrix (MVP) to the shader to transform the vertices.
					
					//These create the proper matrices for the Balloon.
					MatrixMultiply(mMVP, mWORLDVIEW, m_mProjection);

					glUniformMatrix4fv(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 1, GL_FALSE, mMVP.f);
#ifdef DISPLAY_MATRIXES
					printf("Balloon mMVP matrix\n");
					printf("%f %f %f %f\n", mMVP.f[0], mMVP.f[1], mMVP.f[2], mMVP.f[3]);
					printf("%f %f %f %f\n", mMVP.f[4], mMVP.f[5], mMVP.f[6], mMVP.f[7]);
					printf("%f %f %f %f\n", mMVP.f[8], mMVP.f[9], mMVP.f[10], mMVP.f[11]);
					printf("%f %f %f %f\n", mMVP.f[12], mMVP.f[13], mMVP.f[14], mMVP.f[15]);
#endif
				}
					break;
				case eUsWORLDVIEW:
				{
					glUniformMatrix4fv(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 1, GL_FALSE, mWORLDVIEW.f);
				}
					break;
				case eUsWORLDVIEWIT:
				{
					MATRIX mWORLDVIEWI, mWORLDVIEWIT;
					
					MatrixInverseEx(mWORLDVIEWI, mWORLDVIEW);
					MatrixTranspose(mWORLDVIEWIT, mWORLDVIEWI);
					
#ifdef DISPLAY_MATRIXES
					//Try converting to a Matrix3 here.
					//Hmmmmm.
					printf("PowerVR mWORLDVIEWIT matrix\n");
					printf("%f %f %f %f\n", WORLDVIEWIT.f[0], WORLDVIEWIT.f[1], WORLDVIEWIT.f[2], WORLDVIEWIT.f[3]);
					printf("%f %f %f %f\n", WORLDVIEWIT.f[4], WORLDVIEWIT.f[5], WORLDVIEWIT.f[6], WORLDVIEWIT.f[7]);
					printf("%f %f %f %f\n", WORLDVIEWIT.f[8], WORLDVIEWIT.f[9], WORLDVIEWIT.f[10], WORLDVIEWIT.f[11]);
					printf("%f %f %f %f\n", WORLDVIEWIT.f[12], WORLDVIEWIT.f[13], WORLDVIEWIT.f[14], WORLDVIEWIT.f[15]);
#endif
					// Converted the 4x4 matrix to a 3x3 here.  Lighting is now correct.  This is a hack for the time being.
					// PowerVR examples support both 3x3 and 4x4 matrices, while Oolong only supports 4x4.
					// At this time, there is no plan to support 3x3 matrices in Oolong (hence, the hack used here).
					mWORLDVIEWIT.f[3] = mWORLDVIEWIT.f[4];
					mWORLDVIEWIT.f[4] = mWORLDVIEWIT.f[5];
					mWORLDVIEWIT.f[5] = mWORLDVIEWIT.f[6];
				    mWORLDVIEWIT.f[6] = mWORLDVIEWIT.f[8];
					mWORLDVIEWIT.f[7] = mWORLDVIEWIT.f[9];
					mWORLDVIEWIT.f[8] = mWORLDVIEWIT.f[10];
					glUniformMatrix3fv(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 1, GL_FALSE, mWORLDVIEWIT.f);
				}
					break;
				case eUsVIEWIT:
				{
					MATRIX mViewI, mViewIT;
					MatrixInverseEx(mViewI, m_mView);
					MatrixTranspose(mViewIT, mViewI);
					
					// Once again, converting a 4x4 matrix to a 3x3 here.  This is a hack for the time being.
					mViewIT.f[3] = mViewIT.f[4];
					mViewIT.f[4] = mViewIT.f[5];
					mViewIT.f[5] = mViewIT.f[6];
					mViewIT.f[6] = mViewIT.f[8];
					mViewIT.f[7] = mViewIT.f[9];
					mViewIT.f[8] = mViewIT.f[10];
					glUniformMatrix3fv(m_pEffects[m_i32Effect].psUniforms[j].nLocation, 1, GL_FALSE, mViewIT.f);
				}
					break;
				case eUsLIGHTDIRECTION:
				{
					Vec4 vLightDirectionEyeSpace;
					
					// Passes the light direction in eye space to the shader
					MatrixVec4Multiply(vLightDirectionEyeSpace, Vec4(1.0,1.0,-1.0,0.0), m_mView);;
					glUniform3f(m_pEffects[m_i32Effect].psUniforms[j].nLocation, vLightDirectionEyeSpace.x, vLightDirectionEyeSpace.y, vLightDirectionEyeSpace.z);
				}
					break;
				case eUsTEXTURE:
				{
					// Set the sampler variable to the texture unit
					glUniform1i(m_pEffects[m_i32Effect].psUniforms[j].nLocation, m_pEffects[m_i32Effect].psUniforms[j].nIdx);
				}
					break;
			}
		}
		
		location = glGetUniformLocation(m_pEffects[m_i32Effect].pEffect->m_uiProgram, "myEyePos");
		
		if(location != -1)
			glUniform3f(location, vCameraPosition.x, vCameraPosition.y, vCameraPosition.z);
		
		//set animation
		location = glGetUniformLocation(m_pEffects[m_i32Effect].pEffect->m_uiProgram, "fAnim");
		
		if(location != -1)
			glUniform1f(location, fBurnAnim);
		
		location = glGetUniformLocation(m_pEffects[m_i32Effect].pEffect->m_uiProgram, "myFrame");
		
		if(location != -1)
			glUniform1f(location, m_fFrame);
		
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		
		if(g_bBlendShader[m_i32Effect])
		{
			glEnable(GL_BLEND);
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			// Correct render order for alpha blending through culling
			// This happens on Shaders 5 and 8?
			// Draw Back faces
			location = glGetUniformLocation(m_pEffects[m_i32Effect].pEffect->m_uiProgram, "bBackFace");
			glUniform1i(location, 1);
			glCullFace(GL_BACK);
			
			DrawMesh(pMesh);
			
			// Draw Front faces
			glUniform1f(location, 0.0f);
		}
		else
		{
			location = glGetUniformLocation(m_pEffects[m_i32Effect].pEffect->m_uiProgram, "bBackFace");
			glUniform1i(location, 0);
			glDisable(GL_BLEND);
		}
		
		glCullFace(GL_FRONT); 
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		
		// Everything should now be setup, therefore draw the mesh. 
		DrawMesh(pMesh);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		for(j = 0; j < m_pEffects[m_i32Effect].ui32UniformCount; ++j)
		{
			switch(m_pEffects[m_i32Effect].psUniforms[j].nSemantic)
			{
				case eUsPOSITION:
				{
					glDisableVertexAttribArray(m_pEffects[m_i32Effect].psUniforms[j].nLocation);
				}
					break;
				case eUsNORMAL:
				{
					glDisableVertexAttribArray(m_pEffects[m_i32Effect].psUniforms[j].nLocation);
				}
					break;
				case eUsUV:
				{
					glDisableVertexAttribArray(m_pEffects[m_i32Effect].psUniforms[j].nLocation);
				}
					break;
			}
		}
	}
	
	//////// Original Code entered here.
	DrawSkyboxT = GetAverageTimeValueInMS(&DrawSkyboxTimer);
	
	ResetTimer(&DrawUITimer);
	
	// show text on the display
	//AppDisplayText->DisplayDefaultTitle("Skybox 2", "", eDisplayTextLogoIMG);
	
	//AppDisplayText->Flush();	
	
	DrawUIT = GetAverageTimeValueInMS(&DrawUITimer);

	return true;
}

/*!****************************************************************************
 @Function		DrawMesh
 @Input			mesh		The mesh to draw
 @Description	Draws a SPODMesh after the model view matrix has been set and
 the meterial prepared.
 ******************************************************************************/
void DrawMesh(SPODMesh* pMesh)
{
	/*
	 The geometry can be exported in 4 ways:
	 - Non-Indexed Triangle list
	 - Indexed Triangle list
	 - Non-Indexed Triangle strips
	 - Indexed Triangle strips
	 */
	if(!pMesh->nNumStrips)
	{
		if(pMesh->sFaces.pData)
		{
			// Indexed Triangle list
			glDrawElements(GL_TRIANGLES, pMesh->nNumFaces*3, GL_UNSIGNED_SHORT, pMesh->sFaces.pData);
		}
		else
		{
			// Non-Indexed Triangle list
			glDrawArrays(GL_TRIANGLES, 0, pMesh->nNumFaces*3);
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
			}
		}
	}
}


/*******************************************************************************
 * Function Name  : CameraGetFieldOfView
 * Description    : Use this to set the proper m_mProjection matrix for the
 *                  iPhone's necessary parameters.
 *******************************************************************************/
void CameraGetFieldOfView()
{
	VECTOR3	vFrom, vTo, vUp;
	VERTTYPE fFOV;
	
	vUp.x = f2vt(0.0f);
	vUp.y = f2vt(1.0f);
	vUp.z = f2vt(0.0f);
	
	if(m_Scene.nNumCamera)
	{
		// Get Camera data from POD Geometry File.
		fFOV = m_Scene.GetCameraPos(vFrom, vTo, 0);
		// Convert from horizontal FOV to vertical FOV (0.75 assumes a 4:3 aspect ratio).
		fFOV = VERTTYPEMUL(fFOV, 0.75f);
	}
	else
	{
		fFOV = PI / 6;
	}
	
	// Projection matrix is created.
	MatrixPerspectiveFovRH(m_mProjection, fFOV, 
						   CAM_ASPECT, 
						   CAM_NEAR, 
						   CAM_FAR, 
						   1);
}

/*******************************************************************************
 * Function Name  : ComputeViewMatrix
 * Description    : Calculate the view matrix turning around the balloon
 *******************************************************************************/
void ComputeViewMatrix()
{
// PSS: use fixed camera position
#if 0
	fViewAngle = PIOVERTWO;
	
	fViewDistance = f2vt(100.0f);
	fViewAmplitude = f2vt(60.0f);
	fViewAmplitudeAngle = f2vt(0.0f);
	
	fViewUpDownAmplitude = f2vt(50.0f);
	fViewUpDownAngle = f2vt(0.0f);
	
	vTo.x = f2vt(0);
	vTo.y = f2vt(0);
	vTo.z = f2vt(0);
	
	vUp.x = f2vt(0);
	vUp.y = f2vt(1);
	vUp.z = f2vt(0);
#endif
	
	Vec3 vFrom;
	
	// Calculate the distance to balloon.
	//float fDistance = fViewDistance + fViewAmplitude * (float) sin(fViewAmplitudeAngle);
	//fDistance = fDistance / 5.0f;
	VERTTYPE fDistance = fViewDistance + VERTTYPEMUL(fViewAmplitude, sin(fViewAmplitudeAngle));
	fDistance = VERTTYPEMUL(fDistance, 0.2f);
	fViewAmplitudeAngle += 0.004f;
	
	// Calculate the vertical position of the camera
	//float updown = fViewUpDownAmplitude * (float) sin(fViewUpDownAngle);
	//updown = updown / 5.0f;
	VERTTYPE fUpdown = VERTTYPEMUL(fViewUpDownAmplitude, sin(fViewUpDownAngle));
	fUpdown = VERTTYPEMUL(fUpdown, 0.2f);
	fViewUpDownAngle += 0.005f;
	
	// Calculate the angle of the camera around the balloon.
	//vFrom.x = fDistance * (float) cos(fViewAngle);
	//vFrom.y = updown;
	//vFrom.z = fDistance * (float) sin(fViewAngle);
	vFrom.x = VERTTYPEMUL(fDistance, cos(fViewAngle));
	vFrom.y = fUpdown;
	vFrom.z = VERTTYPEMUL(fDistance, sin(fViewAngle));
	fViewAngle += 0.003f;
	
	// Compute and set the matrix.
	MatrixLookAtRH(m_mView, vFrom, vTo, vUp);
	
	// Remember the camera position to draw the Skybox around it
	vCameraPosition = vFrom;
}

/*******************************************************************************
 * Function Name  : DrawSkybox
 * Description    : Draws the Skybox
 *******************************************************************************/
void DrawSkybox()
{	
#ifdef DISPLAY_MATRIXES
	printf("Oolong\n");
	printf("Frame %d begin.\n", frameCount);
	printf("m_mView matrix\n");
	printf("%f %f %f %f\n", m_mView.f[0], m_mView.f[1], m_mView.f[2], m_mView.f[3]);
	printf("%f %f %f %f\n", m_mView.f[4], m_mView.f[5], m_mView.f[6], m_mView.f[7]);
	printf("%f %f %f %f\n", m_mView.f[8], m_mView.f[9], m_mView.f[10], m_mView.f[11]);
	printf("%f %f %f %f\n", m_mView.f[12], m_mView.f[13], m_mView.f[14], m_mView.f[15]);
	printf("Skybox m_mProjection matrix\n");
	printf("%f %f %f %f\n", m_mProjection.f[0], m_mProjection.f[1], m_mProjection.f[2], m_mProjection.f[3]);
	printf("%f %f %f %f\n", m_mProjection.f[4], m_mProjection.f[5], m_mProjection.f[6], m_mProjection.f[7]);
	printf("%f %f %f %f\n", m_mProjection.f[8], m_mProjection.f[9], m_mProjection.f[10], m_mProjection.f[11]);
	printf("%f %f %f %f\n", m_mProjection.f[12], m_mProjection.f[13], m_mProjection.f[14], m_mProjection.f[15]);
#endif
	
	// Use the loaded Skybox shader program
	m_pEffects[0].pEffect->Activate();
	
	int iVertexUniform = 0;
	for(unsigned int j = 0; j < m_pEffects[0].ui32UniformCount; ++j)
	{
		switch(m_pEffects[0].psUniforms[j].nSemantic)
		{
			case eUsPOSITION:
			{
				iVertexUniform = j;
				glEnableVertexAttribArray(m_pEffects[0].psUniforms[j].nLocation);
			}
				break;
			case eUsWORLDVIEWPROJECTION:
			{
				MATRIX mTrans, mMVP, mTransView;
				MatrixIdentity(mTrans);
				MatrixIdentity(mMVP);
				MatrixIdentity(mTransView);
				MatrixTranslation(mTrans, -vCameraPosition.x, -vCameraPosition.y, -vCameraPosition.z);

#ifdef DISPLAY_MATRIXES
				printf("Skybox mTrans matrix\n");
				printf("%f %f %f %f\n", mTrans.f[0], mTrans.f[1], mTrans.f[2], mTrans.f[3]);
				printf("%f %f %f %f\n", mTrans.f[4], mTrans.f[5], mTrans.f[6], mTrans.f[7]);
				printf("%f %f %f %f\n", mTrans.f[8], mTrans.f[9], mTrans.f[10], mTrans.f[11]);
				printf("%f %f %f %f\n", mTrans.f[12], mTrans.f[13], mTrans.f[14], mTrans.f[15]);
#endif
				//mMVP = m_mProjection * mTrans * m_mView;
				
				MatrixMultiply(mTransView, mTrans, m_mView);
				//MatrixMultiply(mMVP, m_mProjection, mTransView);
				// Proper matrices are made here.
				//MatrixMultiply(mTransView, m_mView, mTrans);
				MatrixMultiply(mMVP, mTransView, m_mProjection);
				
				/* Passes the model-view-projection matrix (MVP) to the shader to transform the vertices */
				glUniformMatrix4fv(m_pEffects[0].psUniforms[j].nLocation, 1, GL_FALSE, mMVP.f);

#ifdef DISPLAY_MATRIXES
				printf("mMVP matrix\n");
				printf("%f %f %f %f\n", mMVP.f[0], mMVP.f[1], mMVP.f[2], mMVP.f[3]);
				printf("%f %f %f %f\n", mMVP.f[4], mMVP.f[5], mMVP.f[6], mMVP.f[7]);
				printf("%f %f %f %f\n", mMVP.f[8], mMVP.f[9], mMVP.f[10], mMVP.f[11]);
				printf("%f %f %f %f\n", mMVP.f[12], mMVP.f[13], mMVP.f[14], mMVP.f[15]);
#endif
			}
				break;
			case eUsTEXTURE:
			{
				// Set the sampler variable to the texture unit
				glUniform1i(m_pEffects[0].psUniforms[j].nLocation, m_pEffects[0].psUniforms[j].nIdx);
			}
				break;
		}
	}
	
	for (int i = 0; i < 6; i++)
	{
		// Set Data Pointers
		glVertexAttribPointer(m_pEffects[0].psUniforms[iVertexUniform].nLocation, 
							  3, GL_FLOAT, GL_FALSE, sizeof(VERTTYPE)*3, &g_SkyboxVertices[i*4*3]);
		
		// Draw
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	
	/* Disable States */
	glDisableVertexAttribArray(m_pEffects[0].psUniforms[iVertexUniform].nLocation);
}

/*******************************************************************************
 * Function Name  : ChangeSkyboxTo
 * Description    : Changes the skybox to whichever effect is specified.
 *******************************************************************************/
void ChangeSkyboxTo(SEffect *pSEffect, GLuint ui32NewSkybox)
{
	unsigned int i,i32Cnt;
	const SPFXTexture *psTex;
	psTex = pSEffect->pEffect->GetTextureArray(i32Cnt);
	
	for(i = 0; i < i32Cnt; ++i)
	{
		if(strcmp(g_aszTextureNames[3], psTex[i].p) == 0)
		{
			pSEffect->pEffect->SetTexture(i, ui32NewSkybox, PVRTEX_CUBEMAP);
			return;
		}
	}
}

/*******************************************************************************
 * Function Name  : LoadEffect
 * Description    : Parses through the special effects file for each effect.
 *******************************************************************************/
bool LoadEffect(SEffect *pSEffect, const char * pszEffectName, const char *pszFileName)
{
	if(!pSEffect)
		return false;
	
	unsigned int nUnknownUniformCount;
	string error;
	
	// Load an effect from the file
	if(!pSEffect->pEffect)
	{
		pSEffect->pEffect = new CPFXEffect();
		
		if(!pSEffect->pEffect)
		{
			delete m_pEffectParser;
			printf("Failed to create effect.\n");
			return false;
		}
	}
	
	if(pSEffect->pEffect->Load(*m_pEffectParser, pszEffectName, pszFileName, &error) != SUCCESS)
	{
		printf("Failed to load the effect!\n");
		return false;
	}
	
	// Generate uniform array
	if(pSEffect->pEffect->BuildUniformTable(&pSEffect->psUniforms, &pSEffect->ui32UniformCount, &nUnknownUniformCount,
											psUniformSemantics, sizeof(psUniformSemantics) / sizeof(*psUniformSemantics), &error) != SUCCESS)
	{
		printf("Failed to generate uniform array!\n");
		return false;
	}
	
	if(nUnknownUniformCount)
	{
		printf("Unknown uniform semantic count: %d.\n", nUnknownUniformCount);
	}
	
	/* Set the textures for each effect */
	const SPFXTexture	*psTex;
	unsigned int			nCnt, i,j ;

	psTex = pSEffect->pEffect->GetTextureArray(nCnt);
	
	for(i = 0; i < nCnt; ++i)
	{
		for(j = 0; j < g_ui32TexNo; ++j)
		{
			if(strcmp(g_aszTextureNames[j], psTex[i].p) == 0)
			{
				if(j == 3 || j == 4)
					pSEffect->pEffect->SetTexture(i, m_ui32TextureIDs[j], PVRTEX_CUBEMAP);
				else
					pSEffect->pEffect->SetTexture(i, m_ui32TextureIDs[j]);
				
				break;
			}
		}
	}
	
	return true;
}	


/*******************************************************************************
 * Function Name  : DestroyEffect
 * Description    : Parses through the special effects file for each effect.
 *******************************************************************************/
bool DestroyEffect(SEffect *pSEffect)
{
	if(pSEffect)
	{
		if(pSEffect->pEffect)
		{
			const SPFXTexture *psTex;
			unsigned int			nCnt, i;
			
			psTex = pSEffect->pEffect->GetTextureArray(nCnt);
			
			for(i = 0; i < nCnt; ++i)
			{
				glDeleteTextures(1, &(psTex[i].ui));
			}
			
			delete pSEffect->pEffect;
			pSEffect->pEffect = 0;
		}
		
		delete pSEffect->psUniforms;
	}
	
	return true;
}

/*******************************************************************************
 * Function Name  : LoadTextures
 * Description    : Loads the proper textures as per OpenGLES2 specifications.
 *******************************************************************************/
bool LoadTextures(string* const pErrorStr)
{
	char *buffer = new char[2048];
	GetResourcePathASCII(buffer, 2048);
	char		*filename = new char[2048];
	
	// Load Regular Textures.
	for(int i = 0; i < 3; ++i)
	{
		memset(filename, 0, 2048 * sizeof(char));
		sprintf(filename, "/%s", g_aszTextureNames[i]);
		if(Textures->LoadTextureFromPVR(filename, &m_ui32TextureIDs[i]) != SUCCESS)
		{
			printf("LoadTextures: Texture %d from LoadTextures failed.\n", i);
			delete buffer;
			delete filename;
			return false;
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	
	// Load Cube Maps.
	for(int i = 3; i < 5; ++i)
	{
		memset(filename, 0, 2048 * sizeof(char));
		sprintf(filename, "/%s", g_aszTextureNames[i]);
		if(Textures->LoadTextureFromPVR(filename, &m_ui32TextureIDs[i]) != SUCCESS)
		{
			printf("LoadTextures: Cube Map %d from LoadTextures failed.\n", i);
			delete buffer;
			delete filename;
			return false;
		}
		
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	
	delete buffer;
	delete filename;
	
	return true;
}

/*******************************************************************************
 * Function Name  : SetVertex
 * Description    : Helper method to draw the skybox.
 *******************************************************************************/
void SetVertex(VERTTYPE** Vertices, int index, VERTTYPE x, VERTTYPE y, VERTTYPE z)
{
	(*Vertices)[index*3+0] = x;
	(*Vertices)[index*3+1] = y;
	(*Vertices)[index*3+2] = z;
}

/*******************************************************************************
 * Function Name  : SetUV
 * Description    : Helper method to draw the skybox.
 *******************************************************************************/
void SetUV(VERTTYPE** UVs, int index, VERTTYPE u, VERTTYPE v)
{
	(*UVs)[index*2+0] = u;
	(*UVs)[index*2+1] = v;
}

/*******************************************************************************
 * Function Name  : CreateSkybox
 * Description    : Creates the vertices and texture coordinates for a skybox.
 *******************************************************************************/
void CreateSkybox(float scale, bool adjustUV, int textureSize, VERTTYPE** Vertices, VERTTYPE** UVs)
{
	*Vertices = new VERTTYPE[24*3];
	*UVs = new VERTTYPE[24*2];
	
	VERTTYPE unit = f2vt(1);
	VERTTYPE a0 = 0, a1 = unit;
	
	if (adjustUV)
	{
		VERTTYPE oneover = f2vt(1.0f / textureSize);
		a0 = VERTTYPEMUL(f2vt(4.0f), oneover);
		a1 = unit - a0;
	}
	
	// Front
	SetVertex(Vertices, 0, -unit, +unit, -unit);
	SetVertex(Vertices, 1, +unit, +unit, -unit);
	SetVertex(Vertices, 2, -unit, -unit, -unit);
	SetVertex(Vertices, 3, +unit, -unit, -unit);
	SetUV(UVs, 0, a0, a1);
	SetUV(UVs, 1, a1, a1);
	SetUV(UVs, 2, a0, a0);
	SetUV(UVs, 3, a1, a0);
	
	// Right
	SetVertex(Vertices, 4, +unit, +unit, -unit);
	SetVertex(Vertices, 5, +unit, +unit, +unit);
	SetVertex(Vertices, 6, +unit, -unit, -unit);
	SetVertex(Vertices, 7, +unit, -unit, +unit);
	SetUV(UVs, 4, a0, a1);
	SetUV(UVs, 5, a1, a1);
	SetUV(UVs, 6, a0, a0);
	SetUV(UVs, 7, a1, a0);
	
	// Back
	SetVertex(Vertices, 8 , +unit, +unit, +unit);
	SetVertex(Vertices, 9 , -unit, +unit, +unit);
	SetVertex(Vertices, 10, +unit, -unit, +unit);
	SetVertex(Vertices, 11, -unit, -unit, +unit);
	SetUV(UVs, 8 , a0, a1);
	SetUV(UVs, 9 , a1, a1);
	SetUV(UVs, 10, a0, a0);
	SetUV(UVs, 11, a1, a0);
	
	// Left
	SetVertex(Vertices, 12, -unit, +unit, +unit);
	SetVertex(Vertices, 13, -unit, +unit, -unit);
	SetVertex(Vertices, 14, -unit, -unit, +unit);
	SetVertex(Vertices, 15, -unit, -unit, -unit);
	SetUV(UVs, 12, a0, a1);
	SetUV(UVs, 13, a1, a1);
	SetUV(UVs, 14, a0, a0);
	SetUV(UVs, 15, a1, a0);
	
	// Top
	SetVertex(Vertices, 16, -unit, +unit, +unit);
	SetVertex(Vertices, 17, +unit, +unit, +unit);
	SetVertex(Vertices, 18, -unit, +unit, -unit);
	SetVertex(Vertices, 19, +unit, +unit, -unit);
	SetUV(UVs, 16, a0, a1);
	SetUV(UVs, 17, a1, a1);
	SetUV(UVs, 18, a0, a0);
	SetUV(UVs, 19, a1, a0);
	
	// Bottom
	SetVertex(Vertices, 20, -unit, -unit, -unit);
	SetVertex(Vertices, 21, +unit, -unit, -unit);
	SetVertex(Vertices, 22, -unit, -unit, +unit);
	SetVertex(Vertices, 23, +unit, -unit, +unit);
	SetUV(UVs, 20, a0, a1);
	SetUV(UVs, 21, a1, a1);
	SetUV(UVs, 22, a0, a0);
	SetUV(UVs, 23, a1, a0);
	
	for (int i=0; i<24*3; i++) (*Vertices)[i] = VERTTYPEMUL((*Vertices)[i], f2vt(scale));
}

/*****************************************************************************
 * Function		PVRTDestroySkybox
 * Description	Destroy the memory allocated for a skybox
 *****************************************************************************/
void DestroySkybox(VERTTYPE* Vertices, VERTTYPE* UVs)
{
	delete [] Vertices;
	delete [] UVs;
}



