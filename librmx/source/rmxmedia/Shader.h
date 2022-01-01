/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Shader
*		Support for GLSL shaders.
*/

#pragma once

#include <functional>


// Shader class
class API_EXPORT Shader
{
public:
	enum class ShaderType
	{
		FRAGMENT,
		VERTEX
	};

	enum class BlendMode
	{
		UNDEFINED = -1,
		OPAQUE,
		ALPHA,
		ADD
	};

	static std::function<void(String&, ShaderType)> mShaderSourcePostProcessCallback;

public:
	Shader();
	~Shader();

	bool compile(const String& vsSource, const String& fsSource, const std::map<int, String>* vertexAttribMap = nullptr);

	inline const String& getVertexSource() const	{ return mVertexSource; }
	inline const String& getFragmentSource() const	{ return mFragmentSource; }
	inline const String& getCompileLog() const		{ return mCompileLog; }

	inline BlendMode getBlendMode() const			{ return mBlendMode; }
	inline void setBlendMode(BlendMode mode)		{ mBlendMode = mode; }

	inline GLuint getProgramHandle() const			{ return mProgram; }

	GLuint getUniformLocation(const char* name) const;
	GLuint getAttribLocation(const char* name) const;

	void setParam(const char* name, int param);
	void setParam(const char* name, const Vec2i& param);
	void setParam(const char* name, const Vec3i& param);
	void setParam(const char* name, const Vec4i& param);
	void setParam(const char* name, float param);
	void setParam(const char* name, const Vec2f& param);
	void setParam(const char* name, const Vec3f& param);
	void setParam(const char* name, const Vec4f& param);

	void setMatrix(const char* name, const Mat3f& matrix);
	void setMatrix(const char* name, const Mat4f& matrix);

	void setTexture(const char* name, GLuint handle, GLenum target);
	void setTexture(const char* name, const Texture& texture);
	void setTexture(const char* name, const Texture* texture);		// Deprecated

	void bind();
	void unbind();

	bool load(const String& filename, const String& techname = String(), const String& additionalDefines = String());
	bool load(const std::vector<uint8>& content, const String& techname = String(), const String& additionalDefines = String());

private:
	bool compileShader(GLenum shaderType, GLuint& shaderHandle, const String& source);
	bool linkProgram(const std::map<int, String>* vertexAttribMap = nullptr);

private:
	GLuint mVertexShader;
	GLuint mFragmentShader;
	GLuint mProgram;
	BlendMode mBlendMode = BlendMode::UNDEFINED;
	int mTextureCount;

	String mVertexSource;
	String mFragmentSource;
	String mCompileLog;
};


// ShaderEffect class
class API_EXPORT ShaderEffect
{
public:
	ShaderEffect();
	~ShaderEffect();

	bool load(const String& filename);
	bool load(const std::vector<uint8>& content);
	bool loadFromString(const String& content);
	bool getShader(Shader& shader, int index = 0, const String& additionalDefines = String());
	bool getShader(Shader& shader, const String& name, const String& additionalDefines = String());

private:
	typedef std::vector<String> StringArray;

	struct PartStruct
	{
		String title;
		String content;
	};

	struct TechniqueStruct
	{
		String name;
		StringArray vs_parts;
		StringArray fs_parts;
		StringArray defines;
		std::map<int, String> vertexAttribMap;
		Shader::BlendMode blendmode = Shader::BlendMode::UNDEFINED;
	};

private:
	TechniqueStruct* findTechniqueByName(const String& name);
	bool getShaderInternal(Shader& shader, TechniqueStruct& tech, const String& additionalDefines);
	void readParts(const String& source);
	void parseTechniques(String& source);
	PartStruct* findPartByName(const String& name);
	void buildSourceFromParts(String& source, const std::vector<String>& parts, const String& definitions);
	void preprocessSource(String& source, Shader::ShaderType shaderType);

private:
	std::vector<PartStruct> mParts;
	std::vector<TechniqueStruct> mTechniques;
	String mIncludeDir;
};
