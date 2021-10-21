/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Painter
*		Helper class for 2D rendering with OpenGL.
*/

#pragma once


namespace rmx
{
	class Painter
	{
	public:
		struct PrintOptions
		{
			int   mAlignment = 1;
			int   mSpacing = 0;
			Color mTintColor = Color::WHITE;
		};

	public:
		Painter();
		~Painter();

		void begin();
		void end();

		void enableTextures(bool enable);
		void resetColor();
		void setColor(const Color& color);
		const Color& getLastColor() { return mColor; }

		void drawRect(const Rectf& rect, const Color& color);
		void drawRect(const Rectf& rect, const Texture& texture, const Color& color = Color::WHITE);
		void drawRect(const Rectf& rect, const Texture& texture, const Vec2f& uv0, const Vec2f& uv1, const Color& color = Color::WHITE);
		void drawQuadPatch(int numVertX, int numVertY, float* verticesX, float* verticesY, float* texcrdsX, float* texcrdsY);

		void print(Font& font, const Rectf& rect, const StringReader& text, int alignment = 1, const Color& color = Color::WHITE);
		void print(Font& font, const Rectf& rect, const StringReader& text, const PrintOptions& printOptions);
		FontOutput* getFontOutput(Font& font);

		void resetScissor();
		void setScissor(const Recti& rect);
		void pushScissor(const Recti& rect);
		void popScissor();

	private:
		void reset();

	private:
		bool mTexturesEnabled;
		Color mColor;
		typedef std::vector<Recti> RectStack;
		RectStack mScissorStack;

		struct OutputInfo
		{
			FontOutput* output;
		};

		typedef std::map<FontKey,OutputInfo> OutputInfoMap;
		OutputInfoMap mOutputInfoMap;
	};
}
