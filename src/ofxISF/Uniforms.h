#pragma once

#include "Constants.h"

OFX_ISF_BEGIN_NAMESPACE

#define _S(src) # src

class Shader;
class CodeGenerator;
class ImageUniform;
template <typename T>
class Uniform_;

class Uniform
{
public:

	typedef Ref_<Uniform> Ref;

	Uniform(const string& name, size_t type_id) : name(name), type_id(type_id)
	{}
	virtual ~Uniform() {}

	const string& getName() const { return name; }

	template <typename TT>
	bool isTypeOf() const { return Type2Int<TT>::value() == type_id; }
	
	size_t getTypeID() const { return type_id; }

    virtual ofAbstractParameter& getParameter() = 0;

protected:

	friend class CodeGenerator;
	friend class Shader;
	
	string name;
	size_t type_id;

	virtual string getUniform() const = 0;
	virtual void update(ofShader *shader) = 0;
};

//

class Uniforms
{
public:
    ofParameterGroup parameters;

    Uniforms() {
        parameters.setName("Uniforms");
    }

    ofParameterGroup & getParams() {
        return parameters;
    }

	size_t size() const
	{
		return uniforms.size();
	}
	
    Uniform::Ref getUniform(size_t idx) const {
		return uniforms.at(idx);
	}
	
    Uniform::Ref getUniform(const string& key) const {
		if (uniforms_map.find(key) == uniforms_map.end()) return Uniform::Ref();
		return uniforms_map[key];
	}
	
    bool hasUniform(const string& key) const {
		return uniforms_map.find(key) != uniforms_map.end();
	}

    const vector<Ref_<ImageUniform> >& getImageUniforms() const {
		return image_uniforms;
	}

public:
	
    bool addUniform(const string& key, Uniform::Ref uniform) {
		if (hasUniform(key)) return false;

		uniforms_map[key] = uniform;
        parameters.add(uniform->getParameter());
		updateCache();
		return true;
	}
	
	void removeUniform(const string& key)
	{
		if (!hasUniform(key)) return;
		
		uniforms_map.erase(key);
		updateCache();
	}
	
	template <typename T0, typename T1>
	void setUniform(const string& name, const T1& value);

    void clear() {
		uniforms.clear();
		uniforms_map.clear();
		image_uniforms.clear();
        parameters.clear();
	}

protected:

	vector<Uniform::Ref> uniforms;
	mutable map<string, Uniform::Ref> uniforms_map;
	vector<Ref_<ImageUniform> > image_uniforms;
	
	void updateCache();
};

//

template <typename T>
class Uniform_ : public Uniform
{
public:

	typedef T Type;

    ofParameter<T> parameter;
	T value;
	T min, max;
	bool has_range;

    Uniform_(const string& name, const T& default_value = T()) : Uniform(name, Type2Int<T>::value()), value(default_value), has_range(false) {
        parameter.set(name, default_value);
        parameter.addListener(this, &Uniform_::onParameterChange);
    }

    ofAbstractParameter& getParameter() {
        return parameter;
    }

    void onParameterChange(T & _v) {
        value = _v;
    }

    void setRange(const T& min_, const T& max_) {
		has_range = true;
		min = min_;
		max = max_;
        parameter.setMin(min_);
        parameter.setMax(max_);
	}

	template <typename TT>
    void set(const TT& v) {
		value = v;
        parameter = v;
	}

	template <typename TT>
    TT get() const {
		TT v = value;
		return v;
	}
};

class BoolUniform : public Uniform_<bool>
{
public:

	BoolUniform(const string& name, const bool& default_value = Type()) : Uniform_(name, default_value) {}

	void update(ofShader *shader)
	{
		shader->setUniform1i(name, value);
	}

protected:

	string getUniform() const
	{
		string s = _S(
			uniform bool $NAME$;
		);
		ofStringReplace(s, "$NAME$", getName());
		return s;
	}
};

class FloatUniform : public Uniform_<float>
{
public:

	FloatUniform(const string& name, const float& default_value = Type()) : Uniform_(name, default_value) {}

	void update(ofShader *shader)
	{
		if (has_range) value = ofClamp(value, min, max);
		shader->setUniform1f(name, value);
	}

protected:

	string getUniform() const
	{
		string s = _S(
			uniform float $NAME$;
		);
		ofStringReplace(s, "$NAME$", getName());
		return s;
	}
};

class ColorUniform : public Uniform_<ofFloatColor>
{
public:

