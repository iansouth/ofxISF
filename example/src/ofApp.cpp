#include "ofMain.h"

#include "ofxISF.h"

class ofApp : public ofBaseApp
{
public:
	
	ofxISF::Shader isf;
	ofVideoGrabber video;
	
	void setup()
	{
		ofSetFrameRate(60);
		ofSetVerticalSync(true);
		ofBackground(0);
		
		//video.initGrabber(1280, 720);
		
		isf.setup(1280, 720, GL_RGB32F);
		//isf.load("isf-test.fs");
		isf.load("isf-basic.fs");
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
        cout << "SHADER" << endl;
        isf.dumpShader();
		
		//isf.setImage("inputImage", video.getTextureReference());
	}
	
	void update()
	{
		//video.update();
		
		float t = ofGetElapsedTimef() * 2;
		isf.setUniform<float>("blurAmount", ofNoise(1, 0, 0, t) * 1.5);
		
		isf.update();
	}
	
	void draw()
	{
		isf.draw(0, 0);
	}

	void keyPressed(int key)
	{
		isf.load("isf-basic.fs");
	}

	void keyReleased(int key)
	{
		
	}
	
	void mouseMoved(int x, int y)
	{
	}

	void mouseDragged(int x, int y, int button)
	{
	}

	void mousePressed(int x, int y, int button)
	{
	}

	void mouseReleased(int x, int y, int button)
	{
	}
	
	void windowResized(int w, int h)
	{
	}
};

int main(int argc, const char** argv)
{
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp);
	return 0;
}
