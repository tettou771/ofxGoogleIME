#pragma once

#include <string>

// IME無効化に必要
#ifdef WIN32
#pragma comment(lib,"imm32.lib")
#elif defined __APPLE__
#include <Carbon/Carbon.h>
#endif

#include "ofMain.h"
using namespace std;

class ofxGoogleIME {
public:
	ofxGoogleIME();
	~ofxGoogleIME();

	void enable();
	void disable();
	void clear();

    void draw(ofEventArgs &args);
	void keyPressed(ofKeyEventArgs &key);
    void mousePressed(ofMouseEventArgs &mouse);

	bool isEnabled() { return enabled; }

    // u32stringとして持っている文字列を変換してgetする
	string getString();
    u32string getU32String();
	string getAfterHenkan(int l);
    string getAfterHenkanSubstr(int l, int begin, int end);
	string getBeforeHenkan();
    string getBeforeHenkanSubstr(int begin, int end);
	void setFont(string path, float fontSize);
    void setPos(ofVec2f p);
    void setPos(float x, float y);
    
protected:
	void draw(ofPoint pos);
	void draw(float x, float y);
    
    ofVec2f pos;
	bool enabled = false;

	// 入力されたキーのヒストリー
	char pastPressedKey;
	u32string beforeHenkan; // ひらがな化したあとの部分
    
    // テキスト入力エリア
	vector<u32string> line; // 変換後のテキスト。行ごとにvectorになっている
    // 選択範囲
    typedef tuple<int, int> TextSelectPos;
    TextSelectPos selectBegin, selectEnd;
    void selectCancel() {
        selectBegin = selectEnd = TextSelectPos(0, 0);
    }
    bool isSelected() {
        return selectBegin != selectEnd;
    }
    void selectAll() {
        selectBegin = TextSelectPos(0, 0);
        selectEnd = TextSelectPos(line.size() - 1, line.back().length());
    }
    void deleteSelected() {
        if (!isSelected()) return;
        
        int bl, bn, el, en;
        tie(bl, bn) = selectBegin;
        tie(el,en) = selectEnd;
        
        // 前後関係が逆の時は、入れ替える
        if (bl > el || (bl == el && bn > en)) {
            tie(el, en) = selectBegin;
            tie(bl, bn) = selectEnd;
        }

        int blen = (int)line[bl].length();
        if (blen < bn) bn = blen;
        
        int elen = (int)line[el].length();
        if (elen < en) en = elen;

        // 同じ行内で選択している時は、その行の文字列を削除
        // 行を跨ぐ場合は最初と最後の行の一部を削除して最初の行にマージ
        line[bl] = line[bl].substr(0, bn) + line[el].substr(el, elen - en);
        
        // 間の行を削除
        int delNum = el - bl;
        for (int i=0; i<delNum; ++i) {
            line.erase(line.begin() + bl + 1);
        }
    }
    
    
	// アルファベットの文字列をお尻だけひらがなに変換して追加するメソッド
    void toHiragana(u32string &str, int checkPos);

	ofJson json;

	// ひらがなの文字列をGoogleのAPIで変換し、選択モードに入る
	void henkan();
	// 選択された変換を確定する
	void kakutei();
    // 改行（行を増やしてカーソルを移す）
    void newLine();
    
    // 行移動
    void lineChange(int n);

    // 文字入力のカーソル位置
    int cursorLine; // 何行目を選択中か
    int cursorPos; // 変換後の何文字目か
    int cursorPosBeforeHenkan; // 変換前の何文字目か
    
    // 文字列の任意の位置に文字を追加するメソッド
    void addKey(u32string &target, const char &c, int &p);
    void addStr(u32string &target, const u32string &str, int &p);
    
    // カーソルの位置の文字を一つ削除するメソッド
    void backspaceCharacter(u32string &str, int &pos, bool lineMerge = false);
    void deleteCharacter(u32string &str, int &pos, bool lineMerge = false);
    
    // 確定前の変換中の文字列の候補リスト
	// 描画に使うので、stringのまま持っておく
	vector<vector<u32string>> candidate;
	vector<int> candidateSelected;
	void candidateToggle(int toggle);
	void candidateFocusToggle(int toggle);
	// 変換前の文字列（APIで帰ってきた、分割後の文字列）
	vector<u32string> candidateKana;
    
	// 変換中のとき、フォーカスの当たっているcandidate
	int candidateFocus;

	// 変換中に、Shiftキーを押しながら左右に動かすと
	// 変換候補の切れ目が変わる。そのメソッド
	void candidateLengthChange(bool longer);
	

#ifdef WIN32
	// 互換性確保のため、UTF-32 を Shift-JIS に変換するときに使う
	// getString(), getInputText() のときに戻り値を Shift-JIS にしないと
	// 文字表示などで化けてしまうため
	string UTF32toSjis(u32string srcUTF8);
#endif

	string percentEnc(u32string str);

public:
	static string UTF32toUTF8(const u32string &u32str);
    static string UTF32toUTF8(const char32_t &u32char);
	static u32string UTF8toUTF32(const string &str);
private:
	// ローマ字-ひらがな変換用の辞書
	map<u32string, u32string> romajiToKana;
	void makeDictionary();

	// State
	enum State {
		Eisu,
		Kana,
		KanaNyuryoku,
		KanaHenkan
	};
	State state;

	// 描画用のフォント
	ofTrueTypeFont font;
    
    // モード変更
    void toggleMode();

#ifdef __APPLE__
    // OS側のIME状態を監視して同期
    void startIMEObserver();
    void stopIMEObserver();
    void syncWithSystemIME();
    static void onInputSourceChanged(CFNotificationCenterRef center,
                                     void *observer,
                                     CFNotificationName name,
                                     const void *object,
                                     CFDictionaryRef userInfo);

    // IME候補ウィンドウを抑制するためのView管理
    void setupIMEInterceptView();
    void removeIMEInterceptView();
    void *imeInterceptView;  // ofxGoogleIMEView*
    void *originalContentView;  // NSView*
#endif

    // 変換候補を選ぶときの動き (0-1)
    float movingY;
    
    // カーソルの点滅に使う変数
    float cursorBlinkOffsetTime;
};

