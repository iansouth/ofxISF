#pragma once

#include "Constants.h"
#include "Uniforms.h"
#include "ofMain.h"

OFX_ISF_BEGIN_NAMESPACE

class Shader
{
public:

	Shader()
		:code_generator(uniforms)
		,current_framebuffer(NULL)
		,result_texture(NULL)
		,internalformat(GL_RGB)
	{}

    void setup(int w, int h, int internalformat = GL_RGB) {
		render_size.set(w, h);
		this->internalformat = internalformat;
		
		ofFbo &fbo = framebuffer_map["DEFAULT"];
		fbo.allocate(render_size.x, render_size.y, internalformat);
		fbo.begin();
		ofClear(0);
		fbo.end();
	}

    bool load(const string& path) {
        if (!ofFile::doesFileExist(path)) {
            ofLogError("ofxISF") << "no such file: " << path;
			return false;
		}
        ofLogNotice("ofxISF") << "Loading ISF shader file:" << path;
		
		name = ofFilePath::getBaseName(path);
		string data = ofBufferFromFile(path).getText();
		if (!parse_directive(data, header_directive, shader_directive)) return false;
		if (!reload_shader()) return false;
		return true;
	}

	bool load(const string& header, const string& shader) {
		header_directive = header;
		shader_directive = shader;
		return reload_shader();
	}

    void update() {
		const vector<Ref_<ImageUniform> >& images = uniforms.getImageUniforms();
		bool need_reload_shader = false;
		for (int i = 0; i < images.size(); i++)
		{
            if (images[i]->checkTextureFormatChanged()) {
				need_reload_shader = true;
				break;
			}
		}
		if (need_reload_shader) reload_shader();
		
		current_framebuffer = &framebuffer_map["DEFAULT"];
		current_framebuffer->begin();
		ofClear(0);
		current_framebuffer->end();
		
        if (passes.empty()) {
			render_pass(0);
		}
        else {
            for (int i = 0; i < passes.size(); i++) {
				Pass &pass = passes[i];
                if (!pass.target.empty()) {
					current_framebuffer = &framebuffer_map[pass.target];
				}
                else {
					current_framebuffer = &framebuffer_map["DEFAULT"];
				}
				render_pass(i);
			}
		}
	}

    void draw(float x, float y, float w, float h) {
		current_framebuffer->draw(x, y, w, h);
	}
	
    void draw(float x, float y) {
		current_framebuffer->draw(x, y);
	}
	
    const string& getName() const { return name; }
    const string& getDescription() const { return description; }
    const string& getCredit() const { return credit; }

    const vector<string>& getCategories() const { return categories; }

    const Uniforms& getInputs() const { return input_uniforms; }

    ofTexture& getTextureReference() {
		return *result_texture;
	}
	
    //

    void clear(const ofColor& color) {
        current_framebuffer->begin();
        ofClear(color);
        current_framebuffer->end();
    }

    void clear(float r, float g, float b, float a = 255) {
        current_framebuffer->begin();
        ofClear(r, g, b, a);
        current_framebuffer->end();
    }

    void clear(float b, float a = 255) {
        current_framebuffer->begin();
        ofClear(b, a);
        current_framebuffer->end();
    }

    //
	
    void setImage(ofTexture *img) {
        if (default_image_input_name == "") {
			static bool shown = false;
			if (!shown)
			{
				shown = true;
				ofLogError("Shader") << "no default image input";
			}
			
			return;
		}
		uniforms.setUniform<ofTexture*>(default_image_input_name, img);
	}
	
    void setImage(ofTexture &img) {
		setImage(&img);
	}
	
    void setImage(ofImage &img) {
		setImage(&img.getTextureReference());
	}

	template <typename INT_TYPE, typename EXT_TYPE>
    void setUniform(const string& name, const EXT_TYPE& value) {
		uniforms.setUniform<INT_TYPE>(name, value);
	}
	
    void setImage(const string& name, ofTexture *img) {
		uniforms.setUniform<ofTexture*>(name, img);
	}

    void setImage(const string& name, ofTexture &img) {
		setImage(name, &img);
	}

    void setImage(const string& name, ofImage &img) {
		setImage(name, &img.getTextureReference());
	}
	
	template <typename T>
    bool hasUniform(const string& name) const {
		if (!uniforms.hasUniform(name)) return false;
		if (!uniforms.getUniform(name)->isTypeOf<T>()) return false;
		return true;
	}
	
    bool hasImage(const string& name) const {
		return hasUniform<ofTexture*>(name);
	}
	
    void dumpShader() const {
        cout << "Name: " << getName() << endl;;
        cout << "Description: " << getDescription() << endl;;
        cout << "Credit: " << getCredit() << endl;;
        cout << "Categories: " << getCategories().size() << endl;;
        for (auto const & cat : getCategories())  cout << "    " << cat << endl;
        cout << "Uniforms" << endl;
        auto uniforms = getInputs();
        for (size_t i = 0; i < uniforms.size(); ++i) {
            auto uni = uniforms.getUniform(i);
            cout << "Uniform: " << uni->getName() << " type:" << uni->getTypeID() << endl;
        }
        cout << "Params:" << endl;
        cout << uniforms.getParams() << endl;

        code_generator.dumpShader();
	}

