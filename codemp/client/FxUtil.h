/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2024 contributors

This file is part of the SerenityJediEngine2024 source code.

SerenityJediEngine2024 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

#include "FxPrimitives.h"

bool FX_Free(bool templates); // ditches all active effects;
int FX_Init(refdef_t* refdef); // called in CG_Init to purge the fx system.
void FX_SetRefDef(refdef_t* refdef);
void FX_Add(bool portal); // called every cgame frame to add all fx into the scene.
void FX_Stop(); // ditches all active effects without touching the templates.

CParticle* FX_AddParticle(vec3_t org, vec3_t vel, vec3_t accel,
	float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, float rgb_parm,
	float rotation, float rotation_delta,
	vec3_t min, vec3_t max, float elasticity,
	int death_id, int impact_id,
	int kill_time, qhandle_t shader, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1, int bolt_num = -1);

CLine* FX_AddLine(vec3_t start, vec3_t end,
	float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, float rgb_parm,
	int kill_time, qhandle_t shader, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1, int bolt_num = -1);

CElectricity* FX_AddElectricity(vec3_t start, vec3_t end, float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, float rgb_parm,
	float chaos, int kill_time, qhandle_t shader, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1, int bolt_num = -1);

CTail* FX_AddTail(vec3_t org, vec3_t vel, vec3_t accel,
	float size1, float size2, float size_parm,
	float length1, float length2, float length_parm,
	float alpha1, float alpha2, float alphaParm,
	vec3_t s_rgb, vec3_t e_rgb, float rgb_parm,
	vec3_t min, vec3_t max, float elasticity,
	int death_id, int impact_id,
	int kill_time, qhandle_t shader, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1, int bolt_num = -1);

CCylinder* FX_AddCylinder(vec3_t start, vec3_t normal,
	float size1_s, float size1_e, float size1_parm,
	float size2_s, float size2_e, float size2_parm,
	float length1, float length2, float length_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, float rgb_parm,
	int kill_time, qhandle_t shader, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1, int bolt_num = -1,
	qboolean trace_end = qfalse);

CEmitter* FX_AddEmitter(vec3_t org, vec3_t vel, vec3_t accel,
	float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, float rgb_parm,
	vec3_t angs, vec3_t delta_angs,
	vec3_t min, vec3_t max, float elasticity,
	int death_id, int impact_id, int emitter_id,
	float density, float variance,
	int kill_time, qhandle_t model, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	const CGhoul2Info_v* ghoul2 = nullptr);

CLight* FX_AddLight(vec3_t org, float size1, float size2, float size_parm,
	vec3_t rgb1, vec3_t rgb2, float rgb_parm,
	int kill_time, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1, int bolt_num = -1);

COrientedParticle* FX_AddOrientedParticle(vec3_t org, vec3_t norm, vec3_t vel, vec3_t accel,
	float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, float rgb_parm,
	float rotation, float rotation_delta,
	vec3_t min, vec3_t max, float bounce,
	int death_id, int impact_id,
	int kill_time, qhandle_t shader, int flags,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1,
	CGhoul2Info_v* ghoul2 = nullptr, int entNum = -1, int model_num = -1,
	int bolt_num = -1);

CPoly* FX_AddPoly(const vec3_t* verts, const vec2_t* st, int numVerts,
	vec3_t vel, vec3_t accel,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, float rgb_parm,
	vec3_t rotation_delta, float bounce, int motion_delay,
	int kill_time, qhandle_t shader, int flags);

CFlash* FX_AddFlash(vec3_t origin,
	float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, float rgb_parm,
	int kill_time, qhandle_t shader, int flags = 0,
	EMatImpactEffect mat_impact_fx = MATIMPACTFX_NONE, int fx_parm = -1);

CBezier* FX_AddBezier(vec3_t start, vec3_t end,
	vec3_t control1, vec3_t control1_vel,
	vec3_t control2, vec3_t control2_vel,
	float size1, float size2, float size_parm,
	float alpha1, float alpha2, float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, float rgb_parm,
	int kill_time, qhandle_t shader, int flags = 0);
