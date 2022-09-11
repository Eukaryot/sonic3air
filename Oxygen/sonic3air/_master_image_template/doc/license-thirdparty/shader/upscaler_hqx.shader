
// Hq2x shader
//   - Adapted for use in Oxygen Engine
//  Copyright (C) 2018-2021 Eukaryot
//
// This shader is derived from original "hq2x.glsl" from https://github.com/Armada651/hqx-shader/blob/master/glsl
// Used under GNU General Public License v2, see additional license info below.
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the this software.  If not, see <http://www.gnu.org/licenses/>.


/*
* Copyright (C) 2003 Maxim Stepin ( maxst@hiend3d.com )
*
* Copyright (C) 2010 Cameron Zemek ( grom@zeminvaders.net )
*
* Copyright (C) 2014 Jules Blok ( jules@aerix.nl )
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/





## ----- Shared -------------------------------------------------------------------

#version 130

precision mediump float;
precision mediump int;

#ifdef HQ2X
	#define SCALE 2.0
#endif
#ifdef HQ3X
	#define SCALE 3.0
#endif
#ifdef HQ4X
	#define SCALE 4.0
#endif



## ----- Vertex -------------------------------------------------------------------

in vec4 position;
out vec4 vTexCoord[4];

uniform vec2 GameResolution;

void main()
{
	gl_Position.x = position.x * 2.0 - 1.0;
	gl_Position.y = 1.0 - position.y * 2.0;
	gl_Position.z = 0.0;
	gl_Position.w = 1.0;

	vec2 ps = 1.0 / GameResolution;
	float dx = ps.x;
	float dy = ps.y;

	//   +----+----+----+
	//   |    |    |    |
	//   | w1 | w2 | w3 |
	//   +----+----+----+
	//   |    |    |    |
	//   | w4 | w5 | w6 |
	//   +----+----+----+
	//   |    |    |    |
	//   | w7 | w8 | w9 |
	//   +----+----+----+

	vTexCoord[0].zw = ps;
	vTexCoord[0].xy = position.xy;
	vTexCoord[1] = position.xxxy + vec4(-dx, 0, dx, -dy); //  w1 | w2 | w3
	vTexCoord[2] = position.xxxy + vec4(-dx, 0, dx, 0);   //  w4 | w5 | w6
	vTexCoord[3] = position.xxxy + vec4(-dx, 0, dx, dy);  //  w7 | w8 | w9
}



## ----- Fragment -----------------------------------------------------------------

uniform sampler2D Texture;
uniform sampler2D LUT;
uniform vec2 GameResolution;

in vec4 vTexCoord[4];
out vec4 FragColor;

const mat3 yuv_matrix = mat3(0.299, -0.169, 0.5, 0.587, -0.331, -0.419, 0.114, 0.5, -0.081);
const vec3 yuv_threshold = vec3(48.0 / 255.0, 7.0 / 255.0, 6.0 / 255.0);
const vec3 yuv_offset = vec3(0, 0.5, 0.5);

bool diff(vec3 yuv1, vec3 yuv2)
{
	bvec3 res = greaterThan(abs((yuv1 + yuv_offset) - (yuv2 + yuv_offset)), yuv_threshold);
	return res.x || res.y || res.z;
}

void main()
{
	vec2 fp = fract(vTexCoord[0].xy * GameResolution);
	vec2 quad = sign(-0.5 + fp);

	float dx = vTexCoord[0].z;
	float dy = vTexCoord[0].w;
	vec3 p1 = texture(Texture, vTexCoord[0].xy).rgb;
	vec3 p2 = texture(Texture, vTexCoord[0].xy + vec2(dx, dy) * quad).rgb;
	vec3 p3 = texture(Texture, vTexCoord[0].xy + vec2(dx, 0) * quad).rgb;
	vec3 p4 = texture(Texture, vTexCoord[0].xy + vec2(0, dy) * quad).rgb;

	vec3 w1 = yuv_matrix * texture(Texture, vTexCoord[1].xw).rgb;
	vec3 w2 = yuv_matrix * texture(Texture, vTexCoord[1].yw).rgb;
	vec3 w3 = yuv_matrix * texture(Texture, vTexCoord[1].zw).rgb;

	vec3 w4 = yuv_matrix * texture(Texture, vTexCoord[2].xw).rgb;
	vec3 w5 = yuv_matrix * p1;
	vec3 w6 = yuv_matrix * texture(Texture, vTexCoord[2].zw).rgb;

	vec3 w7 = yuv_matrix * texture(Texture, vTexCoord[3].xw).rgb;
	vec3 w8 = yuv_matrix * texture(Texture, vTexCoord[3].yw).rgb;
	vec3 w9 = yuv_matrix * texture(Texture, vTexCoord[3].zw).rgb;

	bvec3 pattern[3];
	pattern[0] = bvec3(diff(w5, w1), diff(w5, w2), diff(w5, w3));
	pattern[1] = bvec3(diff(w5, w4), false, diff(w5, w6));
	pattern[2] = bvec3(diff(w5, w7), diff(w5, w8), diff(w5, w9));
	bvec4 cross = bvec4(diff(w4, w2), diff(w2, w6), diff(w8, w4), diff(w6, w8));

	vec2 index;
	index.x = dot(vec3(pattern[0]), vec3(1, 2, 4)) +
			  dot(vec3(pattern[1]), vec3(8, 0, 16)) +
			  dot(vec3(pattern[2]), vec3(32, 64, 128));
	index.y = dot(vec4(cross), vec4(1, 2, 4, 8)) * (SCALE * SCALE) +
			  dot(floor(fp * SCALE), vec2(1, SCALE));

	vec2 step = 1.0 / vec2(256.0, 16.0 * (SCALE * SCALE));
	vec2 offset = step / 2.0;
	vec4 weights = texture(LUT, index * step + offset);
	float sum = dot(weights, vec4(1));
	vec3 res = (p1 * weights.x + p2 * weights.y + p3 * weights.z + p4 * weights.w) / sum;

	FragColor = vec4(res, 1.0);
}



## ----- TECH ---------------------------------------------------------------------

technique Standard
{
	blendfunc = opaque;
	vs = Shared + Vertex;
	fs = Shared + Fragment;
	vertexattrib[0] = position;
}

technique Standard_2x : Standard
{
	define = HQ2X;
}

technique Standard_3x : Standard
{
	define = HQ3X;
}

technique Standard_4x : Standard
{
	define = HQ4X;
}