	ColorUniform(const string& name, const ofFloatColor& default_value = Type()) : Uniform_(name, default_value) {}

	void update(ofShader *shader)
	{
		if (has_range)
		{
			value.r = ofClamp(value.r, min.r, max.r);
			value.g = ofClamp(value.g, min.g, max.g);
			value.b = ofClamp(value.b, min.b, max.b);
			value.a = ofClamp(value.a, min.a, max.a);
		}
		shader->setUniform4fv(name, &value.r);
	}

protected:

	string getUniform() const
	{
		string s = _S(
			uniform vec4 $NAME$;
		);
		ofStringReplace(s, "$NAME$", getName());
		return s;
	}
};

class Point2DUniform : public Uniform_<ofVec2f>
{
public:

	Point2DUniform(const string& name, const ofVec2f& default_value = Type()) : Uniform_(name, default_value) {}

	void update(ofShader *shader)
	{
		shader->setUniform2fv(name, value.getPtr());
	}

protected:

	string getUniform() const
	{
		string s = _S(
			uniform vec2 $NAME$;
		);
		ofStringReplace(s, "$NAME$", getName());
		return s;
	}
};

class ImageUniform : public Uniform_<ofTexture*>
{
public:

	ImageUniform(const string& name) : Uniform_(name, NULL), is_rectangle_texture(false) {}

	void update(ofShader *shader)
	{
		if (value == NULL) return;
		int &texture_unit_id = getTextureUnitID();
		shader->setUniformTexture(name, *value, ++texture_unit_id);

		string s = "_$NAME$_pct";
		ofStringReplace(s, "$NAME$", getName());
		
		ofVec2f pct = value->getCoordFromPercent(1, 1);
		shader->setUniform2fv(s, pct.getPtr());
	}

	bool isValid() const { return value != NULL; }

	bool isRectangleTexture() const
	{
		if (value == NULL) return false;
		return value->texData.textureTarget == GL_TEXTURE_RECTANGLE_ARB;
	}
	
	bool checkTextureFormatChanged()
	{
		if (value == NULL) return false;
		bool result = is_rectangle_texture != isRectangleTexture();
		is_rectangle_texture = isRectangleTexture();
		return result;
	}

public:
	
	static void resetTextureUnitID()
	{
		getTextureUnitID() = 0;
	}
	
protected:

	bool is_rectangle_texture;
	
	string getUniform() const
	{
		string s = _S(
			uniform $SAMPLER$ $NAME$;
			uniform vec2 _$NAME$_pct;
		);
		ofStringReplace(s, "$NAME$", getName());
		ofStringReplace(s, "$SAMPLER$", isRectangleTexture() ? "sampler2DRect" : "sampler2D");
		return s;
	}
	
	static int& getTextureUnitID()
	{
		static int id = 0;
		return id;
	}
};

class EventUniform : public Uniform_<bool>
{
public:

	EventUniform(const string& name) : Uniform_(name, false) {}

	void update(ofShader *shader)
	{
		shader->setUniform1i(name, value);
		value = false;
	}

protected:

	string getUniform() const
	{
		string s = _S(
			uniform bool $NAME$;
		);
		ofStringReplace(s, "$NAME$", getName());
		return s;
	}
};

//

template <typename INT_TYPE, typename EXT_TYPE>
inline void Uniforms::setUniform(const string& name, const EXT_TYPE& value)
{
	if (!hasUniform(name))
	{
		ofLogError("ofxISF::Uniforms") << "uniform not found: " << name;
		return;
	}
	
	Uniform::Ref &p = uniforms_map[name];
	if (!p->isTypeOf<INT_TYPE>())
	{
		ofLogError("ofxISF::Uniforms") << "type mismatch";
		return;
	}
	
	Uniform_<INT_TYPE> *ptr = (Uniform_<INT_TYPE>*)p.get();
	ptr->value = value;
}

//

inline void Uniforms::updateCache()
{
	uniforms.clear();
	image_uniforms.clear();
	
	map<string, Uniform::Ref>::iterator it = uniforms_map.begin();
	while (it != uniforms_map.end())
	{
		Uniform::Ref o = it->second;
		uniforms.push_back(o);
		if (o->isTypeOf<ofTexture*>())
		{
			Ref_<ImageUniform> p = o.cast<ImageUniform>();
			image_uniforms.push_back(p);
		}
		it++;
	}
}

#undef _S

OFX_ISF_END_NAMESPACE
