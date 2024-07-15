
## ----- Shared -------------------------------------------------------------------

// Needed for isamplerBuffer
#version 140

precision mediump float;
precision mediump int;

uniform ivec2 Size;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
out vec2 LocalOffset;

uniform vec4 Transform;

void main()
{
	// Calculate local offset
	LocalOffset.x = position.x * float(Size.x);
	LocalOffset.y = position.y * float(Size.y);

	vec2 pos = vec2(Transform.x + position.x * Transform.z, Transform.y + position.y * Transform.w);
	gl_Position = vec4(pos, 0.0, 1.0);
}



## ----- Fragment -----------------------------------------------------------------

in vec2 LocalOffset;
out vec4 FragColor;

#ifdef USE_BUFFER_TEXTURES
	uniform isamplerBuffer Texture;
#else
	uniform sampler2D Texture;
#endif
uniform sampler2D PaletteTexture;
#ifdef USE_TINT_COLOR
	uniform vec4 TintColor;
	uniform vec4 AddedColor;
#endif


vec4 getPaletteColor(int paletteIndex, float paletteOffsetY)
{
#ifdef GL_ES
	int paletteY = paletteIndex / 256;
	int paletteX = paletteIndex - paletteY * 256;
#else
	int paletteX = paletteIndex & 0xff;
	int paletteY = paletteIndex >> 8;
#endif
	vec2 samplePosition = vec2((float(paletteX) + 0.5) / 256.0, (float(paletteY) + 0.5) / 4.0 + paletteOffsetY);
	return texture(PaletteTexture, samplePosition);
}


void main()
{
	int ix = int(LocalOffset.x);
	int iy = int(LocalOffset.y);
#ifdef USE_BUFFER_TEXTURES
	int paletteIndex = texelFetch(Texture, ix + iy * Size.x).x;
#else
	int paletteIndex = int(texture(Texture, vec2(((float(ix) + 0.5) / float(Size.x)), (float(iy) + 0.5) / float(Size.y))).x * 256.0);
#endif

	vec4 color = getPaletteColor(paletteIndex, 0.0);
#ifdef USE_TINT_COLOR
	color = vec4(AddedColor.rgb, 0.0) + color * TintColor;
#endif
#ifdef ALPHA_TEST
	if (color.a < 0.01)
		discard;
#endif

	FragColor = color;
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	vs = Shared + Vertex;
	fs = Shared + Fragment;
	vertexattrib[0] = position;
}

technique TintColor : Standard
{
	define = USE_TINT_COLOR;
}

technique Standard_AlphaTest : Standard
{
	define = ALPHA_TEST;
}

technique TintColor_AlphaTest : Standard
{
	define = ALPHA_TEST;
	define = USE_TINT_COLOR;
}
