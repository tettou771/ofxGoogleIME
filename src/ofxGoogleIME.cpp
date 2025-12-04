#include "ofxGoogleIME.h"

ofxGoogleIME::ofxGoogleIME() {
	ofSetEscapeQuitsApp(false);
	makeDictionary();
    state = Eisu;
    clear();
}

ofxGoogleIME::~ofxGoogleIME() {
}

void ofxGoogleIME::enable() {
    if (enabled) return;

    enabled = true;
    ofAddListener(ofEvents().keyPressed, this, &ofxGoogleIME::keyPressed);
    ofAddListener(ofEvents().mousePressed, this, &ofxGoogleIME::mousePressed);
    ofAddListener(ofEvents().draw, this, &ofxGoogleIME::draw, OF_EVENT_ORDER_AFTER_APP);
}

void ofxGoogleIME::disable() {
    if (!enabled) return;
    
	enabled = false;
    ofRemoveListener(ofEvents().keyPressed, this, &ofxGoogleIME::keyPressed);
    ofRemoveListener(ofEvents().mousePressed, this, &ofxGoogleIME::mousePressed);
    ofRemoveListener(ofEvents().draw, this, &ofxGoogleIME::draw, OF_EVENT_ORDER_AFTER_APP);
}

void ofxGoogleIME::clear() {
	beforeHenkan = U"";
    line.clear();
    line.push_back(U"");
	
	// 候補リストを空にする
	for (auto c : candidate) {
		c.clear();
	}
	candidate.clear();
	candidateSelected.clear();
	candidateFocus = 0;
	candidateKana.clear();
    movingY = 0;
    cursorBlinkOffsetTime = ofGetElapsedTimef();
    cursorLine = cursorPos = cursorPosBeforeHenkan = 0;
    
    switch (state) {
    case KanaNyuryoku:
    case KanaHenkan:
    state = Kana;
        break;
    default:
        break;
    }
}

void ofxGoogleIME::draw(ofEventArgs &args) {
    draw(pos);
}

