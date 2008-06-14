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
CTexture * Textures;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

// Contains the partice utility class
#include "Particle.h"

// Textures
#include "Media/LightTex.h"
#include "Media/FloorTex8.h"

#define WIDTH 320
#define HEIGHT 480

/******************************************************************************
 Defines
 ******************************************************************************/

#define MAX_PARTICLES 600											// Maximum number of particles
#define FACTOR f2vt(0.25f)											// Brightness of the reflected particles
const VECTOR3 vUp = { f2vt(0.0f), f2vt(1.0f), f2vt(0.0f) };		// Up direction. Used for creating the camera

/******************************************************************************
 Structure definitions
 ******************************************************************************/

struct SVtx
{
	VERTTYPE	x, y, z;						// Position
	unsigned char 	u, v;						// TexCoord
};

#ifdef GL_OES_VERSION_1_1
struct SVtxPointSprite
{
	VERTTYPE	x, y, z, size;
};
#endif

struct SColors
{
	unsigned char	r,g,b,a;						// Color
};

// Texture names
GLuint 			texName;
GLuint 			floorTexName;

// Particle instance pointers
particle particles[MAX_PARTICLES];

// View matrix
MATRIX	g_mView;

// Vectors for calculating the view matrix and saving the camera position
VECTOR3 vFrom, vTo;

// Particle geometry buffers
SVtx ParticleVTXBuf[MAX_PARTICLES*4]; // 4 Vertices per Particle - 2 triangles
SColors NormalColor[MAX_PARTICLES*4];
SColors ReflectColor[MAX_PARTICLES*4];
unsigned short ParticleINDXBuf[MAX_PARTICLES*6]; // 3 indices per triangle

#ifdef GL_OES_VERSION_1_1
SVtxPointSprite	ParticleVTXPSBuf[MAX_PARTICLES]; // When using point sprites
GLuint iVertVboID;
GLuint iColAVboID;
GLuint iColBVboID;
GLuint iQuadVboID;
#endif

VERTTYPE floor_quad_verts[4*4];
VERTTYPE floor_quad_uvs[2*4];
SVtx	 quadVTXBuf[4];

// Dynamic state
int		nNumParticles;
float	fRot, fRot2;
bool	bUsePointSprites;
float	point_attenuation_coef;


float rand_positive_float();
float rand_float();
void render_floor();
void spawn_particle(particle *the_particle);
void render_particle(int NmbrOfParticles, bool reflect);
VERTTYPE clamp(VERTTYPE input);


