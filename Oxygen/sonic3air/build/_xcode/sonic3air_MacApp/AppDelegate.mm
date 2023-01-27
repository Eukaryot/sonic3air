//
//  AppDelegate.m
//  sonic3air
//

#import "AppDelegate.h"
#define RMX_LIB
#include "sonic3air/pch.h"
#include "sonic3air/EngineDelegate.h"
#include "sonic3air/version.inc"

#include "oxygen/platform/CrashHandler.h"
#include "oxygen/platform/PlatformFunctions.h"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *wndSetup;
@end

@implementation AppDelegate

EngineDelegate _myDelegate;
EngineMain* _myMain;

-(void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	_wndSetup.delegate = self;
	INIT_RMX;
	INIT_RMXEXT_OGGVORBIS;
	_myMain = new EngineMain(_myDelegate);
	
	Configuration& config = Configuration::instance();
	//Files for this app will go into ~/Library/Application Support/sonic3air/
	NSURL* myAppSupport = [NSFileManager.defaultManager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil];
	if(myAppSupport){
		myAppSupport = [myAppSupport URLByAppendingPathComponent:@"sonic3air/"];
		if(![NSFileManager.defaultManager fileExistsAtPath:myAppSupport.path]){
			[NSFileManager.defaultManager createDirectoryAtURL:myAppSupport withIntermediateDirectories:kAEYes attributes:nil error:nil];
		}
		PlatformFunctions::mExAppDataPath = *String(myAppSupport.path.UTF8String).toWString();
	}
#ifdef ENDUSER
	//This puts us into /sonic3air/Sonic3AIR/ but we can't let dev builds put files directly into Application Support.
	config.mAppDataPath = *String([myAppSupport.path stringByAppendingString:@"/Sonic3AIR/"].UTF8String).toWString();
	//Mount the end user build into its application support directory, because data/cache will be created
	[NSFileManager.defaultManager changeCurrentDirectoryPath:myAppSupport.path];
#else
	//The developer build is mounted relative to the app directory
	NSString* appFolder = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent];
	[NSFileManager.defaultManager changeCurrentDirectoryPath:appFolder];
#endif
	
	bool loadedSettings = config.loadSettings(config.mAppDataPath + L"settings.json", Configuration::SettingsType::STANDARD);
	config.loadSettings(config.mAppDataPath + L"settings_input.json", Configuration::SettingsType::INPUT);
	config.loadSettings(config.mAppDataPath + L"settings_global.json", Configuration::SettingsType::GLOBAL);
	if (loadedSettings)
	{
		NSString* romPath = [NSString stringWithCString:((char*)config.mLastRomPath.c_str()) encoding:NSUTF8StringEncoding];
		if(![NSFileManager.defaultManager fileExistsAtPath:romPath]){
			loadedSettings = NO; //Bad settings are as good as no settings.
		}
	}
	if (loadedSettings)
	{
		config.mRomPath = config.mLastRomPath;
#if DEBUG
		config.mScriptsDir = L"scripts"; //Why is this not loaded again?
#endif
		[self startGame:nil];
	}
	else{
		//Show setup, there were no valid settings.
		[_wndSetup makeKeyAndOrderFront:self];
	}
}


-(void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
}

-(void)windowWillClose:(NSNotification *)notification{
	if(notification.object == _wndSetup){
		if(self.SetupVC.ReadyToPlay){
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 33 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
				[self startGame:self->_SetupVC.RomPath];
			});
		}
		else{
			//Setup closed by user, exit the app.
			[NSApplication.sharedApplication terminate:self];
		}
	}
}

- (IBAction)mnuOpenMods:(id)sender {
#ifdef ENDUSER
	NSURL* myAppSupport = [NSFileManager.defaultManager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil];
	if(myAppSupport){
		myAppSupport = [myAppSupport URLByAppendingPathComponent:@"sonic3air/Sonic3AIR/mods"];
		[NSWorkspace.sharedWorkspace openURL:myAppSupport];
	}
#else
	//Using this option in developer mode is pointless since files are relative to the app
	[NSWorkspace.sharedWorkspace openURL:[NSBundle.mainBundle.bundleURL URLByDeletingLastPathComponent]];
#endif
}

-(void)startGame:(NSString*)romPath{
	try
	{
		Configuration& config = Configuration::instance();
#ifdef ENDUSER
		config.mWindowSize = Vec2i(1200, 740); //Why is the default 960x720?
#endif
		if(romPath){
			config.loadSettings(config.mAppDataPath + L"settings.json", Configuration::SettingsType::STANDARD);
			config.loadSettings(config.mAppDataPath + L"settings_input.json", Configuration::SettingsType::INPUT);
			config.loadSettings(config.mAppDataPath + L"settings_global.json", Configuration::SettingsType::GLOBAL);
			
			std::wstring userRom = *String(romPath.UTF8String).toWString();
			config.mRomPath = userRom;
			config.mLastRomPath = userRom;
			
			// Save our updated settings
			config.saveSettings();
		}
		
#ifdef ENDUSER
		std::wstring resourcePath = *String([NSBundle.mainBundle.resourcePath stringByAppendingString:@"/data"].UTF8String).toWString();
		config.mGameDataPath = resourcePath;
#else
		//Make development build look relative to the app instead
		std::wstring resourcePath = *String([NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent].UTF8String).toWString();
		config.mGameDataPath = resourcePath;
#endif
		
		NSString* appPath = NSBundle.mainBundle.bundlePath;
		int argc = 1;
		char** argv = new char*[argc];
		argv[0] =(char*)[appPath UTF8String];
		_myMain->execute(argc, (char**)argv);
	}
	catch (const std::exception& e)
	{
		RMX_ERROR("Caught unhandled exception in main loop: " << e.what(), );
	}
	//The execute call is blocking, so if we reach this point the user closed the game and we should kill the UI too.
	[NSApplication.sharedApplication terminate:self];
}
@end