	const vector<ofTexture*>& getTextures() const { return textures; }
	
protected:

	ofVec2f render_size;
	int internalformat;
	
	string name;
	string description;
	string credit;
	vector<string> categories;

	vector<Input> inputs;
	vector<PresistentBuffer> presistent_buffers;
	vector<Pass> passes;
	
	string default_image_input_name;
	
	//
	
	Uniforms uniforms;
	Uniforms input_uniforms;
	CodeGenerator code_generator;

	string header_directive;
	string shader_directive;

	map<string, ofFbo> framebuffer_map;
	ofFbo *current_framebuffer;
	
	vector<ofTexture*> textures;
	ofTexture *result_texture;
	
	ofShader shader;

protected:
	
	void render_pass(int index)
	{
		if (!shader.isLoaded()) return;
		
		current_framebuffer->begin();
		
		ofPushStyle();
		ofEnableAlphaBlending();
		ofSetColor(255);
		
		shader.begin();
		shader.setUniform1i("PASSINDEX", index);
		shader.setUniform2fv("RENDERSIZE", render_size.getPtr());
		shader.setUniform1f("TIME", ofGetElapsedTimef());
		
		ImageUniform::resetTextureUnitID();
		
		for (int i = 0; i < uniforms.size(); i++)
			uniforms.getUniform(i)->update(&shader);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		
		glTexCoord2f(1, 0);
		glVertex2f(render_size.x, 0);
		
		glTexCoord2f(1, 1);
		glVertex2f(render_size.x, render_size.y);
		
		glTexCoord2f(0, 1);
		glVertex2f(0, render_size.y);
		glEnd();
		
		shader.end();
		
		ofPopStyle();
		
		current_framebuffer->end();
	}

	bool parse_directive(const string &data, string& header_directive, string& shader_directive)
	{
		const string header_begin = "/*";
		const string header_end = "*/";

		string::size_type begin_pos = data.find_first_of(header_begin) + header_end.size();
		string::size_type end_pos = data.find_first_of(header_end, begin_pos);
		string::size_type count = end_pos - begin_pos;

		if (begin_pos == end_pos
			|| begin_pos == std::string::npos
			|| end_pos == std::string::npos)
		{
			ofLogError("ofxISF") << "invalid format: missing header delective";
			return false;
		}

		header_directive = data.substr(begin_pos, count);

		string::size_type shader_begin = end_pos + header_end.size();
		shader_directive = data.substr(shader_begin);

		return true;
	}
	
	bool reload_shader()
	{
		textures.clear();
		current_framebuffer = &framebuffer_map["DEFAULT"];
		
		textures.push_back(&framebuffer_map["DEFAULT"].getTextureReference());
		
		if (!parse(header_directive)) return false;
		
		for (int i = 0; i < presistent_buffers.size(); i++)
		{
			const PresistentBuffer &buf = presistent_buffers[i];
			ofFbo &fbo = framebuffer_map[buf.name];
			
			if (!fbo.isAllocated())
			{
				fbo.allocate(buf.width, buf.height, internalformat);
				
				fbo.begin();
				ofClear(0);
				fbo.end();
			}

			textures.push_back(&fbo.getTextureReference());

			ImageUniform *uniform = new ImageUniform(buf.name);
			uniform->set(&fbo.getTextureReference());
			uniforms.addUniform(buf.name, Uniform::Ref(uniform));
		}
		
		{
			string result_texture_name = "";
			
			if (!passes.empty())
			{
				result_texture_name = passes.back().target;
			}
			
			if (result_texture_name == "")
				result_texture_name = "DEFAULT";
			
			result_texture = &framebuffer_map[result_texture_name].getTextureReference();
		}
		
		if (!code_generator.generate(shader_directive)) return false;
		
		shader.unload();
		if (!shader.setupShaderFromSource(GL_VERTEX_SHADER, code_generator.getVertexShader()))
		{
			cout << code_generator.getVertexShader() << endl;
			return false;
		}
		
		if (!shader.setupShaderFromSource(GL_FRAGMENT_SHADER, code_generator.getFragmentShader()))
		{
			cout << code_generator.getFragmentShader() << endl;
			return false;
		}
		
		if (!shader.linkProgram())
		{
			return false;
		}
		
		return true;
	}
	