bool CShell::InitApplication()
{
	//	LOGFUNC("InitApplication()");
	
	AppDisplayText = new CDisplayText;  
	Textures = new CTexture;
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
//		LOG("Display text textures loaded", Logger::LOG_DATA);
				printf("Display text textures loaded\n");
	/*
	 Initializes variables.
	 */
	nNumParticles = 0;
	fRot = 0;
	fRot2 = 0;
	vFrom.x	= f2vt(0.0f); vFrom.y	= f2vt(45.0f); vFrom.z	= f2vt(120.0f);
	vTo.x	= f2vt(0.0f); vTo.y		= f2vt(20.0f); vTo.z	= f2vt(-1.0f);
	
	MATRIX		MyPerspMatrix;
//	float			width  = (float)PVRShellGet(prefWidth);
//	float			height =  (float)PVRShellGet(prefHeight);
//	int				err;
//	SPVRTContext	Context;
	
	// Initialize Extensions
//	glExtensions.Init();
#ifdef GL_OES_VERSION_1_1
	bUsePointSprites = 1;
#else
	bUsePointSprites = 0;
#endif
	
	/*
	 Load textures.
	 */
	if (!Textures->LoadTextureFromPointer((void*)LightTex, &texName))
	{
		//PVRShellSet(prefExitMessage, "ERROR: Cannot load texture\n");
		return false;
	}
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (!Textures->LoadTextureFromPointer((void*)FloorTex8, &floorTexName))
	{
//		PVRShellSet(prefExitMessage, "ERROR: Cannot load texture\n");
		return false;
	}
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	myglTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	/*
	 Creates the projection matrix.
	 */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//if (PVRShellGet(prefIsRotated) && PVRShellGet(prefFullScreen))
	//{
	//	myglRotate(f2vt(90), f2vt(0), f2vt(0), f2vt(1));
	//	width = (float)PVRShellGet(prefHeight);
	//	height = (float)PVRShellGet(prefWidth);
	//}
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(45.0f*(PIf/180.0f)), f2vt((float)WIDTH/(float)HEIGHT), f2vt(10.0f), f2vt(1200.0f), true);
	myglMultMatrix(MyPerspMatrix.f);
	
	/*
	 Calculates the attenuation coefficient for the points drawn.
	 */
	double H = HEIGHT;
	double h = 2.0/MyPerspMatrix.f[5];
	double D0 = sqrt(2.0)*H/h;
	double k = 1.0/(1.0 + 2.0 * (1/MyPerspMatrix.f[5])*(1/MyPerspMatrix.f[5]));
	point_attenuation_coef = (float)(1.0/(D0*D0)*k);
	
	/*
	 Creates the model view matrix.
	 */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	MatrixLookAtRH(g_mView, vFrom, vTo, vUp);
	myglLoadMatrix(g_mView.f);
	
	/*
	 Pre-Set TexCoords since they never change.
	 Pre-Set the Index Buffer.
	 */
	for (int i=0; i<MAX_PARTICLES;i++)
	{
		ParticleVTXBuf[i*4+0].u=0;
		ParticleVTXBuf[i*4+0].v=0;
		
		ParticleVTXBuf[i*4+1].u=1;
		ParticleVTXBuf[i*4+1].v=0;
		
		ParticleVTXBuf[i*4+2].u=0;
		ParticleVTXBuf[i*4+2].v=1;
		
		ParticleVTXBuf[i*4+3].u=1;
		ParticleVTXBuf[i*4+3].v=1;
		
		ParticleINDXBuf[i*6+0]=(i*4)+0;
		ParticleINDXBuf[i*6+1]=(i*4)+1;
		ParticleINDXBuf[i*6+2]=(i*4)+2;
		ParticleINDXBuf[i*6+3]=(i*4)+2;
		ParticleINDXBuf[i*6+4]=(i*4)+1;
		ParticleINDXBuf[i*6+5]=(i*4)+3;
	}
	
	/*
	 If we run OpenGL ES 1.1, then use vertex buffers.
	 */
#ifdef GL_OES_VERSION_1_1
	iVertVboID=0;
	iColAVboID=0;
	iColBVboID=0;
	iQuadVboID=0;
	glGenBuffers(1, &iVertVboID);
	glGenBuffers(1, &iColAVboID);
	glGenBuffers(1, &iColBVboID);
	glGenBuffers(1, &iQuadVboID);
#endif
	
	/*
	 Preset the floor uvs and vertices as they never change.
	 */
	VECTOR3	pos = { 0, 0, 0 };
	
	float szby2 = 100;
	quadVTXBuf[0].x = floor_quad_verts[0]  = pos.x - f2vt(szby2);
	quadVTXBuf[0].y = floor_quad_verts[1]  = pos.y;
	quadVTXBuf[0].z = floor_quad_verts[2]  = pos.z - f2vt(szby2);
	
	quadVTXBuf[1].x = floor_quad_verts[3]  = pos.x + f2vt(szby2);
	quadVTXBuf[1].y = floor_quad_verts[4]  = pos.y;
	quadVTXBuf[1].z = floor_quad_verts[5]  = pos.z - f2vt(szby2);
	
	quadVTXBuf[2].x = floor_quad_verts[6]  = pos.x - f2vt(szby2);
	quadVTXBuf[2].y = floor_quad_verts[7]  = pos.y;
	quadVTXBuf[2].z = floor_quad_verts[8]  = pos.z + f2vt(szby2);
	
	quadVTXBuf[3].x = floor_quad_verts[9]  = pos.x + f2vt(szby2);
	quadVTXBuf[3].y = floor_quad_verts[10] = pos.y;
	quadVTXBuf[3].z = floor_quad_verts[11] = pos.z + f2vt(szby2);
	
	floor_quad_uvs[0] = f2vt(0);
	floor_quad_uvs[1] = f2vt(0);
	quadVTXBuf[0].u = 0;
	quadVTXBuf[0].v = 0;
	
	floor_quad_uvs[2] = f2vt(1);
	floor_quad_uvs[3] = f2vt(0);
	quadVTXBuf[1].u = 255;
	quadVTXBuf[1].v = 0;
	
	floor_quad_uvs[4] = f2vt(0);
	floor_quad_uvs[5] = f2vt(1);
	quadVTXBuf[2].u = 0;
	quadVTXBuf[2].v = 255;
	
	floor_quad_uvs[6] = f2vt(1);
	floor_quad_uvs[7] = f2vt(1);
	quadVTXBuf[3].u = 255;
	quadVTXBuf[3].v = 255;
	
	/*
	 If we run OpenGL ES 1.1, then use vertex buffers.
	 */
