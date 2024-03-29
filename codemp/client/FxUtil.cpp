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

#include "client.h"
#include "FxScheduler.h"

vec3_t WHITE = { 1.0f, 1.0f, 1.0f };

struct SEffectList
{
	CEffect* mEffect;
	int mKillTime;
	bool mPortal;
};

constexpr auto PI = 3.14159f;

SEffectList effectList[MAX_EFFECTS];
SEffectList* nextValidEffect;
SFxHelper theFxHelper;

int activeFx = 0;
int drawnFx;
qboolean fxInitialized = qfalse;

//-------------------------
// FX_Free
//
// Frees all FX
//-------------------------
bool FX_Free(const bool templates)
{
	for (auto& i : effectList)
	{
		if (i.mEffect)
		{
			delete i.mEffect;
		}

		i.mEffect = nullptr;
	}

	activeFx = 0;

	theFxScheduler.Clean(templates);
	return true;
}

//-------------------------
// FX_Stop
//
// Frees all active FX but leaves the templates
//-------------------------
void FX_Stop()
{
	for (auto& i : effectList)
	{
		if (i.mEffect)
		{
			delete i.mEffect;
		}

		i.mEffect = nullptr;
	}

	activeFx = 0;

	theFxScheduler.Clean(false);
}

//-------------------------
// FX_Init
//
// Preps system for use
//-------------------------
int FX_Init(refdef_t* refdef)
{
	//	FX_Free( true );
	if (fxInitialized == qfalse)
	{
		fxInitialized = qtrue;

		for (auto& i : effectList)
		{
			i.mEffect = nullptr;
		}
	}
	nextValidEffect = &effectList[0];

#ifdef _DEBUG
	fx_freeze = Cvar_Get("fx_freeze", "0", CVAR_CHEAT);
#endif
	fx_debug = Cvar_Get("fx_debug", "0", CVAR_TEMP);
	fx_countScale = Cvar_Get("fx_countScale", "1", CVAR_ARCHIVE_ND);
	fx_nearCull = Cvar_Get("fx_nearCull", "16", CVAR_ARCHIVE_ND);
	fx_optimizedParticles = Cvar_Get("fx_optimizedParticles", "0", CVAR_ARCHIVE);

	theFxHelper.ReInit(refdef);

	return true;
}

void FX_SetRefDef(refdef_t* refdef)
{
	theFxHelper.refdef = refdef;
}

//-------------------------
// FX_FreeMember
//-------------------------
static void FX_FreeMember(SEffectList* obj)
{
	obj->mEffect->Die();
	delete obj->mEffect;
	obj->mEffect = nullptr;

	// May as well mark this to be used next
	nextValidEffect = obj;

	activeFx--;
}

//-------------------------
// FX_GetValidEffect
//
// Finds an unused effect slot
//
// Note - in the editor, this function may return NULL, indicating that all
// effects are being stopped.
//-------------------------
static SEffectList* FX_GetValidEffect()
{
	if (nextValidEffect->mEffect == nullptr)
	{
		return nextValidEffect;
	}

	int i;
	SEffectList* ef;

	// Blah..plow through the list till we find something that is currently untainted
	for (i = 0, ef = effectList; i < MAX_EFFECTS; i++, ef++)
	{
		if (ef->mEffect == nullptr)
		{
			return ef;
		}
	}

	// report the error.
#ifndef FINAL_BUILD
	theFxHelper.Print("FX system out of effects\n");
#endif

	// Hmmm.. just trashing the first effect in the list is a poor approach
	FX_FreeMember(&effectList[0]);

	// Recursive call
	return nextValidEffect;
}

//-------------------------
// FX_Add
//
// Adds all fx to the view
//-------------------------
void FX_Add(const bool portal)
{
	int i;
	SEffectList* ef;

	drawnFx = 0;

	int num_fx = activeFx; //but stop when there can't be any more left!
	for (i = 0, ef = effectList; i < MAX_EFFECTS && num_fx; i++, ef++)
	{
		if (ef->mEffect != nullptr)
		{
			--num_fx;
			if (portal != ef->mPortal)
			{
				continue; //this one does not render in this scene
			}
			// Effect is active
			if (theFxHelper.mTime > ef->mKillTime)
			{
				// Clean up old effects, calling any death effects as needed
				// this flag just has to be cleared otherwise death effects might not happen correctly
				ef->mEffect->ClearFlags(FX_KILL_ON_IMPACT);
				FX_FreeMember(ef);
			}
			else
			{
				if (ef->mEffect->Update() == false)
				{
					// We've been marked for death
					FX_FreeMember(ef);
				}
			}
		}
	}

	if (fx_debug->integer && !portal)
	{
		theFxHelper.Print("Active    FX: %i\n", activeFx);
		theFxHelper.Print("Drawn     FX: %i\n", drawnFx);
		theFxHelper.Print("Scheduled FX: %i High: %i\n", theFxScheduler.NumScheduledFx(),
			theFxScheduler.GetHighWatermark());
	}
}

