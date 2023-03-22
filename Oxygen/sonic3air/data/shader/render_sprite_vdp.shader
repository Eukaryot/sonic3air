
## ----- Shared -------------------------------------------------------------------

// Needed for isamplerBuffer
#version 140

precision highp float;
precision highp int;

uniform ivec2 Size;
uniform int FirstPattern;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
out vec3 LocalOffset;

uniform ivec3 Position;		// With z = priority flag (0 or 1)
uniform ivec2 GameResolution;
uniform int WaterLevel;

void main()
{
	// Calculate local offset
	LocalOffset.x = position.x * float(Size.x * 8);
	LocalOffset.y = position.y * float(Size.y * 8);

	// Flip if necessary
	vec2 transformedVertex = position.xy;
#ifdef GL_ES
	if ((FirstPattern - int(FirstPattern / 0x1000) * 0x1000) >= 0x0800)
		transformedVertex.x = (1.0 - transformedVertex.x);
	if ((FirstPattern - int(FirstPattern / 0x2000) * 0x2000) >= 0x1000)
		transformedVertex.y = (1.0 - transformedVertex.y);
#else
	if ((FirstPattern & 0x0800) != 0)
		transformedVertex.x = (1.0 - transformedVertex.x);
	if ((FirstPattern & 0x1000) != 0)
		transformedVertex.y = (1.0 - transformedVertex.y);
#endif

	// Transform local -> screen space
	transformedVertex.x = float(Position.x) + transformedVertex.x * float(Size.x * 8);
	transformedVertex.y = float(Position.y) + transformedVertex.y * float(Size.y * 8);

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
	uniform isamplerBuffer PatternCacheTexture;
#else
	uniform sampler2D PatternCacheTexture;
#endif
uniform sampler2D PaletteTexture;
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
	int patternX = ix / 8;
	int patternY = iy / 8;
	int localX = ix - patternX * 8;
	int localY = iy - patternY * 8;

	int patternIndex = FirstPattern + patternX * Size.y + patternY;
#ifdef GL_ES
	int atex = ((patternIndex - int(patternIndex / 32768) * 32768) / 8192) * 16;
#else
	int atex = (patternIndex >> 9) & 0x30;
#endif

	int patternCacheLookupIndexX = localX + localY * 8;
#ifdef GL_ES
	int patternCacheLookupIndexY = (patternIndex - int(patternIndex / 2048) * 2048);
#else
	int patternCacheLookupIndexY = patternIndex & 0x07ff;
#endif
#ifdef USE_BUFFER_TEXTURES
	int patternCacheLookupIndex = patternCacheLookupIndexX + patternCacheLookupIndexY * 64;
	int paletteIndex = texelFetch(PatternCacheTexture, patternCacheLookupIndex).x;
#else
	int paletteIndex = int(texture(PatternCacheTexture, vec2((float(patternCacheLookupIndexX) + 0.5) / 64.0, (float(patternCacheLookupIndexY) + 0.5) / 2048.0)).x * 256.0);
#endif
	paletteIndex += atex;

	vec4 color = getPaletteColor(paletteIndex, clamp(LocalOffset.z, 0.0, 0.5));
	color = vec4(AddedColor.rgb, 0.0) + color * TintColor;
	if (color.a < 0.01)
		discard;

	FragColor = color;
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	vs = Shared + Vertex;
	fs = Shared + Fragment;
	vertexattrib[0] = position;
}