#ifdef GL_OES_VERSION_1_1
	glBindBuffer(GL_ARRAY_BUFFER, iQuadVboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SVtx)*4, quadVTXBuf,GL_STATIC_DRAW);
#endif
	
	return true;
}

bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;

	// Release textures
	Textures->ReleaseTexture(texName);
	Textures->ReleaseTexture(floorTexName);
	
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
	int				i;
	VECTOR3		TPos;
	MATRIX		RotationMatrixY;
	
	// Set up the viewport
	glViewport(0,0, WIDTH, HEIGHT);
	
	// Clear color and depth buffers
	myglClearColor(f2vt(0), f2vt(0), f2vt(0), f2vt(0));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Enables depth testing
	glEnable(GL_DEPTH_TEST);
	
	/*
	 Modify per-frame variables controlling the particle mouvements.
	 */
	float speedctrl = (float) (FSIN(fRot*0.01f)+1.0f)/2.0f;
	float stopnum = 0.8f;
	float step = 0.1f;
	if (speedctrl > stopnum)
	{
		step = 0.0f;
	}
	
	/*
	 Generate particles as needed.
	 */
	if ((nNumParticles < MAX_PARTICLES) && (speedctrl <= stopnum))
	{
		int num_to_gen = (int) (rand_positive_float()*(MAX_PARTICLES/100.0));
		
		if (num_to_gen==0)
		{
			num_to_gen=1;
		}
		
		for (i = 0; (i < num_to_gen) && (nNumParticles < MAX_PARTICLES); i++)
		{
			spawn_particle(&particles[nNumParticles++]);
		}
	}
	
	/*
	 Build rotation matrix around axis Y.
	 */
	MatrixRotationY(RotationMatrixY, f2vt((fRot2*PIf)/180.0f));
	VERTTYPE pMatrix[16];
	for(i=0; i<16;i++)
	{
		pMatrix[i] = RotationMatrixY.f[i];
	}
	
	
	if(!bUsePointSprites)
	{
		for(i = 0; i < nNumParticles; i++)
		{
			/*
			 Transform particle with rotation matrix.
			 */
			TPos.x =		VERTTYPEMUL(pMatrix[ 0],particles[i].Position.x) +
			VERTTYPEMUL(pMatrix[ 4],particles[i].Position.y) +
			VERTTYPEMUL(pMatrix[ 8],particles[i].Position.z) +
			pMatrix[12];
			TPos.y =		VERTTYPEMUL(pMatrix[ 1],particles[i].Position.x) +
			VERTTYPEMUL(pMatrix[ 5],particles[i].Position.y) +
			VERTTYPEMUL(pMatrix[ 9],particles[i].Position.z) +
			pMatrix[13];
			TPos.z =		VERTTYPEMUL(pMatrix[ 2],particles[i].Position.x) +
			VERTTYPEMUL(pMatrix[ 6],particles[i].Position.y) +
			VERTTYPEMUL(pMatrix[10],particles[i].Position.z) +
			pMatrix[14];
			
			/*
			 Creates the particle geometry.
			 */
			VERTTYPE szby2 = particles[i].size;
			
			ParticleVTXBuf[i*4+0].x  = TPos.x - szby2;
			ParticleVTXBuf[i*4+0].y  = TPos.y - szby2;
			ParticleVTXBuf[i*4+0].z  = TPos.z;
			
			ParticleVTXBuf[i*4+1].x  = TPos.x + szby2;
			ParticleVTXBuf[i*4+1].y  = TPos.y - szby2;
			ParticleVTXBuf[i*4+1].z  = TPos.z;
			
			ParticleVTXBuf[i*4+2].x  = TPos.x - szby2;
			ParticleVTXBuf[i*4+2].y  = TPos.y + szby2;
			ParticleVTXBuf[i*4+2].z  = TPos.z;
			
			ParticleVTXBuf[i*4+3].x  = TPos.x + szby2;
			ParticleVTXBuf[i*4+3].y  = TPos.y + szby2;
			ParticleVTXBuf[i*4+3].z  = TPos.z;
			
			NormalColor[i*4+0].r  = vt2b(particles[i].Color.x);
			NormalColor[i*4+0].g  = vt2b(particles[i].Color.y);
			NormalColor[i*4+0].b  = vt2b(particles[i].Color.z);
			NormalColor[i*4+0].a  = (unsigned char)255;
			
			NormalColor[i*4+1].r  = vt2b(particles[i].Color.x);
			NormalColor[i*4+1].g  = vt2b(particles[i].Color.y);
			NormalColor[i*4+1].b  = vt2b(particles[i].Color.z);
			NormalColor[i*4+1].a  = (unsigned char)(255);
			
			NormalColor[i*4+2].r  = vt2b(particles[i].Color.x);
			NormalColor[i*4+2].g  = vt2b(particles[i].Color.y);
			NormalColor[i*4+2].b  = vt2b(particles[i].Color.z);
			NormalColor[i*4+2].a  = (unsigned char)(255);
			
			NormalColor[i*4+3].r  = vt2b(particles[i].Color.x);
			NormalColor[i*4+3].g  = vt2b(particles[i].Color.y);
			NormalColor[i*4+3].b  = vt2b(particles[i].Color.z);
			NormalColor[i*4+3].a  = (unsigned char)(255);
			
			ReflectColor[i*4+0].r  = vt2b(VERTTYPEMUL(particles[i].Color.x,FACTOR));
			ReflectColor[i*4+0].g  = vt2b(VERTTYPEMUL(particles[i].Color.y,FACTOR));
			ReflectColor[i*4+0].b  = vt2b(VERTTYPEMUL(particles[i].Color.z,FACTOR));
			ReflectColor[i*4+0].a  = (unsigned char)(255);
			
			ReflectColor[i*4+1].r  = vt2b(VERTTYPEMUL(particles[i].Color.x,FACTOR));
			ReflectColor[i*4+1].g  = vt2b(VERTTYPEMUL(particles[i].Color.y,FACTOR));
			ReflectColor[i*4+1].b  = vt2b(VERTTYPEMUL(particles[i].Color.z,FACTOR));
			ReflectColor[i*4+1].a  = (unsigned char)(255);
			
			ReflectColor[i*4+2].r  = vt2b(VERTTYPEMUL(particles[i].Color.x,FACTOR));
			ReflectColor[i*4+2].g  = vt2b(VERTTYPEMUL(particles[i].Color.y,FACTOR));
			ReflectColor[i*4+2].b  = vt2b(VERTTYPEMUL(particles[i].Color.z,FACTOR));
			ReflectColor[i*4+2].a  = (unsigned char)(255);
			
			ReflectColor[i*4+3].r  = vt2b(VERTTYPEMUL(particles[i].Color.x,FACTOR));
			ReflectColor[i*4+3].g  = vt2b(VERTTYPEMUL(particles[i].Color.y,FACTOR));
			ReflectColor[i*4+3].b  = vt2b(VERTTYPEMUL(particles[i].Color.z,FACTOR));
			ReflectColor[i*4+3].a  = (unsigned char)(255);
		}
	}
	
	/*
	 Setup VertexBuffer for particles if we are using OpenGL ES 1.1.
	 */
