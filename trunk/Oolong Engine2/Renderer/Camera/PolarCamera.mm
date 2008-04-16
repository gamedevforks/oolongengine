//---------------------------------------------------------
//
// PolarCamera.mm
// Oolong Engine
//
// Description: Provides a camera that rotates around the object in origin
//
// Last modified: November 5th, 2007
// based on code developed by Andrew Willmott, September 2007
//---------------------------------------------------------
#import "PolarCamera.h"
//#include "../Core/Math/FixedPoint.h"
//#include "../Core/Math/FixedPointAPI.h"
//#include "../Core/Math/Matrix.h"
#include "Mathematics.h"

//#include <OpenGLES/gl.h>

#define WIDESCREEN 0

id idFrame;


void UpdatePolarCamera()
{
	[idFrame UpdateCamera];
}


@implementation UIGLCameraView

- (id)initWithFrame:(CGRect)frame 
{
    mZUp = 0;
	
	gTouch = [TouchScreen alloc];
	    
    [self resetView];
	
	idFrame = [super initWithFrame:frame];
    return idFrame;
}

- (void) resetView
{
    mCameraTheta = 45;
    mCameraPhi = 30;
    mCameraDistance = 7;
    mCameraOffsetX = 0;
    mCameraOffsetY = 0;
    mCameraFOV = 70;

    mStartCameraTheta = 0;
    mStartCameraPhi = 0;
    mStartCameraDistance = 0;
    mStartCameraOffsetX = 0;
    mStartCameraOffsetY = 0;

    mMouseX = 0;
    mMouseY = 0;
    mMouseDownX = 0;
    mMouseDownY = 0;
}

-(void) UpdateCamera
{
    int tw, th;
	
	tw = 320;
	th = 480;

   // eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_WIDTH,  &tw);
    //eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_HEIGHT, &th);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
	MATRIX	MyPerspMatrix;
	MatrixPerspectiveFovRH(MyPerspMatrix, f2vt(mCameraFOV), f2vt(((float) tw / (float) th)), f2vt(0.1f), f2vt(1000.0f), WIDESCREEN);
	myglMultMatrix(MyPerspMatrix.f);
    
	if (mZUp)
        myglRotate(-90, 1, 0, 0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (mZUp)
        myglTranslate(mCameraOffsetX, mCameraDistance, mCameraOffsetY);
    else
        myglTranslate(mCameraOffsetX, mCameraOffsetY, -mCameraDistance);

    myglRotate(mCameraPhi,   1, 0, 0);
    myglRotate(mCameraTheta, 0, 0, 1);
}


- (void) mouseDown:(GSEvent*)event
{
    CGRect location = [gTouch GetLocationInWindow:event];
    float x = CGRectGetMinX(location);
    float y = CGRectGetMinY(location);
    
    int startCount = (int) [gTouch GetDeltaX:event];
    if (startCount == 2)    // simultaneous double-finger tap
        [self resetView];
        
    int count = (int) [gTouch GetDeltaY:event];

    mMouseDownX = x;
    mMouseDownY = y;
    mButtonDown = count;
    
    mStartCameraTheta    = mCameraTheta;
    mStartCameraPhi      = mCameraPhi;
    mStartCameraDistance = mCameraDistance;
    mStartCameraOffsetX  = mCameraOffsetX;
    mStartCameraOffsetY  = mCameraOffsetY;
}

- (void)mouseUp:(GSEvent*)event
{
    mButtonDown = 0;    // we only ever get one mouse-up event, when the last finger goes up
}


- (void)mouseDragged:(GSEvent*)event
{
    CGRect location = [gTouch GetLocationInWindow:event];
    float x = location.origin.x;
    float y = location.origin.y;
    int count = (int) [gTouch GetDeltaY:event];

    if (mButtonDown != count)
    {
        [self mouseDown: event];
    }
    
    enum tMode
    {
        kModeRotate,
        kModePanXY,
        kModeZoomX,
        kModeZoomY,
        kModeZoomXY,
        kModeNone
    };

    int mode = kModeNone;

    if (mButtonDown == 3)
        mode = kModePanXY;
    else if (mButtonDown == 2)
        mode = kModeZoomXY;
    else if (mButtonDown == 1)
        mode = kModeRotate;

#if WIDESCREEN
    float dy = (x - mMouseDownX) / 400.0;
    float dx = -(y - mMouseDownY) / 400.0;
#else
    float dx = (x - mMouseDownX) / 400.0;
    float dy = (y - mMouseDownY) / 400.0;
#endif

    switch (mode)
    {
    case kModePanXY:
        mCameraOffsetX = mStartCameraOffsetX + 4.0 * dx;
        mCameraOffsetY = mStartCameraOffsetY - 4.0 * dy;
        break;
        
    case kModeZoomX:
        mCameraDistance = mStartCameraDistance + 4.0 * dx;
        break;
        
    case kModeZoomY:
        mCameraDistance = mStartCameraDistance - 4.0 * dy;
        break;
        
    case kModeZoomXY:
        mCameraDistance = mStartCameraDistance + 4.0 * (dx - dy);
        break;
        
    case kModeRotate:
        mCameraTheta = mStartCameraTheta + dx * 180;
        mCameraPhi   = mStartCameraPhi   + dy * 180;
        if (mCameraPhi < -90) 
            mCameraPhi = -90;
        if (mCameraPhi >  90) 
            mCameraPhi =  90;
        break;
    }
}

@end



