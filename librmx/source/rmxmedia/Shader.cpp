/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


/* ----- Shader -------------------------------------------------------------------------------------------------- */

std::function<void(String&, Shader::ShaderType)> Shader::mShaderSourcePostProcessCallback;


Shader::Shader()
{
	mVertexShader = 0;
	mFragmentShader = 0;
	mProgram = 0;
	mBlendMode = BlendMode::UNDEFINED;
	mTextureCount = 0;
}

Shader::~Shader()
{
	if (mProgram != 0)
		glDeleteProgram(mProgram);
	if (mVertexShader != 0)
		glDeleteShader(mVertexShader);
	if (mFragmentShader != 0)
		glDeleteShader(mFragmentShader);
}

bool Shader::compile(const String& vsSource, const String& fsSource, const std::map<int, String>* vertexAttribMap)
{
	// Compile shaders
	mVertexSource = vsSource;
	mFragmentSource = fsSource;
	if (!compileShader(GL_VERTEX_SHADER, mVertexShader, vsSource))
		return false;
	if (!compileShader(GL_FRAGMENT_SHADER, mFragmentShader, fsSource))
		return false;

	// Link to a program
	if (!linkProgram(vertexAttribMap))
		return false;

	// Done
	return true;
}

GLuint Shader::getUniformLocation(const char* name) const
{
	return glGetUniformLocation(mProgram, name);
}

GLuint Shader::getAttribLocation(const char* name) const
{
	return glGetAttribLocation(mProgram, name);
}

void Shader::setParam(const char* name, int param)
{
	glUniform1i(getUniformLocation(name), param);
}

void Shader::setParam(const char* name, const Vec2i& param)
{
	glUniform2iv(getUniformLocation(name), 1, *param);
}

void Shader::setParam(const char* name, const Vec3i& param)
{
	glUniform3iv(getUniformLocation(name), 1, *param);
}

void Shader::setParam(const char* name, const Vec4i& param)
{
	glUniform4iv(getUniformLocation(name), 1, *param);
}

void Shader::setParam(const char* name, float param)
{
	glUniform1f(getUniformLocation(name), param);
}

void Shader::setParam(const char* name, const Vec2f& param)
{
	glUniform2fv(getUniformLocation(name), 1, *param);
}

void Shader::setParam(const char* name, const Vec3f& param)
{
	glUniform3fv(getUniformLocation(name), 1, *param);
}

void Shader::setParam(const char* name, const Vec4f& param)
{
	glUniform4fv(getUniformLocation(name), 1, *param);
}

void Shader::setMatrix(const char* name, const Mat3f& matrix)
{
	glUniformMatrix3fv(getUniformLocation(name), 1, true, *matrix);
}

void Shader::setMatrix(const char* name, const Mat4f& matrix)
{
	glUniformMatrix4fv(getUniformLocation(name), 1, true, *matrix);
}

void Shader::setTexture(const char* name, GLuint handle, GLenum target)
{
	int number = mTextureCount;
	glActiveTexture(GL_TEXTURE0 + number);
	glBindTexture(target, handle);
	glUniform1i(getUniformLocation(name), number);
	++mTextureCount;
}

void Shader::setTexture(const char* name, const Texture& texture)
{
	setTexture(name, texture.getHandle(), texture.getType());
}

void Shader::setTexture(const char* name, const Texture* texture)
{
	if (nullptr != texture)
	{
		setTexture(name, texture->getHandle(), texture->getType());
	}
}

void Shader::bind()
{
	if (mBlendMode != BlendMode::UNDEFINED)
	{
		switch (mBlendMode)
		{
			case BlendMode::OPAQUE: glBlendFunc(GL_ONE, GL_ZERO);  break;
			case BlendMode::ALPHA:  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  break;
			case BlendMode::ADD:    glBlendFunc(GL_ONE, GL_ONE);   break;
			default:				glBlendFunc(GL_ONE, GL_ZERO);  break;
		}
	}
	glUseProgram(mProgram);
	mTextureCount = 0;
}

void Shader::unbind()
{
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
}

bool Shader::load(const String& filename, const String& techname, const String& additionalDefines)
{
	ShaderEffect effect;
	if (!effect.load(filename))
		return false;

	effect.getShader(*this, techname, additionalDefines);
	return true;
}

