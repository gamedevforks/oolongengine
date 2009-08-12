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

-(void)myMovieFinishedCallback:(NSNotification*)aNotification
{
    MPMoviePlayerController* _theMovie=[aNotification object];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:MPMoviePlayerPlaybackDidFinishNotification object:_theMovie];
    [_theMovie release];
	

    [window makeKeyAndVisible];

	// create our rendering timer
	[NSTimer scheduledTimerWithTimeInterval:(1.0 / kFPS) target:self selector:@selector(update) userInfo:nil repeats:YES];	
}

- (void)applicationDidFinishLaunching:(UIApplication *)application { 
	// Override point for customization after application launch
	
	if(!shell->InitApplication())
		printf("InitApplication error\n");
	
	NSString *name = @"Title";
	NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"m4v"];
	theMovie=[[[MPMoviePlayerController alloc] initWithContentURL:[NSURL fileURLWithPath:path]] retain];
	theMovie.movieControlMode = MPMovieControlModeHidden;
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(myMovieFinishedCallback:)
												 name:MPMoviePlayerPlaybackDidFinishNotification
											   object:theMovie];
	[theMovie play];
	[name release];
	
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
