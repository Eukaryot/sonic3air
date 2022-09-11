//
//  SetupViewController.m
//  sonic3air
//

#import "SetupViewController.h"
#import <zlib.h>

@interface SetupViewController ()

@end

@implementation SetupViewController

- (void)awakeFromNib {
	[super awakeFromNib];
	_ReadyToPlay = NO;
	NSTimeZone* systemTime = [NSTimeZone systemTimeZone];
	float timeZoneHour = systemTime.secondsFromGMT / 60.0 / 60.0;
	
	if([[NSLocale.preferredLanguages.firstObject substringToIndex:2] isEqualToString:@"ja"]){
		//Prefered language is in the format ja-JP, ja-US, etc.
		//Display the Japanese cart if the user's primary language is set to Japanese
		_imgCart.image = [NSImage imageNamed:@"s3kcart_jp"];
	}
	else if(timeZoneHour <= -3.0){
		_imgCart.image = [NSImage imageNamed:@"s3kcart_us"];
	}
	else {
		_imgCart.image = [NSImage imageNamed:@"s3kcart_eu"];
	}
}

- (IBAction)btnVisitSteam:(id)sender {
	[NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://store.steampowered.com/app/71162"]];
}

- (IBAction)btnFindSteamRom:(id)sender {
	NSString* steamRomPath = [NSHomeDirectory() stringByAppendingString:@"/Library/Application Support/Steam/SteamApps/common/Sega Classics/uncompressed ROMs/Sonic_Knuckles_wSonic3.bin"];
	if([NSFileManager.defaultManager fileExistsAtPath:steamRomPath]){
		self.RomPath = steamRomPath;
		self.ReadyToPlay = YES;
		[_wndSetup close];
	}
	else{
		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:@"Game not found"];
		[alert setInformativeText:@"A Steam installation of Sonic 3 & Knuckles could not be located on your maachine. If one exists, please locate the file named Sonic_Knuckles_wSonic3.bin and manually select it."];
		[alert addButtonWithTitle:@"OK"];
		[alert setAlertStyle:NSAlertStyleCritical];
		[alert beginSheetModalForWindow:_wndSetup completionHandler:^(NSModalResponse returnCode) { }];
	}
}

- (IBAction)btnPickRomFile:(id)sender {
	NSOpenPanel* nop = [NSOpenPanel openPanel];
	nop.allowedFileTypes = @[@"bin",@"gen"];
	[nop beginSheetModalForWindow:_wndSetup completionHandler:^(NSModalResponse result) {
		if(result == NSModalResponseOK){
			bool validFile = NO;
			long long fileSize = [NSFileManager.defaultManager attributesOfItemAtPath:nop.URL.path error:nil].fileSize;
			if(fileSize == 0x400000){
				//"RomCheck":   "size=0x400000, overwrite=0x2001f0:0x4a, checksum=0x0c06aa82",
				NSData* file = [NSData dataWithContentsOfURL:nop.URL];
				unsigned char* fileData = malloc(0x400000);
				[file getBytes:fileData];
				fileData[0x2001f0] = 0x4a;
				unsigned long checksum = crc32(0, fileData, 0x400000);
				if(checksum == 0x0c06aa82){
					validFile = YES;
				}
				free(fileData);
			}
			if(validFile){
				self.RomPath = nop.URL.path;
				self.ReadyToPlay = YES;
				[self->_wndSetup close];
			}
			else{
				NSAlert *alert = [[NSAlert alloc] init];
				[alert setMessageText:@"Incorrect file"];
				[alert setInformativeText:@"The selected file is not Sonic 3 and Knuckles."];
				[alert addButtonWithTitle:@"OK"];
				[alert setAlertStyle:NSAlertStyleCritical];
				[alert beginSheetModalForWindow:self->_wndSetup completionHandler:^(NSModalResponse returnCode) { }];
			}
		}
	}];
}

@end
