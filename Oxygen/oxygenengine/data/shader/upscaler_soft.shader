
## ----- Shared -------------------------------------------------------------------

#version 130

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

uniform sampler2D Texture;
uniform vec2 GameResolution;
uniform float PixelFactor;
#ifdef USE_SCANLINES
	uniform float ScanlinesIntensity;
#endif

void main()
{
	vec2 uv;

	float x = uv0.x * GameResolution.x;
	float ix = floor(x + 0.5);
	float fx = x - ix;
	fx = clamp(fx * PixelFactor, -0.5, 0.5);
	uv.x = (ix + fx) / GameResolution.x;

	float y = uv0.y * GameResolution.y;
	float iy = floor(y + 0.5);
	float fy = y - iy;
#ifdef USE_SCANLINES
	float colorMultiplier = 1.0 - (0.5 - abs(fy)) * ScanlinesIntensity;
#endif

	fy = clamp(fy * PixelFactor, -0.5, 0.5);
	uv.y = (iy + fy) / GameResolution.y;

	vec4 color = texture(Texture, uv);
#ifdef USE_SCANLINES
	color.rgb *= colorMultiplier;
#endif
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

technique Scanlines : Standard
{
	define = USE_SCANLINES;
}
