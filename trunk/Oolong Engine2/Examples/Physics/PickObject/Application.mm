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
//////////////////////////////////
///Quick Bullet integration
///Todo: move data into class, instead of globals
#include "btBulletDynamicsCommon.h"
#define MAX_PROXIES 8192
static btDiscreteDynamicsWorld* sDynamicsWorld=0;
static btCollisionConfiguration* sCollisionConfig=0;
static btCollisionDispatcher* sCollisionDispatcher=0;
static btSequentialImpulseConstraintSolver* sConstraintSolver;
static btBroadphaseInterface* sBroadphase=0;
btAlignedObjectArray<btCollisionShape*> sCollisionShapes;
btAlignedObjectArray<btRigidBody*> sBoxBodies;
btRigidBody* sFloorPlaneBody=0;
int numBodies = 50;

//////////////////////////////////

//#include "Log.h"
#include "App.h"
#include "Mathematics.h"
#include "GraphicsDevice.h"
#include "UI.h"
#include "Macros.h"
#include "TouchScreen.h"

#include <stdio.h>
#include <sys/time.h>

CDisplayText * AppDisplayText;
int iCurrentTick = 0, iStartTick = 0, iFps = 0, iFrames = 0;

int frames;
float frameRate;

// touch screen values
TouchScreenValues *TouchScreen;
btVector3 Ray;

btVector3 GetRayTo(int x,int y, float nearPlane, float farPlane, btVector3 cameraUp, btVector3 CameraPosition, btVector3 CameraTargetPosition);

bool CShell::InitApplication()
{
//	LOGFUNC("InitApplication()");
	
	AppDisplayText = new CDisplayText;  
	
	if(AppDisplayText->SetTextures(WindowHeight, WindowWidth))
//		LOG("Display text textures loaded", Logger::LOG_DATA);
				printf("Display text textures loaded\n");


	sCollisionConfig = new btDefaultCollisionConfiguration();
	
	///the maximum size of the collision world. Make sure objects stay within these boundaries
	///Don't make the world AABB size too large, it will harm simulation quality and performance
	btVector3 worldAabbMin(-10000,-10000,-10000);
	btVector3 worldAabbMax(10000,10000,10000);
	sBroadphase = new btAxisSweep3(worldAabbMin,worldAabbMax,MAX_PROXIES);
	sCollisionDispatcher = new btCollisionDispatcher(sCollisionConfig);
	sConstraintSolver = new btSequentialImpulseConstraintSolver;
	sDynamicsWorld = new btDiscreteDynamicsWorld(sCollisionDispatcher,sBroadphase,sConstraintSolver,sCollisionConfig);
	sDynamicsWorld->setGravity(btVector3(0,-10,0));
	//btCollisionShape* shape = new btBoxShape(btVector3(1,1,1));
	{
		btTransform groundTransform;
		groundTransform.setIdentity();
		btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),0);
		btScalar mass(0.);	//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0,0,0);
		if (isDynamic)
				groundShape->calculateLocalInertia(mass,localInertia);
		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundShape,localInertia);
		sFloorPlaneBody = new btRigidBody(rbInfo);
		//add the body to the dynamics world
		sDynamicsWorld->addRigidBody(sFloorPlaneBody);
	}
	for (int i=0;i<numBodies;i++)
	{
		btTransform bodyTransform;
		bodyTransform.setIdentity();
		bodyTransform.setOrigin(btVector3(0,10+i*2,0));
		btCollisionShape* boxShape = new btBoxShape(btVector3(1,1,1));
		btScalar mass(1.);//positive mass means dynamic/moving  object
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0,0,0);
		if (isDynamic)
				boxShape->calculateLocalInertia(mass,localInertia);
		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(bodyTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,boxShape,localInertia);
		btRigidBody* boxBody=new btRigidBody(rbInfo);
		sBoxBodies.push_back(boxBody);
		//add the body to the dynamics world
		sDynamicsWorld->addRigidBody(boxBody);
	}
	
	return true;
}

bool CShell::QuitApplication()
{
	AppDisplayText->ReleaseTextures();
	
	delete AppDisplayText;
	
	///cleanup Bullet stuff
	delete sDynamicsWorld;
	sDynamicsWorld =0;
	delete sConstraintSolver;
	sConstraintSolver=0;
	delete sCollisionDispatcher;
	sCollisionDispatcher=0;
	delete sBroadphase;
	sBroadphase=0;
	delete sCollisionConfig;
	sCollisionConfig=0;
	//////////////
	return true;
}

bool CShell::InitView()
{
    glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    glDisable(GL_CULL_FACE);
	
//	UpdatePolarCamera();

	//Set the OpenGL projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	MATRIX	MyPerspMatrix;
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(70), f2vt(((float) 320 / (float) 480)), f2vt(1.0f), f2vt(10000.0f), 0);
	myglMultMatrix(MyPerspMatrix.f);

