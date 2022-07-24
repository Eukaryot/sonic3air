
## ----- Shared -------------------------------------------------------------------

#version 130

precision mediump float;
precision mediump int;

uniform ivec2 Size;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
out vec2 uv0;

uniform ivec3 Position;		// With z = priority flag (0 or 1)
uniform ivec2 PivotOffset;
uniform vec4 Transformation;
uniform ivec2 GameResolution;

void main()
{
	// Calculate local offset
	vec2 LocalOffset;
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

	uv0 = position.xy;
}



## ----- Fragment -----------------------------------------------------------------

in vec2 uv0;
out vec4 FragColor;

uniform sampler2D SpriteTexture;
uniform vec4 TintColor;
uniform vec4 AddedColor;

void main()
{
	vec4 color = texture(SpriteTexture, uv0.xy);
	color = color * TintColor + AddedColor;
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

technique Standard_AlphaTest : Standard
{
	blendfunc = alpha;
	define = ALPHA_TEST;
}
