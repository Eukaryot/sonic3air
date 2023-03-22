
## ----- Shared -------------------------------------------------------------------

// Needed for isamplerBuffer
#version 140

precision mediump float;
precision mediump int;

uniform ivec2 Size;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
out vec3 LocalOffset;

uniform ivec3 Position;		// With z = priority flag (0 or 1)
uniform ivec2 PivotOffset;
uniform vec4 Transformation;
uniform ivec2 GameResolution;
uniform int WaterLevel;

void main()
{
	// Calculate local offset
	LocalOffset.x = position.x * float(Size.x);
	LocalOffset.y = position.y * float(Size.y);

	// Transform
	vec2 v = LocalOffset.xy + vec2(PivotOffset.xy);
	vec2 transformedVertex;
	transformedVertex.x = v.x * Transformation.x + v.y * Transformation.y;
	transformedVertex.y = v.x * Transformation.z + v.y * Transformation.w;

	// Transform local -> screen space
	transformedVertex.x = float(Position.x) + transformedVertex.x;
	transformedVertex.y = float(Position.y) + transformedVertex.y;

	// Transform screen space -> view space
	gl_Position.x = transformedVertex.x / float(GameResolution.x) * 2.0 - 1.0;
	gl_Position.y = transformedVertex.y / float(GameResolution.y) * 2.0 - 1.0;
	gl_Position.z = float(Position.z) * 0.5;
	gl_Position.w = 1.0;

	// Calculate water offset
	LocalOffset.z = transformedVertex.y - float(WaterLevel);
}



## ----- Fragment -----------------------------------------------------------------

in vec3 LocalOffset;
out vec4 FragColor;

#ifdef USE_BUFFER_TEXTURES
	uniform isamplerBuffer SpriteTexture;
#else
	uniform sampler2D SpriteTexture;
#endif
uniform sampler2D PaletteTexture;
uniform int Atex;
uniform vec4 TintColor;
uniform vec4 AddedColor;


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
	int paletteIndex = Atex + texelFetch(SpriteTexture, ix + iy * Size.x).x;
#else
	int paletteIndex = Atex + int(texture(SpriteTexture, vec2(((float(ix) + 0.5) / float(Size.x)), (float(iy) + 0.5) / float(Size.y))).x * 256.0);
#endif

	vec4 color = getPaletteColor(paletteIndex, clamp(LocalOffset.z, 0.0, 0.5));
	color = vec4(AddedColor.rgb, 0.0) + color * TintColor;
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

technique Standard_AlphaTest : Standard
{
	define = ALPHA_TEST;
}