	bool parse(const string& header_directive)
	{
        description = "";
        credit = "";
        categories.clear();
        input_uniforms.clear();
        presistent_buffers.clear();
        passes.clear();

        ofLogVerbose("ofxISF") << "Parsing JSON:\n" << header_directive;
        ofJson json;
        try {
            json = ofJson::parse(header_directive);
        } catch(std::exception &err) {
            ofLogError("ofxISF") << "Header failed to parse as JSON : " << err.what();
            return false;
        }
        if ( ! json.is_object() ) {
            ofLogError("ofxISF") << "Header is not a JSON object, can't parse!";
            return false;
        }

        if (json.count("DESCRIPTION") == 1) description = json["DESCRIPTION"];
        if (json.count("CREDIT") == 1)      credit = json["CREDIT"];

        if (json.count("CATEGORIES") == 1 && json["CATEGORIES"].is_array()) {
            for (auto &el: json["CATEGORIES"]) categories.push_back(el);
        }

        for (auto& el : json["INPUTS"]) {
            Uniform::Ref uniform = setup_input_uniform(el);
            if (uniform) {
                string name = uniform->getName();
                if (uniforms.hasUniform(name) && uniforms.getUniform(name)->getTypeID() != uniform->getTypeID()) {
                    // uniform type changed
                    uniforms.removeUniform(name);
                }
                uniforms.addUniform(name, uniform);
                input_uniforms.addUniform(name, uniform);
            }
        }

        if ( json.count("PERSISTENT_BUFFERS") == 1 ) {
            if ( json["PERSISTENT_BUFFERS"].is_array() ) {
                for ( auto & el : json["PERSISTENT_BUFFERS"] ) {
                    string name = el;
                    PresistentBuffer buf;
                    buf.name = name;
                    buf.width = render_size.x;
                    buf.height = render_size.y;
                    presistent_buffers.push_back(buf);
                }
            }
            else if ( json["PERSISTENT_BUFFERS"].is_object() ) {
                throw "not implemented yet";
            }
        }

		if ( json.count("PASSES") == 1 && json["PASSES"].is_array() ) {
            for ( auto & pass : json["PASSES"] ) {
				string target = pass["TARGET"];
				// TODO: uniform expression
				//string width  = pass["WIDTH"];
				//string height = pass["HEIGHT"];

				Pass o;
				o.target = target;
				passes.push_back(o);
			}
		}

		return true;
	}

	Uniform::Ref setup_input_uniform(const ofJson& json)
    {
		Uniform::Ref uniform = NULL;

        string name = json["NAME"];
        string type = json["TYPE"];
        bool hasDefault = json.count("DEFAULT") == 1;

		if (type == "image") {
            if (default_image_input_name == "") default_image_input_name = name;
			uniform = new ImageUniform(name);
		}
		else if (type == "bool") {
            bool def = false;
            if (hasDefault) {
                if ( json["DEFAULT"].is_boolean() ) def = json["DEFAULT"];
                if ( json["DEFAULT"].is_number() )  int(json["DEFAULT"]) == 0 ? def = false : def = true;
            }
			uniform = new BoolUniform(name, def);
		}
		else if (type == "float") {
            float def = 0.0f;
            if (hasDefault) def = json["DEFAULT"];
			FloatUniform *o = new FloatUniform(name, def);
			if (json.count("MIN") == 1 && json.count("MAX") == 1) {
				float m0 = json["MIN"];
				float m1 = json["MAX"];
				o->setRange(m0, m1);
			}
			uniform = o;
		}
		else if (type == "color")
		{
			ofFloatColor def;
            if ( hasDefault && json["DEFAULT"].is_array() ) {
                if ( json["DEFAULT"].size() == 4 ) {
                    def.r = json["DEFAULT"][0];
                    def.r = json["DEFAULT"][1];
                    def.r = json["DEFAULT"][2];
                    def.r = json["DEFAULT"][3];
                }
            }
			uniform = new ColorUniform(name, def);
		}
		else if (type == "event") {
			uniform = new EventUniform(name);
		}
		else if (type == "point2D") {
			ofVec2f def;
            if ( hasDefault && json["DEFAULT"].is_array() && json["DEFAULT"].size() == 2 ) {
                def = ofVec2f(json["DEFAULT"][0], json["DEFAULT"][1]);
            }
            auto o = new Point2DUniform(name, def);
            if (json.count("MIN") == 1 && json.count("MAX") == 1) {
                ofVec2f min(json["MIN"][0], json["MIN"][0]);
                ofVec2f max(json["MAX"][0], json["MAX"][0]);
                o->setRange(min, max);
            }
            uniform = o;
        }
        else if (type == "long") {
            int def = 0;
            if (hasDefault) def = json["DEFAULT"];
            auto *o = new LongUniform(name, def);
            if ( json.count("LABELS") == 1 && json["LABELS"].is_array()
                 && json.count("VALUES") == 1 && json["VALUES"].is_array() ) {
                for ( size_t i = 0; i < json["LABELS"].size(); ++i ) {
                    o->pushValue(json["VALUES"][i], json["LABELS"][i]);
                }
            }
            uniform = o;
        }

        return uniform;
    }
};

OFX_ISF_END_NAMESPACE