#ifdef GL_OES_VERSION_1_1
	else
	{
		for(i = 0; i < nNumParticles; i++)
		{
			/* Transform particle with rotation matrix */
			
			ParticleVTXPSBuf[i].x =		VERTTYPEMUL(pMatrix[ 0],particles[i].Position.x) +
			VERTTYPEMUL(pMatrix[ 4],particles[i].Position.y) +
			VERTTYPEMUL(pMatrix[ 8],particles[i].Position.z) +
			pMatrix[12];
			ParticleVTXPSBuf[i].y =		VERTTYPEMUL(pMatrix[ 1],particles[i].Position.x) +
			VERTTYPEMUL(pMatrix[ 5],particles[i].Position.y) +
			VERTTYPEMUL(pMatrix[ 9],particles[i].Position.z) +
			pMatrix[13];
			ParticleVTXPSBuf[i].z =		VERTTYPEMUL(pMatrix[ 2],particles[i].Position.x) +
			VERTTYPEMUL(pMatrix[ 6],particles[i].Position.y) +
			VERTTYPEMUL(pMatrix[10],particles[i].Position.z) +
			pMatrix[14];
			
			ParticleVTXPSBuf[i].size = particles[i].size;
			
			NormalColor[i].r  = vt2b(particles[i].Color.x);
			NormalColor[i].g  = vt2b(particles[i].Color.y);
			NormalColor[i].b  = vt2b(particles[i].Color.z);
			NormalColor[i].a  = (unsigned char)255;
			
			ReflectColor[i].r  = vt2b(VERTTYPEMUL(particles[i].Color.x,FACTOR));
			ReflectColor[i].g  = vt2b(VERTTYPEMUL(particles[i].Color.y,FACTOR));
			ReflectColor[i].b  = vt2b(VERTTYPEMUL(particles[i].Color.z,FACTOR));
			ReflectColor[i].a  = (unsigned char)255;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, iVertVboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SVtxPointSprite)*nNumParticles, ParticleVTXPSBuf,GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, iColAVboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SColors)*nNumParticles, NormalColor,GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, iColBVboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SColors)*nNumParticles, ReflectColor,GL_DYNAMIC_DRAW);
