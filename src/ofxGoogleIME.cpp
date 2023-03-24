#include "ofxGoogleIME.h"

ofxGoogleIME::ofxGoogleIME() {
	ofSetEscapeQuitsApp(false);
	makeDictionary();
    movingY = 0;
}

ofxGoogleIME::~ofxGoogleIME() {
}

void ofxGoogleIME::enable() {
	enabled = true;
	ofRegisterKeyEvents(this);

	clear();
}

void ofxGoogleIME::disable() {
	enabled = false;
	ofUnregisterKeyEvents(this);
}

void ofxGoogleIME::clear() {
	beforeKana = U"";
	beforeHenkan = U"";
	afterHenkan = U"";
	
	// 候補リストを空にする
	for (auto c : candidate) {
		c.clear();
	}
	candidate.clear();
	candidateSelected.clear();
	candidateFocus = 0;
	candidateKana.clear();
}

void ofxGoogleIME::keyPressed(ofKeyEventArgs & key) {
	switch (key.key) {
		// escで文字をクリア
	case OF_KEY_ESC:
		clear();
		break;

		// BSで一文字削除
	case OF_KEY_BACKSPACE:
		// 一文字以上あれば、後ろの一文字を削除する
		if (beforeKana.length() > 0) {
			beforeKana = beforeKana.substr(0, beforeKana.length() - 1);
		}
		else if (beforeHenkan.length() > 0) {
			beforeHenkan = beforeHenkan.substr(0, beforeHenkan.length() - 1);
		}
		else if (afterHenkan.length() > 0) {
			afterHenkan = afterHenkan.substr(0, afterHenkan.length() - 1);
		}
		break;

		// スペースキー（stateのトグルか、変換）
	case ' ':
		// Ctrl + Space で Eisu, Kana トグル
		if ((ofGetKeyPressed(OF_KEY_COMMAND) && key.key == ' ')) {
            toggleMode();
		}

		// 入力切替ではない場合
		else {
			switch (state) {
			case Eisu:
				afterHenkan += U" ";
				break;
			case Kana:
				afterHenkan += U"　";
				break;
			case KanaNyuryoku:
				henkan();
				break;
			case KanaHenkan:
				// 変換候補をトグルする
				candidateToggle(1);
				break;
			}
		}

		break;
            
    case '`':
        // Alt + '`' で Eisu, Kana トグル
        if (ofGetKeyPressed(OF_KEY_ALT)) {
            toggleMode();
        }

        break;
		// 上下カーソルキー
	case OF_KEY_UP:
		switch (state) {
		case KanaHenkan:
			candidateToggle(-1);
			break;
		}
		break;
	case OF_KEY_DOWN:
		switch (state) {
		case KanaHenkan:
			candidateToggle(1);
			break;
		}
		break;

		// 左右カーソルキー
	case OF_KEY_LEFT:
		switch (state) {
		case KanaHenkan:
			// shiftを押していたら候補の長さを帰る
			if (ofGetKeyPressed(OF_KEY_SHIFT)) candidateLengthChange(false);
			else candidateFocusToggle(-1);
			break;
		}
		break;
	case OF_KEY_RIGHT:
		switch (state) {
		case KanaHenkan:
			// shiftを押していたら候補の長さを帰る
			if (ofGetKeyPressed(OF_KEY_SHIFT)) candidateLengthChange(true);
			else candidateFocusToggle(1);
			break;
		}
		break;

		// 決定キーで実行、キーをクリアする
	case OF_KEY_RETURN:
		// 何らかの文字列があれば
		// スタック上の文字列を確定して追加
		switch (state) {
		case Eisu:
		case Kana:
			afterHenkan += U"\n";
			break;

		case KanaNyuryoku:
			beforeHenkan += beforeKana;
			afterHenkan += beforeHenkan;
			beforeKana = U"";
			beforeHenkan = U"";
			state = Kana;
			break;

		case KanaHenkan:
			kakutei();
			state = Kana;
			break;
		}

		// 通常の文字キーの場合
	default:

		// 文字の入力
		if (32 <= key.key && key.key <= 126) {
			// 変換候補を選んでいるときに文字を入力すると
			// その時点で確定する
			if (state == KanaHenkan) {
				kakutei();
				state = KanaNyuryoku;
			}

			beforeKana += key.key;

			switch (state) {
			case Eisu:
				afterHenkan += beforeKana;
				beforeKana = U"";
				break;

				// かな系のstate
			case Kana:
				alphabetToHiragana(beforeKana, beforeHenkan);
				state = KanaNyuryoku;
				break;
			case KanaNyuryoku:
				alphabetToHiragana(beforeKana, beforeHenkan);
				break;
			}

			break;
		}
	}

	// 直前に押したキーを保存
	pastPressedKey = key.key;
}

