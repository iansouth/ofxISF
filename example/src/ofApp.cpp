#include "ofMain.h"
#include "ofxISF.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp
{
public:

    ofxISF::Shader isf;
    ofVideoGrabber video;

    ofxPanel         gui;

    void setup() {
        ofSetFrameRate(60);
        ofSetVerticalSync(true);
        ofBackground(0);

        video.initGrabber(1280, 720);

        gui.setup("panel"); // most of the time you don't need a name but don't forget to call setup

        //GL_RGBA32F_ARB
        //isf.setup(1280, 720, GL_RGB32F);
        isf.setup(1280, 720, GL_RGBA);
        loadISF("isf-test.fs");
    }

    bool loadISF(const string & path) {
        if ( ! isf.load(path) ) return false;

        cout << "Name: " << isf.getName() << endl;;
        cout << "Description: " << isf.getDescription() << endl;;
        cout << "Credit: " << isf.getCredit() << endl;;
        cout << "Categories: " << isf.getCategories().size() << endl;;
        for (auto const & cat : isf.getCategories()) {
            cout << "    " << cat << endl;
        }
        cout << "Uniforms" << endl;
        auto uniforms = isf.getInputs();
        for (size_t i = 0; i < uniforms.size(); ++i) {
            auto uni = uniforms.getUniform(i);
            cout << "Uniform: " << uni->getName() << " type:" << uni->getTypeID() << endl;
        }

        cout << "Params :)" << endl;
        cout << uniforms.getParams() << endl;

        isf.setImage("inputImage", video.getTextureReference());

        auto params = uniforms.getParams();
        gui.clear();
        gui.add(params);

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
        gui.draw();
    }

    void keyPressed(int key) {
        if ( key == 'l' ) {
            ofFileDialogResult result = ofSystemLoadDialog("Load file");
            if(result.bSuccess) {
                string path = result.getPath();
                loadISF(path);
            }
        }
        else if ( key == 'd' ) {
            cout << "SHADER" << endl;
            isf.dumpShader();
        }
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
