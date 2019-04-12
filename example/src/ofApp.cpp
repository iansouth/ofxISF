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

    void setup() {
        ofSetFrameRate(60);
        ofSetVerticalSync(true);
        ofBackground(0);

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
        guiLoadBtn.addListener(this, &ofApp::loadDialog);
        gui.add(guiInputs.setup("Inputs"));
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
        guiInputs.clear();
        guiInputs.add(params);

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
        if (bGuiVisible == true) gui.draw();
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
