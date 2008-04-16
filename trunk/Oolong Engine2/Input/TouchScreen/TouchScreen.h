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

//
// it seems like GraphicsServices has very similar function stubs
// ... need to check out how to make those work
//

//#import <GraphicsServices/GraphicsServices.h>
#import <UIKit/UIView.h>

struct __GSEvent;
typedef struct __GSEvent GSEvent;
typedef GSEvent *GSEventRef;

struct CGPoint;
struct CGRect;
typedef struct CGPoint CGPoint;
typedef struct CGRect CGRect;

int GSEventIsChordingHandEvent(GSEvent *ev);
int GSEventGetClickCount(GSEvent *ev);
CGRect GSEventGetLocationInWindow(GSEvent *ev);
float GSEventGetDeltaX(GSEvent *ev);
float GSEventGetDeltaY(GSEvent *ev);
CGRect  GSEventGetInnerMostPathPosition(GSEvent *ev);
CGRect GSEventGetOuterMostPathPosition(GSEvent *ev);
void GSEventVibrateForDuration(float secs);


@interface TouchScreen : UIView
{

}

//
// the GSEvent* functions can not be called in a *.mm file
// therefore we have those wrappers here ...
//
- (CGRect) GetLocationInWindow:(GSEvent*)event;
- (int) GetDeltaX:(GSEvent*)event;
- (int) GetDeltaY:(GSEvent*)event;

@end
