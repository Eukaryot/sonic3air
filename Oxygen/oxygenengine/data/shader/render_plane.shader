
## ----- Shared -------------------------------------------------------------------

#version 140		// Needed for isamplerBuffer

precision highp float;
precision highp int;



## ----- Vertex -------------------------------------------------------------------

in vec4 position;
out vec3 LocalOffset;

uniform ivec4 ActiveRect;
uniform ivec2 GameResolution;
uniform int WaterLevel;

void main()
{
	ivec2 screenPosition;
	screenPosition.x = ActiveRect.x + int(position.x * float(ActiveRect.z) + 0.5);
	screenPosition.y = ActiveRect.y + int(position.y * float(ActiveRect.w) + 0.5);

	LocalOffset.x = float(screenPosition.x);
	LocalOffset.y = float(screenPosition.y);
	LocalOffset.z = float(screenPosition.y - WaterLevel) / float(GameResolution.y);

	gl_Position.x = float(screenPosition.x) / float(GameResolution.x) * 2.0 - 1.0;
	gl_Position.y = float(screenPosition.y) / float(GameResolution.y) * 2.0 - 1.0;
	gl_Position.z = 0.5;	// Has an effect only if depth write is enabled (it is for the priority plane, i.e. when PriorityFlag == 1)
	gl_Position.w = 1.0;
}



## ----- Fragment -----------------------------------------------------------------

in vec3 LocalOffset;
out vec4 FragColor;

#ifdef USE_BUFFER_TEXTURES
	uniform isamplerBuffer IndexTexture;
	uniform isamplerBuffer PatternCacheTexture;
#else
	uniform sampler2D IndexTexture;
	uniform sampler2D PatternCacheTexture;
#endif

uniform sampler2D PaletteTexture;
uniform ivec4 PlayfieldSize;
uniform int PriorityFlag;		// 0 or 1

#ifdef HORIZONTAL_SCROLLING
	#ifdef USE_BUFFER_TEXTURES
		uniform isamplerBuffer HScrollOffsetsTexture;
	#else
		uniform sampler2D HScrollOffsetsTexture;
	#endif
#else
	uniform int ScrollOffsetX;
#endif

#ifdef VERTICAL_SCROLLING
	#ifdef USE_BUFFER_TEXTURES
		uniform isamplerBuffer VScrollOffsetsTexture;
	#else
		uniform sampler2D VScrollOffsetsTexture;
	#endif
	uniform int VScrollOffsetBias;
#else
	uniform int ScrollOffsetY;
#endif