#endif
	
	// clean up render states
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	
	/*
	 Draw floor.
	 */
	
	// Save modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	myglRotate(f2vt(-fRot), f2vt(0.0f), f2vt(1.0f), f2vt(0.0f));
	
	// setup render states
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	
	// Set texture and texture environment
	glBindTexture(GL_TEXTURE_2D, floorTexName);
	myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	// Render floor
	render_floor();
	
	// clean up render states
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glPopMatrix();
	
	
	/*
	 Render particles reflections.
	 */
	
	// set up render states
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	myglTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, texName);
	
	// Set model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	myglScale(f2vt(1.0f), f2vt(-1.0f), f2vt(1.0f));
	myglTranslate(f2vt(0.0f), f2vt(0.01f), f2vt(0.0f));
	
#ifdef GL_OES_VERSION_1_1
	glEnable(GL_POINT_SPRITE_OES);
#endif
	if (((int)(nNumParticles*0.5f))>0)
	{
		render_particle(((int)(nNumParticles*0.5f)),true);
	}
	glPopMatrix();
	
	/*
	 Render particles.
	 */
	
	// Sets the model view matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	if (nNumParticles>0)
	{
        render_particle(nNumParticles,false);
	}
	
	glPopMatrix();
#ifdef GL_OES_VERSION_1_1
	glDisable(GL_POINT_SPRITE_OES);
