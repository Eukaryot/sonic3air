/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"

#ifdef RMX_WITH_OPENGL_SUPPORT


/* ----- Shader -------------------------------------------------------------------------------------------------- */

void Shader::unbindShader()
{
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
}

Shader::Shader()
{
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
	const GLuint loc = glGetUniformLocation(mProgram, name);
	RMX_ASSERT(loc != 0xffffffff, "Could not resolve uniform name " << name);

	// Just for debugging uniforms if needed
/*
	GLint count = 0;
	glGetObjectParameterivARB(mProgram, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &count);
	for (GLint k = 0; k < count; ++k)
	{
		GLchar buffer[32];
		GLsizei length = 0;
		GLint size = 0;
		GLenum type;
		glGetActiveUniform(mProgram, k, 32, &length, &size, &type, buffer);
		_asm nop;
	}
*/
	return loc;
}

GLuint Shader::getAttribLocation(const char* name) const
{
	return glGetAttribLocation(mProgram, name);
}

void Shader::setParam(GLuint loc, int param)
{
	glUniform1i(loc, param);
}

void Shader::setParam(GLuint loc, const Vec2i& param)
{
	glUniform2iv(loc, 1, *param);
}

void Shader::setParam(GLuint loc, const Vec3i& param)
{
	glUniform3iv(loc, 1, *param);
}

void Shader::setParam(GLuint loc, const Vec4i& param)
{
	glUniform4iv(loc, 1, *param);
}

void Shader::setParam(GLuint loc, const Recti& param)
{
	glUniform4iv(loc, 1, param.mData);
}

void Shader::setParam(GLuint loc, float param)
{
	glUniform1f(loc, param);
}

void Shader::setParam(GLuint loc, const Vec2f& param)
{
	glUniform2fv(loc, 1, *param);
}

void Shader::setParam(GLuint loc, const Vec3f& param)
{
	glUniform3fv(loc, 1, *param);
}

void Shader::setParam(GLuint loc, const Vec4f& param)
{
	glUniform4fv(loc, 1, *param);
}

void Shader::setParam(GLuint loc, const Rectf& param)
{
	glUniform4fv(loc, 1, param.mData);
}

void Shader::setMatrix(GLuint loc, const Mat3f& matrix)
{
	glUniformMatrix3fv(loc, 1, true, *matrix);
}

void Shader::setMatrix(GLuint loc, const Mat4f& matrix)
{
	glUniformMatrix4fv(loc, 1, true, *matrix);
}

void Shader::setTexture(GLuint loc, GLuint handle, GLenum target)
{
	int number = mTextureCount;
	glActiveTexture(GL_TEXTURE0 + number);
	glBindTexture(target, handle);
	glUniform1i(loc, number);
	++mTextureCount;
}

void Shader::setTexture(GLuint loc, const Texture& texture)
{
	setTexture(loc, texture.getHandle(), texture.getType());
}

void Shader::bind()
{
	if (mBlendMode != BlendMode::UNDEFINED)
	{
		bool handled = false;
		if (mShaderApplyBlendModeCallback)
		{
			handled = Shader::mShaderApplyBlendModeCallback(mBlendMode);
		}

		if (!handled)
		{
			switch (mBlendMode)
			{
				case BlendMode::OPAQUE:	glBlendFunc(GL_ONE, GL_ZERO);  break;
				case BlendMode::ALPHA:	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  break;
				case BlendMode::ADD:	glBlendFunc(GL_ONE, GL_ONE);   break;
				default:				glBlendFunc(GL_ONE, GL_ZERO);  break;
			}
		}
	}
	glUseProgram(mProgram);
	resetTextureCount();
}