void ofxGoogleIME::keyPressed(ofKeyEventArgs & key) {
    // モディファイやキー押しの場合
#ifdef TARGET_OS_MAC
    char ctrl = OF_KEY_COMMAND;
#else
    char ctrl = OF_KEY_CONTROL;
#endif
    
    // Ctrl
    if (ofGetKeyPressed(ctrl)){
        // Ctrl + Space で Eisu, Kana トグル
        switch (key.key) {
        case ' ':
            toggleMode();
            break;
        case 'c':
            // TODO
            // copy text
            break;
        case 'v':
            // paste
            {
                string clip = ofGetClipboardString();
                for (auto c : clip) {
                    if (c == '\n') newLine();
                    else addKey(line[cursorLine], c, cursorPos);
                }
            }
            break;
            
        default:break;
        }
    }
    
    // Alt
    else if (ofGetKeyPressed(OF_KEY_ALT)) {
        switch (key.key) {
        case '`':
        case '~':
            // Alt + '`' または '~' で Eisu, Kana トグル
            // ALTを押していなければ、通過して通常の文字キーとして処理される
            toggleMode();
            break;
        default:break;
        }
    }
    
    // Ctrl, Alt を押してない場合
    else {
        
        switch (key.key) {
            // escで変換前文字をクリア
        case OF_KEY_ESC:
            beforeHenkan = U"";
            cursorPosBeforeHenkan = 0;
            break;
            
            // BSで一文字削除
        case OF_KEY_BACKSPACE:
            // まず、選択範囲を削除
            deleteSelected();

            // 変換中なら、今の状態で確定する
            if (state == KanaHenkan) {
                kakutei();
            }
            
            // 一文字以上あれば変換中。カーソルの手前一文字を削除する
            if (beforeHenkan.length() > 0) {
                backspaceCharacter(beforeHenkan, cursorPosBeforeHenkan);
                // もし文字列が空になっていたら、Kanaに戻す
                if (beforeHenkan.length() == 0) state = Kana;
            }
            else {
                backspaceCharacter(line[cursorLine], cursorPos, true);
            }
            break;
            
            // DELで右を一文字削除
        case OF_KEY_DEL:
            // まず、選択範囲を削除
            deleteSelected();
            
            // 変換中なら、今の状態で確定する
            if (state == KanaHenkan) {
                kakutei();
            }
            
            if (beforeHenkan.length() > 0) {
                deleteCharacter(beforeHenkan, cursorPosBeforeHenkan);
                // もし文字列が空になっていたら、Kanaに戻す
                if (beforeHenkan.length() == 0) state = Kana;
            }
            else {
                deleteCharacter(line[cursorLine], cursorPos, true);
            }
            break;
            
            // スペースキー（変換）
        case ' ':
            // 入力切替ではない場合
            switch (state) {
            case Eisu:
            case Kana:
                addKey(line[cursorLine], ' ', cursorPos);
                break;
            case KanaNyuryoku:
                henkan();
                break;
            case KanaHenkan:
                // 変換候補をトグルする
                candidateToggle(1);
                break;
            }
            
            break;
            
            // これもstateのトグル
        case OF_KEY_F1:
        case 244: // 全角/半角
            toggleMode();
            break;
            
            // 上下カーソルキー
        case OF_KEY_UP:
            switch (state) {
            case KanaHenkan:
                candidateToggle(-1);
                break;
            case KanaNyuryoku:
                henkan();
                break;
            default:
                lineChange(-1);
                break;
            }
            break;
        case OF_KEY_DOWN:
            switch (state) {
            case KanaHenkan:
                candidateToggle(1);
                break;
            case KanaNyuryoku:
                henkan();
                break;
            default:
                lineChange(1);
                break;
            }
            break;
            
            // 左右カーソルキー
        case OF_KEY_LEFT:
            switch (state) {
                // 変換中
            case KanaHenkan:
                // shiftを押していたら候補の長さを変える
                if (ofGetKeyPressed(OF_KEY_SHIFT)) candidateLengthChange(false);
                else candidateFocusToggle(-1);
                break;
                
                // 変換前はカーソル移動
            case Eisu:
            case Kana:
                // shiftを押していたら選択範囲の長さを変える
                if (ofGetKeyPressed(OF_KEY_SHIFT)) {
                    //TODO
                }else{
                    if (cursorPos > 0) {
                        cursorPos--;
                    }
                }
                break;
                // 変換前のひらがな入力中は、その中で移動
            case KanaNyuryoku:
                if (cursorPosBeforeHenkan > 0) {
                    cursorPosBeforeHenkan--;
                }
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
                // 変換前はカーソル移動
            case Eisu:
            case Kana:
                // shiftを押していたら選択範囲の長さを変える
                if (ofGetKeyPressed(OF_KEY_SHIFT)) {
                    //TODO
                }else{
                    cursorPos++;
                    if (cursorPos > line[cursorLine].length()) {
                        cursorPos = (int)line[cursorLine].length();
                    }
                }
                break;
                // 変換前のひらがな入力中は、その中で移動
            case KanaNyuryoku:
                cursorPosBeforeHenkan++;
                if (cursorPosBeforeHenkan > beforeHenkan.length()) {
                    cursorPosBeforeHenkan = (int)beforeHenkan.length();
                }
                break;
            }
            break;
            
            // 決定キーで実行、キーをクリアする
        case OF_KEY_RETURN:
            switch (state) {
                // 変換する文字列がなければ、改行(新しい行)を追加
            case Eisu:
            case Kana:
                newLine();
                break;
                
                // 何らかの変換前文字列があれば
                // スタック上の文字列を確定して追加
            case KanaNyuryoku:
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
                
                switch (state) {
                    // 直接入力
                case Eisu:
                    addKey(line[cursorLine], key.key, cursorPos);
                    break;
                    
                    // かな系のstate
                case Kana:
                    state = KanaNyuryoku;
                case KanaNyuryoku:
                    addKey(beforeHenkan, key.key, cursorPosBeforeHenkan);
                    toHiragana(beforeHenkan, cursorPosBeforeHenkan);
                    break;
                    
                    // 変換中だとしたら最初に解除されているので、ここは通らない
                default:
                    break;
                }
                
                break;
            }
        }
    }

	// 直前に押したキーを保存
	pastPressedKey = key.key;
    
    // カーソルの点滅を初期化
    cursorBlinkOffsetTime = ofGetElapsedTimef();
}