#endif
	
	VECTOR3 Force = { f2vt(0.0f), f2vt(0.0f), f2vt(0.0f) };
	Force.x = f2vt(1000.0f*(float)FSIN(fRot*0.01f));
	
	for(i = 0; i < nNumParticles; i++)
	{
		/*
		 Move the particle.
		 If the particle exceeds its lifetime, create a new one in its place.
		 */
		if(particles[i].step(f2vt(step), Force))
		{
			spawn_particle(&particles[i]);
		}
	}
	
	// clean up render states
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	
	// Increase rotation angles
	fRot += 1;
	fRot2 = fRot + 36;
	
	// Unbinds the vertex buffer if we are using OpenGL ES 1.1
#ifdef GL_OES_VERSION_1_1
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
	
	
	if(!bUsePointSprites)
	{
		// show text on the display
		AppDisplayText->DisplayDefaultTitle("Particle System", "", eDisplayTextLogoIMG);
	}
#ifdef GL_OES_VERSION_1_1
	else
	{
		// show text on the display
		AppDisplayText->DisplayDefaultTitle("Particles", "(using point sprites with buffer objects)", eDisplayTextLogoIMG);
	}
#endif

	AppDisplayText->Flush();	
	
	return true;
}

/*!****************************************************************************
 @Function		rand_float
 @Return		float		random float from -1 to 1
 @Description	returns a random float in range -1 to 1.
 ******************************************************************************/
float rand_float()
{
	return (rand()/(float)RAND_MAX) * 2.0f - 1.0f;
}

/*!****************************************************************************
 @Function		rand_positive_float
 @Return		float		random float from 0 to 1
 @Description	returns a random float in range 0 to 1.
 ******************************************************************************/
float rand_positive_float()
{
	return rand()/(float)RAND_MAX;
}

/*!****************************************************************************
 @Function		spawn_particle
 @Output		the_particle	particle to initialize
 @Description	initializes the specified particle with randomly chosen parameters.
 ******************************************************************************/
