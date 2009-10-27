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

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES1/gl.h>

#include "OolongReadBlend.h"
#include "btBulletDynamicsCommon.h"


//#define SWAP_COORDINATE_SYSTEMS
#ifdef SWAP_COORDINATE_SYSTEMS

#define IRR_X 0
#define IRR_Y 2
#define IRR_Z 1

#define IRR_X_M 1.f
#define IRR_Y_M 1.f
#define IRR_Z_M 1.f

///also winding is different
#define IRR_TRI_0_X 0
#define IRR_TRI_0_Y 2
#define IRR_TRI_0_Z 1

#define IRR_TRI_1_X 0
#define IRR_TRI_1_Y 3
#define IRR_TRI_1_Z 2
#else
#define IRR_X 0
#define IRR_Y 1
#define IRR_Z 2

#define IRR_X_M 1.f
#define IRR_Y_M 1.f
#define IRR_Z_M 1.f

///also winding is different
#define IRR_TRI_0_X 0
#define IRR_TRI_0_Y 1
#define IRR_TRI_0_Z 2

#define IRR_TRI_1_X 0
#define IRR_TRI_1_Y 2
#define IRR_TRI_1_Z 3
#endif

GfxObject::GfxObject(GLuint vboId,btCollisionObject* colObj)
:m_ui32Vbo(vboId),
m_colObj(colObj)
{
}


void GfxObject::render()
{
	glPushMatrix();
	float m[16];
	m_colObj->getWorldTransform().getOpenGLMatrix(m);
	
	glMultMatrixf(m);
		
//	glScalef(0.1,0.1,0.1);
	
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);	
	glTexCoordPointer(2, GL_FLOAT, sizeof(GfxVertex), &m_vertices[0].m_uv[0]);
	
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glColor4f(0, 1, 0, 1);
    glVertexPointer(3, GL_FLOAT, sizeof(GfxVertex), &m_vertices[0].m_position.getX());
 //   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
	for (int i=0;i<m_indices.size();i++)
	{
		if (m_indices[i] > m_vertices.size())
		{
			printf("out of bounds: m_indices[%d]=%d but m_vertices.size()=%d",i,m_indices[i],m_vertices.size());
		}
	}
	glDrawElements(GL_TRIANGLES,m_indices.size(),GL_UNSIGNED_SHORT,&m_indices[0]);

	glPopMatrix();

}
	


OolongBulletBlendReader::OolongBulletBlendReader(class btDynamicsWorld* destinationWorld)
:BulletBlendReader(destinationWorld)
{
	m_cameraTrans.setIdentity();
}
	
