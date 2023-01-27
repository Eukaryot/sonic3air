//
//  main.m
//  sonic3air-ios
//
// NOTE: The iOS build is not functional at this time. You can compile & run it but
// you'll get a black screen. The code was checked in early because the macOS project
// was restructured to build the same way as iOS and it was easier to keep them together.

#import <UIKit/UIKit.h>
#define RMX_LIB
#include "sonic3air/pch.h"
#include "sonic3air/EngineDelegate.h"
#include "sonic3air/version.inc"

#include "oxygen/platform/CrashHandler.h"
#include "oxygen/platform/PlatformFunctions.h"

EngineDelegate _myDelegate;
EngineMain* _myMain;

int main(int argc, char * argv[]) {
	INIT_RMX;
	INIT_RMXEXT_OGGVORBIS;
	_myMain = new EngineMain(_myDelegate);
	
	Configuration& config = Configuration::instance();
	//Files for this app will go into ~/Documents/
	NSURL* myDocuments = [NSFileManager.defaultManager URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil];
	if(myDocuments){
		NSURL* modsFolder = [myDocuments URLByAppendingPathComponent:@"mods/"];
		if(![NSFileManager.defaultManager fileExistsAtPath:modsFolder.path]){
			//Creating the mods folder is what causes the game to appear in the Files app
			[NSFileManager.defaultManager createDirectoryAtURL:modsFolder withIntermediateDirectories:YES attributes:nil error:nil];
		}
		PlatformFunctions::mExAppDataPath = *String(myDocuments.path.UTF8String).toWString();
	}
	config.mAppDataPath = *String([myDocuments.path stringByAppendingString:@"/"].UTF8String).toWString();
	//Mount the end user build into its application support directory, because data/cache will be created
	[NSFileManager.defaultManager changeCurrentDirectoryPath:myDocuments.path];
	
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
	else{
		NSString* steamRomPath = [myDocuments.path stringByAppendingString:@"/Sonic_Knuckles_wSonic3.bin"];
		if([NSFileManager.defaultManager fileExistsAtPath:steamRomPath]){
			config.mLastRomPath = *String(steamRomPath.UTF8String).toWString();
			loadedSettings = YES;
		}
	}
	
	//loadedSettings = NO;//TEMP code
	if (loadedSettings)
	{
		config.mRomPath = config.mLastRomPath;
#if DEBUG
		config.mScriptsDir = L"scripts"; //Why is this not loaded again?
#endif
		
		try
		{
			std::wstring resourcePath = *String([NSBundle.mainBundle.resourcePath stringByAppendingString:@"/data"].UTF8String).toWString();
			config.mGameDataPath = resourcePath;
			//Force software mode until someone figures out why the SDL window isn't being created properly in GL mode.
			config.mAutoDetectRenderMethod = NO;
			config.mRenderMethod = Configuration::RenderMethod::SOFTWARE;
			config.saveSettings();
			
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
	}
	else{
		//No game data installed, show information explaining what to do
		const SDL_MessageBoxButtonData buttons[] = {
			{ 0, 0, "OK" },
		};
		const SDL_MessageBoxData messageboxdata = {
			SDL_MESSAGEBOX_INFORMATION, NULL,
			"ROM not installed",
			"Please install Sonic_Knuckles_wSonic3.bin into the Sonic 3 AIR folder using the Files app, then relaunch the game. The game will now exit when you tap OK.",
			SDL_arraysize(buttons), buttons, NULL
		};
		int buttonid;
		if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
			SDL_Log("error displaying message box");
			return NO;
		}
		//Close the app, which is not something iOS apps are supposed to do on their own.
		UIApplication *app = [UIApplication sharedApplication];
		[app performSelector:@selector(suspend)];
		[NSThread sleepForTimeInterval:2.0];
		exit(0);
	}
	
	return YES;
}
