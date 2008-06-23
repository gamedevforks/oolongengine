/******************************************************************************

 @File         Particle.h

 @Title        Particle class for OGLESParticles.cpp

 @Copyright    Copyright (C) 2005 - 2007 by Imagination Technologies Limited.

 @Platform     Independant

 @Description  Requires the OGLESShell.

******************************************************************************/
//#include "OGLESTools.h"

#ifndef PVRTFIXEDPOINTENABLE
	#define vt2b(a) (unsigned char) (a)
#else
	#define vt2b(a) (unsigned char) (a>>16)
#endif


class particle
{
public:
	// dynamic properties
	VECTOR3		Position;
	VECTOR3		Velocity;
	VECTOR3		Color;
	VERTTYPE	age;

// inherent properties
	VERTTYPE	lifetime;
	float		mass;

	VERTTYPE	size;
	
	VECTOR3		Initial_Color;
	VECTOR3		Halfway_Color;
	VECTOR3		End_Color;

public:
	particle() { }		// allow default construct
	  particle(const VECTOR3 &Pos, const VECTOR3 &Vel, float m, VERTTYPE life) :
	  Position(Pos), Velocity(Vel), age(f2vt(0)), lifetime(life), mass(m), size(f2vt(0))  { }

	bool step(VERTTYPE delta_t, VECTOR3 &aForce)
	{
		VECTOR3 Accel;
		VECTOR3 Force = aForce;
	
		if (Position.y < f2vt(0))
		{
			if(delta_t != f2vt(0.0))
			{
				//Force.x += f2vt(0.0f);
				Force.y += VERTTYPEMUL(VERTTYPEMUL(VERTTYPEMUL(f2vt(0.5f),Velocity.y),Velocity.y),f2vt(mass)) + f2vt(9.8f*mass);
				//Force.z += f2vt(0.0f);
			}
		}

		VERTTYPE inv_mass = f2vt(1.0f/mass);
		Accel.x = f2vt(0.0f) + VERTTYPEMUL(Force.x,inv_mass);
		Accel.y = f2vt(-9.8f) + VERTTYPEMUL(Force.y,inv_mass);
		Accel.z = f2vt(0.0f) + VERTTYPEMUL(Force.z,inv_mass);

		Velocity.x += VERTTYPEMUL(delta_t,Accel.x);
		Velocity.y += VERTTYPEMUL(delta_t,Accel.y);
		Velocity.z += VERTTYPEMUL(delta_t,Accel.z);
		
		Position.x += VERTTYPEMUL(delta_t,Velocity.x);
		Position.y += VERTTYPEMUL(delta_t,Velocity.y);
		Position.z += VERTTYPEMUL(delta_t,Velocity.z);
		age += delta_t;

		if(age <= lifetime/2)
		{
			VERTTYPE mu = f2vt(vt2f(age) / (vt2f(lifetime)/2.0f));
			Color.x = VERTTYPEMUL((f2vt(1)-mu),Initial_Color.x) + VERTTYPEMUL(mu,Halfway_Color.x);
			Color.y = VERTTYPEMUL((f2vt(1)-mu),Initial_Color.y) + VERTTYPEMUL(mu,Halfway_Color.y);
			Color.z = VERTTYPEMUL((f2vt(1)-mu),Initial_Color.z) + VERTTYPEMUL(mu,Halfway_Color.z);
		}
		else
		{
			VERTTYPE mu = f2vt((vt2f(age-lifetime)/2.0f) / (vt2f(lifetime)/2.0f));
			Color.x = VERTTYPEMUL((f2vt(1)-mu),Halfway_Color.x) + VERTTYPEMUL(mu,End_Color.x);
			Color.y = VERTTYPEMUL((f2vt(1)-mu),Halfway_Color.y) + VERTTYPEMUL(mu,End_Color.y);
			Color.z = VERTTYPEMUL((f2vt(1)-mu),Halfway_Color.z) + VERTTYPEMUL(mu,End_Color.z);
		}

		return (age >= lifetime);
	}
};


