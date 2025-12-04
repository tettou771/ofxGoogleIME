#import "ofxGoogleIMEView.h"

@implementation ofxGoogleIMEView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _originalView = nil;
    }
    return self;
}

- (void)setOriginalView:(NSView *)view {
    _originalView = view;
}

// キーイベントは元のViewに転送
- (void)keyDown:(NSEvent *)event {
    if (_originalView) {
        [_originalView keyDown:event];
    }
}

- (void)keyUp:(NSEvent *)event {
    if (_originalView) {
        [_originalView keyUp:event];
    }
}

- (void)flagsChanged:(NSEvent *)event {
    if (_originalView) {
        [_originalView flagsChanged:event];
    }
}

// マウスイベントも転送
- (void)mouseDown:(NSEvent *)event {
    if (_originalView) {
        [_originalView mouseDown:event];
    }
}

- (void)mouseUp:(NSEvent *)event {
    if (_originalView) {
        [_originalView mouseUp:event];
    }
}

- (void)mouseMoved:(NSEvent *)event {
    if (_originalView) {
        [_originalView mouseMoved:event];
    }
}

- (void)mouseDragged:(NSEvent *)event {
    if (_originalView) {
        [_originalView mouseDragged:event];
    }
}

- (void)scrollWheel:(NSEvent *)event {
    if (_originalView) {
        [_originalView scrollWheel:event];
    }
}

- (void)rightMouseDown:(NSEvent *)event {
    if (_originalView) {
        [_originalView rightMouseDown:event];
    }
}

- (void)rightMouseUp:(NSEvent *)event {
    if (_originalView) {
        [_originalView rightMouseUp:event];
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

#pragma mark - NSTextInputClient Protocol (空実装でIME候補ウィンドウを抑制)

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    // 何もしない - IMEからの確定文字を無視
    // キー入力はkeyDown経由で処理される
}

- (void)doCommandBySelector:(SEL)selector {
    // 何もしない
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    // 何もしない - IMEの未確定文字列を無視することで候補ウィンドウを抑制
}

- (void)unmarkText {
    // 何もしない
}

- (NSRange)selectedRange {
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange)markedRange {
    return NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText {
    return NO;
}

- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {
    return nil;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange {
    // 画面外に配置してIME候補ウィンドウを見えなくする
    return NSMakeRect(-10000, -10000, 0, 0);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return NSNotFound;
}

@end
