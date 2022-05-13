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

#include "oxygen/base/CrashHandler.h"
#include "oxygen/base/PlatformFunctions.h"

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
	config.mAppDataPath = *String(myDocuments.path.UTF8String).toWString();
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
			Configuration& config = Configuration::instance();
			//TODO: Change this to match the current device:
			//config.mWindowSize = Vec2i(1200, 740);
			
			std::wstring resourcePath = *String([NSBundle.mainBundle.resourcePath stringByAppendingString:@"/data"].UTF8String).toWString();
			config.mGameDataPath = resourcePath;
			
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
		//This is literally just a square right now because I was trying to confirm that video works
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			printf("SDL could not be initialized!\n"
				   "SDL_Error: %s\n", SDL_GetError());
			return 0;
		}
		SDL_Window *window = SDL_CreateWindow("Sonic 3 AIR", 0, 0, 640, 400, SDL_WINDOW_SHOWN);
	   if(!window)
	   {
		   printf("Window could not be created!\n"
				  "SDL_Error: %s\n", SDL_GetError());
	   }
	   else
	   {
		   SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		   if(!renderer)
		   {
			   printf("Renderer could not be created!\nSDL_Error: %s\n", SDL_GetError());
		   }
		   else
		   {
			   SDL_Rect square;
			   square.x = 220;
			   square.y = 100;
			   square.w = 200;
			   square.h = 200;

			   while(true)
			   {
				   SDL_Event e;
				   SDL_WaitEvent(&e);
				   if(e.type == SDL_QUIT)
				   {
					   break;
				   }
				   SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
				   SDL_RenderClear(renderer);
				   SDL_SetRenderDrawColor(renderer, 0x55, 0x55, 0xFF, 0xFF);
				   SDL_RenderFillRect(renderer, &square);
				   SDL_RenderPresent(renderer);
			   }
			   SDL_DestroyRenderer(renderer);
		   }
		   SDL_DestroyWindow(window);
	   }
	}
	
	return YES;
}