bool Shader::load(const std::vector<uint8>& content, const String& techname, const String& additionalDefines)
{
	ShaderEffect effect;
	if (!effect.load(content))
		return false;

	effect.getShader(*this, techname, additionalDefines);
	return true;
}

bool Shader::compileShader(GLenum shaderType, GLuint& shaderHandle, const String& source)
{
	int len = source.length();
	const GLchar* src = (GLchar*)(*source);

	shaderHandle = glCreateShader(shaderType);
	glShaderSource(shaderHandle, 1, &src, &len);
	glCompileShader(shaderHandle);

	GLint compiled;
	glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compiled);
	if (compiled != 0)
		return true;

	// Error output
	mCompileLog << "Error(s) compiling " << ((shaderType == GL_FRAGMENT_SHADER) ? "fragment" : "vertex") << " shader:\n";
	GLint maximumLogLength = 0;
	glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maximumLogLength);
	if (maximumLogLength > 1)
	{
		String infolog;
		infolog.expand(maximumLogLength);
		GLsizei logLength = 0;
		glGetShaderInfoLog(shaderHandle, maximumLogLength, &logLength, (GLchar*)*infolog);
		infolog.recount();
		mCompileLog << infolog;
	}
	mCompileLog << "\n";
	return false;
}

bool Shader::linkProgram(const std::map<int, String>* vertexAttribMap)
{
	mProgram = glCreateProgram();
	glAttachShader(mProgram, mVertexShader);
	glAttachShader(mProgram, mFragmentShader);

	// Bind vertex atrributes (must be done just before linking)
	if (nullptr != vertexAttribMap)
	{
		for (const auto& pair : *vertexAttribMap)
		{
			glBindAttribLocation(mProgram, pair.first, *pair.second);
		}
	}

	glLinkProgram(mProgram);

	GLint linked;
	glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);
	if (linked != 0)
		return true;

	// Error output
	mCompileLog << "Error(s) linking:\n";
	GLint maximumLogLength = 0;
	glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &maximumLogLength);
	if (maximumLogLength > 1)
	{
		String infolog;
		infolog.expand(maximumLogLength);
		GLsizei logLength = 0;
		glGetProgramInfoLog(mProgram, maximumLogLength, &logLength, (GLchar*)*infolog);
		infolog.recount();
		mCompileLog << infolog;
	}
	mCompileLog << "\n";
	return false;
}



/* ----- ShaderEffect -------------------------------------------------------------------------------------------- */

ShaderEffect::ShaderEffect()
{
}

ShaderEffect::~ShaderEffect()
{
}

bool ShaderEffect::load(const String& filename)
{
	mIncludeDir.clear();
	String filecontents;
	if (!filecontents.loadFile(*filename))
		return false;

	loadFromString(filecontents);

	mIncludeDir = filename;
	const int k = mIncludeDir.findChars("\\/", mIncludeDir.length()-1, -1);
	mIncludeDir.remove(k+1, -1);
	return true;
}

bool ShaderEffect::load(const std::vector<uint8>& content)
{
	String filecontents;
	filecontents.add((char*)&content[0], (int)content.size());

	loadFromString(filecontents);
	return true;
}

bool ShaderEffect::loadFromString(const String& content)
{
	mIncludeDir.clear();
	readParts(content);
	for (unsigned int i = 0; i < mParts.size(); ++i)
	{
		if (mParts[i].title == "TECH")
			parseTechniques(mParts[i].content);
	}
	return true;
}

bool ShaderEffect::getShader(Shader& shader, const String& name, const String& additionalDefines)
{
	if (name.empty())
		return getShader(shader);

	TechniqueStruct* tech = findTechniqueByName(name);
	if (nullptr != tech)
		return getShaderInternal(shader, *tech, additionalDefines);

	return false;
}

bool ShaderEffect::getShader(Shader& shader, int index, const String& additionalDefines)
{
	if (index < 0 || index >= (int)mTechniques.size())
		return false;

	return getShaderInternal(shader, mTechniques[index], additionalDefines);
}

ShaderEffect::TechniqueStruct* ShaderEffect::findTechniqueByName(const String& name)
{
	for (TechniqueStruct& tech : mTechniques)
	{
		if (tech.name == name)
		{
			return &tech;
		}
	}
	return nullptr;
}

