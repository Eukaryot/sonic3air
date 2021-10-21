
## ----- Shared -------------------------------------------------------------------

#version 140		// Needed for isamplerBuffer

precision mediump float;
precision mediump int;



## ----- Vertex -------------------------------------------------------------------

in vec4 position;
out vec2 uv0;

void main()
{
	uv0.xy = position.xy;
	gl_Position.x = position.x * 2.0 - 1.0;
	gl_Position.y = 1.0 - position.y * 2.0;
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
}



## ----- Fragment -----------------------------------------------------------------

in vec2 uv0;
out vec4 FragColor;

uniform isamplerBuffer IndexTexture;
uniform isamplerBuffer PatternCacheTexture;
uniform sampler2D PaletteTexture;
uniform ivec4 PlayfieldSize;
uniform int HighlightPrio;		// 0 or 1

void main()
{
	int ix = int(uv0.x * float(PlayfieldSize.x));
	int iy = int(uv0.y * float(PlayfieldSize.y));
	int patternX = ix / 8;
	int patternY = iy / 8;
	int localX = ix % 8;
	int localY = iy % 8;

	int patternIndex = texelFetch(IndexTexture, patternX + patternY * PlayfieldSize.z).x;

	int flipVariation = (patternIndex >> 11) & 0x03;
	int atex = (patternIndex >> 9) & 0x30;

	int patternCacheLookupIndex = localX + localY * 8 + flipVariation * 64 + (patternIndex & 0x07ff) * 256;
	int paletteIndex = atex + texelFetch(PatternCacheTexture, patternCacheLookupIndex).x;

	vec4 color = texture(PaletteTexture, vec2(float(paletteIndex) / 256.0, 0.0));
	if ((patternIndex >> 15) < HighlightPrio)
		color.rgb *= 0.3;

	// Only for debugging
/*
	vec3 bgcolor = vec3(1.0, 1.0, 1.0) * ((((localX + localY) & 0x02) == 0) ? 0.2 : 0.1);
	color.rgb = color.rgb * color.a + bgcolor * (1.0 - color.a);
*/
	color.a = 1.0;

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
