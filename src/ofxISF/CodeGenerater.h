#pragma once

#include <regex>

#include "Constants.h"
#include "Uniforms.h"

OFX_ISF_BEGIN_NAMESPACE

static string VertShaderSrc = R"END(
#version 120

uniform int PASSINDEX;
uniform vec2 RENDERSIZE;
varying vec2 vv_FragNormCoord;

void vv_vertShaderInit(void)
{
	gl_Position = ftransform();
	vv_FragNormCoord = vec2(gl_MultiTexCoord0.x, gl_MultiTexCoord0.y);
}

$UNIFORMS$

void main(void)
{
	vv_vertShaderInit();
}
)END";

static string FragShaderSrc = R"END(
#version 120


// ISF Uniforms

uniform int PASSINDEX;
uniform vec2 RENDERSIZE;
varying vec2 vv_FragNormCoord;
uniform float TIME;

$UNIFORMS$

// ISF Texture (Image) functions

vec4 IMG_NORM_PIXEL_2D(sampler2D sampler, vec2 pct, vec2 normLoc)
{
	vec2 coord = normLoc;
	return texture2D(sampler, coord * pct);
}
vec4 IMG_PIXEL_2D(sampler2D sampler, vec2 pct, vec2 loc)
{
	return IMG_NORM_PIXEL_2D(sampler, pct, loc / RENDERSIZE);
}
vec4 IMG_THIS_NORM_PIXEL_2D(sampler2D sampler, vec2 pct)
{
	vec2 coord = vv_FragNormCoord;
	return texture2D(sampler, coord * pct);
}
vec4 IMG_THIS_PIXEL_2D(sampler2D sampler, vec2 pct)
{
	return IMG_THIS_NORM_PIXEL_2D(sampler, pct);
}
vec4 IMG_NORM_PIXEL_RECT(sampler2DRect sampler, vec2 pct, vec2 normLoc)
{
	vec2 coord = normLoc;
	return texture2DRect(sampler, coord * RENDERSIZE);
}
vec4 IMG_PIXEL_RECT(sampler2DRect sampler, vec2 pct, vec2 loc)
{
	return IMG_NORM_PIXEL_RECT(sampler, pct, loc / RENDERSIZE);
}
vec4 IMG_THIS_NORM_PIXEL_RECT(sampler2DRect sampler, vec2 pct)
{
	vec2 coord = vv_FragNormCoord;
	return texture2DRect(sampler, coord * RENDERSIZE);
}
vec4 IMG_THIS_PIXEL_RECT(sampler2DRect sampler, vec2 pct)
{
	return IMG_THIS_NORM_PIXEL_RECT(sampler, pct);
}

// ISF Source

$ISF_SOURCE$
)END";

class ImageDecl
{
public:

	ImageDecl() {}
	ImageDecl(const Ref_<ImageUniform> &uniform) : uniform(uniform) {}

	string getImgThisPixelString() const
	{
		string s;
		if (uniform->isRectangleTexture())
			s = "IMG_THIS_PIXEL_RECT($IMAGE$, _$IMAGE$_pct)";
		else
			s = "IMG_THIS_PIXEL_2D($IMAGE$, _$IMAGE$_pct)";
		ofStringReplace(s, "$IMAGE$", uniform->getName());
		return s;
	}

	string getImgThisNormPixelString() const
	{
		string s;
		if (uniform->isRectangleTexture())
			s = "IMG_THIS_NORM_PIXEL_RECT($IMAGE$, _$IMAGE$_pct)";
		else
			s = "IMG_THIS_NORM_PIXEL_2D($IMAGE$, _$IMAGE$_pct)";
		ofStringReplace(s, "$IMAGE$", uniform->getName());
		return s;
	}

	string getImgPixlString() const
	{
		string s;
		if (uniform->isRectangleTexture())
			s = "IMG_PIXEL_RECT($IMAGE$, _$IMAGE$_pct,";
		else
			s = "IMG_PIXEL_2D($IMAGE$, _$IMAGE$_pct,";
		ofStringReplace(s, "$IMAGE$", uniform->getName());
		return s;
	}

	string getImgNormPixelString() const
	{
		string s;
		if (uniform->isRectangleTexture())
			s = "IMG_NORM_PIXEL_RECT($IMAGE$, _$IMAGE$_pct,";
		else
			s = "IMG_NORM_PIXEL_2D($IMAGE$, _$IMAGE$_pct,";
		ofStringReplace(s, "$IMAGE$", uniform->getName());
		return s;
	}

protected:

	Ref_<ImageUniform> uniform;
};

class CodeGenerator
{
public:

	CodeGenerator(Uniforms &uniforms) : uniforms(uniforms) {}