bool ShaderEffect::getShaderInternal(Shader& shader, TechniqueStruct& tech, const String& additionalDefines)
{
	String definitions;
	for (unsigned int j = 0; j < tech.defines.size(); ++j)
	{
		definitions << "#define " << tech.defines[j] << "\n";
	}
	if (!additionalDefines.empty())
	{
		std::vector<String> parts;
		additionalDefines.split(parts, ',');
		for (String& part : parts)
		{
			part.trimWhitespace();
			if (!part.empty())
			{
				definitions << "#define " << part << "\n";
			}
		}
	}

	String vsSource;
	buildSourceFromParts(vsSource, tech.vs_parts, definitions);
	preprocessSource(vsSource, Shader::ShaderType::VERTEX);

	String fsSource;
	buildSourceFromParts(fsSource, tech.fs_parts, definitions);
	preprocessSource(fsSource, Shader::ShaderType::FRAGMENT);

	if (!shader.compile(vsSource, fsSource, &tech.vertexAttribMap))
		return false;

	shader.setBlendMode(tech.blendmode);
	return true;
}

void ShaderEffect::readParts(const String& source)
{
	mParts.clear();
	mParts.reserve(16);
	PartStruct* currPart = nullptr;
	int pos = 0;
	String line;
	while (pos < source.length())
	{
		pos = source.getLine(line, pos);
		if (line.startsWith("##"))
		{
			int a = line.skipChars("# -", 2, +1);
			int b = line.findChars("# -", a, +1);
			mParts.push_back(PartStruct());
			currPart = &mParts.back();
			currPart->title.makeSubString(line, a, b-a);
			currPart->content.clear();
		}
		else if (currPart)
		{
			currPart->content << line << (char)13 << char(10);
		}
	}
}

void ShaderEffect::parseTechniques(String& source)
{
	mTechniques.clear();
	int a = -1;
	int b, c;
	while (source.includes("technique", a))
	{
		TechniqueStruct& tech = vectorAdd(mTechniques);
		String inheritedName;

		a = source.skipChars(" \t\n\r", a+9, +1);
		b = source.findChars(" \t\n\r:{", a, +1);
		tech.name.makeSubString(source, a, b-a);

		// Check for inheritance
		b = source.skipChars(" \t\n\r", b, +1);
		if (source[b] == ':')
		{
			a = source.skipChars(" \t\n\r", b+1, +1);
			b = source.findChars(" \t\n\r{", a, +1);
			inheritedName.makeSubString(source, a, b-a);

			TechniqueStruct* parentTech = findTechniqueByName(inheritedName);
			if (nullptr != parentTech)
			{
				// Copy everything from parent
				tech.vs_parts = parentTech->vs_parts;
				tech.fs_parts = parentTech->fs_parts;
				tech.defines = parentTech->defines;
				tech.vertexAttribMap = parentTech->vertexAttribMap;
				tech.blendmode = parentTech->blendmode;
			}
		}

		a = source.skipChars(" \t\n\r{", b, +1);
		b = source.findChar('}', a, +1);
		String techcontents;
		techcontents.makeSubString(source, a, b-a);
		source.remove(0, b+1);

		StringArray techlines;
		techcontents.split(techlines, ';');
		for (StringArray::iterator it = techlines.begin(); it != techlines.end(); ++it)
		{
			c = it->findChar('=', 0, +1);
			if (c >= it->length())
				continue;

			a = it->skipChars(" \t\n\r", 0, +1);
			b = it->skipChars(" \t\n\r", c-1, -1) + 1;
			String identifier;
			identifier.makeSubString(*it, a, b-a);
			identifier.lowerCase();
			a = it->skipChars(" \t\n\r", c+1, +1);
			b = it->skipChars(" \t\n\r", it->length()-1, -1) + 1;
			String value;
			value.makeSubString(*it, a, b-a);

			if (identifier == "vs" || identifier == "fs")
			{
				StringArray& parts = (identifier[0] == 'v') ? tech.vs_parts : tech.fs_parts;
				parts.clear();		// In case it was inherited, forget about that now

				StringArray includes;
				value.split(includes, '+');
				for (StringArray::iterator k = includes.begin(); k != includes.end(); ++k)
				{
					a = k->skipChars(" \t\n\r", 0, +1);
					b = k->skipChars(" \t\n\r", k->length()-1, -1) + 1;
					parts.push_back(k->getSubString(a, b-a));
				}
			}
			else if (identifier == "blendfunc")
			{
				value.lowerCase();
				if (value == "alpha")
					tech.blendmode = Shader::BlendMode::ALPHA;
				else if (value == "add")
					tech.blendmode = Shader::BlendMode::ADD;
				else
					tech.blendmode = Shader::BlendMode::OPAQUE;
			}
			else if (identifier == "define" || identifier == "defines")
			{
				tech.defines.push_back(value);
			}
			else if (identifier.startsWith("vertexattrib[") && identifier.endsWith("]"))
			{
				const String numberString = identifier.getSubString(13, identifier.length()-14);
				const int index = numberString.parseInt();
				tech.vertexAttribMap[index] = value;
			}
		}
	}
}