void ofxGoogleIME::keyReleased(ofKeyEventArgs & key) {
}

string ofxGoogleIME::getAll() {
	return UTF32toUTF8(afterHenkan + beforeHenkan + beforeKana);
}

string ofxGoogleIME::getAfterHenkan() {
	return UTF32toUTF8(afterHenkan);
}

string ofxGoogleIME::getBeforeHenkan() {
	return UTF32toUTF8(beforeHenkan + beforeKana);
}

void ofxGoogleIME::setFont(string path, float fontSize) {
	ofTrueTypeFontSettings settings(path, fontSize);
	settings.addRanges(ofAlphabet::Latin);
	settings.addRanges(ofAlphabet::Japanese);
	font.load(settings);
}

void ofxGoogleIME::draw(ofPoint pos) {
	draw(pos.x, pos.y);
}

// 入力中の文字列をx,y座標を始点として描画
void ofxGoogleIME::draw(float x, float y) {
	if (!font.isLoaded()) {
		ofLogError("ofxGoogleIME") << "font is not loaded." << endl;
		return;
	}

	float fontSize = font.getSize();
    movingY *= 0.7;

	// それぞれの文字列
	string after = getAfterHenkan();
	string before = getBeforeHenkan();
	float afterW = font.getStringBoundingBox(after, 0, 0).width;
	vector<float> candidateW;
	float candidateTotalW = 0;
	for (int i = 0; i < candidate.size(); ++i) {
		auto utf8String = UTF32toUTF8(candidate[i][candidateSelected[i]]);
		candidateW.push_back(font.getStringBoundingBox(utf8String, 0, 0).width);
		candidateTotalW += candidateW.back();
	}
	float beforeW = font.getStringBoundingBox(before, 0, 0).width;
	float margin = font.getSize() * 0.1;

	ofPushMatrix();
	switch (state) {
	case Kana:
	case Eisu:
		ofTranslate(x, y);

		// 確定後
		//ofSetColor(30);
		font.drawString(getAfterHenkan(), 0, 0);

		break;
	case KanaNyuryoku:
		ofTranslate(x, y);

		// 確定後
		//ofSetColor(30);
		font.drawString(getAfterHenkan(), 0, 0);

		// 変換前
		//ofSetColor(180, 180, 0);
		font.drawString(getBeforeHenkan(), margin + afterW, 0);
		break;
	case KanaHenkan:
		ofTranslate(x, y);

		// 確定後
		//ofSetColor(30);
		font.drawString(getAfterHenkan(), 0, 0);

		ofTranslate(afterW + margin, 0);

		// 選択されたものだけハイライトする
		for (int i = 0; i < candidate.size(); ++i) {
			auto current = UTF32toUTF8(candidate[i][candidateSelected[i]]);
            
            // 変換中はアンダーバーを引く
            ofSetLineWidth(2);
            ofDrawLine(0, fontSize * 0.2, candidateW[i], fontSize * 0.2);

			// フォーカスが合っている場合はハイライトして、候補も書く
			if (i == candidateFocus) {
                ofPushStyle();
                ofFill();
                ofSetColor(90, 100);
				ofDrawRectangle(0, -fontSize * 1.2, candidateW[i], fontSize * 1.4);
                ofPopStyle();

                // 上下に他の候補も含めて並べる
                for (int j = 0; j < candidate[i].size(); ++j) {
                    float offsetY = (j - (int)candidateSelected[i] + movingY) * fontSize * 1.5;
                    auto str = UTF32toUTF8(candidate[i][j]);
                    font.drawString(str, 0, offsetY);
                }
            }
            else {
                // 洗濯中でなければ一つだけ描画
                font.drawString(current, 0, 0);
            }
            
			ofTranslate(candidateW[i] + margin, 0);
		}
		break;
	}

	ofPopMatrix();
}

