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
#include "MemoryManager.h"
#include "UI.h"
#include "Macros.h"
#include "Timing.h"
#include "Pathes.h"
#include "ResourceFile.h"
#include "Surface.h"
#include "CShader.h"
#include "TouchScreen.h"

#include <stdio.h>
#include <sys/time.h>

CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

structTimer DrawCubeTimer;
float DrawCubeT;

structTimer DrawUITimer;
float DrawUIT;

char *shaders_name[] = {
	(char *)"Color",
	(char *)"ColorVertexLight",
	(char *)"ColorLight",
	(char *)"Texture",
	(char *)"TextureLight",
	(char *)"NormalMap",
	(char *)"Parallax",
	(char *)"Wobble",
	(char *)"Brick",
	(char *)"Polkadot3D",
	(char *)"Cloud1",
	(char *)"Toon",
	(char *)"VertexNoise",
	(char *)"Fur",
	(char *)"Eroded",
};
int total_shaders = sizeof(shaders_name) / sizeof(shaders_name[0]);
int active_shaders;


CShader *pShader[256];

CTexture *texture1;
GLuint texID_rock = -1;
GLuint texID_rockNormal = -1;
GLuint texID_HUDLeft = -1;
GLuint texID_HUDRight = -1;

float light_position[4] = { 0, 0, 0, 0 };
float light_direction[4] = { 0, 0, 1, 0 };			// Directional light
float light_ambient[] = { 0.3, 0.3, 0.3, 1.0 };
float light_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
float light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

float material_ambient[] = { 0.3, 0.3, 0.3, 1.0 };
float material_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
float material_specular[] = { 0.5, 0.5, 0.5, 1.0 };
float material_shininess = 16;

// Declare the fragment and vertex shaders.
static float noise1_inc = 0.2;


bool bTouching = false;

float NoiseOffset[] = { 0, 0, 0 };
	
GLuint ui32Vbo = 0;					    // Vertex buffer object handle
GLuint ui32HUD = 0;					    // Vertex buffer object handle
int nVertexVbo = 0;

// View Angle for animation
float			m_fAngleY;

// Matrices for Views, Projections, mViewProjection.
MATRIX mView, mProjection, mViewProjection, mOrthoProj, mMVP, mModel, mModelView, mRotate, mRotateY, mRotateX;
Vec3 vTo, vFrom;

bool CShell::InitApplication()
{
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth,FALSE))
		printf("Display text textures loaded\n");
	
	StartTimer(&DrawCubeTimer);
	StartTimer(&DrawUITimer);
	
	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000
	if( __OPENGLES_VERSION >= 2 )
	{
		for( int i=0; i<total_shaders; i++ ) {
			char vertShaderName[64];
			char fragShaderName[64];
			
			sprintf( vertShaderName, "/%s.vert", shaders_name[i] );
			sprintf( fragShaderName, "/%s.frag", shaders_name[i] );
			pShader[i] = new CShader( vertShaderName, fragShaderName );
		}
		active_shaders = 2;
	}
#endif
	texture1 = new CTexture();
	texture1->LoadTextureFromImageFile( "/rock.png", &texID_rock );
	texture1->LoadTextureFromImageFile( "/rockNH.png", &texID_rockNormal );
	texture1->LoadTextureFromImageFile( "/left_button.png", &texID_HUDLeft );
	texture1->LoadTextureFromImageFile( "/right_button.png", &texID_HUDRight );
	
	// We're going to draw a cube, so let's create a vertex buffer first!
	{
		CKlein *pKlein = new CKlein();
		ui32Vbo = pKlein->GenVBO(48);
		nVertexVbo = pKlein->totalVertex;
	}
	
	{
		// Create Quad 128 x 128 for HUD (Head Up Display)
		GLfloat verts[] = 
		{
			 32.0f, 32.0f, 0.0f,		1.0f, 0.0f,
			-32.0f, 32.0f, 0.0f,		0.0f, 0.0f,
			 32.0f,-32.0f, 0.0f,		1.0f, 1.0f,
			-32.0f,-32.0f, 0.0f,		0.0f, 1.0f
		};
		
		glGenBuffers(1, &ui32HUD);
		glBindBuffer(GL_ARRAY_BUFFER, ui32HUD);
		
		unsigned int uiSize = 4 * (sizeof(GLfloat) * 5);
		glBufferData(GL_ARRAY_BUFFER, uiSize, verts, GL_STATIC_DRAW);
	}
	
	
	vTo = Vec3(0.0f, 0.0f, 150.0f);
	vFrom = Vec3(0.0f, 0.0f, 0.0f);
	MatrixLookAtRH(mView, vFrom, vTo, Vec3(0,1,0));
	
	MatrixPerspectiveFovRH(mProjection, f2vt(40*PIf/180.0f), f2vt(((float) 320 / (float) 480)), f2vt(0.1f), f2vt(1000.0f), 0);
	MatrixOrthoRH(mOrthoProj, 320, 480, -1, 1, 0);
	
	m_fAngleY = 0.0;	
	
	return true;
}

