//
//  SkeletonXIBAppDelegate.h
//  SkeletonXIB
//
//  Created by Jim on 7/25/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "GraphicsDevice.h"

@interface AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
	EAGLView2 *glView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet EAGLView2 *glView;

@end

