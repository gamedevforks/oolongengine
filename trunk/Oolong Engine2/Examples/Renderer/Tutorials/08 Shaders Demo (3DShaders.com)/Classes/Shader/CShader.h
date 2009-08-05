/*
 *  CShader.h
 *  Shaders
 *
 *  Created by Jim on 8/1/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __CSHADER_H__
#define __CSHADER_H__

#include <TargetConditionals.h>
#include <Availability.h>
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 30000
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#else
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#endif
#include "Mathematics.h"

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0
#define TEXCOORD_ARRAY	1
#define NORMAL_ARRAY	2
#define TANGENT_ARRAY	3
#define BINORMAL_ARRAY	4

class CShader {
public:
	CShader( const char *vertShaderFilename, const char *fragShaderFilename );
	~CShader();
	
	GLuint uiFragShader, uiVertShader;		// Used to hold the fragment and vertex shader handles
	GLuint uiProgramObject;					// Used to hold the program handle (made out of the two previous shaders)

	// Handles for the uniform variables.
	int PMVMatrixHandle;
	int MVMatrixHandle;
	int TIMMatrixHandle;	
	int ModelMatrixHandle;
	int ViewMatrixHandle;
	
	int EyePositionHandle;
	int LightDirectionHandle;
	int LightPositionHandle;
	
	int MaterialShininessHandle;
	
	int AmbientICHandle;
	int DiffuseICHandle;
	int SpecularICHandle;
	
	int ColorHandle;	
	int TextureHandle;
	int NormalMapHandle;	
	
	int NoiseOffsetHandle;	

};


#endif //__CSHADER_H__