bool CShell::QuitApplication()
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 )
	{
		// Frees the OpenGL handles for the program and the 2 shaders
		for( int i=0; i<total_shaders; i++ ) {
			delete( pShader[i] );
		}
	}
#endif
	
	// Delete the VBO as it is no longer needed
	glDeleteBuffers(1, &ui32Vbo);
	
	AppDisplayText->ReleaseTextures();
	delete AppDisplayText;
	
	return true;
}

bool CShell::UpdateScene()
{		
	ResetTimer(&DrawCubeTimer);
	
	TouchScreenValues* touchValue = GetValuesTouchScreen();
	if( !bTouching && !touchValue->TouchesEnd ) {
		bTouching = true;
	}
	if( bTouching ) {
		if( touchValue->TouchesEnd ) {
			bTouching = false;
			if( touchValue->LocationXTouchesEnded > 160 ) {
				active_shaders++;
				if( active_shaders >= total_shaders )
					active_shaders = 0;
			}
			else {
				active_shaders--;
				if( active_shaders < 0 ) 
					active_shaders = total_shaders-1;
			}
			NoiseOffset[0] = 0;
			NoiseOffset[1] = 0;
		}
	}
	
	// Sets the clear color.
	// The colours are passed per channel (red,green,blue,alpha) as float values from 0.0 to 1.0
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Rotate the model matrix
	MatrixIdentity(mModel);
	MatrixIdentity(mRotate);
	MatrixIdentity(mRotateY);
	MatrixIdentity(mRotateX);
	// Set the model 10 units back into the screen.
	MatrixTranslation(mModel, 0.0f, 0.0f, 4.5f);
	// Rotate the model by a small amount.
	MatrixRotationY(mRotateY, m_fAngleY);
	//MatrixRotationX(mRotateX, m_fAngleY);
	MatrixMultiply(mRotate, mRotateY, mRotateX);
	MatrixMultiply(mModel, mRotate, mModel);
	

	// Calculate view projection matrix.
	MatrixMultiply(mViewProjection, mView, mProjection);
	
	// Calculate model view projection matrix
	MatrixMultiply(mMVP, mModel, mViewProjection);
	MatrixMultiply(mModelView, mModel, mView);
	

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000
	if( __OPENGLES_VERSION >= 2 ) {
		MATRIX mTIM;
		MatrixInverse( mTIM, mModel );
		MatrixTranspose( mTIM, mTIM );
		
		glUseProgram(pShader[active_shaders]->uiProgramObject);

		// Then passes the matrix to that variable
		glUniformMatrix4fv( pShader[active_shaders]->PMVMatrixHandle, 1, GL_FALSE, mMVP.f);
		glUniformMatrix4fv( pShader[active_shaders]->MVMatrixHandle, 1, GL_FALSE, mModelView.f);
		glUniformMatrix4fv( pShader[active_shaders]->ModelMatrixHandle, 1, GL_FALSE, mModel.f);
		glUniformMatrix4fv( pShader[active_shaders]->ViewMatrixHandle, 1, GL_FALSE, mView.f);
		glUniformMatrix4fv( pShader[active_shaders]->TIMMatrixHandle, 1, GL_FALSE, mTIM.f);
		
		glUniform4f( pShader[active_shaders]->EyePositionHandle, 0, 0, 0, 1 );
		glUniform3f( pShader[active_shaders]->LightPositionHandle, light_position[0], light_position[1], light_position[2] );
		glUniform3f( pShader[active_shaders]->LightDirectionHandle, light_direction[0], light_direction[1], light_direction[2] );
		
		glUniform4f( pShader[active_shaders]->AmbientICHandle, material_ambient[0]*light_ambient[0], material_ambient[1]*light_ambient[1], material_ambient[2]*light_ambient[2], material_ambient[3]*light_ambient[3]);
		glUniform4f( pShader[active_shaders]->DiffuseICHandle, material_diffuse[0]*light_diffuse[0], material_diffuse[1]*light_diffuse[1], material_diffuse[2]*light_diffuse[2], material_diffuse[3]*light_diffuse[3]);
		glUniform4f( pShader[active_shaders]->SpecularICHandle, material_specular[0]*light_specular[0], material_specular[1]*light_specular[1], material_specular[2]*light_specular[2], material_specular[3]*light_specular[3]);
		
		glUniform1f( pShader[active_shaders]->MaterialShininessHandle, material_shininess);
		
		glUniform3f( pShader[active_shaders]->NoiseOffsetHandle, NoiseOffset[0], NoiseOffset[1], NoiseOffset[2] );

	}
	else 
#endif
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(mProjection.f);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(mView.f);
		glMultMatrixf(mModel.f);
	}
	
	++frames;
	CFTimeInterval			TimeInterval;
	CFTimeInterval			frameTime;
	frameRate = GetFps(frames, TimeInterval, &frameTime);

	m_fAngleY -= 1.0f * frameTime;

	NoiseOffset[0] += 0.1 * frameTime;
	
	NoiseOffset[1] += noise1_inc * frameTime;
	if( NoiseOffset[1] >= 1.0 && noise1_inc > 0 ) 
		noise1_inc = -noise1_inc;
	else if( NoiseOffset[1] < 0 && noise1_inc < 0 )
		noise1_inc = -noise1_inc;
	
	//if( NoiseOffset[0] > 1.0 )
	//	NoiseOffset[0] -= 1.0;

	AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f Cube: %3.2fms UI: %3.2fms", frameRate, DrawCubeT, DrawUIT);
	
	return true;
}

