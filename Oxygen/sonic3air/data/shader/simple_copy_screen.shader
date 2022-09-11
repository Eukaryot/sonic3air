
## ----- Shared -------------------------------------------------------------------

#version 130

precision mediump float;
precision mediump int;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
out vec2 uv0;

void main()
{
	uv0.xy = position.xy;
	gl_Position.x = position.x * 2.0 - 1.0;
	gl_Position.y = position.y * 2.0 - 1.0;
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;
}



## ----- Fragment -----------------------------------------------------------------

in vec2 uv0;
out vec4 FragColor;

uniform sampler2D Texture;

void main()
{
	FragColor = texture(Texture, uv0);
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	blendfunc = opaque;
	vs = Shared + Vertex;
	fs = Shared + Fragment;
	vertexattrib[0] = position;
}
