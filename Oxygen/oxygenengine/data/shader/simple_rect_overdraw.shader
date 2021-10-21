
## ----- Shared -------------------------------------------------------------------

#version 130

precision mediump float;
precision mediump int;



## ----- Vertex -------------------------------------------------------------------

in vec4 position;
out vec2 uv0;
uniform vec4 Rect;

void main()
{
	vec2 pos = vec2(Rect.x + position.x * Rect.z, Rect.y + position.y * Rect.w);
	uv0.xy = pos.xy;
	gl_Position.x = pos.x * 2.0 - 1.0;
	gl_Position.y = pos.y * 2.0 - 1.0;
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
}



## ----- Fragment -----------------------------------------------------------------

in vec2 uv0;
out vec4 FragColor;

uniform sampler2D Texture;

void main()
{
	vec4 color = texture(Texture, uv0);
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