void ofxGoogleIME::mousePressed(ofMouseEventArgs &mouse) {
    auto bbox = font.getStringBoundingBox(getString(), pos.x, pos.y);
    
    // bbox内をクリックしたかどうか
    if (!bbox.inside(mouse.x, mouse.y)) return;
    
    // 変換中なら確定してしまう
    kakutei();
    
    ofVec2f rel = ofVec2f(mouse.x, mouse.y) - bbox.position;
    
    int lineNumber = ofMap(rel.y, 0, bbox.height, 0, line.size(), true);
    
    // 微妙な下端の位置をクリックして範囲外なら範囲内に収める
    lineNumber = MIN(lineNumber, (int)line.size() - 1);
    
    // クリックしている文字を調べる
    auto lineBbox = font.getStringBoundingBox(UTF32toUTF8(line[lineNumber]), pos.x, pos.y + font.getLineHeight() * lineNumber);
    int posNumber = ofMap(mouse.x, 0, lineBbox.width, 0, line[lineNumber].size());
    MIN(posNumber, line[cursorLine].size());
    
    // 選択する
    cursorLine = lineNumber;
    cursorPos = posNumber;
}

string ofxGoogleIME::getString() {
    string all = "";
    for (auto &a : line) {
        all += UTF32toUTF8(a);
        if (a != line.back()) {
            all += '\n';
        }
    }
	return all;
}

u32string ofxGoogleIME::getU32String() {
    u32string all = U"";
    for (auto &a : line) {
        all += a;
        if (a != line.back()) {
            all += U'\n';
        }
    }
    return all;
}

string ofxGoogleIME::getAfterHenkan(int l) {
    if (0 <= l && l < line.size()) {
        return UTF32toUTF8(line[l]);
    }
    else {
        return "";
    }
}

string ofxGoogleIME::getAfterHenkanSubstr(int l, int begin, int end) {
    return UTF32toUTF8(line[l].substr(begin, end));
}

string ofxGoogleIME::getBeforeHenkan() {
	return UTF32toUTF8(beforeHenkan);
}

string ofxGoogleIME::getBeforeHenkanSubstr(int begin, int end) {
    return UTF32toUTF8(beforeHenkan.substr(begin, end));
}

void ofxGoogleIME::setFont(string path, float fontSize) {
	ofTrueTypeFontSettings settings(path, fontSize);
	settings.addRanges(ofAlphabet::Latin);
	settings.addRanges(ofAlphabet::Japanese);
    settings.addRange(ofUnicode::KatakanaHalfAndFullwidthForms);
    settings.addRange(ofUnicode::range{0x301, 0x303F}); // 日本語の句読点などの記号
	font.load(settings);
}

void ofxGoogleIME::setPos(ofVec2f p) {
    pos = p;
}

