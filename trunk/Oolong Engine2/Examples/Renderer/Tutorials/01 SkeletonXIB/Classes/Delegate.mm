//
//  SkeletonXIBAppDelegate.m
//  SkeletonXIB
//
//  Created by Jim on 7/25/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "Delegate.h"
#import "App.h"

@implementation AppDelegate

@synthesize window;
@synthesize glView;

//CONSTANTS:
#define kFPS			60.0
#define kSpeed			10.0

static CShell *shell = NULL;

- (void) update
{
	if(!shell->UpdateScene())
		printf("UpdateScene error\n");
	
    if(!shell->RenderScene())
		printf("RenderScene error\n");
	
	[glView swapBuffers];
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {    

	
	if(!shell->InitApplication())
		printf("InitApplication error\n");
	
    // Override point for customization after application launch
    [window makeKeyAndVisible];

	// create our rendering timer
	[NSTimer scheduledTimerWithTimeInterval:(1.0 / kFPS) target:self selector:@selector(update) userInfo:nil repeats:YES];	
}

- (void)applicationWillTerminate:(UIApplication *)application {
	if(!shell->QuitApplication())
		printf("QuitApplication error\n");	
	if( glView )
		[glView releaseContext];
}

- (void)dealloc {
    //[window release];
    [super dealloc];
}


@end