void spawn_particle(particle *the_particle)
{
	VECTOR3	ParticleSource = { f2vt(0), f2vt(0), f2vt(0) };
	VECTOR3	ParticleSourceVariability = { f2vt(1), f2vt(0), f2vt(1) };
	VECTOR3	ParticleVelocity = { f2vt(0), f2vt(30), f2vt(0) };
	VECTOR3	ParticleVelocityVariability = { f2vt(4), f2vt(15), f2vt(4) };
	VERTTYPE particle_lifetime = f2vt(8);
	VERTTYPE particle_lifetime_variability = f2vt(1.0);
	float particle_mass = 100;
	float particle_mass_variability = 0;
	float t;
	
	/*
	 Creates the particle position.
	 */
	VECTOR3 Pos;
	t = rand_float();
	Pos.x = ParticleSource.x + VERTTYPEMUL(f2vt(t),ParticleSourceVariability.x);
	t = rand_float();
	Pos.y = ParticleSource.y + VERTTYPEMUL(f2vt(t),ParticleSourceVariability.y);
	t = rand_float();
	Pos.z = ParticleSource.z + VERTTYPEMUL(f2vt(t),ParticleSourceVariability.z);
	
	/*
	 Creates the particle velocity.
	 */
	VECTOR3 Vel;
	t = rand_float();
	Vel.x = ParticleVelocity.x + VERTTYPEMUL(f2vt(t),ParticleVelocityVariability.x);
	t = rand_float();
	Vel.y = ParticleVelocity.y + VERTTYPEMUL(f2vt(t),ParticleVelocityVariability.y);
	t = rand_float();
	Vel.z = ParticleVelocity.z + VERTTYPEMUL(f2vt(t),ParticleVelocityVariability.z);
	
	/*
	 Creates the particle lifetime and mass.
	 */
	VERTTYPE life = particle_lifetime + VERTTYPEMUL(f2vt(rand_float()), particle_lifetime_variability);
	float mass = particle_mass + rand_float() * particle_mass_variability;
	
	/*
	 Creates the particle from these characteristics.
	 */
	*the_particle = particle(Pos,Vel,mass,life);
	
	/*
	 Creates the particle colors.
	 */
	VECTOR3 ParticleInitialColor = { f2vt(0.6f*255.0f), f2vt(0.5f*255.0f), f2vt(0.5f*255.0f) };
	VECTOR3 ParticleInitialColorVariability = { f2vt(0.2f*255.0f), f2vt(0.2f*255.0f), f2vt(0.2f*255.0f) };
	
	VECTOR3 ParticleHalfwayColor = { f2vt(1.0f*255.0f), f2vt(0.0f), f2vt(0.0f) };
	VECTOR3 ParticleHalfwayColorVariability = { f2vt(0.8f*255.0f), f2vt(0.0f), f2vt(0.3f*255.0f) };
	
	VECTOR3 ParticleEndColor = { f2vt(0.0f), f2vt(0.0f), f2vt(0.0f) };
	VECTOR3 ParticleEndColorVariability = { f2vt(0.0f), f2vt(0.0f), f2vt(0.0f) };
	
	VERTTYPE randomvalue = f2vt(rand_float());
	the_particle->Color.x = the_particle->Initial_Color.x = clamp(ParticleInitialColor.x + VERTTYPEMUL(ParticleInitialColorVariability.x,randomvalue));
	the_particle->Color.y = the_particle->Initial_Color.y = clamp(ParticleInitialColor.y + VERTTYPEMUL(ParticleInitialColorVariability.y,randomvalue));
	the_particle->Color.z = the_particle->Initial_Color.z = clamp(ParticleInitialColor.z + VERTTYPEMUL(ParticleInitialColorVariability.z,randomvalue));
	
	t = rand_float();
	the_particle->Halfway_Color.x = clamp(ParticleHalfwayColor.x + VERTTYPEMUL(f2vt(t),ParticleHalfwayColorVariability.x));
	t = rand_float();
	the_particle->Halfway_Color.y = clamp(ParticleHalfwayColor.y + VERTTYPEMUL(f2vt(t),ParticleHalfwayColorVariability.y));
	t = rand_float();
	the_particle->Halfway_Color.z = clamp(ParticleHalfwayColor.z + VERTTYPEMUL(f2vt(t),ParticleHalfwayColorVariability.z));
	
	t = rand_float();
	the_particle->End_Color.x = clamp(ParticleEndColor.x + VERTTYPEMUL(f2vt(t),ParticleEndColorVariability.x));
	t = rand_float();
	the_particle->End_Color.y = clamp(ParticleEndColor.y + VERTTYPEMUL(f2vt(t),ParticleEndColorVariability.y));
	t = rand_float();
	the_particle->End_Color.z = clamp(ParticleEndColor.z + VERTTYPEMUL(f2vt(t),ParticleEndColorVariability.z));
	
	/*
	 Creates the particle size using a perturbation.
	 */
	VERTTYPE particle_size = f2vt(2.0f);
	VERTTYPE particle_size_variation = f2vt(1.5f);
	t = rand_float();
	the_particle->size = particle_size + VERTTYPEMUL(f2vt(t),particle_size_variation);
}

/*!****************************************************************************
 @Function		render_particle
 @Input			NmbrOfParticles		number of particles to initialize
 @Input			bReflect			should we use the reflection color ?
 @Description	Renders the specified set of particles, optionally using the
 reflection color.
 ******************************************************************************/