//	glOrthof(-40 / 2, 40 / 2, -60 / 2, 60 / 2, -1, 1);
	
	static CFTimeInterval	startTime = 0;
	CFTimeInterval			time;
	
	//Calculate our local time
	time = CFAbsoluteTimeGetCurrent();
	if(startTime == 0)
	startTime = time;
	time = time - startTime;
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, -10.0, -30.0f);
//	glRotatef(50.0f * fmod(time, 360.0), 0.0, 1.0, 1.0);
	
	//
	// Touch screen support
	//
	// touch screen coordinates go from 0, 0 in the upper left corner to
	// 320, 480 in the lower right corner
	TouchScreen = GetValuesTouchScreen();
	
	// the center of the ray coordinates are located in the middle of the 
	// screen ... in the x direction the values range from -9875..9875 in x direction and
	// 15.000..-15000 in the y direction
	Ray = btVector3(0.0f, 0.0f, 0.0f);
	

	if(TouchScreen->TouchesEnd == false)
	{
		Ray = GetRayTo(TouchScreen->LocationXTouchesMoved, TouchScreen->LocationYTouchesMoved, 
					   1.0f, 10000.0f, btVector3(0.0f, 1.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f), 
					   btVector3(0.0f, 0.0f, -1.0f));
		
		AppDisplayText->DisplayText(0, 10, 0.4f, RGBA(255,255,255,255), "touchesBegan: X: %3.2f Y: %3.2f Count: %3.2f Tab Count %3.2f", 
									TouchScreen->LocationXTouchesBegan, TouchScreen->LocationYTouchesBegan, TouchScreen->CountTouchesBegan, TouchScreen->TapCountTouchesBegan);
		AppDisplayText->DisplayText(0, 14, 0.4f, RGBA(255,255,255,255), "touchesMoved: X: %3.2f Y: %3.2f Count: %3.2f Tab Count %3.2f", 
									TouchScreen->LocationXTouchesMoved, TouchScreen->LocationYTouchesMoved, TouchScreen->CountTouchesMoved, TouchScreen->TapCountTouchesMoved);
		AppDisplayText->DisplayText(0, 18, 0.4f, RGBA(255,255,255,255), "Ray: X: %3.2f Y: %3.2f Z: %3.2f", Ray.getX(), Ray.getY(), Ray.getZ());

	}
	
	return true;
}

bool CShell::ReleaseView()
{
	return true;
}

btVector3 GetRayTo(int x,int y, float nearPlane, float farPlane, btVector3 cameraUp, btVector3 CameraPosition, btVector3 CameraTargetPosition)
{
	float top = 1.f;
	float bottom = -1.f;
	float tanFov = (top - bottom) * 0.5f / nearPlane;
	float fov = 2.0 * atanf (tanFov);
	
	btVector3	rayFrom = CameraPosition;
	btVector3 rayForward = (CameraTargetPosition - CameraPosition);
	rayForward.normalize();
	//float fPlane = 10000.f;
	rayForward *= farPlane;
	
	btVector3 rightOffset;
	btVector3 vertical = cameraUp;
	
	btVector3 hor;
	
	hor = rayForward.cross(vertical);
	hor.normalize();
	vertical = hor.cross(rayForward);
	vertical.normalize();
	
	float tanfov = tanf(0.5f*fov);
	
	float aspect = (float)480 / (float)320;
	
	hor *= 2.f * farPlane * tanfov;
	vertical *= 2.f * farPlane * tanfov;
	
	if (aspect<1)
	{
		hor/=aspect;
	} else
	{
		vertical*=aspect;
	}
	
	btVector3 rayToCenter = rayFrom + rayForward;
	btVector3 dHor = hor * 1.f/float(320);
	btVector3 dVert = vertical * 1.f/float(480);
	
	btVector3 rayTo = rayToCenter - 0.5f * hor + 0.5f * vertical;
	rayTo += x * dHor;
	rayTo -= y * dVert;
	return rayTo;
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
		AppDisplayText->DisplayText(0, 6, 0.4f, RGBA(255,255,255,255), "fps: %3.2f", frameRate);
		time = currTime;
		frames = 0;
	}
	float deltaTime = 1.f/60.f;
	if (sDynamicsWorld)
		sDynamicsWorld->stepSimulation(deltaTime);
	
	
	return true;
}

btRigidBody* pickedBody = 0;//for deactivation state
btPoint2PointConstraint* PickConstraint = 0;
int gPickingConstraintId = 0;
btVector3 gOldPickingPos;
float gOldPickingDist  = 0.f;


bool CShell::RenderScene()
{
	float worldMat[16];
	for (int i=0;i<numBodies;i++)
	{
		sBoxBodies[i]->getCenterOfMassTransform().getOpenGLMatrix(worldMat);
		glPushMatrix();
		glMultMatrixf(worldMat);
		
		
		if(TouchScreen->TouchesEnd == false)
		{			
			btVector3 CameraPosition = btVector3(0.0f, 0.0f, 0.0f);
		
			btCollisionWorld::ClosestRayResultCallback rayCallback(CameraPosition, Ray);
		
			if (sDynamicsWorld)
			{
				sDynamicsWorld->rayTest(CameraPosition, Ray, rayCallback);
				if (rayCallback.HasHit())
				{
					btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObject);
					if (body)
					{
						body->setActivationState(ACTIVE_TAG);
						btVector3 impulse = Ray;
						impulse.normalize();
						float impulseStrength = 10.f;
						impulse *= impulseStrength;
						btVector3 relPos = rayCallback.m_hitPointWorld - body->getCenterOfMassPosition();
						body->applyImpulse(impulse,relPos);
					}
				}
			}
		}
		
		

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

		glEnableClientState(GL_VERTEX_ARRAY);
    
		glColor4f(0, 1, 0, 1);
		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
		glColor4f(1, 0, 1, 1);
		glVertexPointer(3, GL_FLOAT, 0, verts + 12);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
		glColor4f(0, 0, 1, 1);
		glVertexPointer(3, GL_FLOAT, 0, verts + 24);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
		glColor4f(1, 1, 0, 1);
		glVertexPointer(3, GL_FLOAT, 0, verts + 36);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
		glColor4f(1, 0, 0, 1);
		glVertexPointer(3, GL_FLOAT, 0, verts + 48);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
		glColor4f(0, 1, 1, 1);
		glVertexPointer(3, GL_FLOAT, 0, verts + 60);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
		glPopMatrix();
	}
	
	// show text on the display
	AppDisplayText->DisplayDefaultTitle("Falling Cubes", "", eDisplayTextLogoIMG);
	
	AppDisplayText->Flush();	
	
	return true;
}

