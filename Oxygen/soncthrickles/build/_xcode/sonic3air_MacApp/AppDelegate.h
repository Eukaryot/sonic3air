//
//  AppDelegate.h
//  sonic3air
//

#import <Cocoa/Cocoa.h>
#import "SetupViewController.h"

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (weak) IBOutlet SetupViewController *SetupVC;


@end