//-------------------------
// FX_AddPrimitive
//
// Note - in the editor, this function may change *pEffect to NULL, indicating that
// all effects are being stopped.
//-------------------------
extern bool gEffectsInPortal; //from FXScheduler.cpp so i don't have to pass it in on EVERY FX_ADD*
void FX_AddPrimitive(CEffect** p_effect, const int kill_time)
{
	SEffectList* item = FX_GetValidEffect();

	item->mEffect = *p_effect;
	item->mKillTime = theFxHelper.mTime + kill_time;
	item->mPortal = gEffectsInPortal; //global set in AddScheduledEffects

	activeFx++;

	// Stash these in the primitive so it has easy access to the vals
	(*p_effect)->SetTimeStart(theFxHelper.mTime);
	(*p_effect)->SetTimeEnd(theFxHelper.mTime + kill_time);
}

//-------------------------
//  FX_AddParticle
//-------------------------
CParticle* FX_AddParticle(vec3_t org, vec3_t vel, vec3_t accel, const float size1, const float size2,
	const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, const float rgb_parm,
	const float rotation, const float rotation_delta,
	vec3_t min, vec3_t max, const float elasticity,
	const int death_id, const int impact_id,
	const int kill_time, const qhandle_t shader, const int flags = 0,
	const EMatImpactEffect mat_impact_fx, const int fx_parm,
	CGhoul2Info_v* ghoul2, const int entNum, const int model_num,
	const int bolt_num)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding effects when the system is paused
		return nullptr;
	}

	auto fx = new CParticle;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(org);
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(org);
		}
		fx->SetOrigin1(org);
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);
		fx->SetVel(vel);
		fx->SetAccel(accel);

		// RGB----------------
		fx->SetRGBStart(s_rgb);
		fx->SetRGBEnd(e_rgb);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetFlags(flags);
		fx->SetShader(shader);
		fx->SetRotation(rotation);
		fx->SetRotationDelta(rotation_delta);
		fx->SetElasticity(elasticity);
		fx->SetMin(min);
		fx->SetMax(max);
		fx->SetDeathFxID(death_id);
		fx->SetImpactFxID(impact_id);

		fx->Init();

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddLine
//-------------------------
CLine* FX_AddLine(vec3_t start, vec3_t end, const float size1, const float size2, const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, const float rgb_parm,
	const int kill_time, const qhandle_t shader, const int flags = 0,
	const EMatImpactEffect mat_impact_fx, const int fx_parm,
	CGhoul2Info_v* ghoul2, const int entNum, const int model_num, const int bolt_num)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding new effects when the system is paused
		return nullptr;
	}

	auto fx = new CLine;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(start); //offset from bolt pos
			fx->SetVel(end); //vel is the vector offset from bolt+orgOffset
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(start);
			fx->SetOrigin2(end);
		}
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);

		// RGB----------------
		fx->SetRGBStart(s_rgb);
		fx->SetRGBEnd(e_rgb);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetShader(shader);
		fx->SetFlags(flags);

		fx->SetSTScale(1.0f, 1.0f);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddElectricity
//-------------------------
CElectricity* FX_AddElectricity(vec3_t start, vec3_t end, const float size1, const float size2, const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, const float rgb_parm,
	const float chaos, const int kill_time, const qhandle_t shader, const int flags = 0,
	const EMatImpactEffect mat_impact_fx /*MATIMPACTFX_NONE*/, const int fx_parm /*-1*/,
	CGhoul2Info_v* ghoul2/*0*/, const int entNum/*-1*/, const int model_num/*-1*/,
	const int bolt_num/*-1*/)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding new effects when the system is paused
		return nullptr;
	}

	auto fx = new CElectricity;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(start); //offset
			fx->SetVel(end); //vel is the vector offset from bolt+orgOffset
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(start);
			fx->SetOrigin2(end);
		}
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);

		// RGB----------------
		fx->SetRGBStart(s_rgb);
		fx->SetRGBEnd(e_rgb);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetShader(shader);
		fx->SetFlags(flags);
		fx->SetChaos(chaos);

		fx->SetSTScale(1.0f, 1.0f);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
		// in the editor, fx may now be NULL?
		if (fx)
		{
			fx->Initialize();
		}
	}

	return fx;
}