void render_particle(int NmbrOfParticles, bool bReflect)
{
	if(!bUsePointSprites)
	{
		/*
		 If we are not using point sprites,
		 draw the regular particles geometry.
		 */
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,VERTTYPEENUM,sizeof(SVtx),&ParticleVTXBuf[0].x);
		
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2,GL_BYTE,sizeof(SVtx),&ParticleVTXBuf[0].u);
		
		glEnableClientState(GL_COLOR_ARRAY);
		if(bReflect)
		{
	        glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(SColors),&ReflectColor[0].r);
		}
		else
		{
			glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(SColors),&NormalColor[0].r);
		}
		
		glDrawElements(GL_TRIANGLES, NmbrOfParticles*6, GL_UNSIGNED_SHORT, &ParticleINDXBuf[0] );
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}
#ifdef GL_OES_VERSION_1_1
	else
	{
		/*
		 If point sprites are availables,
		 use them to draw the particles.
		 */
		glBindBuffer(GL_ARRAY_BUFFER, iVertVboID);
		
		glEnableClientState(GL_VERTEX_ARRAY);
		//glVertexPointer(3,VERTTYPEENUM,sizeof(SVtxPointSprite),&ParticleVTXPSBuf[0].x);  // Old Point Sprite (nonVBO code)
		glVertexPointer(3,VERTTYPEENUM,sizeof(SVtxPointSprite),0);
		
		myglTexEnv( GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE );
		
		glEnableClientState(GL_POINT_SIZE_ARRAY_OES);
		//glPointSizePointerOES(VERTTYPEENUM,sizeof(SVtxPointSprite),&ParticleVTXPSBuf[0].size);  // Old Point Sprite (nonVBO code)
		glPointSizePointerOES(VERTTYPEENUM,sizeof(SVtxPointSprite),(GLvoid*) (sizeof(VERTTYPE)*3));
		
#ifndef PVRTFIXEDPOINTENABLE
		float coefs[4] = { 0, 0, point_attenuation_coef, 0 };
		glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION,coefs);
#else
		// Note: point_attenuation_coef will be too small to represent as a fixed point number,
		// So use an approximation to the attenuation (fixed attenuation of 0.01) instead.
		VERTTYPE coefs[4] = { f2vt(0.01f), f2vt(0.0f), f2vt(0.0f), f2vt(0.0f) };
		myglPointParameterv(GL_POINT_DISTANCE_ATTENUATION,coefs);
#endif
		glEnableClientState(GL_COLOR_ARRAY);
		if(bReflect)
		{
			glBindBuffer(GL_ARRAY_BUFFER, iColBVboID);
			//glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(SColors),&ReflectColor[0].r);  // Old Point Sprite (nonVBO code)
			glColorPointer(4,GL_UNSIGNED_BYTE,0,0);
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, iColAVboID);
			//glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(SColors),&NormalColor[0].r);  // Old Point Sprite (nonVBO code)
			glColorPointer(4,GL_UNSIGNED_BYTE,0,0);
		}
		
		glDrawArrays(GL_POINTS, 0, NmbrOfParticles);
		
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_POINT_SIZE_ARRAY_OES);
		glDisableClientState(GL_COLOR_ARRAY);
	}
#endif
}

/*!****************************************************************************
 @Function		clamp
 @Input			X			number to clamp
 @Return		VERTTYPE	clamped number
 @Description	Clamps the argument to 0-255.
 ******************************************************************************/
VERTTYPE clamp(VERTTYPE X)
{
	if (X<f2vt(0.0f))
	{
		X=f2vt(0.0f);
	}
	else if(X>f2vt(255.0f))
	{
		X=f2vt(255.0f);
	}
	
	return X;
}

/*!****************************************************************************
 @Function		render_floor
 @Description	Renders the floor as a quad.
 ******************************************************************************/
void render_floor()
{
#ifndef GL_OES_VERSION_1_1
	/*
	 If we run OpenGL ES 1.1,
	 draw the floor using vertex buffers.
	 */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,VERTTYPEENUM,0,floor_quad_verts);
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2,VERTTYPEENUM,0,floor_quad_uvs);
	
	myglColor4(f2vt(1), f2vt(1), f2vt(1), f2vt(0.5));
	
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
	/*
	 Draw the floor using regular geometry for the quad.
	 */
	glBindBuffer(GL_ARRAY_BUFFER, iQuadVboID);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3,VERTTYPEENUM,sizeof(SVtx),0);
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2,GL_BYTE,sizeof(SVtx),(const GLvoid*) (3*sizeof(VERTTYPE)));
	
	myglColor4(f2vt(1), f2vt(1), f2vt(1), f2vt(0.5));
	
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}


