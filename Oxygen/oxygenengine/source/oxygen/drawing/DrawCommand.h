/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class DrawerTexture;
class DrawCommandFactory;


enum class DrawerBlendMode
{
	NONE,
	ALPHA
};

enum class DrawerSamplingMode
{
	POINT,
	BILINEAR
};

enum class DrawerWrapMode
{
	CLAMP,
	REPEAT
};

struct DrawerMeshVertex		// TODO: Rename to "DrawerMeshVertex_P2_T2" to be more specific here
{
	Vec2f mPosition;
	Vec2f mTexcoords;
};

struct DrawerMeshVertex_P2_C4
{
	Vec2f mPosition;
	Color mColor;
};


class DrawCommand
{
public:
	enum class Type
	{
		UNDEFINED = 0,
		SET_WINDOW_RENDER_TARGET,
		SET_RENDER_TARGET,
		RECT,
		UPSCALED_RECT,
		SPRITE,
		MESH,
		MESH_VERTEX_COLOR,
		SET_BLEND_MODE,
		SET_SAMPLING_MODE,
		SET_WRAP_MODE,
		PRINT_TEXT,
		PRINT_TEXT_W,
		PUSH_SCISSOR,
		POP_SCISSOR
	};

public:
	static DrawCommandFactory mFactory;

public:
	inline Type getType() const  { return mType; }

	template<typename T> T& as()			  { return static_cast<T&>(*this); }
	template<typename T> const T& as() const  { return static_cast<const T&>(*this); }

protected:
	inline virtual ~DrawCommand() {}
	inline DrawCommand(Type type) : mType(type) {}

private:
	Type mType;
};



class SetWindowRenderTargetDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<SetWindowRenderTargetDrawCommand>;

protected:
	SetWindowRenderTargetDrawCommand(const Recti& viewport) : DrawCommand(Type::SET_WINDOW_RENDER_TARGET), mViewport(viewport) {}

public:
	Recti mViewport;
};


class SetRenderTargetDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<SetRenderTargetDrawCommand>;

protected:
	SetRenderTargetDrawCommand(DrawerTexture& texture, const Recti& viewport) : DrawCommand(Type::SET_RENDER_TARGET), mTexture(&texture), mViewport(viewport) {}

public:
	DrawerTexture* mTexture = nullptr;
	Recti mViewport;
};


class RectDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<RectDrawCommand>;

protected:
	RectDrawCommand(const Recti& rect, const Color& color) : DrawCommand(Type::RECT), mRect(rect), mColor(color) {}
	RectDrawCommand(const Recti& rect, DrawerTexture& texture) : DrawCommand(Type::RECT), mRect(rect), mTexture(&texture) {}
	RectDrawCommand(const Recti& rect, DrawerTexture& texture, const Color& tintColor) : DrawCommand(Type::RECT), mRect(rect), mTexture(&texture), mColor(tintColor) {}
	RectDrawCommand(const Recti& rect, DrawerTexture& texture, const Vec2f& uv0, const Vec2f& uv1, const Color& tintColor) : DrawCommand(Type::RECT), mRect(rect), mTexture(&texture), mColor(tintColor), mUV0(uv0), mUV1(uv1) {}

public:
	Recti mRect;
	DrawerTexture* mTexture = nullptr;
	Color mColor = Color::WHITE;
	Vec2f mUV0 = Vec2f(0.0f, 0.0f);
	Vec2f mUV1 = Vec2f(1.0f, 1.0f);
};


class UpscaledRectDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<UpscaledRectDrawCommand>;

protected:
	UpscaledRectDrawCommand(const Recti& rect, DrawerTexture& texture) : DrawCommand(Type::UPSCALED_RECT), mRect(rect), mTexture(&texture) {}

public:
	Recti mRect;
	DrawerTexture* mTexture = nullptr;
};


class SpriteDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<SpriteDrawCommand>;

protected:
	SpriteDrawCommand(Vec2i position, uint64 spriteKey, const Color& tintColor, Vec2f scale) : DrawCommand(Type::SPRITE), mPosition(position), mSpriteKey(spriteKey), mTintColor(tintColor), mScale(scale) {}

public:
	Vec2i mPosition;
	uint64 mSpriteKey;
	Color mTintColor;
	Vec2f mScale;
};


class MeshDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<MeshDrawCommand>;

protected:
	MeshDrawCommand(const std::vector<DrawerMeshVertex>& triangles, DrawerTexture& texture) : DrawCommand(Type::MESH), mTriangles(triangles), mTexture(&texture) {}
	MeshDrawCommand(std::vector<DrawerMeshVertex>&& triangles, DrawerTexture& texture) : DrawCommand(Type::MESH), mTriangles(triangles), mTexture(&texture) {}

public:
	std::vector<DrawerMeshVertex> mTriangles;
	DrawerTexture* mTexture = nullptr;
};


class MeshVertexColorDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<MeshVertexColorDrawCommand>;

protected:
	MeshVertexColorDrawCommand(const std::vector<DrawerMeshVertex_P2_C4>& triangles) : DrawCommand(Type::MESH_VERTEX_COLOR), mTriangles(triangles) {}
	MeshVertexColorDrawCommand(std::vector<DrawerMeshVertex_P2_C4>&& triangles) : DrawCommand(Type::MESH_VERTEX_COLOR), mTriangles(triangles) {}

public:
	std::vector<DrawerMeshVertex_P2_C4> mTriangles;
};


class SetBlendModeDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<SetBlendModeDrawCommand>;

protected:
	SetBlendModeDrawCommand(DrawerBlendMode blendMode) : DrawCommand(Type::SET_BLEND_MODE), mBlendMode(blendMode) {}

public:
	DrawerBlendMode mBlendMode;
};


class SetSamplingModeDrawCommand final : public DrawCommand
{
	friend class ObjectPoolBase<SetSamplingModeDrawCommand>;

protected:
	SetSamplingModeDrawCommand(DrawerSamplingMode samplingMode) : DrawCommand(Type::SET_SAMPLING_MODE), mSamplingMode(samplingMode) {}

public:
	DrawerSamplingMode mSamplingMode;
};


class SetWrapModeDrawCommand final : public DrawCommand
{
	friend class ObjectPoolBase<SetWrapModeDrawCommand>;

protected:
	SetWrapModeDrawCommand(DrawerWrapMode wrapMode) : DrawCommand(Type::SET_WRAP_MODE), mWrapMode(wrapMode) {}

public:
	DrawerWrapMode mWrapMode;
};


class PrintTextDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<PrintTextDrawCommand>;

protected:
	PrintTextDrawCommand(Font& font, const Recti& rect, const String& text, int alignment = 1, Color color = Color::WHITE) :
		DrawCommand(Type::PRINT_TEXT), mFont(&font), mRect(rect), mText(text)
	{
		mPrintOptions.mAlignment = alignment;
		mPrintOptions.mTintColor = color;
	}

	PrintTextDrawCommand(Font& font, const Recti& rect, const String& text, const rmx::Painter::PrintOptions& printOptions) :
		DrawCommand(Type::PRINT_TEXT), mFont(&font), mRect(rect), mText(text), mPrintOptions(printOptions)
	{}

public:
	Font* mFont = nullptr;
	Recti mRect;
	String mText;
	rmx::Painter::PrintOptions mPrintOptions;
};


class PrintTextWDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<PrintTextWDrawCommand>;

protected:
	PrintTextWDrawCommand(Font& font, const Recti& rect, const WString& text, int alignment = 1, Color color = Color::WHITE) :
		DrawCommand(Type::PRINT_TEXT_W), mFont(&font), mRect(rect), mText(text)
	{
		mPrintOptions.mAlignment = alignment;
		mPrintOptions.mTintColor = color;
	}

	PrintTextWDrawCommand(Font& font, const Recti& rect, const WString& text, const rmx::Painter::PrintOptions& printOptions) :
		DrawCommand(Type::PRINT_TEXT_W), mFont(&font), mRect(rect), mText(text), mPrintOptions(printOptions)
	{}

public:
	Font* mFont = nullptr;
	Recti mRect;
	WString mText;
	rmx::Painter::PrintOptions mPrintOptions;
};


class PushScissorDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<PushScissorDrawCommand>;

protected:
	PushScissorDrawCommand(const Recti& rect) : DrawCommand(Type::PUSH_SCISSOR), mRect(rect) {}

public:
	Recti mRect;
};