//-------------------------
//  FX_AddTail
//-------------------------
CTail* FX_AddTail(vec3_t org, vec3_t vel, vec3_t accel,
	const float size1, const float size2, const float size_parm,
	const float length1, const float length2, const float length_parm,
	const float alpha1, const float alpha2, const float alphaParm,
	vec3_t s_rgb, vec3_t e_rgb, const float rgb_parm,
	vec3_t min, vec3_t max, const float elasticity,
	const int death_id, const int impact_id,
	const int kill_time, const qhandle_t shader, const int flags = 0,
	const EMatImpactEffect mat_impact_fx /*MATIMPACTFX_NONE*/, const int fx_parm /*-1*/,
	CGhoul2Info_v* ghoul2/*0*/, const int entNum/*-1*/, const int model_num/*-1*/, const int bolt_num/*-1*/)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding effects when the system is paused
		return nullptr;
	}

	auto fx = new CTail;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(org);
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(org);
		}
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);
		fx->SetVel(vel);
		fx->SetAccel(accel);

		// RGB----------------
		fx->SetRGBStart(s_rgb);
		fx->SetRGBEnd(e_rgb);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alphaParm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alphaParm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Length----------------
		fx->SetLengthStart(length1);
		fx->SetLengthEnd(length2);

		if ((flags & FX_LENGTH_PARM_MASK) == FX_LENGTH_WAVE)
		{
			fx->SetLengthParm(length_parm * PI * 0.001f);
		}
		else if (flags & FX_LENGTH_PARM_MASK)
		{
			fx->SetLengthParm(length_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetFlags(flags);
		fx->SetShader(shader);
		fx->SetElasticity(elasticity);
		fx->SetMin(min);
		fx->SetMax(max);
		fx->SetSTScale(1.0f, 1.0f);
		fx->SetDeathFxID(death_id);
		fx->SetImpactFxID(impact_id);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddCylinder
//-------------------------
CCylinder* FX_AddCylinder(vec3_t start, vec3_t normal,
	const float size1_s, const float size1_e, const float size1_parm,
	const float size2_s, const float size2_e, const float size2_parm,
	const float length1, const float length2, const float length_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, const float rgb_parm,
	const int kill_time, const qhandle_t shader, const int flags,
	const EMatImpactEffect mat_impact_fx /*MATIMPACTFX_NONE*/, const int fx_parm /*-1*/,
	CGhoul2Info_v* ghoul2/*0*/, const int entNum/*-1*/, const int model_num/*-1*/,
	const int bolt_num/*-1*/,
	const qboolean trace_end)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding new effects when the system is paused
		return nullptr;
	}

	auto fx = new CCylinder;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(start); //offset
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(start);
		}
		fx->SetTraceEnd(trace_end);

		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);
		fx->SetOrigin1(start);
		fx->SetNormal(normal);

		// RGB----------------
		fx->SetRGBStart(rgb1);
		fx->SetRGBEnd(rgb2);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size1----------------
		fx->SetSizeStart(size1_s);
		fx->SetSizeEnd(size1_e);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size1_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size1_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size2----------------
		fx->SetSize2Start(size2_s);
		fx->SetSize2End(size2_e);

		if ((flags & FX_SIZE2_PARM_MASK) == FX_SIZE2_WAVE)
		{
			fx->SetSize2Parm(size2_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE2_PARM_MASK)
		{
			fx->SetSize2Parm(size2_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Length1---------------
		fx->SetLengthStart(length1);
		fx->SetLengthEnd(length2);

		if ((flags & FX_LENGTH_PARM_MASK) == FX_LENGTH_WAVE)
		{
			fx->SetLengthParm(length_parm * PI * 0.001f);
		}
		else if (flags & FX_LENGTH_PARM_MASK)
		{
			fx->SetLengthParm(length_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetShader(shader);
		fx->SetFlags(flags);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddEmitter
//-------------------------
CEmitter* FX_AddEmitter(vec3_t org, vec3_t vel, vec3_t accel,
	const float size1, const float size2, const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, const float rgb_parm,
	vec3_t angs, vec3_t delta_angs,
	vec3_t min, vec3_t max, const float elasticity,
	const int death_id, const int impact_id, const int emitter_id,
	const float density, const float variance,
	const int kill_time, const qhandle_t model, const int flags = 0,
	const EMatImpactEffect mat_impact_fx, const int fx_parm,
	const CGhoul2Info_v* ghoul2)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding effects when the system is paused
		return nullptr;
	}

	auto fx = new CEmitter;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			assert(0); //not done
			//			fx->SetBoltinfo( ghoul2, entNum, modelNum, boltNum );
		}
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);
		fx->SetOrigin1(org);
		fx->SetVel(vel);
		fx->SetAccel(accel);

		// RGB----------------
		fx->SetRGBStart(rgb1);
		fx->SetRGBEnd(rgb2);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetAngles(angs);
		fx->SetAngleDelta(delta_angs);
		fx->SetFlags(flags);
		fx->SetModel(model);
		fx->SetElasticity(elasticity);
		fx->SetMin(min);
		fx->SetMax(max);
		fx->SetDeathFxID(death_id);
		fx->SetImpactFxID(impact_id);
		fx->SetEmitterFxID(emitter_id);
		fx->SetDensity(density);
		fx->SetVariance(variance);
		fx->SetOldTime(theFxHelper.mTime);

		fx->SetLastOrg(org);
		fx->SetLastVel(vel);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddLight
//-------------------------
CLight* FX_AddLight(vec3_t org, const float size1, const float size2, const float size_parm,
	vec3_t rgb1, vec3_t rgb2, const float rgb_parm,
	const int kill_time, const int flags = 0,
	const EMatImpactEffect mat_impact_fx /*MATIMPACTFX_NONE*/, const int fx_parm /*-1*/,
	CGhoul2Info_v* ghoul2/*0*/, const int entNum/*-1*/, const int model_num/*-1*/,
	const int bolt_num/*-1*/)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding effects when the system is paused
		return nullptr;
	}

	auto fx = new CLight;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(org); //offset
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(org);
		}
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);

		// RGB----------------
		fx->SetRGBStart(rgb1);
		fx->SetRGBEnd(rgb2);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetFlags(flags);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddOrientedParticle
//-------------------------
COrientedParticle* FX_AddOrientedParticle(vec3_t org, vec3_t norm, vec3_t vel, vec3_t accel,
	const float size1, const float size2, const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, const float rgb_parm,
	const float rotation, const float rotation_delta,
	vec3_t min, vec3_t max, const float bounce,
	const int death_id, const int impact_id,
	const int kill_time, const qhandle_t shader, const int flags = 0,
	const EMatImpactEffect mat_impact_fx /*MATIMPACTFX_NONE*/,
	const int fx_parm /*-1*/,
	CGhoul2Info_v* ghoul2/*0*/, const int entNum/*-1*/, const int model_num/*-1*/,
	const int bolt_num/*-1*/)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding effects when the system is paused
		return nullptr;
	}

	auto fx = new COrientedParticle;

	if (fx)
	{
		if (flags & FX_RELATIVE && ghoul2 != nullptr)
		{
			fx->SetOrigin1(nullptr);
			fx->SetOrgOffset(org); //offset
			fx->SetBoltinfo(ghoul2, entNum, model_num, bolt_num);
		}
		else
		{
			fx->SetOrigin1(org);
		}
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);
		fx->SetOrigin1(org);
		fx->SetNormal(norm);
		fx->SetVel(vel);
		fx->SetAccel(accel);

		// RGB----------------
		fx->SetRGBStart(rgb1);
		fx->SetRGBEnd(rgb2);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetFlags(flags);
		fx->SetShader(shader);
		fx->SetRotation(rotation);
		fx->SetRotationDelta(rotation_delta);
		fx->SetElasticity(bounce);
		fx->SetMin(min);
		fx->SetMax(max);
		fx->SetDeathFxID(death_id);
		fx->SetImpactFxID(impact_id);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddPoly
//-------------------------
CPoly* FX_AddPoly(const vec3_t* verts, const vec2_t* st, const int numVerts,
	vec3_t vel, vec3_t accel,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t rgb1, vec3_t rgb2, const float rgb_parm,
	vec3_t rotation_delta, const float bounce, const int motion_delay,
	const int kill_time, const qhandle_t shader, const int flags)
{
	if (theFxHelper.mFrameTime < 1 || !verts)
	{
		// disallow adding effects when the system is paused or the user doesn't pass in a vert array
		return nullptr;
	}

	auto fx = new CPoly;

	if (fx)
	{
		// Do a cheesy copy of the verts and texture coords into our own structure
		for (int i = 0; i < numVerts; i++)
		{
			VectorCopy(verts[i], fx->mOrg[i]);
			VectorCopy2(st[i], fx->mST[i]);
		}

		fx->SetVel(vel);
		fx->SetAccel(accel);

		// RGB----------------
		fx->SetRGBStart(rgb1);
		fx->SetRGBEnd(rgb2);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetFlags(flags);
		fx->SetShader(shader);
		fx->SetRot(rotation_delta);
		fx->SetElasticity(bounce);
		fx->SetMotionTimeStamp(motion_delay);
		fx->SetNumVerts(numVerts);

		// Now that we've set our data up, let's process it into a useful format
		fx->PolyInit();

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddFlash
//-------------------------
CFlash* FX_AddFlash(vec3_t origin,
	const float size1, const float size2, const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, const float rgb_parm,
	const int kill_time, const qhandle_t shader, const int flags,
	const EMatImpactEffect mat_impact_fx /*MATIMPACTFX_NONE*/, const int fx_parm /*-1*/)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding new effects when the system is paused
		return nullptr;
	}

	if (!shader)
	{
		//yeah..this is bad, I guess, but SP seems to handle it by not drawing the flash, so I will too.
		//assert(shader);
		return nullptr;
	}

	auto fx = new CFlash;

	if (fx)
	{
		fx->SetMatImpactFX(mat_impact_fx);
		fx->SetMatImpactParm(fx_parm);
		fx->SetOrigin1(origin);

		// RGB----------------
		fx->SetRGBStart(s_rgb);
		fx->SetRGBEnd(e_rgb);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetShader(shader);
		fx->SetFlags(flags);

		//		fx->SetSTScale( 1.0f, 1.0f );

		fx->Init();

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}

//-------------------------
//  FX_AddBezier
//-------------------------
CBezier* FX_AddBezier(vec3_t start, vec3_t end,
	vec3_t control1, vec3_t control1_vel,
	vec3_t control2, vec3_t control2_vel,
	const float size1, const float size2, const float size_parm,
	const float alpha1, const float alpha2, const float alpha_parm,
	vec3_t s_rgb, vec3_t e_rgb, const float rgb_parm,
	const int kill_time, const qhandle_t shader, const int flags)
{
	if (theFxHelper.mFrameTime < 1)
	{
		// disallow adding new effects when the system is paused
		return nullptr;
	}

	auto fx = new CBezier;

	if (fx)
	{
		fx->SetOrigin1(start);
		fx->SetOrigin2(end);

		fx->SetControlPoints(control1, control2);
		fx->SetControlVel(control1_vel, control2_vel);

		// RGB----------------
		fx->SetRGBStart(s_rgb);
		fx->SetRGBEnd(e_rgb);

		if ((flags & FX_RGB_PARM_MASK) == FX_RGB_WAVE)
		{
			fx->SetRGBParm(rgb_parm * PI * 0.001f);
		}
		else if (flags & FX_RGB_PARM_MASK)
		{
			// rgbParm should be a value from 0-100..
			fx->SetRGBParm(rgb_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Alpha----------------
		fx->SetAlphaStart(alpha1);
		fx->SetAlphaEnd(alpha2);

		if ((flags & FX_ALPHA_PARM_MASK) == FX_ALPHA_WAVE)
		{
			fx->SetAlphaParm(alpha_parm * PI * 0.001f);
		}
		else if (flags & FX_ALPHA_PARM_MASK)
		{
			fx->SetAlphaParm(alpha_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		// Size----------------
		fx->SetSizeStart(size1);
		fx->SetSizeEnd(size2);

		if ((flags & FX_SIZE_PARM_MASK) == FX_SIZE_WAVE)
		{
			fx->SetSizeParm(size_parm * PI * 0.001f);
		}
		else if (flags & FX_SIZE_PARM_MASK)
		{
			fx->SetSizeParm(size_parm * 0.01f * kill_time + theFxHelper.mTime);
		}

		fx->SetShader(shader);
		fx->SetFlags(flags);

		fx->SetSTScale(1.0f, 1.0f);

		FX_AddPrimitive(reinterpret_cast<CEffect**>(&fx), kill_time);
	}

	return fx;
}