void DrawHUD()
{	
	glDisable( GL_DEPTH_TEST );
	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 ) 
	{
		glUseProgram(pShader[3]->uiProgramObject);
		
		glUniformMatrix4fv( pShader[3]->PMVMatrixHandle, 1, GL_FALSE, mOrthoProj.f);
		
		glBindBuffer(GL_ARRAY_BUFFER, ui32HUD);
		glEnableVertexAttribArray(VERTEX_ARRAY);
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 20, 0);	// Stride = 20 bytes
		
		glEnableVertexAttribArray(TEXCOORD_ARRAY);
		glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, 20, (void *)12);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glActiveTexture( GL_TEXTURE0 );
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID_HUDLeft);
		glUniform1i( pShader[3]->TextureHandle, 0 );
		
		glUniform4f(pShader[3]->ColorHandle, 1.0, 1.0, 1.0, 1.0);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		
		MATRIX mat, mMove;
		MatrixTranslation(mMove, -160+32, -240+32, 0);
		MatrixMultiply(mat, mMove, mOrthoProj);
		glUniformMatrix4fv( pShader[3]->PMVMatrixHandle, 1, GL_FALSE, mat.f);
		
		glDrawArrays(GL_TRIANGLE_STRIP,  0, 4);
		
		glBindTexture(GL_TEXTURE_2D, texID_HUDRight);
		glUniform1i( pShader[3]->TextureHandle, 0 );
		MatrixTranslation(mMove, +160-32, -240+32, 0);
		MatrixMultiply(mat, mMove, mOrthoProj);
		glUniformMatrix4fv( pShader[3]->PMVMatrixHandle, 1, GL_FALSE, mat.f);
		
		glDrawArrays(GL_TRIANGLE_STRIP,  0, 4);
	}
	else
#endif
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(mOrthoProj.f);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glActiveTexture( GL_TEXTURE0 );
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID_HUDLeft);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32HUD);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 20, 0);	// Stride = 20 bytes
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		glTexCoordPointer(2, GL_FLOAT, 20, (void *)12);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glDisableClientState(GL_COLOR_ARRAY);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		glDrawArrays(GL_TRIANGLE_STRIP,  0, 4);
	}	
	glActiveTexture( GL_TEXTURE0 );
	glDisable(GL_TEXTURE_2D);
}

