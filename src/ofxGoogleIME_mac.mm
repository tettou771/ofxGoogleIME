// macOS固有の実装

#ifdef __APPLE__

#include "ofxGoogleIME.h"
#include "ofxGoogleIMEView.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void ofxGoogleIME::startIMEObserver() {
    CFNotificationCenterAddObserver(
        CFNotificationCenterGetDistributedCenter(),
        this,
        onInputSourceChanged,
        kTISNotifySelectedKeyboardInputSourceChanged,
        NULL,
        CFNotificationSuspensionBehaviorDeliverImmediately
    );
}

void ofxGoogleIME::stopIMEObserver() {
    CFNotificationCenterRemoveObserver(
        CFNotificationCenterGetDistributedCenter(),
        this,
        kTISNotifySelectedKeyboardInputSourceChanged,
        NULL
    );
}

void ofxGoogleIME::onInputSourceChanged(CFNotificationCenterRef center,
                                        void *observer,
                                        CFNotificationName name,
                                        const void *object,
                                        CFDictionaryRef userInfo) {
    // observerはofxGoogleIMEインスタンスへのポインタ
    ofxGoogleIME *ime = static_cast<ofxGoogleIME*>(observer);
    if (ime) {
        ime->syncWithSystemIME();
    }
}

void ofxGoogleIME::syncWithSystemIME() {
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    if (source) {
        CFStringRef sourceID = (CFStringRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
        if (sourceID) {
            // 日本語入力ソースかどうかを判定
            // "Japanese"または"Hiragana"が含まれていれば日本語モード
            bool isJapanese = (CFStringFind(sourceID, CFSTR("Japanese"), 0).location != kCFNotFound) ||
                              (CFStringFind(sourceID, CFSTR("Hiragana"), 0).location != kCFNotFound);

            if (isJapanese) {
                // 日本語モードに切り替え（ただし変換中は維持）
                if (state == Eisu) {
                    state = Kana;
                }
            } else {
                // 英数モードに切り替え
                if (state == Kana || state == KanaNyuryoku) {
                    // 入力中の文字があれば確定
                    if (beforeHenkan.length() > 0) {
                        addStr(line[cursorLine], beforeHenkan, cursorPos);
                        beforeHenkan = U"";
                        cursorPosBeforeHenkan = 0;
                    }
                    state = Eisu;
                } else if (state == KanaHenkan) {
                    kakutei();
                    state = Eisu;
                }
            }
        }
        CFRelease(source);
    }
}

void ofxGoogleIME::setupIMEInterceptView() {
    if (imeInterceptView != nullptr) return;

    // GLFWウィンドウからNSWindowを取得
    GLFWwindow* glfwWin = (GLFWwindow*)ofGetWindowPtr()->getWindowContext();
    if (!glfwWin) return;

    NSWindow* nsWindow = glfwGetCocoaWindow(glfwWin);
    if (!nsWindow) return;

    NSView* contentView = [nsWindow contentView];
    if (!contentView) return;

    // 元のcontentViewを保存
    originalContentView = (__bridge void*)contentView;

    // カスタムViewを作成
    ofxGoogleIMEView* customView = [[ofxGoogleIMEView alloc] initWithFrame:[contentView frame]];
    [customView setOriginalView:contentView];
    [customView setAutoresizingMask:[contentView autoresizingMask]];

    imeInterceptView = (__bridge_retained void*)customView;

    // contentViewの上にカスタムViewを追加してFirstResponderにする
    [contentView addSubview:customView];
    [nsWindow makeFirstResponder:customView];
}

void ofxGoogleIME::removeIMEInterceptView() {
    if (imeInterceptView == nullptr) return;

    ofxGoogleIMEView* customView = (__bridge_transfer ofxGoogleIMEView*)imeInterceptView;
    imeInterceptView = nullptr;

    // カスタムViewを削除
    [customView removeFromSuperview];

    // 元のcontentViewをFirstResponderに戻す
    if (originalContentView != nullptr) {
        NSView* origView = (__bridge NSView*)originalContentView;
        NSWindow* nsWindow = [origView window];
        if (nsWindow) {
            [nsWindow makeFirstResponder:origView];
        }
        originalContentView = nullptr;
    }
}

#endif