void ofxGoogleIME::setPos(float x, float y) {
    setPos(ofVec2f(x, y));
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

    // 変換するときのエフェクト
    movingY *= 0.7;

    float fontSize = font.getSize();
    
    // 行の高さ
    float lineHeight = font.getLineHeight();
    
    ofPushMatrix();
    ofTranslate(x, y);
    for (int i=0; i<line.size(); ++i) {
        // すでに入力されている行
        if (i != cursorLine) {
            font.drawString(UTF32toUTF8(line[i]), 0, 0);
        }
        // 現在入力中の行
        else {
            // それぞれの文字列
            string after = getAfterHenkan(cursorLine);
            string before = getBeforeHenkan();
            float afterW = font.stringWidth(after);
            vector<float> candidateW;
            float candidateTotalW = 0;
            for (int i = 0; i < candidate.size(); ++i) {
                auto utf8String = UTF32toUTF8(candidate[i][candidateSelected[i]]);
                
                // candidateWは
                // 候補選択中のものだけは全候補の長いものに合わせる
                float w = 0;
                if (i == candidateFocus) {
                    for (auto &c : candidate[i]) {
                        w = MAX(w, font.stringWidth(UTF32toUTF8(c)));
                    }
                } else {
                    w =                     font.stringWidth(UTF32toUTF8(candidate[i][candidateSelected[i]]));
                }
                candidateW.push_back(w);
                candidateTotalW += candidateW.back();
            }
            float beforeW = font.stringWidth(before);
            float margin = font.getSize() * 0.1;
            
            // 入力カーソルを描画する関数
            auto drawCursor = [=](float x, float y) {
                if (fmod(ofGetElapsedTimef() - cursorBlinkOffsetTime, 0.8) < 0.4) {
                    ofSetLineWidth(2);
                    ofDrawLine(x + 1, y, x + 1, y - fontSize * 1.2);
                }
            };
            
            ofPushMatrix();
            switch (state) {
            case Kana:
            case Eisu:
            case KanaNyuryoku:
                if (beforeHenkan.length() == 0) {
                    // 確定後
                    font.drawString(getAfterHenkan(cursorLine), 0, 0);
                    
                    // cursor
                    drawCursor(font.stringWidth(getAfterHenkanSubstr(cursorLine,0, cursorPos)), 0);
                }
                else {
                    // 確定後の部分のカーソルの前
                    font.drawString(getAfterHenkanSubstr(cursorLine,0, cursorPos), 0, 0);
                    
                    // 描画位置を移動
                    ofTranslate(font.stringWidth(getAfterHenkanSubstr(cursorLine,0, cursorPos)) + margin, 0);
                    
                    // 変換前のかな
                    font.drawString(getBeforeHenkan(), 0, 0);

                    // 入力中はアンダーバーを引く
                    ofSetLineWidth(2);
                    float w = font.stringWidth(getBeforeHenkan());
                    ofDrawLine(0, fontSize * 0.2, w, fontSize * 0.2);

                    // cursor
                    drawCursor(font.stringWidth(getBeforeHenkanSubstr(0, cursorPosBeforeHenkan)), 0);
                    
                    // 描画位置を移動
                    ofTranslate(w + margin, 0);
                    
                    // 確定後の部分のカーソルの後
                    font.drawString(getAfterHenkanSubstr(cursorLine,cursorPos, (int)line[cursorLine].length() - cursorPos), 0, 0);
                }
                
                break;
            case KanaHenkan:
                // 確定後の部分のカーソルの前
                font.drawString(getAfterHenkanSubstr(cursorLine,0, cursorPos), 0, 0);
                
                // 描画位置を移動
                ofTranslate(font.stringWidth(getAfterHenkanSubstr(cursorLine,0, cursorPos)) + margin, 0);
                
                // 確定前の部分
                for (int i = 0; i < candidate.size(); ++i) {
                    auto current = UTF32toUTF8(candidate[i][candidateSelected[i]]);
                    
                    // 変換中はアンダーバーを引く
                    ofSetLineWidth(2);
                    ofDrawLine(0, fontSize * 0.2, candidateW[i], fontSize * 0.2);
                    
                    // フォーカスが合っている場合はハイライトして、候補も書く
                    if (i == candidateFocus) {
                        // 1行の高さ
                        float lh = font.getLineHeight();
                        
                        ofPushStyle();
                        
                        // 候補全体に座布団を敷く
                        ofFill();
                        ofSetColor(90, 100);
                        ofDrawRectangle(0, lh * (0.2-candidateSelected[i] - 1), candidateW[i], lh * candidate[i].size());
                        ofPopStyle();
                        
                        // 上下に他の候補も含めて並べる
                        for (int j = 0; j < candidate[i].size(); ++j) {
                            float offsetY = (j - (int)candidateSelected[i] + movingY) * lh;
                            auto str = UTF32toUTF8(candidate[i][j]);
                            font.drawString(str, 0, offsetY);
                        }
                    }
                    else {
                        // 選択中でなければ一つだけ描画
                        font.drawString(current, 0, 0);
                    }
                    
                    ofTranslate(candidateW[i] + margin, 0);
                }
                
                // 描画位置を移動
                ofTranslate(font.stringWidth(getBeforeHenkan()) + margin, 0);
                
                // 確定後の部分のカーソルの後
                font.drawString(getAfterHenkanSubstr(cursorLine,cursorPos, (int)line[cursorLine].length() - cursorPos), 0, 0);
                
                break;
            }
            
            ofPopMatrix();
        }
        
        // 次の行
        ofTranslate(0, lineHeight);
    }
    ofPopMatrix();
}

