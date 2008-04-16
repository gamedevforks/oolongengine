//---------------------------------------------------------
//
// PolarCamera.h
// Oolong Engine
//
// Description: Provides a camera that rotates around the object in origin
//
// Last modified: November 5th, 2007
// based on code developed by Andrew Willmott, September 2007
//---------------------------------------------------------
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>


// UICameraView is derived from UIGLView
#import "GraphicsDevice.h"

#import "TouchScreen.h"

////////////////////////////////////////////////////////////////////////////////
// Provides a UIGLView with built-in view controls:
//
//   single-finger drag: rotate around and up/down
//   double-finger drag (or anchor one finger and drag the second up and down): zoom in/out
//   triple-finger drag (anchor two fingers, drag the other left/right or up/down): translate viewpoint
//   two-finger tap: reset view
//
// In drawRect, call positionCamera when you need to set the camera matrices.
@interface UIGLCameraView : EAGLView
//@interface UIGLCameraView : UIGLView
{
    bool    mZUp;               // Set to 0 or 1 to indicate whether we do Z-up or Y-up
    
    // Camera internals
    GLfloat  mCameraTheta;
    GLfloat  mCameraPhi;
    GLfloat  mCameraDistance;
    GLfloat  mCameraOffsetX;
    GLfloat  mCameraOffsetY;
    GLfloat  mCameraFOV;

    // dragging internals
    GLfloat  mStartCameraTheta;
    GLfloat  mStartCameraPhi;
    GLfloat  mStartCameraDistance;
    GLfloat  mStartCameraOffsetX;
    GLfloat  mStartCameraOffsetY;

    int      mButtonDown;
    
    float    mMouseX;
    float    mMouseY;
    float    mMouseDownX;
    float    mMouseDownY;
	
	TouchScreen* gTouch;
}

- (void) UpdateCamera;
// Call to set camera matrices

- (void) resetView;
// override this to provide your own initial settings for mCamera* above.
// also, call to manually reset the view.

@end

void UpdatePolarCamera();
