#pragma once

// 文字コードの変換に必要
#include <codecvt>

// IME無効化に必要
#ifdef WIN32
#pragma comment(lib,"imm32.lib")
#elif defined __APPLE__
#endif

#include "ofMain.h"

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
	string getAll();
	string getAfterHenkan(int l);
    string getAfterHenkanSubstr(int l, int begin, int end);
	string getBeforeHenkan();
    string getBeforeHenkanSubstr(int begin, int end);
	void setFont(string path, float fontSize);
    void setPos(ofVec2f p);
    void setPos(float x, float y);
    
private:
	void draw(ofPoint pos);
	void draw(float x, float y);
    
    ofVec2f pos;
	bool enabled;

	// 入力されたキーのヒストリー
	char pastPressedKey;
	u32string beforeHenkan; // ひらがな化したあとの部分
	vector<u32string> line; // 変換後のテキスト。行ごとにvectorになっている

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

	// リンクエラーの出る変換器
	//wstring_convert<codecvt_utf8<char32_t>, char32_t> convert8_32;
	
	// 変換器(UTF8 UTF32)
	// char32_t を使うとVS2015でリンクエラーとなるので、unit32_t を使っている
	// ソース Qiita http://qiita.com/benikabocha/items/1fc76b8cea404e9591cf
	//wstring_convert<codecvt_utf8<uint32_t>, uint32_t> convert8_32;
    
    // 上記でもエラーが出たので、ChatGPTで聞いた変換器lolo
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert8_32;

	string UTF32toUTF8(const u32string &u32str);
	u32string UTF8toUTF32(const string &str);
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
    
    // 変換候補を選ぶときの動き (0-1)
    float movingY;
    
    // カーソルの点滅に使う変数
    float cursorBlinkOffsetTime;
};