ShaderEffect::PartStruct* ShaderEffect::findPartByName(const String& name)
{
	for (unsigned int i = 0; i < mParts.size(); ++i)
	{
		if (mParts[i].title == name)
			return &mParts[i];
	}
	return nullptr;
}

void ShaderEffect::buildSourceFromParts(String& source, const std::vector<String>& parts, const String& definitions)
{
	String content;
	for (unsigned int j = 0; j < parts.size(); ++j)
	{
		PartStruct* part = findPartByName(parts[j]);
		if (nullptr != part)
		{
			content << *part->content;
		}
	}

	// Check for #version pragma
	//  -> It must be added in front of all definitions
	int pos = 0;
	String line;
	while (pos < content.length())
	{
		const int lastPos = pos;
		pos = content.getLine(line, pos);
		if (line.startsWith("#version"))
		{
			// Start with the version line
			source << line + '\n';

			// Add definitions next
			source << definitions;

			// Add content, but leave out the version line
			source << content.getSubString(0, lastPos);
			source << content.getSubString(pos, -1);
			return;
		}
	}

	// Reached only if no version line was found
	source << definitions;
	source << content;
}

void ShaderEffect::preprocessSource(String& source, Shader::ShaderType shaderType)
{
	String line;

	// Resolve includes
	for (int pos = 0; pos < source.length(); )
	{
		const int start = pos;
		pos = source.getLine(line, pos);
		if (line[0] == '#' && line.startsWith("#include"))
		{
			source.remove(start, pos-start);
			pos = start;

			int a = line.skipChars(" \t", 8, +1);
			if (a >= line.length() || line[a] != '"')
				continue;
			int b = line.findChar('"', a+1, +1);
			if (b >= line.length())
				continue;

			String incname;
			incname.makeSubString(line, a+1, b-a-1);
			if (incname.empty())
				continue;
			incname.insert(mIncludeDir, 0);

			String content;
			if (content.loadFile(*incname) && content.nonEmpty())
			{
				if (content[content.length()-1] != '\n')
					content << '\n';
				source.insert(content, pos);
				pos += content.length();
			}
		}
	}

#ifdef RMX_USE_GLES2
	// GLSL ES 2.0 translation
	String newSource;
	for (int pos = 0; pos < source.length(); )
	{
		const int start = pos;
		pos = source.getLine(line, pos);

		if (line.startsWith("#version"))
		{
			// Replace version info
			line = "#version 100";
		}
		else if (line.startsWith("in "))
		{
			// "in" -> "attribute" or "varying"
			line.remove(0, 2);
			line.insert((shaderType == Shader::ShaderType::VERTEX) ? "attribute" : "varying", 0);
		}
		else if (line.startsWith("out "))
		{
			// "out" -> "varying" or remove entirely
			if (shaderType == Shader::ShaderType::VERTEX)
			{
				line.remove(0, 3);
				line.insert("varying", 0);
			}
			else
			{
				// This assumes the fragment shader output variable is called "FragColor"
				if (line.startsWith("out vec4 FragColor;"))
				{
					line.remove(0, 19);
				}
			}
		}
		else
		{
			// "texture" -> "texture2D"
			int found = 0;
			while (true)
			{
				found = line.findString("texture(", found);
				if (found < 0)
					break;
				line.insert("2D", found + 7);
				found += 10;
			}

			// "FragColor" -> "glFragColor"
			found = 0;
			while (true)
			{
				found = line.findString("FragColor", found);
				if (found < 0)
					break;
				line.insert("gl_", found);
				found += 12;
			}
		}

		newSource << line << "\n";
	}
	source.swap(newSource);
#endif

	if (Shader::mShaderSourcePostProcessCallback)
	{
		Shader::mShaderSourcePostProcessCallback(source, shaderType);
	}
}
