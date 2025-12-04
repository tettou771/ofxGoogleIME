// Windows固有の実装

#ifdef WIN32

#include "ofxGoogleIME.h"

void ofxGoogleIME::startIMEObserver() {
    // 初期状態を取得
    syncWithSystemIME();

    // updateイベントでポーリング
    ofAddListener(ofEvents().update, this, &ofxGoogleIME::checkIMEState);
}

void ofxGoogleIME::stopIMEObserver() {
    ofRemoveListener(ofEvents().update, this, &ofxGoogleIME::checkIMEState);
}

void ofxGoogleIME::checkIMEState(ofEventArgs &args) {
    syncWithSystemIME();
}

void ofxGoogleIME::syncWithSystemIME() {
    // フォアグラウンドウィンドウのIMEコンテキストを取得
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return;

    HIMC hIMC = ImmGetContext(hwnd);
    if (!hIMC) return;

    DWORD dwConvMode = 0, dwSentMode = 0;
    ImmGetConversionStatus(hIMC, &dwConvMode, &dwSentMode);

    // 前回と変化があった場合のみ処理
    if (dwConvMode != lastIMEConversionMode) {
        lastIMEConversionMode = dwConvMode;

        // 日本語モードかどうかを判定
        bool isJapanese = (dwConvMode & IME_CMODE_NATIVE) != 0;

        if (isJapanese) {
            // 日本語モードに切り替え
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

    ImmReleaseContext(hwnd, hIMC);
}

#endif
