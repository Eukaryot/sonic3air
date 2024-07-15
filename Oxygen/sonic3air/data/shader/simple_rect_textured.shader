
## ----- Shared -------------------------------------------------------------------

#version 130

precision mediump float;
precision mediump int;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
out vec2 uv0;

uniform vec4 Transform;

void main()
{
	uv0.xy = position.xy;
	vec2 pos = vec2(Transform.x + position.x * Transform.z, Transform.y + position.y * Transform.w);
	gl_Position = vec4(pos, 0.0, 1.0);
}



## ----- Fragment -----------------------------------------------------------------

in vec2 uv0;
out vec4 FragColor;

uniform sampler2D Texture;
#ifdef USE_TINT_COLOR
	uniform vec4 TintColor;
	uniform vec4 AddedColor;
#endif

void main()
{
	vec4 color = texture(Texture, uv0);
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