	bool generate(const string& isf_glsl_code)
	{
		if (!generate_shader(isf_glsl_code)) return false;

		return true;
	}

	const string& getVertexShader() const { return vert; }
	const string& getFragmentShader() const { return frag; }
	
	void dumpShader() const
	{
		cout << "vert: " << endl;
		cout << vert << endl;
		cout << "===" << endl;
		
		cout << "frag: " << endl;
		cout << frag << endl;
		cout << "===" << endl;
	}

protected:

	Uniforms &uniforms;

	string vert;
	string frag;

protected:

	bool generate_shader(const string& isf_glsl_code)
	{
		const vector<Ref_<ImageUniform> >& images = uniforms.getImageUniforms();
		map<string, ImageDecl> image_decls;
		for (int i = 0; i < images.size(); i++) {
			const Ref_<ImageUniform> &uniform = images[i];
			image_decls[uniform->getName()] = ImageDecl(uniform);
		}

		string isf_source = isf_glsl_code;
		if (!process_lookup_macro(isf_source, image_decls)) return false;

		string uniform_str;
		for (int i = 0; i < uniforms.size(); i++) {
			Uniform::Ref o = uniforms.getUniform(i);
			uniform_str += o->getUniform() + "\n";
		}

		vert = VertShaderSrc;
		ofStringReplace(vert, "$UNIFORMS$", uniform_str);

		frag = FragShaderSrc;
		ofStringReplace(frag, "$UNIFORMS$", uniform_str);
		ofStringReplace(frag, "$ISF_SOURCE$", isf_source);

		return true;
	}

	bool process_lookup_macro(string& isf_source, map<string, ImageDecl> &image_decls)
	{
		{
            string expanded_source = isf_source;
			string pattern = "(IMG_THIS_PIXEL|IMG_THIS_NORM_PIXEL)\\s*\\(\\s*(.*?)\\s*\\)";
            std::regex re(pattern);
            std::sregex_iterator next(isf_source.begin(), isf_source.end(), re);
            std::sregex_iterator end;
            while (next != end) {
                std::smatch m = *next;
                // TODO: check size matches?
				string found(m.str());
                string lookup_name(m[1]);
                string image_name(m[2]);

                // TODO: better to just look over images and regex replace?
				map<string, ImageDecl>::iterator it = image_decls.find(image_name);
				if (it == image_decls.end()) {
					ofLogError("ofxISF::CodeGenerator") << "image name mismatch: " << image_name;
					return false;
				}
				ImageDecl &image_decl = it->second;

				string replace_string;
				if (lookup_name == "IMG_THIS_PIXEL") {
					replace_string = image_decl.getImgThisPixelString();
				}
				else if (lookup_name == "IMG_THIS_NORM_PIXEL") {
					replace_string = image_decl.getImgThisNormPixelString();
				}
				else {
					throw "unknown error";
				}

                ofLogVerbose("ofxISF") << "replace:" << found << " with:" << replace_string;
                ofStringReplace(expanded_source, found, replace_string);

                next++;
			}
            ofLogVerbose("ofxISF") << "=EXPANDED SOURCE" << endl << expanded_source << endl << "=END";
            isf_source = expanded_source;
		}

		{
            string expanded_source = isf_source;
			string pattern = "(IMG_PIXEL|IMG_NORM_PIXEL)\\s*\\(\\s*(.*?)\\s?,";
            std::regex re(pattern);
            std::sregex_iterator next(isf_source.begin(), isf_source.end(), re);
            std::sregex_iterator end;
            while (next != end) {
                std::smatch m = *next;
                // TODO: check size matches?
				string found(m.str());
                string lookup_name(m[1]);
                string image_name(m[2]);

                // TODO: better to just look over images and regex replace?
				map<string, ImageDecl>::iterator it = image_decls.find(image_name);
				if (it == image_decls.end()) {
					ofLogError("ofxISF::CodeGenerator") << "image name mismatch: " << image_name;
					return false;
				}
				ImageDecl &image_decl = it->second;

				string replace_string;
				if (lookup_name == "IMG_PIXEL") {
					replace_string = image_decl.getImgPixlString();
				}
				else if (lookup_name == "IMG_NORM_PIXEL") {
					replace_string = image_decl.getImgNormPixelString();
				}
				else {
					throw "unknown error";
				}

                ofLogVerbose("ofxISF") << "replace:" << found << " with:" << replace_string;
                ofStringReplace(expanded_source, found, replace_string);

                next++;
			}
            ofLogVerbose("ofxISF") << "=EXPANDED SOURCE" << endl << expanded_source << endl << "=END";
            isf_source = expanded_source;
		}
		
		return true;
	}
};

OFX_ISF_END_NAMESPACE
