/*
 GameKit .blend file reader for Oolong Engine
 Copyright (c) 2009 Erwin Coumans http://gamekit.googlecode.com
 
 This software is provided 'as-is', without any express or implied warranty.
 In no event will the authors be held liable for any damages arising from the use of this software.
 Permission is granted to anyone to use this software for any purpose, 
 including commercial applications, and to alter it and redistribute it freely, 
 subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef OOLONG_READ_BLEND_H
#define OOLONG_READ_BLEND_H

#include "BulletBlendReader.h"
#include "readblend.h"
#include "blendtype.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"
class btCollisionObject;

struct GfxVertex
{
	btVector3 m_position;
	float	m_uv[2];
};

struct GfxObject
{
	GLuint m_ui32Vbo;  // Vertex buffer object handle
	int	m_numVerts;
	btAlignedObjectArray<unsigned short int>	m_indices;
	btAlignedObjectArray<GfxVertex>	m_vertices;
	
	btCollisionObject* m_colObj;
	
	GfxObject(GLuint vboId,btCollisionObject* colObj);
	
	void render();
	
};

class OolongBulletBlendReader : public BulletBlendReader
{
public:
	btAlignedObjectArray<GfxObject>	m_graphicsObjects;
	
	
	OolongBulletBlendReader(class btDynamicsWorld* destinationWorld);
	
	virtual ~OolongBulletBlendReader();
	
	///for each Blender Object, this method will be called to convert/retrieve data from the bObj
	virtual void* createGraphicsObject(_bObj* tmpObject, btCollisionObject* bulletObject);
	
	virtual	void	addCamera(_bObj* tmpObject);
	
	virtual	void	addLight(_bObj* tmpObject);
	
	virtual void	convertLogicBricks();
	
	virtual void	createParentChildHierarchy();
	
};


#endif //OOLONG_READ_BLEND_H