void main()
{
	int ix = int(LocalOffset.x);
	int iy = int(LocalOffset.y);

#ifdef HORIZONTAL_SCROLLING
	#ifdef USE_BUFFER_TEXTURES
		int scrollOffsetLookupX = texelFetch(HScrollOffsetsTexture, iy).x;
	#else
		vec2 texelScrollX = texture(HScrollOffsetsTexture, vec2((float(iy) + 0.5) / 256.0, 0.5)).ra;
		int scrollOffsetLookupXH = int(mod(texelScrollX.y * 256.0, 16.0));
		int scrollOffsetLookupXL = int(texelScrollX.x * 256.0);
		int scrollOffsetLookupX = scrollOffsetLookupXL + scrollOffsetLookupXH * 256;
	#endif
#else
	int scrollOffsetLookupX = ScrollOffsetX;
#endif

#ifdef VERTICAL_SCROLLING
	int vx = ix - VScrollOffsetBias;
	#ifdef GL_ES
		vx = int((vx - int(vx / 0x200) * 0x200) / 0x10);
	#else
		vx = (vx & 0x1f0) >> 4;
	#endif
	#ifdef USE_BUFFER_TEXTURES
		int scrollOffsetLookupY = texelFetch(VScrollOffsetsTexture, vx).x;
	#else
		vec2 texelScrollY = texture(VScrollOffsetsTexture, vec2((float(vx) + 0.5) / 32.0, 0.5)).ra;
		int scrollOffsetLookupYH = int(mod(texelScrollY.y * 256.0, 16.0));
		int scrollOffsetLookupYL = int(texelScrollY.x * 256.0);
		int scrollOffsetLookupY = scrollOffsetLookupYL + scrollOffsetLookupYH * 256;
	#endif
#else
	int scrollOffsetLookupY = ScrollOffsetY;
#endif

	ix += scrollOffsetLookupX;
	iy += scrollOffsetLookupY;
#ifdef GL_ES
	ix = ix - int(ix / 0x1000) * 0x1000;
	iy = iy - int(iy / PlayfieldSize.y) * PlayfieldSize.y;
#else
	ix = ix & 0x0fff;
	iy = iy & (PlayfieldSize.y - 1);
#endif

#ifdef NOREPEAT_SCROLLOFFSETS
	if (ix >= PlayfieldSize.x)
		discard;
#else
	#ifdef GL_ES
		ix -= int(ix / PlayfieldSize.x) * PlayfieldSize.x;
	#else
		ix &= (PlayfieldSize.x - 1);
	#endif
#endif

	int patternX = int(ix / 8);
	int patternY = int(iy / 8);
	int localX = ix - patternX * 8;
	int localY = iy - patternY * 8;

#ifdef USE_BUFFER_TEXTURES
	int patternIndex = texelFetch(IndexTexture, patternX + patternY * PlayfieldSize.z).x;
	if (int(patternIndex / 0x8000) != PriorityFlag)
		discard;
	// No support for GL_ES here
#else
	vec2 texel = texture(IndexTexture, vec2((float(patternX + patternY * PlayfieldSize.z) + 0.5) / float(PlayfieldSize.z * PlayfieldSize.w), 0.5)).ra;
	int patternIndexH = int(texel.y * 255.5);
	int patternIndexL = int(texel.x * 255.5);
	if ((patternIndexH >= 0x80) != (PriorityFlag >= 1))
		discard;
	#ifndef GL_ES
		int patternIndex = patternIndexL + patternIndexH * 256;
	#endif
#endif

#ifdef GL_ES
	int atex = int((patternIndexH - int(patternIndexH / 0x80) * 0x80) / 0x20) * 0x10;
	localX = ((patternIndexH - int(patternIndexH / 0x10) * 0x10) < 0x08) ? localX : (7 - localX);
	localY = ((patternIndexH - int(patternIndexH / 0x20) * 0x20) < 0x10) ? localY : (7 - localY);
#else
	int atex = (patternIndex >> 9) & 0x30;
	localX = ((patternIndex & 0x0800) == 0) ? localX : (7 - localX);
	localY = ((patternIndex & 0x1000) == 0) ? localY : (7 - localY);
#endif

	int patternCacheLookupIndexX = localX + localY * 8;
#ifdef GL_ES
	int patternCacheLookupIndexY = patternIndexL + (patternIndexH - int(patternIndexH / 8) * 8) * 0x100;
#else
	int patternCacheLookupIndexY = patternIndex & 0x07ff;
#endif
#ifdef USE_BUFFER_TEXTURES
	int paletteIndex = texelFetch(PatternCacheTexture, patternCacheLookupIndexX + patternCacheLookupIndexY * 64).x;
#else
	int paletteIndex = int(texture(PatternCacheTexture, vec2((float(patternCacheLookupIndexX) + 0.5) / 64.0, (float(patternCacheLookupIndexY) + 0.5) / 2048.0)).x * 256.0);
#endif
	paletteIndex += atex;

	vec4 color = texture(PaletteTexture, vec2(float(paletteIndex) / 256.0, LocalOffset.z + 0.5));
#ifdef ALPHA_TEST
	if (color.a < 0.01)
		discard;
#endif

	FragColor = color;
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	blendfunc = opaque;
	vs = Shared + Vertex;
	fs = Shared + Fragment;
	vertexattrib[0] = position;
}

technique HorizontalScrolling : Standard
{
	define = HORIZONTAL_SCROLLING;
}

technique HorizontalScrollingNoRepeat : Standard
{
	define = HORIZONTAL_SCROLLING;
	define = NOREPEAT_SCROLLOFFSETS;
}

technique VerticalScrolling : Standard
{
	define = VERTICAL_SCROLLING;
}

technique HorizontalVerticalScrolling : Standard
{
	define = HORIZONTAL_SCROLLING;
	define = VERTICAL_SCROLLING;
}