void Shader::unbind()
{
	unbindShader();
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

	return effect.getShader(*this, techname, additionalDefines);
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
		for (const auto& [index, attributeName] : *vertexAttribMap)
		{
			glBindAttribLocation(mProgram, index, *attributeName);
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
	for (PartStruct& part : mParts)
	{
		if (part.mTitle == "TECH")
			parseTechniques(part.mContent);
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
		if (tech.mName == name)
			return &tech;
	}
	return nullptr;
}

bool ShaderEffect::getShaderInternal(Shader& shader, TechniqueStruct& tech, const String& additionalDefines)
{
	String definitions;
	for (unsigned int j = 0; j < tech.mDefines.size(); ++j)
	{
		definitions << "#define " << tech.mDefines[j] << "\n";
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
	buildSourceFromParts(vsSource, tech.mVertexShaderParts, definitions);
	preprocessSource(vsSource, Shader::ShaderType::VERTEX);

	String fsSource;
	buildSourceFromParts(fsSource, tech.mFragmentShaderParts, definitions);
	preprocessSource(fsSource, Shader::ShaderType::FRAGMENT);

	if (!shader.compile(vsSource, fsSource, &tech.mVertexAttribMap))
		return false;

	shader.setBlendMode(tech.mBlendMode);
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
			currPart->mTitle.makeSubString(line, a, b-a);
			currPart->mContent.clear();
		}
		else if (currPart)
		{
			currPart->mContent << line << (char)13 << char(10);
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
		tech.mName.makeSubString(source, a, b-a);

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
				tech.mVertexShaderParts = parentTech->mVertexShaderParts;
				tech.mFragmentShaderParts = parentTech->mFragmentShaderParts;
				tech.mDefines = parentTech->mDefines;
				tech.mVertexAttribMap = parentTech->mVertexAttribMap;
				tech.mBlendMode = parentTech->mBlendMode;
			}
		}

		a = source.skipChars(" \t\n\r{", b, +1);
		b = source.findChar('}', a, +1);
		String techcontents;
		techcontents.makeSubString(source, a, b-a);
		source.remove(0, b+1);

		std::vector<String> techlines;
		techcontents.split(techlines, ';');
		std::vector<String> includes;
		for (const String& line : techlines)
		{
			c = line.findChar('=', 0, +1);
			if (c >= line.length())
				continue;

			a = line.skipChars(" \t\n\r", 0, +1);
			b = line.skipChars(" \t\n\r", c-1, -1) + 1;
			String identifier;
			identifier.makeSubString(line, a, b-a);
			identifier.lowerCase();
			a = line.skipChars(" \t\n\r", c+1, +1);
			b = line.skipChars(" \t\n\r", line.length()-1, -1) + 1;
			String value;
			value.makeSubString(line, a, b-a);

			if (identifier == "vs" || identifier == "fs")
			{
				std::vector<String>& parts = (identifier[0] == 'v') ? tech.mVertexShaderParts : tech.mFragmentShaderParts;
				parts.clear();		// In case it was inherited, forget about that now

				includes.clear();
				value.split(includes, '+');
				for (const String& include : includes)
				{
					a = include.skipChars(" \t\n\r", 0, +1);
					b = include.skipChars(" \t\n\r", include.length()-1, -1) + 1;
					parts.push_back(include.getSubString(a, b-a));
				}
			}
			else if (identifier == "blendfunc")
			{
				value.lowerCase();
				if (value == "alpha")
					tech.mBlendMode = Shader::BlendMode::ALPHA;
				else if (value == "add")
					tech.mBlendMode = Shader::BlendMode::ADD;
				else
					tech.mBlendMode = Shader::BlendMode::OPAQUE;
			}
			else if (identifier == "define" || identifier == "defines")
			{
				tech.mDefines.push_back(value);
			}
			else if (identifier.startsWith("vertexattrib[") && identifier.endsWith("]"))
			{
				const String numberString = identifier.getSubString(13, identifier.length()-14);
				const int index = numberString.parseInt();
				tech.mVertexAttribMap[index] = value;
			}
		}
	}
}

ShaderEffect::PartStruct* ShaderEffect::findPartByName(const String& name)
{
	for (PartStruct& part : mParts)
	{
		if (part.mTitle == name)
			return &part;
	}
	return nullptr;
}

void ShaderEffect::buildSourceFromParts(String& source, const std::vector<String>& parts, const String& definitions)
{
	String content;
	for (const String& partName : parts)
	{
		PartStruct* part = findPartByName(partName);
		if (nullptr != part)
		{
			content << *part->mContent;
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

#if defined(RMX_USE_GLES2) && !defined(PLATFORM_VITA)
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

#endif