class PopScissorDrawCommand final : public DrawCommand
{
friend class ObjectPoolBase<PopScissorDrawCommand>;

protected:
	PopScissorDrawCommand() : DrawCommand(Type::POP_SCISSOR) {}
};


class DrawCommandFactory
{
public:
	ObjectPool<SetWindowRenderTargetDrawCommand> mSetWindowRenderTargetDrawCommands;
	ObjectPool<SetRenderTargetDrawCommand>		 mSetRenderTargetDrawCommands;
	ObjectPool<RectDrawCommand>					 mRectDrawCommands;
	ObjectPool<UpscaledRectDrawCommand>			 mUpscaledRectDrawCommands;
	ObjectPool<SpriteDrawCommand>				 mSpriteDrawCommands;
	ObjectPool<MeshDrawCommand>					 mMeshDrawCommands;
	ObjectPool<MeshVertexColorDrawCommand>		 mMeshVertexColorDrawCommands;
	ObjectPool<SetBlendModeDrawCommand>			 mSetBlendModeDrawCommands;
	ObjectPool<SetSamplingModeDrawCommand>		 mSetSamplingModeDrawCommands;
	ObjectPool<SetWrapModeDrawCommand>			 mSetWrapModeDrawCommands;
	ObjectPool<PrintTextDrawCommand>			 mPrintTextDrawCommands;
	ObjectPool<PrintTextWDrawCommand>			 mPrintTextWDrawCommands;
	ObjectPool<PushScissorDrawCommand>			 mPushScissorDrawCommands;
	ObjectPool<PopScissorDrawCommand>			 mPopScissorDrawCommands;

public:
	void destroy(DrawCommand& drawCommand)
	{
		switch (drawCommand.getType())
		{
			case DrawCommand::Type::UNDEFINED:					break;	// This should never happen anyways
			case DrawCommand::Type::SET_WINDOW_RENDER_TARGET:	mSetWindowRenderTargetDrawCommands.destroyObject(drawCommand.as<SetWindowRenderTargetDrawCommand>());  break;
			case DrawCommand::Type::SET_RENDER_TARGET:			mSetRenderTargetDrawCommands.destroyObject(drawCommand.as<SetRenderTargetDrawCommand>());  break;
			case DrawCommand::Type::RECT:						mRectDrawCommands.destroyObject(drawCommand.as<RectDrawCommand>());  break;
			case DrawCommand::Type::UPSCALED_RECT:				mUpscaledRectDrawCommands.destroyObject(drawCommand.as<UpscaledRectDrawCommand>());  break;
			case DrawCommand::Type::SPRITE:						mSpriteDrawCommands.destroyObject(drawCommand.as<SpriteDrawCommand>());  break;
			case DrawCommand::Type::MESH:						mMeshDrawCommands.destroyObject(drawCommand.as<MeshDrawCommand>());  break;
			case DrawCommand::Type::MESH_VERTEX_COLOR:			mMeshVertexColorDrawCommands.destroyObject(drawCommand.as<MeshVertexColorDrawCommand>());  break;
			case DrawCommand::Type::SET_BLEND_MODE:				mSetBlendModeDrawCommands.destroyObject(drawCommand.as<SetBlendModeDrawCommand>());  break;
			case DrawCommand::Type::SET_SAMPLING_MODE:			mSetSamplingModeDrawCommands.destroyObject(drawCommand.as<SetSamplingModeDrawCommand>());  break;
			case DrawCommand::Type::SET_WRAP_MODE:				mSetWrapModeDrawCommands.destroyObject(drawCommand.as<SetWrapModeDrawCommand>());  break;
			case DrawCommand::Type::PRINT_TEXT:					mPrintTextDrawCommands.destroyObject(drawCommand.as<PrintTextDrawCommand>());  break;
			case DrawCommand::Type::PRINT_TEXT_W:				mPrintTextWDrawCommands.destroyObject(drawCommand.as<PrintTextWDrawCommand>());  break;
			case DrawCommand::Type::PUSH_SCISSOR:				mPushScissorDrawCommands.destroyObject(drawCommand.as<PushScissorDrawCommand>());  break;
			case DrawCommand::Type::POP_SCISSOR:				mPopScissorDrawCommands.destroyObject(drawCommand.as<PopScissorDrawCommand>());  break;
		}
	}
};