bool CShell::RenderScene()
{

	
	
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000	
	if( __OPENGLES_VERSION >= 2 ) 
	{
		glActiveTexture( GL_TEXTURE0 );
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID_rockNormal);
		
		glActiveTexture( GL_TEXTURE1 );
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texID_rock);
		
		glUseProgram(pShader[active_shaders]->uiProgramObject);
		
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		
		/*
		 Enable the custom vertex attribute at index VERTEX_ARRAY.
		 We previously just bound that index to the variable in our shader "vec4 MyVertex;"
		 */
		glEnableVertexAttribArray(VERTEX_ARRAY);
		glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 14*4, 0);
		
		glEnableVertexAttribArray(TEXCOORD_ARRAY);
		glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, 14*4, (void *)12);
		
		glEnableVertexAttribArray(NORMAL_ARRAY);
		glVertexAttribPointer(NORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, 14*4, (void *)20);	
		
		glEnableVertexAttribArray(TANGENT_ARRAY);
		glVertexAttribPointer(TANGENT_ARRAY, 3, GL_FLOAT, GL_FALSE, 14*4, (void *)32);	

		glEnableVertexAttribArray(BINORMAL_ARRAY);
		glVertexAttribPointer(BINORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, 14*4, (void *)44);	
		
		glUniform1i( pShader[active_shaders]->TextureHandle, 1 );
		glUniform1i( pShader[active_shaders]->NormalMapHandle, 0 );

		if( strcmp(shaders_name[active_shaders], "Color") == 0 ) {
			glUniform4f(pShader[active_shaders]->ColorHandle, 1.0, 0.0, 0.0, 1.0);
		}
		else
			glUniform4f(pShader[active_shaders]->ColorHandle, 1.0, 1.0, 1.0, 1.0);
		
		if( strcmp(shaders_name[active_shaders], "Fur") == 0 ) {
			float SHELLS = 5;
			float FUR_HEIGHT = .125;
			float SHELL_TRANSPARENCY = .35;
			
			glDisable(GL_BLEND);
			
			glUniform4f( pShader[active_shaders]->DiffuseICHandle, 0.4, 0.4, 0.4, 1.0 );
			glUniform4f( pShader[active_shaders]->SpecularICHandle, 0.05, 0.05, 0.05, 1.0 );
			glUniform1f(glGetUniformLocation(pShader[active_shaders]->uiProgramObject, "material_shininess"), 4);
			glUniform1f(glGetUniformLocation(pShader[active_shaders]->uiProgramObject, "transparency"), 1);
			glUniform1f(glGetUniformLocation(pShader[active_shaders]->uiProgramObject, "furHeight"), 0);
			glUniform1f(glGetUniformLocation(pShader[active_shaders]->uiProgramObject, "spacing"), 1);
			glDrawArrays(GL_TRIANGLE_STRIP,  0, nVertexVbo);
			
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
						
			glUniform1f(glGetUniformLocation(pShader[active_shaders]->uiProgramObject, "transparency"), SHELL_TRANSPARENCY);
			glEnable(GL_CULL_FACE);
			for (int i = 1; i < SHELLS; i++)
			{
				glUniform1f(glGetUniformLocation(pShader[active_shaders]->uiProgramObject, "furHeight"), (float)i / (SHELLS / FUR_HEIGHT));
				glDrawArrays(GL_TRIANGLE_STRIP,  0, nVertexVbo);
			}
			
		}
		else if( strcmp(shaders_name[active_shaders], "Eroded") == 0 ) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			
			glDisable(GL_CULL_FACE);
			glDrawArrays(GL_TRIANGLE_STRIP,  0, nVertexVbo);
		}
		else {
			glDisable(GL_BLEND);
			
			glDrawArrays(GL_TRIANGLE_STRIP,  0, nVertexVbo);
		}
	}
	else
#endif
	{
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 14*4, 0);
		
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, nVertexVbo);
		
		// Bind the VBO so we can fill it with data
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	DrawHUD();

	DrawCubeT = GetAverageTimeValueInMS(&DrawCubeTimer);
	
	ResetTimer(&DrawUITimer);
	
	// show text on the display
	char title[128];
	sprintf( title, "Shaders (%s)", shaders_name[active_shaders] );
	AppDisplayText->DisplayDefaultTitle(title, "", eDisplayTextLogoIMG);
	AppDisplayText->Flush();	
	
	DrawUIT = GetAverageTimeValueInMS(&DrawUITimer);

	return true;
}
