#pragma once

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

// NSTextInputClientプロトコルを空実装したNSView
// IMEの候補ウィンドウを抑制するために使用
@interface ofxGoogleIMEView : NSView <NSTextInputClient>

@property (nonatomic, weak) NSView *originalView;

- (void)setOriginalView:(NSView *)view;

@end

#endif