void ofxGoogleIME::toHiragana(u32string &str, int checkPos) {
    // チェックするのは、入力中前後3つの文字列
    int begin = MAX(0, checkPos - 3);
    int end = MIN(checkPos + 3, (int)str.length());
    
    // アルファベットかどうか判定する関数
    auto isAlphabet = [](char32_t c) {
        return 0 < c && c < 128;
    };
    
    // 長い文字列でヒットする場合を優先して探すので、3からデクリメントする
    for (int len = 3; len > 0; len--) {
        for (int b = begin; b < end - len + 1; ++b) {
            int e = b + len;

            // len文字切り取った文字列
            u32string s = str.substr(b, len);

            // その文字列がひらがなに対応しているかどうか
            bool keyExist = romajiToKana.count(s) != 0;

            // 対応しているものが見つかったら、それを変換してすり替える
            if (keyExist) {
                u32string kana = romajiToKana[s];
                str = str.substr(0, b) + kana + str.substr(e, str.length() - e);
            }
        }
    }
    
    // 入力中の手前の部分で2連続で同じアルファベットだったら "っ" にする
    for (int b = begin; b < checkPos; ++b) {
        if (isAlphabet(beforeHenkan[b]) && beforeHenkan[b] == beforeHenkan[b+1]) {
            beforeHenkan[b] = U'っ';
        }
    }
    
    // 入力中の手前の部分で、単体のnが残っていたら "ん" にする
    for (int b = begin; b < checkPos; ++b) {
        if (beforeHenkan[b] == U'n') {
            if (b < cursorPosBeforeHenkan - 1 && beforeHenkan[b+1] != u'y') {
                beforeHenkan[b] = U'ん';
            }
        }
    }
}

void ofxGoogleIME::henkan() {
	state = KanaHenkan;

	// tryつきlog
	auto tryCout = [](ofJson a, int n) {
		for (int i = 0; i < n; ++i) {
			ofLogVerbose("ofxGoogleIME") << '\t';
		}
		try {
            string astr = a;
			ofLogVerbose("ofxGoogleIME") << astr << endl;
			return true;
		}
		catch (exception e) {
			ofLogVerbose("ofxGoogleIME") << e.what() << endl;
			return false;
		}
	};

	// もし、beforeHenkan の最後に n が残っていたら それを ん にする
	if (beforeHenkan.length() > 0 && beforeHenkan[beforeHenkan.length() - 1] == U'n') {
        beforeHenkan[beforeHenkan.length() - 1] = U'ん';
	}

	string encoded = percentEnc(beforeHenkan);
	ofLogVerbose("ofxGoogleIME") << "percentEnc: " << encoded << endl;
    
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
            
            // candidateSelectedを、カーソルがあっている位置以降をゼロにする
            for (int cs = candidateFocus; cs < candidate.size(); ++cs) {
                candidateSelected[cs] = 0;
            }
		}
	}
	else {
		ofLogVerbose("ofxGoogleIME") << "GJI API error." << endl;
	}

	beforeHenkan = U"";
    cursorPosBeforeHenkan = 0;
}