void ofxGoogleIME::alphabetToHiragana(u32string & in, u32string & out) {
	// テーブルから該当のひらがなを探す
	int iMax = MAX(3, in.length());
	for (int i = 3; i > 0; --i) {
		if (in.length() >= i) {
			// うしろからi文字切り取った文字列
			u32string s = in.substr(in.length() - i, i);
			//cout << "s: " << UTF32toUTF8(s) << endl;

			// その文字列がひらがなに対応しているかどうか
			bool keyExist = romajiToKana.count(s) != 0;

			// 対応しているものが見つかったら、それをkanaStackに入れる
			if (keyExist) {
				u32string kana = romajiToKana[s];
				in = in.substr(0, in.length() - i);

				// もし n が一文字だけ残っていたらそれを "ん" にする
				if (in == U"n") {
					kana = U"ん" + kana;
					in = in.substr(0, in.length() - 1);
				}
				// アルファベットが残ったら頭につけたままにする
				kana = in + kana;

				out += kana;

				// ここで処理を終了する
				// returnしなければ、この次の "っ" 判定にうつる
				return;
			}
		}
	}

	// 2連続で同じアルファベットだったら "っ" にする
	if (in.length() >= 2 && in[0] == in[1]) {
		in = in.substr(in.length() - 1, 1);
		out += U"っ";
		return;
	}
}

