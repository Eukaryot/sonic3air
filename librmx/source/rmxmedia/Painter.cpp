/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


namespace rmx
{

	Painter::Painter()
	{
		reset();
	}

	Painter::~Painter()
	{
	}

	void Painter::reset()
	{
		mColor = Color::WHITE;
		mTexturesEnabled = false;

		// Note: Most of the Painter class actually only makes sense with OpenGL...
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef ALLOW_LEGACY_OPENGL
			glColor(mColor);
		#endif
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_SCISSOR_TEST);
		}
	}

	void Painter::begin()
	{
		FTX::Video->setPixelView();
		reset();
	}

	void Painter::end()
	{
		resetScissor();
	}

	void Painter::enableTextures(bool enable)
	{
		if (mTexturesEnabled == enable)
			return;

		mTexturesEnabled = enable;
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
			glEnable_Toggle(GL_TEXTURE_2D, enable);
		}
	}

	void Painter::resetColor()
	{
		if (mColor == Color::WHITE)
			return;

		mColor = Color::WHITE;
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef ALLOW_LEGACY_OPENGL
			glColor3f(1.0f, 1.0f, 1.0f);
		#else
			RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
		#endif
		}
	}

	void Painter::setColor(const Color& color)
	{
		if (mColor == color)
			return;

		mColor = color;
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef ALLOW_LEGACY_OPENGL
			glColor(color);
		#else
			RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
		#endif
		}
	}

	void Painter::drawRect(const Rectf& rect, const Color& color)
	{
		enableTextures(false);
		setColor(color);
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
		#ifdef ALLOW_LEGACY_OPENGL
			glRectf(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
		#else
			RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
		#endif
		}
	}

	void Painter::drawRect(const Rectf& rect, const Texture& texture, const Color& color)
	{
	#ifdef ALLOW_LEGACY_OPENGL
		enableTextures(true);
		texture.bind();
		setColor(color);
		::drawRect(rect);
	#else
		RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
	#endif
	}

	void Painter::drawRect(const Rectf& rect, const Texture& texture, const Vec2f& uv0, const Vec2f& uv1, const Color& color)
	{
	#ifdef ALLOW_LEGACY_OPENGL
		enableTextures(true);
		texture.bind();
		setColor(color);
		float texcoords[4] = { uv0.x, uv0.y, uv1.x, uv1.y };
		::drawRect(rect, texcoords);
	#else
		RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
	#endif
	}

	void Painter::drawQuadPatch(int numVertX, int numVertY, float* verticesX, float* verticesY, float* texcrdsX, float* texcrdsY)
	{
	#ifdef ALLOW_LEGACY_OPENGL
		resetColor();
		::drawQuadPatch(numVertX, numVertY, verticesX, verticesY, texcrdsX, texcrdsY);
	#else
		RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
	#endif
	}

	void Painter::print(Font& font, const Rectf& rect, const StringReader& text, int alignment, const Color& color)
	{
		static PrintOptions printOptions;
		printOptions.mAlignment = alignment;
		printOptions.mTintColor = color;
		print(font, rect, text, printOptions);
	}

	void Painter::print(Font& font, const Rectf& rect, const StringReader& text, const PrintOptions& printOptions)
	{
		OpenGLFontOutput& fontOutput = getOpenGLFontOutput(font);
		const Vec2f pos = font.alignText(rect, text, printOptions.mAlignment);

		std::vector<Font::TypeInfo> typeinfos;
		font.getTypeInfos(typeinfos, pos, text, printOptions.mSpacing);

		enableTextures(true);
		setColor(printOptions.mTintColor);
		fontOutput.print(typeinfos);
	}

	OpenGLFontOutput& Painter::getOpenGLFontOutput(Font& font)
	{
		// Get or create OpenGLFontOutput instance
		OpenGLFontOutput* fontOutput = mapFind(mFontOutputMap, &font);
		if (nullptr != fontOutput)
			return *fontOutput;

		const auto pair = mFontOutputMap.emplace(&font, font);
		return pair.first->second;
	}

	void Painter::resetScissor()
	{
		if (mScissorStack.empty())
			return;

		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
			glDisable(GL_SCISSOR_TEST);
		}
		mScissorStack.clear();
	}

	void Painter::setScissor(const Recti& rect)
	{
		if (mScissorStack.empty())
		{
			glEnable(GL_SCISSOR_TEST);
		}
		mScissorStack.clear();
		mScissorStack.push_back(rect);
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
			glScissor(rect.x, FTX::screenHeight() - rect.y - rect.height, rect.width, rect.height);
		}
	}

	void Painter::pushScissor(const Recti& rect)
	{
		const Recti& oldScissor = (!mScissorStack.empty()) ? mScissorStack.back() : FTX::screenRect();
		if (mScissorStack.empty())
		{
			if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
			{
				glEnable(GL_SCISSOR_TEST);
			}
		}

		Recti newScissor;
		newScissor.intersect(oldScissor, rect);
		mScissorStack.push_back(newScissor);
		if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
		{
			glScissor(newScissor.x, FTX::screenHeight() - newScissor.y - newScissor.height, newScissor.width, newScissor.height);
		}
	}

	void Painter::popScissor()
	{
		assert(!mScissorStack.empty());
		if (mScissorStack.empty())
			return;

		if (mScissorStack.size() == 1)
		{
			resetScissor();
		}
		else
		{
			mScissorStack.pop_back();
			const Recti& newScissor = mScissorStack.back();
			if (FTX::Video->getVideoConfig().renderer == VideoConfig::Renderer::OPENGL)
			{
				glScissor(newScissor.x, FTX::screenHeight() - newScissor.y - newScissor.height, newScissor.width, newScissor.height);
			}
		}
	}

}
