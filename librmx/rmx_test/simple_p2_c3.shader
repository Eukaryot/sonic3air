

## ----- Shared -------------------------------------------------------------------

#version 130

precision highp float;



## ----- Vertex -------------------------------------------------------------------

in vec2 in_Position;
in vec3 in_Color;

out vec3 ex_Color;

void main(void)
{
    gl_Position = vec4(in_Position, 0.0, 1.0);
    ex_Color = in_Color;
}



## ----- Fragment -----------------------------------------------------------------

in  vec3 ex_Color;
out vec4 gl_FragColor;

void main(void)
{
    gl_FragColor = vec4(ex_Color, 1.0);
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	blendfunc = opaque;
	vs = Shared + Vertex;
	fs = Shared + Fragment;
}