void ofxGoogleIME::henkan() {
	state = KanaHenkan;

	// tryつきlog
	auto tryCout = [](ofJson a, int n) {
		for (int i = 0; i < n; ++i) {
			cout << '\t';
		}
		try {
            string astr = a;
			cout << astr << endl;
			return true;
		}
		catch (exception e) {
			cout << e.what() << endl;
			return false;
		}
	};

	// もし、beforeKana の最後に n が残っていたら それを ん にする
	if (beforeKana.length() > 0 && beforeKana.substr(beforeKana.length() - 1, 1) == U"n") {
		beforeKana = beforeKana.substr(0, beforeKana.length() - 1);
		beforeHenkan += beforeKana + U"ん";
	}
	else {
		beforeHenkan += beforeKana;
	}

	string encoded = percentEnc(beforeHenkan);
	cout << "percentEnc: " << encoded << endl;
    
    auto response =  ofLoadURL("http://www.google.com/transliterate?langpair=ja-Hira|ja&text=" + encoded);
    
    // jsonとしてパースできるかどうか試す
    bool result = false;
    try {
        json = ofJson::parse(response.data);
        result = true;
    }
    catch (std::exception& e) {
        // エラーをキャッチして、メッセージを出力する
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

#ifdef _DEBUG
	// デバッグ用に変換結果を保存
	ofFile f;
	f.open("googleAPIresult.txt", ofFile::Append);
	f << "encoded: " << encoded << endl;
	f << json.dump();
	f.close();
#endif

	if (result) {
		// json内のデータ（変換候補）を集める
		for (auto L1 : json) {
			tryCout(L1, 0);
			for (ofJson L2 : L1) {
				bool L2Exist = tryCout(L2, 1);
                
				// tryCoutが成功したら、変換前のかなが入っているので
				// それを集める
				if (L2Exist) {
                    string L2str = L2;
					candidateKana.push_back(UTF8toUTF32(L2str));
				}

				// L2Exist は、各カッコのときL3が存在する
				// なので、tryCoutが失敗したときにL3を調べる
				else {
					// 候補リストを追加
					candidate.push_back(vector<u32string>());
					candidateSelected.push_back(0);

					for (auto L3 : L2) {
						bool L3Exist = tryCout(L3, 2);
						if (L3Exist) {
                            string L3str = L3;

							// 候補リストの中に入れる
							candidate.back().push_back(UTF8toUTF32(L3str));
						}
					}
				}
			}
		}
	}
	else {
		cout << "GJI API error." << endl;
	}

	beforeHenkan = U"";
	beforeKana = U"";
}

void ofxGoogleIME::kakutei() {
	// 選択された候補を確定していく
	for (int i = 0; i < candidate.size(); ++i) {
		afterHenkan += candidate[i][candidateSelected[i]];
	}

	beforeKana = U"";
	beforeHenkan = U"";

	// 候補リストを空にする
	for (auto c : candidate) {
		c.clear();
	}
	candidate.clear();
	candidateSelected.clear();
	candidateFocus = 0;
	candidateKana.clear();

}

// 変換候補をトグルする
// 1でひとつ下に、-1で1つ上の候補にする（という使い方を想定している）
void ofxGoogleIME::candidateToggle(int toggle) {
    int past = candidateSelected[candidateFocus];
	candidateSelected[candidateFocus] += toggle;
	if (candidateSelected[candidateFocus] < 0) {
		candidateSelected[candidateFocus] = (int)candidate[candidateFocus].size() - 1;
	}
	if (candidate[candidateFocus].size() <= candidateSelected[candidateFocus]) {
		candidateSelected[candidateFocus] = 0;
	}
    int current = candidateSelected[candidateFocus];
    
    movingY = current - past;
}

void ofxGoogleIME::candidateFocusToggle(int toggle) {
	candidateFocus += toggle;
	if (candidateFocus < 0) candidateFocus = 0;
	if (candidateFocus >= candidate.size()) candidateFocus = (int)candidate.size() - 1;
}

void ofxGoogleIME::candidateLengthChange(bool longer) {
	// 長さを変更する
	// 長くする場合
	if (longer) {
		// 既に最後の文字列だったら無視する
		if (candidateFocus == candidate.size() - 1) return;

		candidateKana[candidateFocus] += candidateKana[candidateFocus + 1].substr(0, 1);
		candidateKana[candidateFocus + 1] = candidateKana[candidateFocus + 1].substr(1, candidateKana[candidateFocus + 1].length() - 1);
	}
	// 短くする場合
	else {
		// 既に1文字しかなかったら無視する
		if (candidateKana[candidateFocus].length() <= 1) return;

		// 後ろに候補がなかったら追加する
		if (candidateFocus == candidate.size() - 1) candidateKana.push_back(U"");

		candidateKana[candidateFocus + 1] = candidateKana[candidateFocus].substr(candidateKana[candidateFocus].length() - 1, 1)
			+ candidateKana[candidateFocus + 1];
		candidateKana[candidateFocus] = candidateKana[candidateFocus].substr(0, candidateKana[candidateFocus].length() - 1);
	}

	u32string restruct = U"";
	for (int i = 0; i < candidateKana.size(); ++i) {
		// 空ならスキップする
		if (candidateKana[i] == U"") continue;

		// 先頭ではなかったらカンマを挿入する
		if (0 < i) {
			restruct += U",";
		}

		restruct += candidateKana[i];
	}

	beforeHenkan = restruct;

	candidateKana.clear();
	for (auto c : candidate) {
		c.clear();
	}
	candidate.clear();

	henkan();
}

#ifdef WIN32
string ofxGoogleIME::UTF32toSjis(u32string srcu32str) {
	/*
	#include <codecvt>
	が必要

	参考にしたサイト

	Qiita そろそろWindowsでUTF-16とShift-JISの変換方法をC++erらしくまとめようか
	http://qiita.com/yumetodo/items/453d14eff41b805d8fc4

	Qiita stringとwstringの相互変換(utf限定)
	http://qiita.com/landrunner/items/657783a0fe5c0b27b41a
	*/

	string str = UTF32toUTF8(srcu32str);

	wstring_convert < codecvt_utf8<wchar_t>, wchar_t> cv;
	wstring wstr = cv.from_bytes(str);

	static_assert(sizeof(wchar_t) == 2, "this function is windows only");
	const int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	string re(len * 2, '\0');
	if (!WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &re[0], len, nullptr, nullptr)) {
		const auto ec = GetLastError();
		switch (ec) {
		case ERROR_INSUFFICIENT_BUFFER:
			throw runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INSUFFICIENT_BUFFER"); break;
		case ERROR_INVALID_FLAGS:
			throw runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INVALID_FLAGS"); break;
		case ERROR_INVALID_PARAMETER:
			throw runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INVALID_PARAMETER"); break;
		default:
			throw runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: unknown(" + to_string(ec) + ')'); break;
		}
	}
	const size_t real_len = strlen(re.c_str());
	re.resize(real_len);
	re.shrink_to_fit();
	return re;
}
#endif

string ofxGoogleIME::percentEnc(u32string u32str) {
	// まず、UTF8にする
	string u8str = UTF32toUTF8(u32str);

	const int NUM_BEGIN_UTF8 = 48;
	const int CAPITAL_BEGIN_UTF8 = 65;
	const int LOWER_BEGIN_UTF8 = 97;

	int charCode = -1;
	string encoded;
	stringstream out;

	//for文で1byteずつストリームに入れていく
	for (int i = 0; u8str[i] != 0; i++) {
		//文字列中の1byte分のデータを整数値として代入
		charCode = (int)(unsigned char)u8str[i];

		//エンコードする必要の無い文字の判定
		if ((NUM_BEGIN_UTF8 <= charCode && charCode <= NUM_BEGIN_UTF8 + 9)
			|| (CAPITAL_BEGIN_UTF8 <= charCode && charCode <= CAPITAL_BEGIN_UTF8 + 25)
			|| (LOWER_BEGIN_UTF8 <= charCode && charCode <= LOWER_BEGIN_UTF8 + 25)
			|| u8str[i] == '.' || u8str[i] == '_' || u8str[i] == '-' || u8str[i] == '~') {
			//エンコードの必要が無い文字はそのままストリームに入れる
			out << u8str[i];
		}
		else {
			//エンコードする場合は%記号と文字コードの16進数表示をストリームに入れる
			out << '%' << hex << uppercase << charCode;
		}
	}
	//ストリームの文字列をstringのインスタンスに代入しreturn
	encoded = out.str();
	return encoded;
}

string ofxGoogleIME::UTF32toUTF8(const u32string & u32str) {
	return convert8_32.to_bytes(reinterpret_cast<const char32_t*>(u32str.c_str()));
}

u32string ofxGoogleIME::UTF8toUTF32(const string & str) {
	auto A = convert8_32.from_bytes(str);
	return u32string(A.cbegin(), A.cend());
}

void ofxGoogleIME::makeDictionary() {
	// もし空でなかったら一旦消去する
	if (!romajiToKana.empty()) romajiToKana.clear();

	// 清音
	romajiToKana[U"a"] = U"あ";
	romajiToKana[U"i"] = U"い";
	romajiToKana[U"u"] = U"う";
	romajiToKana[U"whu"] = U"う";
	romajiToKana[U"e"] = U"え";
	romajiToKana[U"o"] = U"お";
	romajiToKana[U"ka"] = U"か";
	romajiToKana[U"ca"] = U"か";
	romajiToKana[U"ki"] = U"き";
	romajiToKana[U"ku"] = U"く";
	romajiToKana[U"cu"] = U"く";
	romajiToKana[U"ke"] = U"け";
	romajiToKana[U"ko"] = U"こ";
	romajiToKana[U"co"] = U"こ";
	romajiToKana[U"sa"] = U"さ";
	romajiToKana[U"si"] = U"し";
	romajiToKana[U"ci"] = U"し";
	romajiToKana[U"shi"] = U"し";
	romajiToKana[U"su"] = U"す";
	romajiToKana[U"se"] = U"せ";
	romajiToKana[U"ce"] = U"せ";
	romajiToKana[U"so"] = U"そ";
	romajiToKana[U"ta"] = U"た";
	romajiToKana[U"ti"] = U"ち";
	romajiToKana[U"chi"] = U"ち";
	romajiToKana[U"tu"] = U"つ";
	romajiToKana[U"tsu"] = U"つ";
	romajiToKana[U"te"] = U"て";
	romajiToKana[U"to"] = U"と";
	romajiToKana[U"na"] = U"な";
	romajiToKana[U"ni"] = U"に";
	romajiToKana[U"nu"] = U"ぬ";
	romajiToKana[U"ne"] = U"ね";
	romajiToKana[U"no"] = U"の";
	romajiToKana[U"ha"] = U"は";
	romajiToKana[U"hi"] = U"ひ";
	romajiToKana[U"hu"] = U"ふ";
	romajiToKana[U"fu"] = U"ふ";
	romajiToKana[U"he"] = U"へ";
	romajiToKana[U"ho"] = U"ほ";
	romajiToKana[U"ma"] = U"ま";
	romajiToKana[U"mi"] = U"み";
	romajiToKana[U"mu"] = U"む";
	romajiToKana[U"me"] = U"め";
	romajiToKana[U"mo"] = U"も";
	romajiToKana[U"ra"] = U"ら";
	romajiToKana[U"ri"] = U"り";
	romajiToKana[U"ru"] = U"る";
	romajiToKana[U"re"] = U"れ";
	romajiToKana[U"ro"] = U"ろ";
	romajiToKana[U"ya"] = U"や";
	romajiToKana[U"yu"] = U"ゆ";
	romajiToKana[U"yo"] = U"よ";
	romajiToKana[U"wa"] = U"わ";
	romajiToKana[U"wo"] = U"を";
	romajiToKana[U"nn"] = U"ん";
	romajiToKana[U"xn"] = U"ん";

	// 濁音
	romajiToKana[U"ga"] = U"が";
	romajiToKana[U"gi"] = U"ぎ";
	romajiToKana[U"gu"] = U"ぐ";
	romajiToKana[U"ge"] = U"げ";
	romajiToKana[U"go"] = U"ご";
	romajiToKana[U"za"] = U"ざ";
	romajiToKana[U"zi"] = U"じ";
	romajiToKana[U"ji"] = U"じ";
	romajiToKana[U"zu"] = U"ず";
	romajiToKana[U"ze"] = U"ぜ";
	romajiToKana[U"zo"] = U"ぞ";
	romajiToKana[U"da"] = U"だ";
	romajiToKana[U"di"] = U"ぢ";
	romajiToKana[U"du"] = U"づ";
	romajiToKana[U"de"] = U"で";
	romajiToKana[U"do"] = U"ど";
	romajiToKana[U"ba"] = U"ば";
	romajiToKana[U"bi"] = U"び";
	romajiToKana[U"bu"] = U"ぶ";
	romajiToKana[U"be"] = U"べ";
	romajiToKana[U"bo"] = U"ぼ";

	romajiToKana[U"pa"] = U"ぱ";
	romajiToKana[U"pi"] = U"ぴ";
	romajiToKana[U"pu"] = U"ぷ";
	romajiToKana[U"pe"] = U"ぺ";
	romajiToKana[U"po"] = U"ぽ";

	romajiToKana[U"la"] = U"ぁ";
	romajiToKana[U"xa"] = U"ぁ";
	romajiToKana[U"li"] = U"ぃ";
	romajiToKana[U"xi"] = U"ぃ";
	romajiToKana[U"lu"] = U"ぅ";
	romajiToKana[U"xu"] = U"ぅ";
	romajiToKana[U"le"] = U"ぇ";
	romajiToKana[U"xe"] = U"ぇ";
	romajiToKana[U"lo"] = U"ぉ";
	romajiToKana[U"xo"] = U"ぉ";
	romajiToKana[U"ltu"] = U"っ";
	romajiToKana[U"lya"] = U"ゃ";
	romajiToKana[U"xya"] = U"ゃ";
	romajiToKana[U"lyi"] = U"ぃ";
	romajiToKana[U"xyi"] = U"ぃ";
	romajiToKana[U"lyu"] = U"ゅ";
	romajiToKana[U"xyu"] = U"ゅ";
	romajiToKana[U"lye"] = U"ぇ";
	romajiToKana[U"xye"] = U"ぇ";
	romajiToKana[U"lyo"] = U"ょ";
	romajiToKana[U"xyo"] = U"ょ";

	romajiToKana[U"kya"] = U"きゃ";
	romajiToKana[U"kyi"] = U"きぃ";
	romajiToKana[U"kyu"] = U"きゅ";
	romajiToKana[U"kye"] = U"きぇ";
	romajiToKana[U"kyo"] = U"きょ";

	romajiToKana[U"sya"] = U"しゃ";
	romajiToKana[U"sha"] = U"しゃ";
	romajiToKana[U"syu"] = U"しゅ";
	romajiToKana[U"shu"] = U"しゅ";
	romajiToKana[U"sye"] = U"しぇ";
	romajiToKana[U"she"] = U"しぇ";
	romajiToKana[U"syo"] = U"しょ";
	romajiToKana[U"sho"] = U"しょ";

	romajiToKana[U"cha"] = U"ちゃ";
	romajiToKana[U"cya"] = U"ちゃ";
	romajiToKana[U"tya"] = U"ちゃ";
	romajiToKana[U"cyi"] = U"ちぃ";
	romajiToKana[U"tyi"] = U"ちぃ";
	romajiToKana[U"chu"] = U"ちゅ";
	romajiToKana[U"cyu"] = U"ちゅ";
	romajiToKana[U"tyu"] = U"ちゅ";
	romajiToKana[U"che"] = U"ちぇ";
	romajiToKana[U"cye"] = U"ちぇ";
	romajiToKana[U"tye"] = U"ちぇ";
	romajiToKana[U"cho"] = U"ちょ";
	romajiToKana[U"cyo"] = U"ちょ";
	romajiToKana[U"tyo"] = U"ちょ";

	romajiToKana[U"tha"] = U"てゃ";
	romajiToKana[U"thi"] = U"てぃ";
	romajiToKana[U"thu"] = U"てゅ";
	romajiToKana[U"the"] = U"てぇ";
	romajiToKana[U"tho"] = U"てょ";

	romajiToKana[U"nya"] = U"にゃ";
	romajiToKana[U"nyi"] = U"にぃ";
	romajiToKana[U"nyu"] = U"にゅ";
	romajiToKana[U"nye"] = U"にぇ";
	romajiToKana[U"nyo"] = U"にょ";

	romajiToKana[U"hya"] = U"ひゃ";
	romajiToKana[U"hyi"] = U"ひぃ";
	romajiToKana[U"hyu"] = U"ひゅ";
	romajiToKana[U"hye"] = U"ひぇ";
	romajiToKana[U"hyo"] = U"ひょ";

	romajiToKana[U"mya"] = U"みゃ";
	romajiToKana[U"myi"] = U"みぃ";
	romajiToKana[U"myu"] = U"みゅ";
	romajiToKana[U"mye"] = U"みぇ";
	romajiToKana[U"myo"] = U"みょ";

	romajiToKana[U"rya"] = U"りゃ";
	romajiToKana[U"ryi"] = U"りぃ";
	romajiToKana[U"ryu"] = U"りゅ";
	romajiToKana[U"rye"] = U"りぇ";
	romajiToKana[U"ryo"] = U"りょ";

	romajiToKana[U"gya"] = U"ぎゃ";
	romajiToKana[U"gyi"] = U"ぎぃ";
	romajiToKana[U"gyu"] = U"ぎゅ";
	romajiToKana[U"gye"] = U"ぎぇ";
	romajiToKana[U"gyo"] = U"ぎょ";

	romajiToKana[U"zya"] = U"じゃ";
	romajiToKana[U"ja"] = U"じゃ";
	romajiToKana[U"zyi"] = U"じぃ";
	romajiToKana[U"zyu"] = U"じゅ";
	romajiToKana[U"ju"] = U"じゅ";
	romajiToKana[U"zye"] = U"じぇ";
	romajiToKana[U"je"] = U"じぇ";
	romajiToKana[U"zyo"] = U"じょ";
	romajiToKana[U"jo"] = U"じょ";

	romajiToKana[U"bya"] = U"びゃ";
	romajiToKana[U"byi"] = U"びぃ";
	romajiToKana[U"byu"] = U"びゅ";
	romajiToKana[U"bye"] = U"びぇ";
	romajiToKana[U"byo"] = U"びょ";

	romajiToKana[U"va"] = U"ゔぁ";

	romajiToKana[U"pya"] = U"ぴゃ";
	romajiToKana[U"pyi"] = U"ぴぃ";
	romajiToKana[U"pyu"] = U"ぴゅ";
	romajiToKana[U"pye"] = U"ぴぇ";
	romajiToKana[U"pyo"] = U"ぴょ";

	romajiToKana[U"wyi"] = U"ゐ";
	romajiToKana[U"wye"] = U"ゑ";

	romajiToKana[U"wha"] = U"うぁ";
	romajiToKana[U"whi"] = U"うぃ";
	romajiToKana[U"whe"] = U"うぇ";
	romajiToKana[U"who"] = U"うぉ";

	// 記号
	romajiToKana[U"-"] = U"ー";
	romajiToKana[U","] = U"、";
	romajiToKana[U"."] = U"。";
	romajiToKana[U"!"] = U"！";
	romajiToKana[U"?"] = U"？";
	romajiToKana[U"@"] = U"＠";
	romajiToKana[U"#"] = U"＃";
	romajiToKana[U"$"] = U"＄";
	romajiToKana[U"%"] = U"％";
	romajiToKana[U"^"] = U"＾";
	romajiToKana[U"&"] = U"＆";
	romajiToKana[U"*"] = U"＊";
	romajiToKana[U"("] = U"（";
	romajiToKana[U")"] = U"）";
}

void ofxGoogleIME::toggleMode() {
    switch (state) {
    case Eisu:
        state = Kana;
        break;
    case KanaNyuryoku:
        beforeHenkan += beforeKana;
        beforeKana = U"";
    case KanaHenkan:
        afterHenkan += beforeHenkan;
        beforeHenkan = U"";
    case Kana:
        state = Eisu;
        break;
    }
}
