/*
 *  Surface.cpp
 *  SkeletonXIB
 *
 *  Created by Jim on 8/1/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Surface.h"
#include "Mathematics.h"

GLuint CParametricSurface::GenVBO( int slices )
{
    totalVertex = 0;
    int stacks = slices / 2;
    du = 1.0f / (float) slices;
    dv = 1.0f / (float) stacks;
    //this->tangentLoc = tangentLoc;
    //this->binormalLoc = binormalLoc;
	
    for (float u = 0; u < 1 - du / 2; u += du) {
        if (flipped = Flip(Vec2(u,0))) {
            for (float v = 0; v < 1 + dv / 2; v += dv) {
                Vertex(Vec2(u + du, v));
                totalVertex++;
                Vertex(Vec2(u, v));
                totalVertex++;
            }
        } else {
            for (float v = 0; v < 1 + dv / 2; v += dv) {
                Vertex(Vec2(u, v));
                totalVertex++;
                Vertex(Vec2(u + du, v));
                totalVertex++;
            }
        }
    }
	GLuint ui32Vbo;
	
	glGenBuffers(1, &ui32Vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ui32Vbo);
	unsigned int uiSize = totalVertex * (sizeof(GLfloat) * 14);
	glBufferData(GL_ARRAY_BUFFER, uiSize, vertexBuffer, GL_STATIC_DRAW);
	
    return ui32Vbo;
}

void CParametricSurface::Vertex(Vec2 domain)
{
    Vec3 p0, p1, p2, p3;
    Vec3 normal;
    float u = domain.x;
    float v = domain.y;
	
    Eval(domain, p0);
    Vec2 z1(u + du/2, v);
    Eval(z1, p1);
    Vec2 z2(u + du/2 + du, v);
    Eval(z2, p3);
	
    if (flipped) {
        Vec2 z3(u + du/2, v - dv);
        Eval(z3, p2);
    } else {
        Vec2 z4(u + du/2, v + dv);
        Eval(z4, p2);
    }
	
    const float epsilon = 0.00001f;
	
    Vec3 tangent = p3 - p1;
    Vec3 binormal = p2 - p1;
    MatrixVec3CrossProduct(normal, tangent, binormal);
    if (normal.length() < epsilon)
        normal = p0;
    normal.normalize();
	if (tangent.length() < epsilon)
		MatrixVec3CrossProduct(tangent, binormal, normal);
	tangent.normalize();
	binormal.normalize();
	binormal = binormal * -1.0f;
	 
	/*
    if (CustomAttributeLocation() != -1)
        glVertexAttrib1f(CustomAttributeLocation(), CustomAttributeValue(domain));
	 */
	
    //glNormal(normal);
    //glTexCoord(domain);
    //glVertex(p0);
	int vertexIndex = totalVertex * 14;
	vertexBuffer[vertexIndex++] = p0.x;
	vertexBuffer[vertexIndex++] = p0.y;
	vertexBuffer[vertexIndex++] = p0.z;
	vertexBuffer[vertexIndex++] = domain.x;
	vertexBuffer[vertexIndex++] = domain.y;
	vertexBuffer[vertexIndex++] = normal.x;
	vertexBuffer[vertexIndex++] = normal.y;
	vertexBuffer[vertexIndex++] = normal.z;
	vertexBuffer[vertexIndex++] = tangent.x;
	vertexBuffer[vertexIndex++] = tangent.y;
	vertexBuffer[vertexIndex++] = tangent.z;
	vertexBuffer[vertexIndex++] = binormal.x;
	vertexBuffer[vertexIndex++] = binormal.y;
	vertexBuffer[vertexIndex++] = binormal.z;
}


void CKlein::Eval(Vec2& domain, Vec3& range)
{
    float u = (1 - domain.x) * TWOPIf;
    float v = domain.y * TWOPIf;
	
    float x0 = 3 * cosf(u) * (1 + sinf(u)) + (2 * (1 - cosf(u) / 2)) * cosf(u) * cosf(v);
    float y0  = 8 * sinf(u) + (2 * (1 - cosf(u) / 2)) * sinf(u) * cosf(v);
	
    float x1 = 3 * cosf(u) * (1 + sinf(u)) + (2 * (1 - cosf(u) / 2)) * cosf(v + PIf);
    float y1 = 8 * sinf(u);
	
    range.x = u < PIf ? x0 : x1;
    range.y = u < PIf ? y0 : y1;
    range.z = (2 * (1 - cosf(u) / 2)) * sinf(v);
    range = range / 10;
    range.y = -range.y;
	
    // Tweak the texture coordinates.
    domain.x *= 4;
}

// Flip the normals along a segment of the Klein bottle so that we don't need two-sided lighting.
bool CKlein::Flip(const Vec2& domain)
{
    return (domain.x < .125);
}
