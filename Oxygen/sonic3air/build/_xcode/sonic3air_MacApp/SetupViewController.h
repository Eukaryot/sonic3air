//
//  SetupViewController.h
//  sonic3air
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface SetupViewController : NSViewController
@property (weak) IBOutlet NSImageView *imgCart;
@property (weak) IBOutlet NSWindow *wndSetup;
@property bool ReadyToPlay;
@property NSString* RomPath;
@end

NS_ASSUME_NONNULL_END
