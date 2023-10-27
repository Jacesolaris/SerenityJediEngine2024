/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"

/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish(void)
{
	if (r_shadows->integer != 2) {
		return;
	}
	if (glConfig.stencilBits < 4) {
		return;
	}

	GL_Cull(CT_TWO_SIDED);

	GL_BindToTMU(tr.whiteImage, TB_COLORMAP);

	GL_State(GLS_STENCILTEST_ENABLE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);

	qglStencilFunc(GL_NOTEQUAL, 0, 0xff);
	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	matrix_t projection;
	Matrix16Ortho(0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, projection);

	GL_Cull(CT_TWO_SIDED);
	GLSL_BindProgram(&tr.textureColorShader);
	vec4_t color;
	VectorSet4(color, 0.6f, 0.6f, 0.6f, 1.0f);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, color);
	GLSL_SetUniformMatrix4x4(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, projection);

	vec4i_t dstBox;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	VectorSet4(dstBox, 0, glConfig.vidHeight, glConfig.vidWidth, 0);

	VectorSet4(quadVerts[0], dstBox[0], dstBox[1], 0, 1);
	VectorSet4(quadVerts[1], dstBox[2], dstBox[1], 0, 1);
	VectorSet4(quadVerts[2], dstBox[2], dstBox[3], 0, 1);
	VectorSet4(quadVerts[3], dstBox[0], dstBox[3], 0, 1);

	RB_InstantQuad2(quadVerts, texCoords);

	qglStencilFunc(GL_ALWAYS, 0, 0xff);
}

/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform()
{
	vec3_t	ground{};
	vec3_t	light{};
	vec3_t	light_dir;

	auto xyz = reinterpret_cast<float*>(tess.xyz);

	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	const float ground_dist = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy(backEnd.currentEntity->lightDir, light_dir);
	float d = DotProduct(light_dir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5) {
		VectorMA(light_dir, 0.5 - d, ground, light_dir);
		d = DotProduct(light_dir, ground);
	}
	d = 1.0 / d;

	light[0] = light_dir[0] * d;
	light[1] = light_dir[1] * d;
	light[2] = light_dir[2] * d;

	for (int i = 0; i < tess.num_vertexes; i++, xyz += 4)
	{
		const float h = DotProduct(xyz, ground) + ground_dist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}