OolongBulletBlendReader::~OolongBulletBlendReader()
{
	for (int i=0;i<m_graphicsObjects.size();i++)
	{
		// Delete the VBO as it is no longer needed
		glDeleteBuffers(1, &m_graphicsObjects[i].m_ui32Vbo);
		
	}
}

	
void* OolongBulletBlendReader::createGraphicsObject(_bObj* tmpObject, class btCollisionObject* bulletObject)
{
	GfxObject gfxobject(0,bulletObject);

	
#if 0
	
	const float verts[] =
    {
		1.0f, 1.0f,-1.0f,	
        -1.0f, 1.0f,-1.0f,	
        -1.0f, 1.0f, 1.0f,	
		1.0f, 1.0f, 1.0f,	
		
		1.0f,-1.0f, 1.0f,	
        -1.0f,-1.0f, 1.0f,	
        -1.0f,-1.0f,-1.0f,	
		1.0f,-1.0f,-1.0f,	
		
		1.0f, 1.0f, 1.0f,	
        -1.0f, 1.0f, 1.0f,	
        -1.0f,-1.0f, 1.0f,	
		1.0f,-1.0f, 1.0f,	
		
		1.0f,-1.0f,-1.0f,	
        -1.0f,-1.0f,-1.0f,	
        -1.0f, 1.0f,-1.0f,	
		1.0f, 1.0f,-1.0f,	
		
		1.0f, 1.0f,-1.0f,	
		1.0f, 1.0f, 1.0f,	
		1.0f,-1.0f, 1.0f,	
		1.0f,-1.0f,-1.0f,
		
        -1.0f, 1.0f, 1.0f,	
        -1.0f, 1.0f,-1.0f,	
        -1.0f,-1.0f,-1.0f,	
        -1.0f,-1.0f, 1.0f
	};
	
	GLfloat texcoords[] =
	{
		// Top - facing to +Y		// Texture Coord
		1.0f,0.0f,
		0.0f, 0.0f,	
		0.0f, 1.0f,
		1.0f, 1.0f,
		
		// Bottom - facing to -Y
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		
		// Front - facing to +Z
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		
		// Back - facing to -Z
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		
		// Right - facing to +X
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		
		// Left - facing to -X
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};

	gfxobject.m_vertices.resize(24);
	for (int i=0;i<24;i++)
	{
		gfxobject.m_vertices[i].m_position.setValue(verts[i*3],verts[i*3+1],verts[i*3+2]);
		gfxobject.m_vertices[i].m_uv[0] =texcoords[i*2];
		gfxobject.m_vertices[i].m_uv[1] =texcoords[i*2+1];
	}
	
	unsigned short int indices[]={
		0,1,2,
		0,2,3,
		4,5,6,
		4,6,7,
		8,9,10,
		8,10,11,
		12,13,14,
		12,14,15,
		16,17,18,
		16,18,19,
		20,21,22,
		20,22,23};
	
	for (int i=0;i<36;i++)
		gfxobject.m_indices.push_back(indices[i]);
	
	m_graphicsObjects.push_back(gfxobject);
	
#else

	
	/*
	btRigidBody* body = btRigidBody::upcast(bulletObject);
	IrrMotionState* newMotionState = 0;
	
	if (body)
	{
		if (!bulletObject->isStaticOrKinematicObject())
		{
			newMotionState = new IrrMotionState();
			newMotionState->setWorldTransform(body->getWorldTransform());
			body->setMotionState(newMotionState);
		}
	}
	 */
	
	
	
	if (tmpObject->data.mesh && tmpObject->data.mesh->vert_count && tmpObject->data.mesh->face_count)
	{
		if (tmpObject->data.mesh->vert_count> 16300)
		{
			printf("tmpObject->data.mesh->vert_count = %d\n",tmpObject->data.mesh->vert_count);
			return 0;
		}
		
		int maxVerts = btMin(16300,btMax(tmpObject->data.mesh->face_count*3*2,(tmpObject->data.mesh->vert_count-6)));
		
		GfxVertex* orgVertices= new GfxVertex[tmpObject->data.mesh->vert_count];
		
		for (int v=0;v<tmpObject->data.mesh->vert_count;v++)
		{
			float* vt3 = tmpObject->data.mesh->vert[v].xyz;
			orgVertices[v].m_position.setValue(IRR_X_M*vt3[IRR_X],	IRR_Y_M*vt3[IRR_Y],	IRR_Z_M*vt3[IRR_Z]);
		}
		
		
		int numTriangles=0;
		int numIndices = 0;
		int currentIndex = 0;
		
		int maxNumIndices = tmpObject->data.mesh->face_count*4*2;
		
		int totalVerts = 0;
		
		for (int t=0;t<tmpObject->data.mesh->face_count;t++)
		{
			totalVerts += (tmpObject->data.mesh->face[t].v[3]) ? 6 : 3;
		}
		
		gfxobject.m_vertices.resize(totalVerts);
		
		for (int t=0;t<tmpObject->data.mesh->face_count;t++)
		{
			if (currentIndex>maxNumIndices)
				break;
			
			int originalIndex = tmpObject->data.mesh->face[t].v[IRR_TRI_0_X];
			gfxobject.m_indices.push_back(currentIndex);
			gfxobject.m_vertices[currentIndex].m_position = orgVertices[originalIndex].m_position;
			gfxobject.m_vertices[currentIndex].m_uv[0] = tmpObject->data.mesh->face[t].uv[IRR_TRI_0_X][0];
			gfxobject.m_vertices[currentIndex].m_uv[1] = tmpObject->data.mesh->face[t].uv[IRR_TRI_0_X][1];
			numIndices++;
			currentIndex++;
						
			originalIndex = tmpObject->data.mesh->face[t].v[IRR_TRI_0_Y];
			gfxobject.m_indices.push_back(currentIndex);
			gfxobject.m_vertices[currentIndex].m_position = orgVertices[originalIndex].m_position;
			gfxobject.m_vertices[currentIndex].m_uv[0] = tmpObject->data.mesh->face[t].uv[IRR_TRI_0_Y][0];
			gfxobject.m_vertices[currentIndex].m_uv[1] = tmpObject->data.mesh->face[t].uv[IRR_TRI_0_Y][1];
			numIndices++;
			currentIndex++;
					
			originalIndex = tmpObject->data.mesh->face[t].v[IRR_TRI_0_Z];
			gfxobject.m_indices.push_back(currentIndex);
			gfxobject.m_vertices[currentIndex].m_position = orgVertices[originalIndex].m_position;
			gfxobject.m_vertices[currentIndex].m_uv[0] = tmpObject->data.mesh->face[t].uv[IRR_TRI_0_Z][0];
			gfxobject.m_vertices[currentIndex].m_uv[1] = tmpObject->data.mesh->face[t].uv[IRR_TRI_0_Z][1];
			numIndices++;
			currentIndex++;
			numTriangles++;
			
			if (tmpObject->data.mesh->face[t].v[3])
			{
				originalIndex = tmpObject->data.mesh->face[t].v[IRR_TRI_1_X];
				gfxobject.m_indices.push_back(currentIndex);
				gfxobject.m_vertices[currentIndex].m_position = orgVertices[originalIndex].m_position;
				gfxobject.m_vertices[currentIndex].m_uv[0] = tmpObject->data.mesh->face[t].uv[IRR_TRI_1_X][0];
				gfxobject.m_vertices[currentIndex].m_uv[1] = tmpObject->data.mesh->face[t].uv[IRR_TRI_1_X][1];
				numIndices++;
				currentIndex++;
				
				originalIndex = tmpObject->data.mesh->face[t].v[IRR_TRI_1_Y];
				gfxobject.m_indices.push_back(currentIndex);
				gfxobject.m_vertices[currentIndex].m_position = orgVertices[originalIndex].m_position;
				gfxobject.m_vertices[currentIndex].m_uv[0] = tmpObject->data.mesh->face[t].uv[IRR_TRI_1_Y][0];
				gfxobject.m_vertices[currentIndex].m_uv[1] = tmpObject->data.mesh->face[t].uv[IRR_TRI_1_Y][1];
				numIndices++;
				currentIndex++;
				
				originalIndex = tmpObject->data.mesh->face[t].v[IRR_TRI_1_Z];
				gfxobject.m_indices.push_back(currentIndex);
				gfxobject.m_vertices[currentIndex].m_position = orgVertices[originalIndex].m_position;
				gfxobject.m_vertices[currentIndex].m_uv[0] = tmpObject->data.mesh->face[t].uv[IRR_TRI_1_Z][0];
				gfxobject.m_vertices[currentIndex].m_uv[1] = tmpObject->data.mesh->face[t].uv[IRR_TRI_1_Z][1];
				
				numIndices++;
				currentIndex++;
				
				numTriangles++;
			}
			
			
		}
		delete[] orgVertices;
		
		if (numTriangles>0)
		{
//			static int once = true;
//			if (once)
				m_graphicsObjects.push_back(gfxobject);
//			once = false;
			
//			scene::ISceneNode* node = createMeshNode(newVertices,numVertices,indices,numIndices,numTriangles,bulletObject,tmpObject);
//			
//			if (!meshContainer)
//				meshContainer = new IrrlichtMeshContainer();
//			meshContainer->m_sceneNodes.push_back(node);
//			
//			if (newMotionState && node)
//				newMotionState->addIrrlichtNode(node);
			
			
		}
	}
	
	
#endif
	
	
	return 0;
}



void	OolongBulletBlendReader::addCamera(_bObj* tmpObject)
{
	m_cameraTrans.setOrigin(btVector3(tmpObject->location[IRR_X],tmpObject->location[IRR_Y],tmpObject->location[IRR_Z]));
	btMatrix3x3 mat;
	mat.setEulerZYX(tmpObject->rotphr[0],tmpObject->rotphr[1],tmpObject->rotphr[2]);
	m_cameraTrans.setBasis(mat);
}

	
void	OolongBulletBlendReader::addLight(_bObj* tmpObject)
{
	printf("added Light\n");
}

void	OolongBulletBlendReader::convertLogicBricks()
{
}

void	OolongBulletBlendReader::createParentChildHierarchy()
{
}




