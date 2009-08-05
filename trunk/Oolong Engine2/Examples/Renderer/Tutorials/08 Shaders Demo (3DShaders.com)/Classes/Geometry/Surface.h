/*
 *  Surface.h
 *  SkeletonXIB
 *
 *  Created by Jim on 8/1/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __SURFACE_H__
#define __SURFACE_H__

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

class CParametricSurface  {
public:
	virtual void Eval(Vec2& domain, Vec3& range) = 0;
    virtual bool Flip(const Vec2& domain) { return false; }	
    virtual void Vertex(Vec2 domain);
	
	GLuint GenVBO( int slices );
	int totalVertex;
protected:
    bool flipped;
    float du, dv;	
	float vertexBuffer[12000*14];

};

class CKlein : public CParametricSurface {
public:
	void Eval(Vec2& domain, Vec3& range);
    bool Flip(const Vec2& domain);	
};

#endif // __SURFACE_H__