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
#ifndef VECTOR_H_
#define VECTOR_H_

#include <OpenGLES/ES1/gl.h>


#define VERTTYPE GLfloat
#define VERTTYPEENUM GL_FLOAT

// Floating-point operations
#define VERTTYPEMUL(a,b)			( (VERTTYPE)((a)*(b)) )
#define VERTTYPEDIV(a,b)			( (VERTTYPE)((a)/(b)) )
#define VERTTYPEABS(a)				( (VERTTYPE)(fabs(a)) )

#define f2vt(x)						(x)
#define vt2f(x)						(x)

#define PIOVERTWO				PIOVERTWOf
#define PI						PIf
#define TWOPI					TWOPIf
#define ONE						ONEf

#define X2F(x)		((float)(x)/65536.0f)
#define XMUL(a,b)	( (int)( ((INT64BIT)(a)*(b)) / 65536 ) )
#define XDIV(a,b)	( (int)( (((INT64BIT)(a))<<16)/(b) ) )
#define _ABS(a)		((a) <= 0 ? -(a) : (a) )


// Define a 64-bit type for various platforms
#if defined(__int64) || defined(WIN32)
#define INT64BIT __int64
#elif defined(TInt64)
#define INT64BIT TInt64
#else
#define INT64BIT long long int
#endif

typedef struct _LARGE_INTEGER
	{
		union
		{
			struct
			{
				unsigned long LowPart;
				long HighPart;
			};
			INT64BIT QuadPart;
		};
	} LARGE_INTEGER, *PLARGE_INTEGER;


typedef struct _VECTOR2
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
   inline void set(float _x, float _y) { x = _x; y = _y; }
} VECTOR2;


typedef struct _VECTOR3
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
	float z;	/*!< z coordinate */
   inline void set(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
   inline const _VECTOR3 operator-(const _VECTOR3& rhs) const{ _VECTOR3 tmp = { x - rhs.x, y - rhs.y, z - rhs.z }; return tmp; }
   inline const _VECTOR3 operator+(const _VECTOR3& rhs) const{ _VECTOR3 tmp = { x + rhs.x, y + rhs.y, z + rhs.z }; return tmp; }
   inline const _VECTOR3 operator*(float rhs) const { _VECTOR3 tmp = { x * rhs, y * rhs, z * rhs }; return tmp; }
   inline float lenSquared() const { return x*x + y*y + z*z; }
} VECTOR3;

typedef struct
{
	int x;	/*!< x coordinate */
	int y;	/*!< y coordinate */
	int z;	/*!< z coordinate */
} VECTOR3x;


typedef struct
{
	float x;	/*!< x coordinate */
	float y;	/*!< y coordinate */
	float z;	/*!< z coordinate */
	float w;	/*!< w coordinate */
} VECTOR4;



#endif // VECTOR_H_