void ofxGoogleIME::kakutei() {
    // 変換するべき文字列がなければ無視する
    if (state == Eisu || state == Kana) return;
    
	// 選択された候補を確定していく
    u32string kakuteiStr = U"";

    // かな入力中なら、変換せずそのまま確定
    if (state == KanaNyuryoku) {
        kakuteiStr += beforeHenkan;
    }
    // 変換中なら、今選択されているものを確定
    else if (state == KanaHenkan) {
        for (int i = 0; i < candidate.size(); ++i) {
            kakuteiStr += candidate[i][candidateSelected[i]];
        }
    }
    addStr(line[cursorLine], kakuteiStr, cursorPos);

	beforeHenkan = U"";

	// 候補リストを空にする
	for (auto c : candidate) {
		c.clear();
	}
	candidate.clear();
	candidateSelected.clear();
	candidateFocus = 0;
	candidateKana.clear();
    cursorPosBeforeHenkan = 0;
}

void ofxGoogleIME::newLine() {
    // 新しい行
    line.insert(line.begin() + cursorLine + 1, U"");
    // 新しい行にカーソル以降の文字列を追加
    line[cursorLine + 1] = line[cursorLine].substr(cursorPos, line[cursorLine].length() - cursorPos);
    // 改行した行はカーソル前の文字列に詰める
    line[cursorLine] = line[cursorLine].substr(0, cursorPos);
    
    cursorLine++;
    cursorPos = 0;
}

void ofxGoogleIME::lineChange(int n) {
    if (n == 0) return;
    cursorLine = MAX(0, MIN(cursorLine + n, (int)line.size() - 1));
    if (cursorPos > line[cursorLine].size()) cursorPos = (int)line[cursorLine].length();
}

void ofxGoogleIME::addKey(u32string &target, const char &c, int &p) {
    // もし範囲選択中なら、それを削除
    deleteSelected();
    
    // カーソル位置が範囲外なら修正
    if (p < 0) p = 0;
    if (p > target.length()) p = (int)target.length();
    
    // カーソルの位置に挿入する
    target = target.substr(0, p) + char32_t(c) + target.substr(p, target.length() - p);
    
    // カーソルを移動
    p++;
}

void ofxGoogleIME::addStr(u32string &target, const u32string &str, int &p) {
    // カーソルの位置に挿入する
    target = target.substr(0, p) + str + target.substr(p, target.length() - p);

    // カーソルを移動
    p += str.length();
}

void ofxGoogleIME::backspaceCharacter(u32string &str, int &pos, bool lineMerge) {
    // カーソルが左端なら、改行を削除
    if (pos == 0) {
        if (lineMerge && cursorLine > 0) {
            cursorPos = (int)line[cursorLine - 1].length();
            line[cursorLine - 1] += line[cursorLine];
            line.erase(line.begin() + cursorLine);
            cursorLine--;
        }
    }
    
    // カーソル位置の文字を削除
    else {
        if (str.length() < pos) pos = (int)str.length(); // カーソルが文字数より多い時は、文字の末尾にする
        
        // カーソルの位置の文字だけ削除
        str = str.substr(0, pos - 1) + str.substr(pos, str.length() - pos);
        
        // カーソル位置を一つ手前にする
        pos--;
    }
}

