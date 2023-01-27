//
//  AppDelegate.m
//  oxygenApp
//

#import "AppDelegate.h"
#define RMX_LIB
#include "engineapp/pch.h"
#include "engineapp/EngineDelegate.h"

#include "oxygen/platform/PlatformFunctions.h"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	[self startGame];
}

-(void)startGame{
	// Move out of app container
	NSString* appFolder = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent];
	[NSFileManager.defaultManager changeCurrentDirectoryPath:appFolder];
	
	//Generate fake arguments
	NSString* appPath = NSBundle.mainBundle.bundlePath;
	int argc = 2;
	char** argv = new char*[argc];
	argv[0] =(char*)[appPath UTF8String];
	argv[1] =(char*)[appPath.stringByDeletingLastPathComponent UTF8String];

	// Create engine delegate and angine main instance
	{
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate);
		EngineMain::earlySetup();
		
		//Prepopulate with Steam ROM, if they want a different one this is the dev version so they can use config files.
		NSString* steamRomPath = [NSHomeDirectory() stringByAppendingString:@"/Library/Application Support/Steam/SteamApps/common/Sega Classics/uncompressed ROMs/Sonic_Knuckles_wSonic3.bin"];
		Configuration& config = Configuration::instance();
		config.mAppDataPath = *String(appFolder.UTF8String).toWString();
		if([NSFileManager.defaultManager fileExistsAtPath:steamRomPath]){
			std::wstring userRom = *String(steamRomPath.UTF8String).toWString();
			config.mRomPath = userRom;
			config.mLastRomPath = userRom;
		}

		myMain.execute(argc, argv);
	}
	
	//The execute call is blocking, so if we reach this point the user closed the game and we should kill the UI too.
	[NSApplication.sharedApplication terminate:self];
}
@end
