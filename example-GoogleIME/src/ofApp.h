#pragma once

#include "ofMain.h"
#include "ofxGoogleIME.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mousePressed(int x, int y, int button);
		
    ofxGoogleIME ime;
    ofTrueTypeFont font;
    
    void addText(string str);
    vector<string> texts;
    
};
