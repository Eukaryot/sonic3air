
## ----- Shared -------------------------------------------------------------------

#version 130

precision mediump float;
precision mediump int;



## ----- Vertex -------------------------------------------------------------------

in vec2 position;
in vec4 color;
out vec4 interpolatedColor;

uniform vec4 Transform;

void main()
{
	vec2 pos = vec2(Transform.x + position.x * Transform.z, Transform.y + position.y * Transform.w);

	// Intentionally using a z-value of 0.5
	gl_Position = vec4(pos, 0.5, 1.0);
	interpolatedColor = color;
}



## ----- Fragment -----------------------------------------------------------------

in vec4 interpolatedColor;
out vec4 FragColor;

void main()
{
	FragColor = interpolatedColor;
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	blendfunc = alpha;
	vs = Shared + Vertex;
	fs = Shared + Fragment;
	vertexattrib[0] = position;
}
