#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    string fontName;
#ifdef TARGET_OS_MAC
    fontName = "HiraMinPro-W3";
#elif defined WIN32
    fontName = "Meiryo.ttf";
#else
    fontName = OF_TTF_SANS;
#endif
    ofTrueTypeFontSettings settings(fontName, 20);
    settings.addRanges(ofAlphabet::Latin);
    settings.addRanges(ofAlphabet::Japanese);
    font.load(settings);
    
    ime.setFont(fontName, 20);
    ime.enable();
    ime.setPos(20, 60);

    ofSetBackgroundColor(200);    
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(50);

    int y = 100;
    for (auto &t : texts) {
        font.drawString(t, 20, y);
        y += 5 + font.stringHeight(t);
    }
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {

    case OF_KEY_RETURN:
        // Ctrl
        if (ofGetKeyPressed(OF_KEY_CONTROL)) {
            addText(ime.getAll());
            ime.disable();
            ime.clear();
        }
        break;
    }
}

void ofApp::keyReleased(int key){
    switch (key) {
    case OF_KEY_RETURN:
        if (!ime.isEnabled()) ime.enable();
        break;
    }
}

void ofApp::mousePressed(int x, int y, int button){
}


void ofApp::addText(string str) {
    texts.push_back(str);
}