void ofxGoogleIME::deleteCharacter(u32string &str, int &pos, bool lineMerge) {
    if (str.length() < pos) {
        pos = (int)str.length(); // カーソルが文字数より多い時は、文字の末尾にする
    }
    if (str.length() == pos) {
        // カーソルが右端なら、次の行とマージして次の行を削除
        if (lineMerge && cursorLine + 1 < line.size()) {
            line[cursorLine] += line[cursorLine+1];
            line.erase(line.begin() + cursorLine + 1);
        }
    }
    else {
        // カーソルの位置の文字だけ削除
        str = str.substr(0, pos) + str.substr(pos + 1, str.length() - pos - 1);
    }
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

        restruct += U",";

		restruct += candidateKana[i];
	}

	beforeHenkan = restruct;

	candidateKana.clear();
	for (auto c : candidate) {
		c.clear();
	}
	candidate.clear();
    
    // 動きのイージングエフェクトを停止
    movingY = 0;

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

// UTF-32からUTF-8への変換（自前実装）
string ofxGoogleIME::UTF32toUTF8(const u32string &u32str) {
    string result;
    for (char32_t c : u32str) {
        if (c < 0x80) {
            result += static_cast<char>(c);
        } else if (c < 0x800) {
            result += static_cast<char>(0xC0 | (c >> 6));
            result += static_cast<char>(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            result += static_cast<char>(0xE0 | (c >> 12));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (c >> 18));
            result += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return result;
}

string ofxGoogleIME::UTF32toUTF8(const char32_t &u32char) {
    return UTF32toUTF8(u32string(1, u32char));
}

// UTF-8からUTF-32への変換（自前実装）
u32string ofxGoogleIME::UTF8toUTF32(const string &str) {
    u32string result;
    size_t i = 0;
    while (i < str.size()) {
        unsigned char c = str[i];
        char32_t cp;
        if ((c & 0x80) == 0) {
            cp = c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = (c & 0x1F) << 6;
            cp |= (str[i+1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = (c & 0x0F) << 12;
            cp |= (str[i+1] & 0x3F) << 6;
            cp |= (str[i+2] & 0x3F);
            i += 3;
        } else {
            cp = (c & 0x07) << 18;
            cp |= (str[i+1] & 0x3F) << 12;
            cp |= (str[i+2] & 0x3F) << 6;
            cp |= (str[i+3] & 0x3F);
            i += 4;
        }
        result += cp;
    }
    return result;
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
	romajiToKana[U"hu"] = U"ふ";
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

    // fの列
    romajiToKana[U"fa"] = U"ふぁ";
    romajiToKana[U"fi"] = U"ふぃ";
    romajiToKana[U"fu"] = U"ふ";
    romajiToKana[U"fe"] = U"ふぇ";
    romajiToKana[U"fo"] = U"ふぉ";
    romajiToKana[U"fya"] = U"ふゃ";
    romajiToKana[U"fyi"] = U"ふぃ";
    romajiToKana[U"fyu"] = U"ふゅ";
    romajiToKana[U"fye"] = U"ふぇ";
    romajiToKana[U"fyo"] = U"ふょ";

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
    
    romajiToKana[U"dha"] = U"でゃ";
    romajiToKana[U"dhi"] = U"でぃ";
    romajiToKana[U"dhu"] = U"でゅ";
    romajiToKana[U"dhe"] = U"でぇ";
    romajiToKana[U"dho"] = U"でょ";

	// 記号
	romajiToKana[U"-"] = U"ー";
    romajiToKana[U"+"] = U"＋";
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
    romajiToKana[U"{"] = U"『";
    romajiToKana[U"}"] = U"』";
    romajiToKana[U"["] = U"「";
    romajiToKana[U"]"] = U"」";
    romajiToKana[U"<"] = U"＜";
    romajiToKana[U">"] = U"＞";
    romajiToKana[U"|"] = U"｜";
    romajiToKana[U"/"] = U"・";
}

void ofxGoogleIME::toggleMode() {
    switch (state) {
    case Eisu:
        state = Kana;
        break;
    case KanaNyuryoku:
        kakutei();
    case KanaHenkan:
        kakutei();
    case Kana:
        state = Eisu;
        break;
    }
}
