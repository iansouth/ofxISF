#include "ofMain.h"
#include "ofxISF.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp
{
public:

    ofxISF::Shader isf;
    ofVideoGrabber video;

    ofxPanel            gui;
    ofParameter<bool>   bGuiVisible{"GUI Visible", true};
    ofParameter<string> guiName{"Name", ""};
    ofxButton           guiLoadBtn;
    ofxGuiGroup         guiInputs;

    ofTrueTypeFont font1;

    void setup() {
        ofSetFrameRate(60);
        ofSetVerticalSync(true);
        ofBackground(0);

        font1.load("verdana.ttf", 16, true, true);
        font1.setLineHeight(23.0f);

        video.initGrabber(1280, 720);

        setupGui();

        //isf.setup(1280, 720, GL_RGB32F);
        isf.setup(1280, 720, GL_RGBA);
        loadISF("isf-test.fs");
    }

    void setupGui() {
        gui.setup("ofxISF");
        gui.add(guiName);
        gui.add( guiLoadBtn.setup("Load shader...") );
        gui.add(bGuiVisible);
        gui.add(guiInputs.setup("Inputs"));

        guiLoadBtn.addListener(this, &ofApp::loadDialog);
    }

    void loadDialog() {
        ofFileDialogResult result = ofSystemLoadDialog("Load ISF shader .fs file");
        if(result.bSuccess) {
            string path = result.getPath();
            loadISF(path);
        }
    }

    bool loadISF(const string & path) {
        if ( ! isf.load(path) ) return false;

        guiName = isf.getName();
        ofSetWindowTitle(isf.getName());
        auto uniforms = isf.getInputs();
        auto params = uniforms.getParams();
        guiInputs.clear();
        guiInputs.add(params);

        isf.setImage("inputImage", video.getTextureReference());

        return true;
    }

    void update() {
        video.update();

        //float t = ofGetElapsedTimef() * 2;
        //isf.setUniform<float>("blurAmount", ofNoise(1, 0, 0, t) * 1.5);

        isf.update();
    }

    void draw() {
        isf.draw(0, 0);
        if (bGuiVisible == true) {
            drawInfo();
            gui.draw();
        }
    }

    void drawInfo() {
        string info("");
        info += "Name: " + isf.getName() + "\n";
        info += "Credit: " + isf.getCredit() + "\n";
        info += "Description: " + isf.getDescription() + "\n";
        info += "Categories:  ";
        for (auto const & cat : isf.getCategories()) info += cat;
        ofPushStyle();
        ofSetColor(255, 255, 255, 200);
        font1.drawString(info, 20, ofGetWindowHeight() - 100);
        ofPopStyle();
    }

    void keyPressed(int key) {
        if ( key == OF_KEY_TAB ) { bGuiVisible = !bGuiVisible.get(); }
        else if ( key == 'l' )   { loadDialog(); }
        else if ( key == 'd' )   { isf.dumpShader(); }
        else if ( key == 'F' )   { ofToggleFullscreen(); }
    }

    void keyReleased(int key) {
    }

    void mouseMoved(int x, int y) {
    }

    void mouseDragged(int x, int y, int button) {
    }

    void mousePressed(int x, int y, int button) {
    }

    void mouseReleased(int x, int y, int button) {
    }

    void windowResized(int w, int h) {
    }
};

int main(int argc, const char** argv) {
    ofSetupOpenGL(1280, 720, OF_WINDOW);
    ofRunApp(new ofApp);
    return 0;
}
