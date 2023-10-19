/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
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

// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_headers.h"

#include "cg_media.h"
#include "../game/g_functions.h"
#include "../ghoul2/G2.h"
#include "FxScheduler.h"
#include "../game/wp_saber.h"
#include "../game/g_vehicles.h"

extern void CG_AddSaberBlade(const centity_t* cent, centity_t* scent, int renderfx, int model_index,
	vec3_t origin, vec3_t angles);
extern void CG_CheckSaberInWater(const centity_t* cent, const centity_t* scent, int saber_num, int model_index,
	vec3_t origin,
	vec3_t angles);
extern void CG_ForcePushBlur(const vec3_t org, qboolean dark_side = qfalse);
extern void CG_AddForceSightShell(refEntity_t* ent, const centity_t* cent);
extern qboolean CG_PlayerCanSeeCent(const centity_t* cent);

#define	FX_ALPHA_LINEAR		0x00000001
#define	FX_SIZE_LINEAR		0x00000100

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag(refEntity_t* entity, const refEntity_t* parent,
	const qhandle_t parent_model, const char* tag_name)
{
	orientation_t lerped;

	// lerp the tag
	cgi_R_LerpTag(&lerped, parent_model, parent->oldframe, parent->frame,
		1.0f - parent->backlerp, tag_name);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (int i = 0; i < 3; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(lerped.axis, const_cast<refEntity_t*>(parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag(refEntity_t* entity, const refEntity_t* parent,
	const qhandle_t parent_model, const char* tag_name, orientation_t* tag_orient)
{
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

	// lerp the tag
	cgi_R_LerpTag(&lerped, parent_model, parent->oldframe, parent->frame,
		1.0f - parent->backlerp, tag_name);

	if (tag_orient)
	{
		VectorCopy(lerped.origin, tag_orient->origin);
		for (i = 0; i < 3; i++)
		{
			VectorCopy(lerped.axis[i], tag_orient->axis[i]);
		}
	}

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0; i < 3; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, const_cast<refEntity_t*>(parent)->axis, entity->axis);
}

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
vec3_t* CG_SetEntitySoundPosition(const centity_t* cent)
{
	static vec3_t v3_return;
	if (cent->currentState.solid == SOLID_BMODEL)
	{
		vec3_t origin;

		const float* v = cgs.inlineModelMidpoints[cent->currentState.modelindex];
		VectorAdd(cent->lerpOrigin, v, origin);
		cgi_S_UpdateEntityPosition(cent->currentState.number, origin);
		VectorCopy(origin, v3_return);
	}
	else
	{
		if (cent->currentState.eType == ET_PLAYER
			&& cent->gent
			&& cent->gent->client
			&& cent->gent->ghoul2.IsValid()
			&& cent->gent->ghoul2[0].animModelIndexOffset) //If it has an animOffset it's a cinematic anim
		{
			//I might be running out of my bounding box, so use my headPoint from the last render frame...?
			//NOTE: if I'm not rendered, will this not update correctly?  Would cent->lerpOrigin be any more updated?
			VectorCopy(cent->gent->client->renderInfo.eyePoint, v3_return);
		}
		else
		{
			//just use my org
			VectorCopy(cent->lerpOrigin, v3_return);
		}
		cgi_S_UpdateEntityPosition(cent->currentState.number, v3_return);
	}

	return &v3_return;
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects(const centity_t* cent)
{
	// update sound origins
	vec3_t v3_origin;
	VectorCopy(*CG_SetEntitySoundPosition(cent), v3_origin);

	// add loop sound
	if (cent->currentState.loopSound)
	{
		soundChannel_t chan = CHAN_AUTO;

		const gentity_t* ent = cent->gent;

		if (ent->s.eFlags & EF_LESS_ATTEN)
		{
			chan = CHAN_LESS_ATTEN;
		}

		const sfxHandle_t sfx = cent->currentState.eType == ET_MOVER
			? cent->currentState.loopSound
			: cgs.sound_precache[cent->currentState.loopSound];

		// Only play sound if being drawn.
		if (!(ent->s.eFlags & EF_NODRAW))
		{
			cgi_S_AddLoopingSound(cent->currentState.number, v3_origin/*cent->lerpOrigin*/, vec3_origin, sfx, chan);
		}
	}

	// constant light glow
	if (cent->currentState.constantLight
		&& cent->currentState.eType != ET_GENERAL
		&& cent->currentState.eType != ET_PLAYER
		&& cent->currentState.eType != ET_ITEM
		&& cent->currentState.eType != ET_MISSILE
		&& cent->currentState.eType != ET_MOVER
		&& cent->currentState.eType != ET_BEAM
		&& cent->currentState.eType != ET_TERRAIN)
	{
		const int cl = cent->currentState.constantLight;
		const float r = static_cast<float>(cl & 0xFF) / 255.0;
		const float g = static_cast<float>(cl >> 8 & 0xFF) / 255.0;
		const float b = static_cast<float>(cl >> 16 & 0xFF) / 255.0;
		const float i = static_cast<float>(cl >> 24 & 0xFF) * 4.0;
		cgi_R_AddLightToScene(cent->lerpOrigin, i, r, g, b);
	}
}

void CG_AddRefEntWithTransportEffect(const centity_t* cent, refEntity_t* ent)
{
	// We are a normal thing....
	cgi_R_AddRefEntityToScene(ent);

	if (ent->renderfx & RF_PULSATE && cent->gent->owner && cent->gent->owner->health &&
		!cent->gent->owner->s.number && cent->gent->owner->client && //only for player
		cent->gent->owner->client->ps.saberEntityState == SES_RETURNING &&
		cent->currentState.saberActive == qfalse)
	{
		ent->customShader = cgi_R_RegisterShader("gfx/effects/solidWhite_cull");
		ent->renderfx = RF_RGB_TINT;
		const float wv = sin(cg.time * 0.003f) * 0.08f + 0.1f;
		ent->shaderRGBA[0] = wv * 255;
		ent->shaderRGBA[1] = wv * 255;
		ent->shaderRGBA[2] = wv * 0;
		cgi_R_AddRefEntityToScene(ent);

		for (int i = -4; i < 10; i += 1)
		{
			vec3_t org;
			VectorMA(ent->origin, -i, ent->axis[2], org);

			FX_AddSprite(org, nullptr, nullptr, 5.5f, wv, wv, 0.0f, 0.0f, 1.0f, cgs.media.yellowDroppedSaberShader,
				0x08000000);
		}
		if (cent->gent->owner->s.weapon == WP_SABER)
		{
			//he's still controlling me
			FX_AddSprite(cent->gent->owner->client->renderInfo.handRPoint, nullptr, nullptr, 8.0f, wv, wv, 0.0f, 0.0f,
				1.0f, cgs.media.yellowDroppedSaberShader, 0x08000000);
		}
	}
}

/*
Ghoul2 Insert Start
*/

// Copy the ghoul2 data into the ref ent correctly
void CG_SetGhoul2Info(refEntity_t* ent, const centity_t* cent)
{
	ent->ghoul2 = &cent->gent->ghoul2;
	VectorCopy(cent->currentState.modelScale, ent->modelScale);
	ent->radius = cent->currentState.radius;
	VectorCopy(cent->lerpAngles, ent->angles);
}

// write in the axis and stuff
void G2_BoltToGhoul2Model(const centity_t* cent, refEntity_t* ent)
{
	// extract the wraith ID from the bolt info
	int model_num = cent->currentState.bolt_info >> MODEL_SHIFT;
	model_num &= MODEL_AND;
	int bolt_num = cent->currentState.bolt_info >> BOLT_SHIFT;
	bolt_num &= BOLT_AND;
	int ent_num = cent->currentState.bolt_info >> ENTITY_SHIFT;
	ent_num &= ENTITY_AND;

	mdxaBone_t bolt_matrix;

	// go away and get me the bolt position for this frame please
	gi.G2API_GetBoltMatrix(cent->gent->ghoul2, model_num, bolt_num, &bolt_matrix, cg_entities[ent_num].currentState.angles,
		cg_entities[ent_num].currentState.origin, cg.time, cgs.model_draw,
		cent->currentState.modelScale);

	// set up the axis and origin we need for the actual effect spawning
	ent->origin[0] = bolt_matrix.matrix[0][3];
	ent->origin[1] = bolt_matrix.matrix[1][3];
	ent->origin[2] = bolt_matrix.matrix[2][3];

	ent->axis[0][0] = bolt_matrix.matrix[0][0];
	ent->axis[0][1] = bolt_matrix.matrix[1][0];
	ent->axis[0][2] = bolt_matrix.matrix[2][0];

	ent->axis[1][0] = bolt_matrix.matrix[0][1];
	ent->axis[1][1] = bolt_matrix.matrix[1][1];
	ent->axis[1][2] = bolt_matrix.matrix[2][1];

	ent->axis[2][0] = bolt_matrix.matrix[0][2];
	ent->axis[2][1] = bolt_matrix.matrix[1][2];
	ent->axis[2][2] = bolt_matrix.matrix[2][2];
}

void ScaleModelAxis(refEntity_t* ent)

{
	// scale the model should we need to
	if (ent->modelScale[0] && ent->modelScale[0] != 1.0f)
	{
		VectorScale(ent->axis[0], ent->modelScale[0], ent->axis[0]);
		ent->nonNormalizedAxes = qtrue;
	}
	if (ent->modelScale[1] && ent->modelScale[1] != 1.0f)
	{
		VectorScale(ent->axis[1], ent->modelScale[1], ent->axis[1]);
		ent->nonNormalizedAxes = qtrue;
	}
	if (ent->modelScale[2] && ent->modelScale[2] != 1.0f)
	{
		VectorScale(ent->axis[2], ent->modelScale[2], ent->axis[2]);
		ent->nonNormalizedAxes = qtrue;
	}
}

/*
Ghoul2 Insert End
*/

void CG_AddRadarEnt(const centity_t* cent)
{
	static constexpr size_t num_radar_ents = std::size(cg.radarEntities);
	if (cg.radarEntityCount >= num_radar_ents)
	{
		return;
	}
	cg.radarEntities[cg.radarEntityCount++] = cent->currentState.number;
}

/*
==================
CG_General
==================
*/
static void CG_General(centity_t* cent)
{
	refEntity_t ent;
	entityState_t* s1;

	if (cent->currentState.eFlags2 & EF2_RADAROBJECT)
	{
		CG_AddRadarEnt(cent);
	}

	s1 = &cent->currentState;
	/*
	Ghoul2 Insert Start
	*/

	// if set to invisible, skip
	if (!s1->modelindex && !cent->gent->ghoul2.IsValid())
	{
		return;
	}
	/*
	Ghoul2 Insert End
	*/

	if (s1->eFlags & EF_NODRAW)
	{
		// If you don't like it doing NODRAW, then don't set the flag
		return;
	}

	memset(&ent, 0, sizeof ent);

	// set frame

	if (cent->currentState.eFlags & EF_DISABLE_SHADER_ANIM)
	{
		// by setting the shader time to the current time, we can force an animating shader to not animate
		ent.shaderTime = cg.time * 0.001f;
	}

	if (s1->eFlags & EF_SHADER_ANIM)
	{
		// Deliberately setting it up so that shader anim will completely override any kind of model animation frame setting.
		ent.renderfx |= RF_SETANIMINDEX;
		ent.skinNum = s1->frame;
	}
	else if (s1->eFlags & EF_ANIM_ONCE)
	{
		ent.frame = cent->gent->s.frame;
		ent.renderfx |= RF_CAP_FRAMES;
	}
	else if (s1->eFlags & EF_ANIM_ALLFAST)
	{
		ent.frame = cg.time / 100;
		ent.renderfx |= RF_WRAP_FRAMES;
	}
	else
	{
		ent.frame = s1->frame;
	}
	ent.oldframe = ent.frame;
	ent.backlerp = 0;
	/*
	Ghoul2 Insert Start
	*/
	CG_SetGhoul2Info(&ent, cent);
	/*
	Ghoul2 Insert End
	*/

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.model_draw[s1->modelindex];

	if (!ent.radius)
	{
		// Set default g2 cull radius.
		ent.radius = 60;
	}

	if (s1->eFlags & EF_AUTO_SIZE && cent->gent)
	{
		cgi_R_ModelBounds(ent.hModel, cent->gent->mins, cent->gent->maxs);
		//Only do this once
		cent->gent->s.eFlags &= ~EF_AUTO_SIZE;
	}

	if (cent->currentState.weapon == WP_STUN_BATON)
	{
		int i;
		orientation_t lerped;

		vec3_t start;
		vec3_t end;
		vec3_t r_hand_pos{};

		centity_t* parent;
		mdxaBone_t mat;
		int color;
		color = 0x000020;

		// lerp the tag
		cgi_R_LerpTag(&lerped, cg_weapons[WP_STUN_BATON].missileModel, 0, 0, 1.0, "tag_extrem");

		VectorCopy(cent->lerpOrigin, end);
		for (i = 0; i < 3; i++)
		{
			VectorMA(end, lerped.origin[i], cent->currentState.angles, end);
		}

		parent = &cg_entities[cent->currentState.otherentity_num];
		gi.G2API_GetBoltMatrix(parent->gent->ghoul2, 0, 0, &mat, parent->lerpAngles, parent->lerpOrigin, cg.time,
			cgs.model_draw, cent->currentState.modelScale);

		r_hand_pos[0] = mat.matrix[0][3];
		r_hand_pos[1] = mat.matrix[1][3];
		r_hand_pos[2] = mat.matrix[2][3];

		gi.G2API_GiveMeVectorFromMatrix(mat, ORIGIN, start);
		CG_StunStartpoint(end);
		//GOING OUT
	}

	if (cent->currentState.weapon == WP_MELEE)
	{
		int i;
		orientation_t lerped;

		vec3_t start;
		vec3_t end;

		centity_t* parent;
		mdxaBone_t mat;
		int color;
		color = 0x000020;

		// lerp the tag grapple
		cgi_R_LerpTag(&lerped, cg_weapons[WP_MELEE].missileModel, 0, 0, 1.0, "tag_extrem");

		VectorCopy(cent->lerpOrigin, end);
		for (i = 0; i < 3; i++)
		{
			VectorMA(end, lerped.origin[i], cent->currentState.angles, end);
		}

		parent = &cg_entities[cent->currentState.otherentity_num];
		gi.G2API_GetBoltMatrix(parent->gent->ghoul2, 0, 0, &mat, parent->lerpAngles, parent->lerpOrigin, cg.time,
			cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix(mat, ORIGIN, start);

		vec4_t v4DKGREY2 = { 0.15f, 0.15f, 0.15f };

		FX_AddLine(start, end, 0.5f, 0.0f, 0.5f, v4DKGREY2, v4DKGREY2, 15, cgi_R_RegisterShader("gfx/misc/nav_line"), FX_SIZE_LINEAR);

		CG_GrappleStartpoint(end);
		//GOING OUT
	}

	// player model
	if (s1->number == cg.snap->ps.client_num)
	{
		ent.renderfx |= RF_THIRD_PERSON; // only draw from mirrors
	}
	/*
	Ghoul2 Insert Start
	*/
	// are we bolted to a Ghoul2 model?
	if (s1->bolt_info)
	{
		G2_BoltToGhoul2Model(cent, &ent);
	}
	else
	{
		//-------------------------------------------------------
		// Start of chair
		//-------------------------------------------------------
		if (cent->gent->s.weapon == WP_EMPLACED_GUN || cent->gent->activator && cent->gent->activator->owner &&
			cent->gent->activator->s.eFlags & EF_LOCKED_TO_WEAPON)
		{
			vec3_t temp;

			if (cent->gent->health <= 0 && cent->gent->e_ThinkFunc == thinkF_NULL)
			{
				if (!cent->gent->bounceCount)
				{
					//not an EWeb
					ent.customShader = cgi_R_RegisterShader("models/map_objects/imp_mine/turret_chair_dmg");
				}

				VectorSet(temp, 0, 0, 1);

				// add a big scorch mark under the gun
				CG_ImpactMark(cgs.media.scavMarkShader, cent->lerpOrigin, temp,
					0, 1, 1, 1, 1.0f, qfalse, 92, qtrue);
				CG_ImpactMark(cgs.media.scavMarkShader, cent->lerpOrigin, temp,
					90, 1, 1, 1, 1.0f, qfalse, 48, qtrue);
			}
			else
			{
				VectorSet(temp, 0, 0, 1);

				if (!(cent->gent->svFlags & SVF_INACTIVE))
				{
					if (!cent->gent->bounceCount)
					{
						//not an EWeb
						ent.customShader = cgi_R_RegisterShader("models/map_objects/imp_mine/turret_chair_on");
					}
				}

				// shadow under the gun
				CG_ImpactMark(cgs.media.shadowMarkShader, cent->lerpOrigin, temp,
					0, 1, 1, 1, 1.0f, qfalse, 32, qtrue);
			}
		}

		if (cent->gent->activator && cent->gent->activator->owner &&
			cent->gent->activator->s.eFlags & EF_LOCKED_TO_WEAPON &&
			cent->gent->activator->owner->s.number == cent->currentState.number)
			// gun number must be same as current entities number
		{
			centity_t* cc = &cg_entities[cent->gent->activator->s.number];

			const weaponData_t* w_data = nullptr;

			if (cc->currentState.weapon)
			{
				w_data = &weaponData[cc->currentState.weapon];
			}

			if (!(cc->currentState.eFlags & EF_FIRING) && !(cc->currentState.eFlags & EF_ALT_FIRING))
			{
				// not animating..pausing was leaving the barrels in a bad state
				gi.G2API_PauseBoneAnim(&cent->gent->ghoul2[cent->gent->playerModel], "model_root", cg.time);
			}

			// get alternating muzzle end bolts
			int bolt = cent->gent->handRBolt;
			mdxaBone_t bolt_matrix;

			if (!cc->gent->fxID || bolt == -1)
			{
				bolt = cent->gent->handLBolt;
			}

			if (bolt == -1)
			{
				bolt = 0;
			}
			gi.G2API_GetBoltMatrix(cent->gent->ghoul2, 0, bolt,
				&bolt_matrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
				cgs.model_draw, cent->currentState.modelScale);

			// store the muzzle point and direction so that we can fire in the right direction
			gi.G2API_GiveMeVectorFromMatrix(bolt_matrix, ORIGIN, cc->gent->client->renderInfo.muzzle_point);
			if (cent->gent->bounceCount)
			{
				//EWeb - *sigh* the muzzle tag on this is not aligned like th eone on the emplaced gun... consistency anyone...?
				gi.G2API_GiveMeVectorFromMatrix(bolt_matrix, NEGATIVE_X, cc->gent->client->renderInfo.muzzleDir);
			}
			else
			{
				//Emplaced gun
				gi.G2API_GiveMeVectorFromMatrix(bolt_matrix, POSITIVE_Y, cc->gent->client->renderInfo.muzzleDir);
			}
			cc->gent->client->renderInfo.mPCalcTime = cg.time;

			// HACK: adding in muzzle flashes
			if (cc->muzzleFlashTime > 0 && w_data)
			{
				const char* effect = nullptr;
				cc->muzzleFlashTime = 0;

				// Try and get a default muzzle so we have one to fall back on
				if (w_data->mMuzzleEffect[0])
				{
					effect = &w_data->mMuzzleEffect[0];
				}

				if (cc->currentState.eFlags & EF_ALT_FIRING)
				{
					// We're alt-firing, so see if we need to override with a custom alt-fire effect
					if (w_data->mAltMuzzleEffect[0])
					{
						effect = &w_data->mAltMuzzleEffect[0];
					}
				}

				if (cc->currentState.eFlags & EF_FIRING || cc->currentState.eFlags & EF_ALT_FIRING)
				{
					if (cent->gent->bounceCount)
					{
						//EWeb
						gi.G2API_SetBoneAnimIndex(&cent->gent->ghoul2[cent->gent->playerModel], cent->gent->rootBone,
							2, 4, BONE_ANIM_OVERRIDE_FREEZE, 0.6f, cg.time, -1, -1);
					}
					else
					{
						//Emplaced Gun
						gi.G2API_SetBoneAnimIndex(&cent->gent->ghoul2[cent->gent->playerModel], cent->gent->rootBone,
							0, 3, BONE_ANIM_OVERRIDE_FREEZE, 0.6f, cg.time, -1, -1);
					}

					if (effect)
					{
						// We got an effect and we're firing, so let 'er rip.
						theFxScheduler.PlayEffect(effect, cc->gent->client->renderInfo.muzzle_point,
							cc->gent->client->renderInfo.muzzleDir);
					}
				}
			}

			if (!in_camera && cc->muzzleOverheatTime > 0 && w_data)
			{
				if (!cg.renderingThirdPerson && cg_trueguns.integer)
				{
				}
				else
				{
					const char* effect = nullptr;
					if (w_data->mOverloadMuzzleEffect[0])
					{
						effect = &w_data->mOverloadMuzzleEffect[0];
					}

					if (effect)
					{
						// We got an effect and we're firing, so let 'er rip.
						theFxScheduler.PlayEffect(effect, cc->gent->client->renderInfo.muzzle_point,
							cc->gent->client->renderInfo.muzzleDir);
					}
					cc->muzzleOverheatTime = 0;
				}
			}

			VectorCopy(cent->gent->s.apos.trBase, cent->lerpAngles);
		}
		//-------------------------------------------------------
		// End of chair
		//-------------------------------------------------------

		AnglesToAxis(cent->lerpAngles, ent.axis);
	}

	//copy modelscale, if any
	VectorCopy(cent->currentState.modelScale, ent.modelScale);
	//apply modelscale, if any
	ScaleModelAxis(&ent);

	if (cent->gent->ghoul2.size())
	{
		//FIXME: use a flag for this, not a strcmp
		if (cent->gent->classname && Q_stricmp("limb", cent->gent->classname) == 0)
		{
			//limb, copy RGB
			ent.shaderRGBA[0] = cent->gent->startRGBA[0];
			ent.shaderRGBA[1] = cent->gent->startRGBA[1];
			ent.shaderRGBA[2] = cent->gent->startRGBA[2];
		}
		if (s1->weapon == WP_SABER && cent->gent && cent->gent->owner && cent->gent->owner->inuse)
		{
			//flying lightsaber
			//FIXME: better way to tell what it is would be nice
			if (cent->gent->classname && !Q_stricmp("limb", cent->gent->classname))
			{
				//limb, just add blade
				if (cent->gent->owner->client)
				{
					if (cent->gent->owner->client->ps.saber[0].Length() > 0)
					{
						CG_AddSaberBlade(&cg_entities[cent->gent->owner->s.number],
							&cg_entities[cent->gent->s.number], ent.renderfx, cent->gent->weaponModel[0],
							cent->lerpOrigin, cent->lerpAngles);
					}
					else if (cent->gent->owner->client->ps.saberEventFlags & SEF_INWATER)
					{
						CG_CheckSaberInWater(&cg_entities[cent->gent->owner->s.number],
							&cg_entities[cent->gent->s.number], 0, cent->gent->weaponModel[0],
							cent->lerpOrigin, cent->lerpAngles);
					}
				}
			}
			else
			{
				//thrown saber
				//light?  sound?
				if (cent->gent->owner->client && g_entities[cent->currentState.otherentity_num].client && g_entities[cent
					->currentState.otherentity_num].client->ps.saber[0].Active())
				{
					//saber is in-flight and active, play a sound on it
					if (cent->gent->owner->client->ps.saberEntityState == SES_RETURNING
						&& cent->gent->owner->client->ps.saber[0].type != SABER_STAR)
					{
						cgi_S_AddLoopingSound(cent->currentState.number,
							cent->lerpOrigin, vec3_origin,
							cgs.sound_precache[g_entities[cent->currentState.client_num].client->ps.
							saber[0].soundLoop]);
					}
					else
					{
						int spin_sound;
						if (cent->gent->owner->client->ps.saber[0].spinSound
							&& cgs.sound_precache[cent->gent->owner->client->ps.saber[0].spinSound])
						{
							spin_sound = cgs.sound_precache[cent->gent->owner->client->ps.saber[0].spinSound];
						}
						else if (cent->gent->owner->client->ps.saber[0].type == SABER_SITH_SWORD)
						{
							spin_sound = cgi_S_RegisterSound("sound/weapons/saber/saberspinoff.wav");
						}
						else
						{
							if (cent->gent->owner->client->ps.saber[0].type == SABER_SINGLE
								|| cent->gent->owner->client->ps.saber[0].type == SABER_THIN
								|| cent->gent->owner->client->ps.saber[0].type == SABER_SFX
								|| cent->gent->owner->client->ps.saber[0].type == SABER_CUSTOMSFX
								|| cent->gent->owner->client->ps.saber[0].type == SABER_GRIE
								|| cent->gent->owner->client->ps.saber[0].type == SABER_UNSTABLE)
							{
								spin_sound = cgi_S_RegisterSound("sound/weapons/saber/saberspin2.wav");
							}
							else
							{
								spin_sound = cgi_S_RegisterSound("sound/weapons/saber/saberspin1.wav");
							}
						}
						cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin,
							vec3_origin, spin_sound);
					}
				}
				if (cent->gent->owner->client)
				{
					if (cent->gent->owner->client->ps.saber[0].Length() > 0)
					{
						//only add the blade if it's on
						CG_AddSaberBlade(&cg_entities[cent->gent->owner->s.number],
							&cg_entities[cent->gent->s.number], ent.renderfx, 0,
							cent->lerpOrigin, cent->lerpAngles);
					}
					else if (cent->gent->owner->client->ps.saberEventFlags & SEF_INWATER)
					{
						CG_CheckSaberInWater(&cg_entities[cent->gent->owner->s.number],
							&cg_entities[cent->gent->s.number], 0, 0, cent->lerpOrigin,
							cent->lerpAngles);
					}
				}
				if (cent->gent->owner->health)
				{
					//make sure we can always be seen
					ent.renderfx |= RF_PULSATE;
				}
			}
		}
	}
	else
	{
		if (s1->weapon == WP_SABER && cent->gent && cent->gent->owner)
		{
			//flying lightsaber
			//light?  sound?
			if (cent->gent->owner->client && cent->currentState.saberActive)
			{
				//saber is in-flight and active, play a sound on it
				if (cent->gent->owner->client->ps.saberEntityState == SES_RETURNING
					&& cent->gent->owner->client->ps.saber[0].type != SABER_STAR)
				{
					if (cg_weapons[WP_SABER].firingSound)
					{
						cgi_S_AddLoopingSound(cent->currentState.number,
							cent->lerpOrigin, vec3_origin, cg_weapons[WP_SABER].firingSound);
					}
				}
				else
				{
					int spin_sound;
					if (cent->gent->owner->client->ps.saber[0].spinSound
						&& cgs.sound_precache[cent->gent->owner->client->ps.saber[0].spinSound])
					{
						spin_sound = cgs.sound_precache[cent->gent->owner->client->ps.saber[0].spinSound];
					}
					else if (cent->gent->owner->client->ps.saber[0].type == SABER_SITH_SWORD)
					{
						spin_sound = cgi_S_RegisterSound("sound/weapons/saber/saberspinoff.wav");
					}
					else
					{
						if (cent->gent->owner->client->ps.saber[0].type == SABER_SINGLE
							|| cent->gent->owner->client->ps.saber[0].type == SABER_THIN
							|| cent->gent->owner->client->ps.saber[0].type == SABER_SFX
							|| cent->gent->owner->client->ps.saber[0].type == SABER_CUSTOMSFX
							|| cent->gent->owner->client->ps.saber[0].type == SABER_GRIE
							|| cent->gent->owner->client->ps.saber[0].type == SABER_UNSTABLE)
						{
							spin_sound = cgi_S_RegisterSound("sound/weapons/saber/saberspin2.wav");
						}
						else
						{
							spin_sound = cgi_S_RegisterSound("sound/weapons/saber/saberspin1.wav");
						}
					}
					cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, spin_sound);
				}
			}
			CG_AddSaberBlade(&cg_entities[cent->gent->owner->s.number],
				nullptr, ent.renderfx, 0, nullptr, nullptr);

			if (cent->gent->owner->health)
			{
				//make sure we can always be seen
				ent.renderfx |= RF_PULSATE;
			}
		}
	}
	/*
	Ghoul2 Insert End
	*/

	if (cent->gent && cent->gent->forcePushTime > cg.time)
	{
		//FIXME: if I'm a rather large model, this will look kind of stupid...
		CG_ForcePushBlur(cent->lerpOrigin);
	}
	CG_AddRefEntWithTransportEffect(cent, &ent);

	if (cent->gent
		&& cent->gent->health <= 0
		&& cent->gent->s.weapon == WP_EMPLACED_GUN
		&& cent->gent->e_ThinkFunc)
	{
		// make the gun pulse red to warn about it exploding
		float val = (1.0f - static_cast<float>(cent->gent->nextthink - cg.time) / 3200.0f) * 0.3f;

		ent.customShader = cgi_R_RegisterShader("gfx/effects/solidWhite");
		ent.shaderRGBA[0] = (sin(cg.time * 0.04f) * val * 0.4f + val) * 255;
		ent.shaderRGBA[1] = ent.shaderRGBA[2] = 0;
		ent.renderfx |= RF_RGB_TINT;
		cgi_R_AddRefEntityToScene(&ent);
	}

	//--------------------------
	if (s1->eFlags & EF_FIRING && cent->gent->inuse)
	{
		//special code for adding the beam to the attached tripwire mine
		vec3_t beam_org;
		int handle = 0;
		SEffectTemplate* temp;

		VectorMA(ent.origin, 6.6f, ent.axis[0], beam_org); // forward

		// overriding the effect, so give us a copy first
		temp = theFxScheduler.GetEffectCopy(cgs.effects.tripminelaser, &handle);

		if (temp)
		{
			// have a copy, so get the line element out of there
			CPrimitiveTemplate* prim = theFxScheduler.GetPrimitiveCopy(temp, "line1");

			if (prim)
			{
				// we have the primitive, so modify the endpoint
				prim->mOrigin2X.SetRange(cent->gent->pos4[0], cent->gent->pos4[0]);
				prim->mOrigin2Y.SetRange(cent->gent->pos4[1], cent->gent->pos4[1]);
				prim->mOrigin2Z.SetRange(cent->gent->pos4[2], cent->gent->pos4[2]);

				// have a copy, so get the line element out of there
				CPrimitiveTemplate* primitive_template = theFxScheduler.GetPrimitiveCopy(temp, "line2");

				if (primitive_template)
				{
					// we have the primitive, so modify the cent->gent->pos3point
					primitive_template->mOrigin2X.SetRange(cent->gent->pos4[0], cent->gent->pos4[0]);
					primitive_template->mOrigin2Y.SetRange(cent->gent->pos4[1], cent->gent->pos4[1]);
					primitive_template->mOrigin2Z.SetRange(cent->gent->pos4[2], cent->gent->pos4[2]);

					// play the modified effect
					theFxScheduler.PlayEffect(handle, beam_org, ent.axis[0]);
				}
			}
		}

		theFxScheduler.PlayEffect(cgs.effects.tripminelaserImpactGlow, cent->gent->pos4, ent.axis[0]);
	}

	if (s1->eFlags & EF_PROX_TRIP)
	{
		//special code for adding the glow end to proximity tripmine
		vec3_t beam_org;

		VectorMA(ent.origin, 6.6f, ent.axis[0], beam_org); // forward
		theFxScheduler.PlayEffect(cgs.effects.tripmineglowBit, beam_org, ent.axis[0]);
	}

	if (s1->eFlags & EF_ALT_FIRING)
	{
		// hack for the spotlight
		vec3_t org, axis[3], dir;

		AngleVectors(cent->lerpAngles, dir, nullptr, nullptr);

		CG_GetTagWorldPosition(&ent, "tag_flash", org, axis);

		theFxScheduler.PlayEffect("env/light_cone", org, axis[0]);

		VectorMA(cent->lerpOrigin, cent->gent->radius - 5, dir, org);
		// stay a bit back from the impact point...this may not be enough?

		cgi_R_AddLightToScene(org, 225, 1.0f, 1.0f, 1.0f);
	}

	//-----------------------------------------------------------
	if (cent->gent->flags & (FL_DMG_BY_HEAVY_WEAP_ONLY | FL_SHIELDED))
	{
		// Dumb assumption, but I guess we must be a shielded ion_cannon??  We should probably verify
		// if it's an ion_cannon that's Heavy Weapon only, we don't want to make it shielded do we...?
		if (strcmp("misc_ion_cannon", cent->gent->classname) == 0 && cent->gent->flags & FL_SHIELDED)
		{
			// must be doing "pain"....er, impact
			if (cent->gent->painDebounceTime > cg.time)
			{
				float t = static_cast<float>(cent->gent->painDebounceTime - cg.time) / 1000.0f;

				// Only display when we have damage
				if (t >= 0.0f && t <= 1.0f)
				{
					t *= Q_flrand(0.0f, 1.0f);

					ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = 255.0f * t;
					ent.shaderRGBA[3] = 255;
					ent.renderfx &= ~RF_ALPHA_FADE;
					ent.renderfx |= RF_RGB_TINT;
					ent.customShader = cgi_R_RegisterShader("gfx/misc/ion_shield");

					cgi_R_AddRefEntityToScene(&ent);
				}
			}
		}
	}

	//draw force sight shell around it, too
	if (cg.snap->ps.forcePowersActive & 1 << FP_SEE
		&& cg.snap->ps.client_num != cent->currentState.number
		&& CG_PlayerCanSeeCent(cent))
	{
		//so player can see dark missiles/explosives
		if (s1->weapon == WP_THERMAL
			|| s1->weapon == WP_DET_PACK
			|| s1->weapon == WP_TRIP_MINE
			|| cent->gent && cent->gent->e_UseFunc == useF_ammo_power_converter_use
			|| cent->gent && cent->gent->e_UseFunc == useF_shield_power_converter_use
			|| s1->eFlags & EF_FORCE_VISIBLE)
		{
			//really, we only need to do this for things like thermals, detpacks and tripmines, no?
			CG_AddForceSightShell(&ent, cent);
		}
	}
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker(centity_t* cent)
{
	if (!cent->currentState.client_num)
	{
		// FIXME: use something other than client_num...
		return; // not auto triggering
	}

	if (cg.time < cent->miscTime)
	{
		return;
	}

	cgi_S_StartSound(nullptr, cent->currentState.number, CHAN_ITEM, cgs.sound_precache[cent->currentState.eventParm]);

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.client_num = ent->random * 10;
	cent->miscTime = static_cast<int>(cg.time + cent->currentState.frame * 100 + cent->currentState.client_num * 100 *
		Q_flrand(-1.0f, 1.0f));
}

/*
==================
CG_Item
==================
*/
static void CG_Item(centity_t* cent)
{
	refEntity_t ent;

	const entityState_t* es = &cent->currentState;
	if (es->modelindex >= bg_numItems)
	{
		CG_Error("Bad item index %i on entity", es->modelindex);
	}
	/*
	Ghoul2 Insert Start
	*/

	// if set to invisible, skip
	if (!es->modelindex && !cent->gent->ghoul2.IsValid() || es->eFlags & EF_NODRAW)
	{
		return;
	}
	/*
	Ghoul2 Insert End
	*/
	if (cent->gent && !cent->gent->inuse)
	{
		// Yeah, I know....items were being freed on touch, but it could still get here and draw incorrectly...
		return;
	}

	const gitem_t* item = &bg_itemlist[es->modelindex];

	if (cg_simpleItems.integer)
	{
		memset(&ent, 0, sizeof ent);
		ent.reType = RT_SPRITE;
		VectorCopy(cent->lerpOrigin, ent.origin);
		ent.origin[2] += 16;
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		ent.renderfx |= RF_FORCE_ENT_ALPHA;
		cgi_R_AddRefEntityToScene(&ent);
		return;
	}

	memset(&ent, 0, sizeof ent);

	// items bob up and down continuously
	if (item->giType == IT_HOLOCRON)
	{
		const float scale = 0.005f + cent->currentState.number * 0.00001f;
		cent->lerpOrigin[2] += 4 + cos((cg.time + 1000) * scale) * 3 + 8; // just raised them up a bit
	}

	// autorotate at one of two speeds
	//	if ( item->giType == IT_HEALTH ) {
	//		VectorCopy( cg.autoAnglesFast, cent->lerpAngles );
	//		AxisCopy( cg.autoAxisFast, ent.axis );
	//	} else {
	if (item->giType == IT_HOLOCRON)
	{
		VectorCopy(cg.autoAngles, cent->lerpAngles);
		AxisCopy(cg.autoAxis, ent.axis);
	}
	vec3_t spin_angles;

	//AxisClear( ent.axis );
	VectorCopy(cent->gent->s.angles, spin_angles);

	if (cent->gent->ghoul2.IsValid()
		&& cent->gent->ghoul2.size())
	{
		//since modelindex is used by items as an index into items(not models), we need to ignore the hModel here to force it to use the ghoul2 model if we have one
		ent.hModel = cgs.model_draw[0];
	}
	else
	{
		ent.hModel = cg_items[es->modelindex].models;
	}
	/*
	Ghoul2 Insert Start
	*/
	CG_SetGhoul2Info(&ent, cent);
	/*
	Ghoul2 Insert End
	*/

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// lovely...this is for weapons that should be oriented vertically.  For weapons lockers and such.
	if (cent->gent->spawnflags & 16)
	{
		//VectorClear( spinAngles );
		if (item->giType == IT_WEAPON
			&& item->giTag == WP_SABER)
		{
			if (cent->gent->random)
			{
				//pitch specified
				spin_angles[PITCH] += cent->gent->random;
			}
			else
			{
				spin_angles[PITCH] -= 20;
			}
		}
		else
		{
			spin_angles[PITCH] -= 75;
		}
	}

	if (item->giType != IT_HOLOCRON)
	{
		AnglesToAxis(spin_angles, ent.axis);
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	/*	if (( item->giType == IT_WEAPON ) || ( item->giType == IT_ARMOR ))
		{
			ent.renderfx |= RF_MINLIGHT;
		}
	*/
	// increase the size of the weapons when they are presented as items
	//	if ( item->giType == IT_WEAPON ) {
	//		VectorScale( ent.axis[0], 1.5f, ent.axis[0] );
	//		VectorScale( ent.axis[1], 1.5f, ent.axis[1] );
	//		VectorScale( ent.axis[2], 1.5f, ent.axis[2] );
	//		ent.nonNormalizedAxes = qtrue;
	//	}

	// add to refresh list
	cgi_R_AddRefEntityToScene(&ent);

	if (cg.snap->ps.forcePowersActive & 1 << FP_SEE
		&& cg.snap->ps.client_num != cent->currentState.number
		&& CG_PlayerCanSeeCent(cent))
	{
		CG_AddForceSightShell(&ent, cent);
	}

	if (item->giType == IT_WEAPON
		&& item->giTag == WP_SABER
		&& (!cent->gent || !(cent->gent->spawnflags & 64)))
	{
		ent.customShader = cgi_R_RegisterShader("gfx/effects/solidWhite_cull");
		ent.renderfx = RF_RGB_TINT;
		const float wv = sin(cg.time * 0.002f) * 0.08f + 0.2f;
		ent.shaderRGBA[0] = ent.shaderRGBA[1] = wv * 255;
		ent.shaderRGBA[2] = 0;
		cgi_R_AddRefEntityToScene(&ent);

		for (int i = -4; i < 10; i += 1)
		{
			vec3_t org;
			VectorMA(ent.origin, -i, ent.axis[2], org);

			FX_AddSprite(org, nullptr, nullptr, 10.0f, wv * 0.5f, wv * 0.5f, 0.0f, 0.0f, 1.0f, cgs.media.yellowDroppedSaberShader,
				0x08000000);
		}

		// THIS light looks crappy...maybe it should just be removed...
		cgi_R_AddLightToScene(ent.origin, wv * 100, 1.0f, 1.0f, 0.0f);
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile(centity_t* cent)
{
	refEntity_t ent;

	if (!cent->gent->inuse)
		return;

	entityState_t* s1 = &cent->currentState;
	if (s1->weapon >= WP_NUM_WEAPONS)
	{
		s1->weapon = 0;
	}

	if (cent->currentState.eFlags2 & EF2_RADAROBJECT)
	{
		CG_AddRadarEnt(cent);
	}

	const weaponInfo_t* weapon = &cg_weapons[s1->weapon];
	const weaponData_t* w_data = &weaponData[s1->weapon];

	if (s1->pos.trType != TR_INTERPOLATE)
	{
		// calculate the axis
		VectorCopy(s1->angles, cent->lerpAngles);
	}

	if (s1->otherentity_num2 && (g_vehWeaponInfo[s1->otherentity_num2].iShotFX || g_vehWeaponInfo[s1->otherentity_num2].
		iModel))
	{
		vec3_t forward;

		if (s1->eFlags & EF_USE_ANGLEDELTA)
		{
			AngleVectors(cent->currentState.angles, forward, nullptr, nullptr);
		}
		else
		{
			if (VectorNormalize2(cent->gent->s.pos.trDelta, forward) == 0.0f)
			{
				if (VectorNormalize2(s1->pos.trDelta, forward) == 0.0f)
				{
					forward[2] = 1.0f;
				}
			}
		}

		// hack the scale of the forward vector if we were just fired or bounced...this will shorten up the tail for a split second so tails don't clip so harshly
		int dif = cg.time - cent->gent->s.pos.trTime;

		if (dif < 75)
		{
			if (dif < 0)
			{
				dif = 0;
			}

			const float scale = dif / 75.0f * 0.95f + 0.05f;

			VectorScale(forward, scale, forward);
		}

		CG_PlayEffectID(g_vehWeaponInfo[s1->otherentity_num2].iShotFX, cent->lerpOrigin, forward);
		if (g_vehWeaponInfo[s1->otherentity_num2].iLoopSound)
		{
			vec3_t velocity;
			EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity);
			if (cgs.sound_precache[g_vehWeaponInfo[s1->otherentity_num2].iLoopSound] != NULL_SFX)
			{
				cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, velocity,
					cgs.sound_precache[g_vehWeaponInfo[s1->otherentity_num2].iLoopSound]);
			}
		}
		//add custom model
		if (!g_vehWeaponInfo[s1->otherentity_num2].iModel)
		{
			return;
		}
	}
	else if (s1->powerups & 1 << PW_FORCE_PROJECTILE)
	{
		if (s1->weapon == WP_CONCUSSION)
		{
			FX_DestructionProjectileThink(cent);
			cgi_R_AddLightToScene(cent->lerpOrigin, 125, 1.0f, 0.25f, 0.75f);
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.destructionSound);
			return;
		}
		if (s1->weapon == WP_ROCKET_LAUNCHER)
		{
			FX_BlastProjectileThink(cent);
			cgi_R_AddLightToScene(cent->lerpOrigin, 125, 1.0f, 0.65f, 0.0f);
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.blastSound);
			return;
		}
		if (s1->weapon == WP_DISRUPTOR)
		{
			FX_StrikeProjectileThink(cent);
			cgi_R_AddLightToScene(cent->lerpOrigin, 125, 1.0f, 0.65f, 0.0f);
			return;
		}
	}
	else if (cent->gent->alt_fire)
	{
		// add trails
		if (weapon->alt_missileTrailFunc)
			weapon->alt_missileTrailFunc(cent, weapon);

		// add dynamic light
		if (w_data->alt_missileDlight)
			cgi_R_AddLightToScene(cent->lerpOrigin, w_data->alt_missileDlight,
				w_data->alt_missileDlightColor[0], w_data->alt_missileDlightColor[1],
				w_data->alt_missileDlightColor[2]);

		// add missile sound
		if (weapon->alt_missileSound)
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->alt_missileSound);

		//Don't draw something without a model
		if (weapon->alt_missileModel == NULL_HANDLE)
			return;
	}
	else
	{
		// add trails
		if (weapon->missileTrailFunc)
			weapon->missileTrailFunc(cent, weapon);

		// add dynamic light
		if (w_data->missileDlight)
			cgi_R_AddLightToScene(cent->lerpOrigin, w_data->missileDlight,
				w_data->missileDlightColor[0], w_data->missileDlightColor[1],
				w_data->missileDlightColor[2]);

		// add missile sound
		if (weapon->missileSound)
			cgi_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->missileSound);

		//Don't draw something without a model
		if (weapon->missileModel == NULL_HANDLE)
			return;
	}

	// create the render entity
	memset(&ent, 0, sizeof ent);
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	CG_SetGhoul2Info(&ent, cent);

	if (s1->weapon == WP_STUN_BATON)
	{
		orientation_t lerped;

		vec3_t start;
		vec3_t end;
		vec3_t r_hand_pos{};
		vec3_t BLUER = { 0.0f, 0.0f, 1.0f };

		mdxaBone_t mat;

		// lerp the tag
		cgi_R_LerpTag(&lerped, weapon->missileModel, 0, 0, 1.0, "tag_extrem");

		VectorCopy(cent->lerpOrigin, end);
		for (const float i : lerped.origin)
		{
			VectorMA(end, i, s1->angles, end);
		}

		const centity_t* parent = &cg_entities[s1->otherentity_num];
		gi.G2API_GetBoltMatrix(parent->gent->ghoul2, 1, 0, &mat, parent->lerpAngles, parent->lerpOrigin, cg.time,
			cgs.model_draw, cent->currentState.modelScale);

		r_hand_pos[0] = mat.matrix[0][3];
		r_hand_pos[1] = mat.matrix[1][3];
		r_hand_pos[2] = mat.matrix[2][3];

		gi.G2API_GiveMeVectorFromMatrix(mat, ORIGIN, start);

		FX_AddLine(start, end, 0.1f, 0.5f, 0.0f, BLUER, BLUER, 300, cgi_R_RegisterShader("gfx/effects/blueline"), 0);

		CG_StunStartpoint(end);
	}

	if (s1->weapon == WP_MELEE)
	{
		orientation_t lerped;

		vec3_t start;
		vec3_t end;

		mdxaBone_t mat;

		// lerp the tag grapple
		cgi_R_LerpTag(&lerped, weapon->missileModel, 0, 0, 1.0, "tag_extrem");

		VectorCopy(cent->lerpOrigin, end);
		for (const float i : lerped.origin)
		{
			VectorMA(end, i, s1->angles, end);
		}

		const centity_t* parent = &cg_entities[s1->otherentity_num];
		gi.G2API_GetBoltMatrix(parent->gent->ghoul2, 1, 0, &mat, parent->lerpAngles, parent->lerpOrigin, cg.time,
			cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix(mat, ORIGIN, start);

		vec4_t v4DKGREY2 = { 0.15f, 0.15f, 0.15f };

		FX_AddLine(start, end, 0.5f, 0.0f, 0.5f, v4DKGREY2, v4DKGREY2, 15, cgi_R_RegisterShader("gfx/misc/whiteline2"), FX_SIZE_LINEAR);

		CG_GrappleStartpoint(end);
	}

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.renderfx = RF_NOSHADOW;

	if (s1->otherentity_num2 && g_vehWeaponInfo[s1->otherentity_num2].iModel && cgs.model_draw[g_vehWeaponInfo[s1->
		otherentity_num2].iModel] != NULL_HANDLE)
		ent.hModel = cgs.model_draw[g_vehWeaponInfo[s1->otherentity_num2].iModel];
	else if (cent->gent->alt_fire)
		ent.hModel = weapon->alt_missileModel;
	else
		ent.hModel = weapon->missileModel;

	// spin as it moves
	if (s1->apos.trType != TR_INTERPOLATE)
	{
		// convert direction of travel into axis
		if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
		{
			ent.axis[0][2] = 1;
		}
		if (s1->pos.trType != TR_STATIONARY)
		{
			if (s1->eFlags & EF_MISSILE_STICK)
				RotateAroundDirection(ent.axis, cg.time * 0.5f); //Did this so regular missiles don't get broken
			else
				RotateAroundDirection(ent.axis, cg.time * 0.25f); //JFM:FLOAT FIX
		}
		else
		{
			if (s1->eFlags & EF_MISSILE_STICK)
				RotateAroundDirection(ent.axis, static_cast<float>(s1->pos.trTime) * 0.5f);
			else
				RotateAroundDirection(ent.axis, static_cast<float>(s1->time));
		}
	}
	else
	{
		AnglesToAxis(cent->lerpAngles, ent.axis);
	}

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups(&ent, s1->powerups, nullptr);

	if (cg.snap->ps.forcePowersActive & 1 << FP_SEE
		&& cg.snap->ps.client_num != cent->currentState.number
		&& CG_PlayerCanSeeCent(cent))
	{
		//so player can see dark missiles/explosives
		if (s1->weapon == WP_THERMAL
			|| s1->weapon == WP_DET_PACK
			|| s1->weapon == WP_TRIP_MINE
			|| s1->eFlags & EF_FORCE_VISIBLE)
		{
			//really, we only need to do this for things like thermals, detpacks and tripmines, no?
			CG_AddForceSightShell(&ent, cent);
		}
	}
}

/*
===============
CG_Mover
===============
*/

constexpr auto DOOR_OPENING = 1;
constexpr auto DOOR_CLOSING = 2;
constexpr auto DOOR_OPEN = 3;
constexpr auto DOOR_CLOSED = 4;

static void CG_Mover(centity_t* cent)
{
	refEntity_t ent;

	const entityState_t* s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof ent);

	if (cent->currentState.eFlags2 & EF2_HYPERSPACE)
	{
		//I'm the hyperspace brush
		qboolean draw_me = qfalse;

		if (cg.predicted_player_state.m_iVehicleNum
			&& cent->gent->s.hyperSpaceTime
			&& cg.time - cent->gent->s.hyperSpaceTime < HYPERSPACE_TIME
			&& cg.time - cent->gent->s.hyperSpaceTime > 1000)
		{
			if (cg.snap
				&& cg.snap->ps.pm_type == PM_INTERMISSION)
			{
				//in the intermission, stop drawing hyperspace ent
			}
			else if (cent->gent->s.eFlags2 & EF2_HYPERSPACE)
			{
				//actually hyperspacing now
				const float time_frac = static_cast<float>(cg.time - cent->gent->s.hyperSpaceTime - 1000) / (
					HYPERSPACE_TIME - 1000);
				if (time_frac < HYPERSPACE_TELEPORT_FRAC + 0.1f)
				{
					//still in hyperspace or just popped out
					const float alpha = time_frac < 0.5f ? time_frac / 0.5f : 1.0f;
					draw_me = qtrue;
					VectorMA(cg.refdef.vieworg, 1000.0f + (1.0f - time_frac) * 1000.0f, cg.refdef.viewaxis[0],
						cent->lerpOrigin);
					VectorSet(cent->lerpAngles, cg.refdef.viewangles[PITCH], cg.refdef.viewangles[YAW] - 90.0f, 0);
					ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = 255;
					ent.shaderRGBA[3] = alpha * 255;
				}
			}
		}
		if (!draw_me)
		{
			//else, never draw
			return;
		}
	}

	if (cent->currentState.eFlags2 & EF2_RADAROBJECT)
	{
		CG_AddRadarEnt(cent);
	}

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx = RF_NOSHADOW;
	/*
	Ghoul2 Insert Start
	*/

	CG_SetGhoul2Info(&ent, cent);
	/*
	Ghoul2 Insert End
	*/
	// flicker between two skins (FIXME?)
	ent.skinNum = cg.time >> 6 & 1;

	// get the model, either as a bmodel or a modelindex
	if (s1->solid == SOLID_BMODEL)
	{
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	}
	else
	{
		ent.hModel = cgs.model_draw[s1->modelindex];
	}

	// If there isn't an hModel for this mover, an RGB axis model will get drawn.
	if (!ent.hModel)
	{
		return;
	}

	if (cent->currentState.eFlags & EF_DISABLE_SHADER_ANIM)
	{
		// by setting the shader time to the current time, we can force an animating shader to not animate
		ent.shaderTime = cg.time * 0.001f;
	}

	// add the secondary model
	if (s1->solid == SOLID_BMODEL && s1->modelindex2)
	{
		if (!(s1->eFlags & EF_NODRAW))
		{
			// add to refresh list
			CG_AddRefEntWithTransportEffect(cent, &ent);
		}
		if (!VectorCompare(vec3_origin, cent->gent->modelAngles))
		{
			//we have a rotational offset for the model for this brush
			vec3_t model_angles;
			VectorAdd(cent->lerpAngles, cent->gent->modelAngles, model_angles);
			AnglesToAxis(model_angles, ent.axis);
		}
		ent.hModel = cgs.model_draw[s1->modelindex2];
	}

	// I changed it to always do it because nodraw seemed like it should actually do what it says. Be aware that if you change this,
	//	the movers for the shooting gallery on doom_detention will break.
	if (s1->eFlags & EF_NODRAW)
	{
		return;
	}
	//fall through and render the hModel or...

	//We're a normal model being moved, animate our model
	ent.skinNum = 0;
	if (s1->eFlags & EF_ANIM_ONCE)
	{
		//FIXME: needs to anim at once per 100 ms
		ent.frame = cent->gent->s.frame;
		ent.renderfx |= RF_CAP_FRAMES;
	}
	else if (s1->eFlags & EF_ANIM_ALLFAST)
	{
		ent.frame = cg.time / 100;
		ent.renderfx |= RF_WRAP_FRAMES;
	}
	else
	{
		ent.frame = s1->frame;
	}

	if (s1->eFlags & EF_SHADER_ANIM)
	{
		ent.renderfx |= RF_SETANIMINDEX;
		ent.skinNum = s1->frame;
	}

	// add to refresh list
	CG_AddRefEntWithTransportEffect(cent, &ent);

	if (cg.snap->ps.forcePowersActive & 1 << FP_SEE
		&& cg.snap->ps.client_num != cent->currentState.number
		&& s1->eFlags & EF_FORCE_VISIBLE)
	{
		//so player can see func_breakables
		CG_AddForceSightShell(&ent, cent);
	}
}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam(const centity_t* cent, const int color)
{
	refEntity_t ent;

	const entityState_t* s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof ent);
	VectorCopy(s1->pos.trBase, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	AxisClear(ent.axis);
	ent.reType = RT_BEAM;
	ent.skinNum = color;

	ent.renderfx = RF_NOSHADOW;
	/*
	Ghoul2 Insert Start
	*/
	CG_SetGhoul2Info(&ent, cent);

	/*
	Ghoul2 Insert End
	*/

	// add to refresh list
	cgi_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
extern void CG_GrappleTrail(centity_t* ent, const weaponInfo_t* wi);

static void CG_Grapple(centity_t* cent)
{
	refEntity_t ent;

	const entityState_t* s1 = &cent->currentState;
	const weaponInfo_t* weapon = &cg_weapons[WP_MELEE];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

	// Will draw cable if needed
	CG_GrappleTrail(cent, weapon);

	// create the render entity
	memset(&ent, 0, sizeof ent);
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
	{
		ent.axis[0][2] = 1;
	}

	cgi_R_AddRefEntityToScene(&ent);
}

static vec2_t st[] =
{
	{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

void CG_Cube(vec3_t mins, vec3_t maxs, vec3_t color, const float alpha)
{
	vec3_t point[4]{}, rot = { 0, 0, 0 };
	int vec[3]{};
	int axis;

	for (axis = 0, vec[0] = 0, vec[1] = 1, vec[2] = 2; axis < 3; axis++, vec[0]++, vec[1]++, vec[2]++)
	{
		for (int& i : vec)
		{
			if (i > 2)
			{
				i = 0;
			}
		}

		point[0][vec[1]] = mins[vec[1]];
		point[0][vec[2]] = mins[vec[2]];

		point[1][vec[1]] = mins[vec[1]];
		point[1][vec[2]] = maxs[vec[2]];

		point[2][vec[1]] = maxs[vec[1]];
		point[2][vec[2]] = maxs[vec[2]];

		point[3][vec[1]] = maxs[vec[1]];
		point[3][vec[2]] = mins[vec[2]];

		//- face
		point[0][vec[0]] = point[1][vec[0]] = point[2][vec[0]] = point[3][vec[0]] = mins[vec[0]];

		FX_AddPoly(point, st, 4, nullptr, nullptr, alpha, alpha, 0.0f,
			color, color, 0.0f, rot, 0.0f, 0.0f,
			100, cgs.media.solidWhiteShader, 0);

		//+ face
		point[0][vec[0]] = point[1][vec[0]] = point[2][vec[0]] = point[3][vec[0]] = maxs[vec[0]];

		FX_AddPoly(point, st, 4, nullptr, nullptr, alpha, alpha, 0.0f,
			color, color, 0.0f, rot, 0.0f, 0.0f,
			100, cgs.media.solidWhiteShader, 0);
	}
}

void CG_CubeOutline(vec3_t mins, vec3_t maxs, const int time, const unsigned int color)
{
	vec3_t point1{}, point2{}, point3{}, point4{};
	int vec[3]{};
	int axis;

	for (axis = 0, vec[0] = 0, vec[1] = 1, vec[2] = 2; axis < 3; axis++, vec[0]++, vec[1]++, vec[2]++)
	{
		for (int& i : vec)
		{
			if (i > 2)
			{
				i = 0;
			}
		}

		point1[vec[1]] = mins[vec[1]];
		point1[vec[2]] = mins[vec[2]];

		point2[vec[1]] = mins[vec[1]];
		point2[vec[2]] = maxs[vec[2]];

		point3[vec[1]] = maxs[vec[1]];
		point3[vec[2]] = maxs[vec[2]];

		point4[vec[1]] = maxs[vec[1]];
		point4[vec[2]] = mins[vec[2]];

		//- face
		point1[vec[0]] = point2[vec[0]] = point3[vec[0]] = point4[vec[0]] = mins[vec[0]];

		CG_BlockLine(point1, point2, time, color, 1);
		CG_BlockLine(point2, point3, time, color, 1);
		CG_BlockLine(point1, point4, time, color, 1);
		CG_BlockLine(point4, point3, time, color, 1);

		//+ face
		point1[vec[0]] = point2[vec[0]] = point3[vec[0]] = point4[vec[0]] = maxs[vec[0]];

		CG_BlockLine(point1, point2, time, color, 1);
		CG_BlockLine(point2, point3, time, color, 1);
		CG_BlockLine(point1, point4, time, color, 1);
		CG_BlockLine(point4, point1, time, color, 1);
	}
}

void G_TestLine(vec3_t start, vec3_t end, const int color, const int time)
{
	gentity_t* te = G_TempEntity(start, EV_TESTLINE);
	VectorCopy(start, te->s.origin);
	VectorCopy(end, te->s.origin2);
	te->s.time2 = time;
	te->s.weapon = color;
	te->svFlags |= SVF_BROADCAST;
}

void G_DebugBoxLines(vec3_t mins, vec3_t maxs, const int duration)
{
	vec3_t start;
	vec3_t end;

	const float x = maxs[0] - mins[0];
	const float y = maxs[1] - mins[1];

	// top of box
	VectorCopy(maxs, start);
	VectorCopy(maxs, end);
	start[0] -= x;
	G_TestLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] -= y;
	G_TestLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] += x;
	G_TestLine(start, end, 0x00000ff, duration);
	G_TestLine(start, maxs, 0x00000ff, duration);
	// bottom of box
	VectorCopy(mins, start);
	VectorCopy(mins, end);
	start[0] += x;
	G_TestLine(start, end, 0x00000ff, duration);
	end[0] = start[0];
	end[1] += y;
	G_TestLine(start, end, 0x00000ff, duration);
	start[1] = end[1];
	start[0] -= x;
	G_TestLine(start, end, 0x00000ff, duration);
	G_TestLine(start, mins, 0x00000ff, duration);
}

/*
===============
CG_Portal
===============
*/
static void CG_Portal(const centity_t* cent)
{
	refEntity_t ent;

	const entityState_t* s1 = &cent->currentState;

	//FIXME: this tends to give a bad axis[1], perhaps we
	//should just do the VectorSubtraction here rather than
	//on the game side.  Would also allow camera to follow
	//a moving target.

	// create the render entity
	memset(&ent, 0, sizeof ent);
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	PerpendicularVector(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract(vec3_origin, ent.axis[1], ent.axis[1]);

	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.frame = s1->frame; // rotation speed
	ent.skinNum = static_cast<int>(s1->client_num / 256.0 * 360); // roll offset

	/*
	Ghoul2 Insert Start
	*/
	CG_SetGhoul2Info(&ent, cent);
	/*
	Ghoul2 Insert End
	*/

	// add to refresh list
	cgi_R_AddRefEntityToScene(&ent);
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover(const vec3_t in, const int mover_num, const int at_time, vec3_t out)
{
	vec3_t old_origin, origin, delta_origin;
	//	vec3_t	oldAngles, angles, deltaAngles;

	if (mover_num <= 0)
	{
		VectorCopy(in, out);
		return;
	}

	const centity_t* cent = &cg_entities[mover_num];
	if (cent->currentState.eType != ET_MOVER)
	{
		VectorCopy(in, out);
		return;
	}

	EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, old_origin);
	//	EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, oldAngles );

	EvaluateTrajectory(&cent->currentState.pos, at_time, origin);
	//	EvaluateTrajectory( &cent->currentState.apos, atTime, angles );

	VectorSubtract(origin, old_origin, delta_origin);
	//	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd(in, delta_origin, out);

	// FIXME: origin change when on a rotating object
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
extern char* vtos(const vec3_t v);
#if 1
void CG_CalcEntityLerpPositions(centity_t* cent)
{
	if (cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_VEHICLE && cent->nextState)
		//cent->currentState.vehicleIndex != VEHICLE_NONE )
	{
		const float f = cg.frameInterpolation;

		cent->currentState.vehicleAngles[0] = LerpAngle(cent->currentState.vehicleAngles[0],
			cent->nextState->vehicleAngles[0], f);
		cent->currentState.vehicleAngles[1] = LerpAngle(cent->currentState.vehicleAngles[1],
			cent->nextState->vehicleAngles[1], f);
		cent->currentState.vehicleAngles[2] = LerpAngle(cent->currentState.vehicleAngles[2],
			cent->nextState->vehicleAngles[2], f);
	}

	if (cent->currentState.number == cg.snap->ps.client_num)
	{
		// if the player, take position from prediction
		VectorCopy(cg.predicted_player_state.origin, cent->lerpOrigin);
		VectorCopy(cg.predicted_player_state.viewangles, cent->lerpAngles);
		/*
		Ghoul2 Insert Start
		*/
		//		LerpBoneAngleOverrides(cent);
		/*
		Ghoul2 Insert End
		*/
		return;
	}

	//FIXME: prediction on clients in timescale results in jerky positional translation
	if (cent->interpolate)
	{
		// if the entity has a valid next state, interpolate a value between the frames
		// unless it is a mover with a known start and stop
		vec3_t current, next;

		// it would be an internal error to find an entity that interpolates without
		// a snapshot ahead of the current one
		if (cg.nextSnap == nullptr)
		{
			CG_Error("CG_AddCEntity: cg.nextSnap == NULL");
		}

		const float f = cg.frameInterpolation;

		if (cent->currentState.apos.trType == TR_INTERPOLATE && cent->nextState)
		{
			EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current);
			EvaluateTrajectory(&cent->nextState->apos, cg.nextSnap->serverTime, next);

			cent->lerpAngles[0] = LerpAngle(current[0], next[0], f);
			cent->lerpAngles[1] = LerpAngle(current[1], next[1], f);
			cent->lerpAngles[2] = LerpAngle(current[2], next[2], f);

			/*
			if(cent->gent && cent->currentState.client_num != 0 && !VectorCompare(current, next))
			{
				Com_Printf("%s last/next/lerp apos %s/%s/%s, f = %4.2f\n", cent->gent->script_targetname, vtos(current), vtos(next), vtos(cent->lerpAngles), f);
			}
			*/
			/*
			Ghoul2 Insert Start
			*/
			// now the nasty stuff - this will interpolate all ghoul2 models bone angle overrides per model attached to this cent
			/*
			if (cent->gent->ghoul2.size())
			{
				LerpBoneAngleOverrides(cent);
			}
			*/
			/*
			Ghoul2 Insert End
			*/
		}
		if (cent->currentState.pos.trType == TR_INTERPOLATE && cent->nextState)
		{
			// this will linearize a sine or parabolic curve, but it is important
			// to not extrapolate player positions if more recent data is available
			EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current);
			EvaluateTrajectory(&cent->nextState->pos, cg.nextSnap->serverTime, next);

			cent->lerpOrigin[0] = current[0] + f * (next[0] - current[0]);
			cent->lerpOrigin[1] = current[1] + f * (next[1] - current[1]);
			cent->lerpOrigin[2] = current[2] + f * (next[2] - current[2]);

			/*
			if ( cent->gent && cent->currentState.client_num != 0 )
			{
				Com_Printf("%s last/next/lerp pos %s/%s/%s, f = %4.2f\n", cent->gent->script_targetname, vtos(current), vtos(next), vtos(cent->lerpOrigin), f);
			}
			*/
			return; //FIXME: should this be outside this if?
		}
	}
	else
	{
		if (cent->currentState.apos.trType == TR_INTERPOLATE)
		{
			EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, cent->lerpAngles);
		}
		if (cent->currentState.pos.trType == TR_INTERPOLATE)
		{
			EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
			/*
			if(cent->gent && cent->currentState.client_num != 0 )
			{
				Com_Printf("%s last/next/lerp pos %s, f = 1.0\n", cent->gent->script_targetname, vtos(cent->lerpOrigin) );
			}
			*/
			return;
		}
	}

	// FIXME: if it's blocked, it wigs out, draws it in a predicted spot, but never
	// makes it there - we need to predict it in the right place if this is happens...

	// just use the current frame and evaluate as best we can
	const trajectory_t* pos_data = &cent->currentState.pos;
	{
		const gentity_t* ent = &g_entities[cent->currentState.number];

		if (ent && ent->inuse)
		{
			if (ent->s.eFlags & EF_BLOCKED_MOVER || ent->s.pos.trType == TR_STATIONARY)
			{
				//this mover has stopped moving and is going to wig out if we predict it
				//based on last frame's info- cut across the network and use the currentOrigin
				VectorCopy(ent->currentOrigin, cent->lerpOrigin);
				pos_data = nullptr;
			}
			else
			{
				pos_data = &ent->s.pos;
			}
		}
	}

	if (pos_data)
	{
		EvaluateTrajectory(pos_data, cg.time, cent->lerpOrigin);
	}

	// FIXME: this will stomp an apos trType of TR_INTERPOLATE!!
	EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	// adjust for riding a mover
	CG_AdjustPositionForMover(cent->lerpOrigin, cent->currentState.groundentity_num, cg.time, cent->lerpOrigin);
	/*
	Ghoul2 Insert Start
	*/
	// now the nasty stuff - this will interpolate all ghoul2 models bone angle overrides per model attached to this cent
	/*
	if (cent->gent->ghoul2.size())
	{
		LerpBoneAngleOverrides(cent);
	}
	*/
	/*
	Ghoul2 Insert End
	*/
	// FIXME: perform general error decay?
}
#else
void CG_CalcEntityLerpPositions(centity_t* cent)
{
	if (cent->currentState.number == cg.snap->ps.client_num)
	{
		// if the player, take position from prediction
		VectorCopy(cg.predicted_player_state.origin, cent->lerpOrigin);
		VectorCopy(cg.predicted_player_state.viewangles, cent->lerpAngles);
		OutputDebugString(va("b=(%6.2f,%6.2f,%6.2f)\n", cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2]));
		return;
	}

	if (cent->currentState.number != cg.snap->ps.client_num && cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE)
	{
		if (cent->interpolate)
		{
			OutputDebugString(va("[%3d] interp %4.2f t=%6d  st = %6d  nst = %6d     b=%6.2f   nb=%6.2f\n",
				cent->currentState.number,
				cg.frameInterpolation,
				cg.time,
				cg.snap->serverTime,
				cg.nextSnap->serverTime,
				cent->currentState.pos.trBase[0],
				cent->nextState.pos.trBase[0]));
		}
		else
		{
			OutputDebugString(va("[%3d] nonext %4.2f t=%6d  st = %6d  nst = %6d     b=%6.2f   nb=%6.2f\n",
				cent->currentState.number,
				cg.frameInterpolation,
				cg.time,
				cg.snap->serverTime,
				0,
				cent->currentState.pos.trBase[0],
				0.0f));
		}
	}

	//FIXME: prediction on clients in timescale results in jerky positional translation
	if (cent->interpolate &&
		(cent->currentState.number == cg.snap->ps.client_num ||
			cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE))
	{
		vec3_t		current, next;
		float		f;

		// it would be an internal error to find an entity that interpolates without
		// a snapshot ahead of the current one
		if (cg.nextSnap == nullptr)
		{
			CG_Error("CG_AddCEntity: cg.nextSnap == NULL");
		}

		f = cg.frameInterpolation;

		EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current);
		EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime, next);

		cent->lerpAngles[0] = LerpAngle(current[0], next[0], f);
		cent->lerpAngles[1] = LerpAngle(current[1], next[1], f);
		cent->lerpAngles[2] = LerpAngle(current[2], next[2], f);

		EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current);
		EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, next);

		cent->lerpOrigin[0] = current[0] + f * (next[0] - current[0]);
		cent->lerpOrigin[1] = current[1] + f * (next[1] - current[1]);
		cent->lerpOrigin[2] = current[2] + f * (next[2] - current[2]);
		return;
	}
	// just use the current frame and evaluate as best we can
	trajectory_t* posData = &cent->currentState.pos;
	{
		gentity_t* ent = &g_entities[cent->currentState.number];

		if (ent && ent->inuse)
		{
			if (ent->s.eFlags & EF_BLOCKED_MOVER || ent->s.pos.trType == TR_STATIONARY)
			{//this mover has stopped moving and is going to wig out if we predict it
				//based on last frame's info- cut across the network and use the currentOrigin
				VectorCopy(ent->currentOrigin, cent->lerpOrigin);
				posData = nullptr;
			}
			else
			{
				posData = &ent->s.pos;
				EvaluateTrajectory(&ent->s.pos, cg.time, cent->lerpOrigin);
			}
		}
		else
		{
			EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin);
		}
	}
	EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	// adjust for riding a mover
	CG_AdjustPositionForMover(cent->lerpOrigin, cent->currentState.groundentity_num, cg.time, cent->lerpOrigin);
}
#endif
/*
===============
CG_AddLocalSet
===============
*/

static void CG_AddLocalSet(centity_t* cent)
{
	cent->gent->setTime = cgi_S_AddLocalSet(cent->gent->soundSet, cg.refdef.vieworg, cent->lerpOrigin,
		cent->gent->s.number, cent->gent->setTime);
}

/*
-------------------------
CAS_GetBModelSound
-------------------------
*/

sfxHandle_t CAS_GetBModelSound(const char* name, const int stage)
{
	return cgi_AS_GetBModelSound(name, stage);
}

void CG_DLightThink(const centity_t* cent)
{
	if (cent->gent)
	{
		const float t_delta = cg.time - cent->gent->painDebounceTime;
		float percentage = t_delta / cent->gent->speed;
		vec3_t org;
		vec4_t current_rgba{};
		const gentity_t* owner;
		int i;

		if (percentage >= 1.0f)
		{
			//We hit the end
			percentage = 1.0f;
			switch (cent->gent->pushDebounceTime)
			{
			case 0: //Fading from start to final
				if (cent->gent->spawnflags & 8)
				{
					//PULSER
					if (t_delta - cent->gent->speed - cent->gent->wait >= 0)
					{
						//Time to start fading down
						cent->gent->painDebounceTime = cg.time;
						cent->gent->pushDebounceTime = 1;
						percentage = 0.0f;
					}
				}
				else
				{
					//Stick on startRGBA
					percentage = 0.0f;
				}
				break;
			case 1: //Fading from final to start
				if (t_delta - cent->gent->speed - cent->gent->radius >= 0)
				{
					//Time to start fading up
					cent->gent->painDebounceTime = cg.time;
					cent->gent->pushDebounceTime = 0;
					percentage = 0.0f;
				}
				break;
			case 2: //Fading from 0 intensity to start intensity
				//Time to start fading from start to final
				cent->gent->painDebounceTime = cg.time;
				cent->gent->pushDebounceTime = 0;
				percentage = 0.0f;
				break;
			case 3: //Fading from current intensity to 0 intensity
				//Time to turn off
				cent->gent->misc_dlight_active = qfalse;
				cent->gent->e_clThinkFunc = clThinkF_NULL;
				cent->gent->s.eType = ET_GENERAL;
				cent->gent->svFlags &= ~SVF_BROADCAST;
				return;
			default:
				break;
			}
		}

		switch (cent->gent->pushDebounceTime)
		{
		case 0: //Fading from start to final
			for (i = 0; i < 4; i++)
			{
				current_rgba[i] = cent->gent->startRGBA[i] + (cent->gent->finalRGBA[i] - cent->gent->startRGBA[i]) *
					percentage;
			}
			break;
		case 1: //Fading from final to start
			for (i = 0; i < 4; i++)
			{
				current_rgba[i] = cent->gent->finalRGBA[i] + (cent->gent->startRGBA[i] - cent->gent->finalRGBA[i]) *
					percentage;
			}
			break;
		case 2: //Fading from 0 intensity to start
			for (i = 0; i < 3; i++)
			{
				current_rgba[i] = cent->gent->startRGBA[i];
			}
			current_rgba[3] = cent->gent->startRGBA[3] * percentage;
			break;
		case 3: //Fading from current intensity to 0
			for (i = 0; i < 3; i++)
			{
				//FIXME: use last
				current_rgba[i] = cent->gent->startRGBA[i];
			}
			current_rgba[3] = cent->gent->startRGBA[3] - cent->gent->startRGBA[3] * percentage;
			break;
		default:
			return;
		}

		if (cent->gent->owner)
		{
			owner = cent->gent->owner;
		}
		else
		{
			owner = cent->gent;
		}

		if (owner->s.pos.trType == TR_INTERPOLATE)
		{
			VectorCopy(cg_entities[owner->s.number].lerpOrigin, org);
		}
		else
		{
			VectorCopy(owner->currentOrigin, org);
		}

		cgi_R_AddLightToScene(org, current_rgba[3] * 10, current_rgba[0], current_rgba[1], current_rgba[2]);
	}
}

void CG_Limb(const centity_t* cent)
{
	//first time we're drawn, remove us from the owner ent
	if (cent->gent && cent->gent->owner && cent->gent->owner->ghoul2.size())
	{
		gentity_t* owner = cent->gent->owner;
		if (cent->gent->aimDebounceTime)
		{
			//done with dismemberment, just waiting to mark owner dismemberable again
			if (cent->gent->aimDebounceTime > cg.time)
			{
				//still waiting
				return;
			}
			//done!
			owner->client->dismembered = false;
			//done!
			cent->gent->e_clThinkFunc = clThinkF_NULL;
		}
		else
		{
			extern cvar_t* g_saberRealisticCombat;

			if (cent->gent->target) //stubTagName )
			{
				//add smoke to cap surf, spawn effect
				if (cent->gent->delay <= cg.time)
				{
					//debounced so it only plays effect once every 50ms
					const class_t npc_class = cent->gent->owner->client->NPC_class;

					if (npc_class != CLASS_ATST && npc_class != CLASS_GONK &&
						npc_class != CLASS_INTERROGATOR && npc_class != CLASS_MARK1 &&
						npc_class != CLASS_MARK2 && npc_class != CLASS_MOUSE &&
						npc_class != CLASS_PROBE && npc_class != CLASS_PROTOCOL &&
						npc_class != CLASS_R2D2 && npc_class != CLASS_R5D2 &&
						npc_class != CLASS_SEEKER && npc_class != CLASS_SENTRY &&
						npc_class != CLASS_SBD && npc_class != CLASS_BATTLEDROID &&
						npc_class != CLASS_DROIDEKA && npc_class != CLASS_OBJECT &&
						npc_class != CLASS_ASSASSIN_DROID && npc_class != CLASS_SABER_DROID)
					{
						//Only the humanoids bleed
						const int new_bolt = gi.G2API_AddBolt(&owner->ghoul2[owner->playerModel], cent->gent->target);
						if (new_bolt != -1)
						{
							cent->gent->delay = cg.time + 1500;
							CG_PlayEffectBolted("saber/limb_bolton", owner->playerModel, new_bolt, owner->s.number, owner->s.origin); //ent origin used to make FX culling work
						}
					}
					else
					{
						//Only the humanoids bleed
						const int new_bolt = gi.G2API_AddBolt(&owner->ghoul2[owner->playerModel], cent->gent->target);
						if (new_bolt != -1)
						{
							cent->gent->delay = cg.time + 2500;
							CG_PlayEffectBolted("env/small_fire", owner->playerModel, new_bolt, owner->s.number, owner->s.origin); //ent origin used to make FX culling work
						}
					}
				}
			}

			if (cent->gent->target2) //limbName
			{
				//turn the limb off
				//NOTE: MUST use G2SURFACEFLAG_NODESCENDANTS
				gi.G2API_SetSurfaceOnOff(&owner->ghoul2[owner->playerModel], cent->gent->target2, 0x00000100);
				//G2SURFACEFLAG_NODESCENDANTS
			}
			if (cent->gent->target3) //stubCapName )
			{
				//turn on caps
				gi.G2API_SetSurfaceOnOff(&owner->ghoul2[owner->playerModel], cent->gent->target3, 0);
			}
			if (owner->weaponModel[0] > 0)
			{
				//the corpse hasn't dropped their weapon
				if (cent->gent->count == BOTH_DISMEMBER_RARM || cent->gent->count == BOTH_DISMEMBER_TORSO1)
				{
					//FIXME: is this first check needed with this lower one?
					gi.G2API_RemoveGhoul2Model(owner->ghoul2, owner->weaponModel[0]);
					owner->weaponModel[0] = -1;
				}
			}
			if (owner->client->NPC_class == CLASS_PROTOCOL || g_saberRealisticCombat->integer)
			{
				//wait 100ms before allowing owner to be dismembered again
				cent->gent->aimDebounceTime = cg.time + 100;
				return;
			}
			//done!
			cent->gent->e_clThinkFunc = clThinkF_NULL;
		}
	}
}

extern Vehicle_t* G_IsRidingVehicle(const gentity_t* p_ent);
qboolean MatrixMode = qfalse;
qboolean SaberlockCamMode = qfalse;
extern cvar_t* g_skippingcin;

void CG_MatrixEffect(const centity_t* cent)
{
	float MATRIX_EFFECT_TIME = 1000.0f;

	if (cent->currentState.bolt_info & MEF_MULTI_SPIN)
	{
		//multiple spins
		if (cent->currentState.time2 > 0)
		{
			//with a custom amount of time per spin
			MATRIX_EFFECT_TIME = cent->currentState.time2;
		}
	}
	else
	{
		if (cent->currentState.eventParm
			&& cent->currentState.eventParm != MATRIX_EFFECT_TIME)
		{
			//not a falling multi-spin, so stretch out the effect over the whole desired length (or: condenses it, too)
			MATRIX_EFFECT_TIME = cent->currentState.eventParm;
		}
	}
	const float total_elapsed_time = static_cast<float>(cg.time - cent->currentState.time);
	float elapsed_time = total_elapsed_time;
	bool stop_effect = total_elapsed_time > cent->currentState.eventParm || cg.missionStatusShow || in_camera;

	if (!stop_effect && cent->currentState.bolt_info & MEF_HIT_GROUND_STOP && g_entities[cent->currentState.
		otherentity_num].client)
	{
		if (g_entities[cent->currentState.otherentity_num].client->ps.groundentity_num != ENTITYNUM_NONE)
		{
			stop_effect = true;
		}
		else if (g_entities[cent->currentState.otherentity_num].client->NPC_class == CLASS_VEHICLE)
		{
			const Vehicle_t* p_veh = g_entities[cent->currentState.otherentity_num].m_pVehicle;
			if (p_veh && !(p_veh->m_ulFlags & VEH_FLYING))
			{
				stop_effect = true;
			}
		}
	}
	if (!stop_effect && cent->currentState.bolt_info & MEF_LOOK_AT_ENEMY)
	{
		if (!g_entities[cent->currentState.otherentity_num].lastEnemy ||
			!g_entities[cent->currentState.otherentity_num].lastEnemy->inuse)
		{
			stop_effect = true;
		}
	}

	if (stop_effect)
	{
		//time is up or this is a falling spin and they hit the ground or mission end screen is up
		cg.overrides.active &= ~(CG_OVERRIDE_3RD_PERSON_RNG | CG_OVERRIDE_3RD_PERSON_ANG | CG_OVERRIDE_3RD_PERSON_POF);
		cg.overrides.thirdPersonHorzOffset = 0;
		cg.overrides.thirdPersonAngle = 0;
		cg.overrides.thirdPersonPitchOffset = 0;
		cg.overrides.thirdPersonRange = 0;

		if (g_skippingcin->integer)
		{
			//skipping?  don't mess with timescale
		}
		else
		{
			//set it back to 1
			cgi_Cvar_Set("timescale", "1.0");
		}
		MatrixMode = qfalse;
		cent->gent->e_clThinkFunc = clThinkF_NULL;
		cent->gent->e_ThinkFunc = thinkF_G_FreeEntity;
		cent->gent->nextthink = cg.time + 500;
		return;
	}
	while (elapsed_time > MATRIX_EFFECT_TIME)
	{
		elapsed_time -= MATRIX_EFFECT_TIME;
	}

	MatrixMode = qtrue;

	if (cent->currentState.bolt_info & MEF_LOOK_AT_ENEMY)
	{
		vec3_t to_enemy;
		vec3_t to_enemy_angles;

		VectorCopy(cg_entities[g_entities[cent->currentState.otherentity_num].lastEnemy->s.number].lerpOrigin, to_enemy);
		VectorSubtract(cg_entities[cent->currentState.otherentity_num].lerpOrigin, to_enemy, to_enemy);
		vectoangles(to_enemy, to_enemy_angles);

		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
		cg.overrides.thirdPersonAngle =
			to_enemy_angles[1] -
			cg_entities[cent->currentState.otherentity_num].lerpAngles[1] +
			145.0f;
		cg.overrides.thirdPersonAngle = AngleNormalize180(cg.overrides.thirdPersonAngle);

		const float MATRIX_EFFECT_TIME_HALF = MATRIX_EFFECT_TIME / 2.0f;
		float X = 1.0f;
		if (elapsed_time > MATRIX_EFFECT_TIME_HALF)
		{
			X -= (elapsed_time - MATRIX_EFFECT_TIME_HALF) / MATRIX_EFFECT_TIME_HALF;
		}
		cg.overrides.thirdPersonAngle *= X;
		cg.overrides.thirdPersonPitchOffset = 0.0f;
		cg.overrides.thirdPersonRange = cg_thirdPersonRange.value * 3.0f;
	}

	if (!(cent->currentState.bolt_info & MEF_NO_SPIN))
	{
		//rotate around them
		//rotate
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
		cg.overrides.thirdPersonAngle = 360.0f * elapsed_time / MATRIX_EFFECT_TIME;

		if (cent->currentState.bolt_info & MEF_REVERSE_SPIN)
		{
			cg.overrides.thirdPersonAngle *= -1;
		}
	}

	//do all the slowdown and vert bob stuff
	if (cent->currentState.angles2[0])
	{
		cgi_Cvar_Set("timescale", va("%4.2f", cent->currentState.angles2[0]));
	}
	else if (!(cent->currentState.bolt_info & MEF_NO_TIMESCALE))
	{
		//ramp the timescale
		//slowdown
		float timescale = elapsed_time / MATRIX_EFFECT_TIME;
		if (timescale < 0.01f)
		{
			timescale = 0.01f;
		}
		cgi_Cvar_Set("timescale", va("%4.2f", timescale));
	}
	else
	{
		//FIXME: MEF_HIT_GROUND_STOP: if they're on the ground, stop spinning and stop timescale
	}

	if (!(cent->currentState.bolt_info & MEF_NO_VERTBOB))
	{
		//bob the pitch
		//pitch
		//dip - FIXME: use pitchOffet?
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_POF;
		cg.overrides.thirdPersonPitchOffset = cg_thirdPersonPitchOffset.value;
		if (elapsed_time < MATRIX_EFFECT_TIME * 0.33f)
		{
			cg.overrides.thirdPersonPitchOffset -= 30.0f * elapsed_time / (MATRIX_EFFECT_TIME * 0.33);
		}
		else if (elapsed_time > MATRIX_EFFECT_TIME * 0.66f)
		{
			cg.overrides.thirdPersonPitchOffset -= 30.0f * (MATRIX_EFFECT_TIME - elapsed_time) / (MATRIX_EFFECT_TIME *
				0.33);
		}
		else
		{
			cg.overrides.thirdPersonPitchOffset -= 30.0f;
		}
	}

	if (!(cent->currentState.bolt_info & MEF_NO_RANGEVAR))
	{
		//vary the camera range
		//pull back
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
		cg.overrides.thirdPersonRange = cg_thirdPersonRange.value;
		if (elapsed_time < MATRIX_EFFECT_TIME * 0.33)
		{
			cg.overrides.thirdPersonRange += 80.0f * elapsed_time / (MATRIX_EFFECT_TIME * 0.33);
		}
		else if (elapsed_time > MATRIX_EFFECT_TIME * 0.66)
		{
			cg.overrides.thirdPersonRange += 80.0f * (MATRIX_EFFECT_TIME - elapsed_time) / (MATRIX_EFFECT_TIME * 0.33);
		}
		else
		{
			cg.overrides.thirdPersonRange += 80.0f;
		}
	}
}

void CG_StasisEffect(const centity_t* cent)
{
	float STASIS_EFFECT_TIME = 75.0f;

	if (cent->currentState.bolt_info & MEF_MULTI_SPIN)
	{
		//multiple spins
		if (cent->currentState.time2 > 0)
		{
			//with a custom amount of time per spin
			STASIS_EFFECT_TIME = cent->currentState.time2;
		}
	}
	else
	{
		if (cent->currentState.eventParm
			&& cent->currentState.eventParm != STASIS_EFFECT_TIME)
		{
			//not a falling multi-spin, so stretch out the effect over the whole desired length (or: condenses it, too)
			STASIS_EFFECT_TIME = cent->currentState.eventParm;
		}
	}
	const float total_elapsed_time = static_cast<float>(cg.time - cent->currentState.time);
	float elapsed_time = total_elapsed_time;
	bool stop_effect = total_elapsed_time > cent->currentState.eventParm || cg.missionStatusShow || in_camera;

	if (!stop_effect && cent->currentState.bolt_info & MEF_HIT_GROUND_STOP && g_entities[cent->currentState.
		otherentity_num].client)
	{
		if (g_entities[cent->currentState.otherentity_num].client->ps.groundentity_num != ENTITYNUM_NONE)
		{
			stop_effect = true;
		}
		else if (g_entities[cent->currentState.otherentity_num].client->NPC_class == CLASS_VEHICLE)
		{
			const Vehicle_t* p_veh = g_entities[cent->currentState.otherentity_num].m_pVehicle;
			if (p_veh && !(p_veh->m_ulFlags & VEH_FLYING))
			{
				stop_effect = true;
			}
		}
	}
	if (!stop_effect && cent->currentState.bolt_info & MEF_LOOK_AT_ENEMY)
	{
		if (!g_entities[cent->currentState.otherentity_num].lastEnemy ||
			!g_entities[cent->currentState.otherentity_num].lastEnemy->inuse)
		{
			stop_effect = true;
		}
	}

	if (stop_effect)
	{
		//time is up or this is a falling spin and they hit the ground or mission end screen is up
		cg.overrides.active &= ~(CG_OVERRIDE_3RD_PERSON_RNG | CG_OVERRIDE_3RD_PERSON_ANG | CG_OVERRIDE_3RD_PERSON_POF);
		cg.overrides.thirdPersonHorzOffset = 0;
		cg.overrides.thirdPersonAngle = 0;
		cg.overrides.thirdPersonPitchOffset = 0;
		cg.overrides.thirdPersonRange = 0;

		if (g_skippingcin->integer)
		{
			//skipping?  don't mess with timescale
		}
		else
		{
			//set it back to 1
			cgi_Cvar_Set("timescale", "1.0");
		}
		MatrixMode = qfalse;
		cent->gent->e_clThinkFunc = clThinkF_NULL;
		cent->gent->e_ThinkFunc = thinkF_G_FreeEntity;
		cent->gent->nextthink = cg.time + 500;
		return;
	}
	while (elapsed_time > STASIS_EFFECT_TIME)
	{
		elapsed_time -= STASIS_EFFECT_TIME;
	}

	MatrixMode = qtrue;

	if (cent->currentState.bolt_info & MEF_LOOK_AT_ENEMY)
	{
		vec3_t to_enemy;
		vec3_t to_enemy_angles;

		VectorCopy(cg_entities[g_entities[cent->currentState.otherentity_num].lastEnemy->s.number].lerpOrigin, to_enemy);
		VectorSubtract(cg_entities[cent->currentState.otherentity_num].lerpOrigin, to_enemy, to_enemy);
		vectoangles(to_enemy, to_enemy_angles);

		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
		cg.overrides.thirdPersonAngle =
			to_enemy_angles[1] -
			cg_entities[cent->currentState.otherentity_num].lerpAngles[1] +
			145.0f;
		cg.overrides.thirdPersonAngle = AngleNormalize180(cg.overrides.thirdPersonAngle);

		const float STASIS_EFFECT_TIME_HALF = STASIS_EFFECT_TIME / 2.0f;
		float X = 1.0f;
		if (elapsed_time > STASIS_EFFECT_TIME_HALF)
		{
			X -= (elapsed_time - STASIS_EFFECT_TIME_HALF) / STASIS_EFFECT_TIME_HALF;
		}
		cg.overrides.thirdPersonAngle *= X;
		cg.overrides.thirdPersonPitchOffset = 0.0f;
		cg.overrides.thirdPersonRange = cg_thirdPersonRange.value * 3.0f;
	}

	//do all the slowdown and vert bob stuff
	if (cent->currentState.angles2[0])
	{
		cgi_Cvar_Set("timescale", va("%4.2f", cent->currentState.angles2[0]));
	}
	else if (!(cent->currentState.bolt_info & MEF_NO_TIMESCALE))
	{
		//ramp the timescale
		//slowdown
		float timescale = elapsed_time / STASIS_EFFECT_TIME;
		if (timescale < 0.01f)
		{
			timescale = 0.01f;
		}
		cgi_Cvar_Set("timescale", va("%4.2f", timescale));
	}
	else
	{
		//FIXME: MEF_HIT_GROUND_STOP: if they're on the ground, stop spinning and stop timescale
	}
}

static void CG_Think(const centity_t* cent)
{
	if (!cent->gent)
	{
		return;
	}

	CEntity_ThinkFunc(cent); //	cent->gent->clThink(cent);
}

static void CG_Clouds(const centity_t* cent)
{
	refEntity_t ent = {};

	VectorCopy(cent->lerpOrigin, ent.origin);

	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;

	ent.radius = cent->gent->radius;
	ent.backlerp = cent->gent->wait;

	ent.reType = RT_CLOUDS;

	if (cent->gent->spawnflags & 1) // TUBE type, the one with a hole in the middle
	{
		ent.rotation = cent->gent->random;
		ent.renderfx = RF_GROW; // tube flag
	}

	if (cent->gent->spawnflags & 2) // ALT type, uses a different shader
	{
		ent.customShader = cgi_R_RegisterShader("gfx/world/haze2");
	}
	else
	{
		ent.customShader = cgi_R_RegisterShader("gfx/world/haze");
	}

	cgi_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity(centity_t* cent)
{
	// event-only entities will have been dealt with already
	if (cent->currentState.eType >= ET_EVENTS)
	{
		return;
	}

	//we must have restarted the game
	if (!cent->gent)
	{
		return;
	}

	cent->snapShotTime = cg.time;

	// calculate the current origin
	CG_CalcEntityLerpPositions(cent);

	// add automatic effects
	CG_EntityEffects(cent);

	// add local sound set if any
	if (cent->gent && cent->gent->soundSet && cent->gent->soundSet[0] && cent->currentState.eType != ET_MOVER)
	{
		CG_AddLocalSet(cent);
	}
	/*
	Ghoul2 Insert Start
	*/
	// do this before we copy the data to refEnts
	if (cent->gent->ghoul2.IsValid())
	{
		trap_G2_SetGhoul2ModelIndexes(cent->gent->ghoul2, cgs.model_draw, cgs.skins);
	}

	/*
	Ghoul2 Insert End
	*/

	switch (cent->currentState.eType)
	{
	default:
		CG_Error("Bad entity type: %i\n", cent->currentState.eType);
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
	case ET_TERRAIN:
		break;
	case ET_GENERAL:
		CG_General(cent);
		break;
	case ET_PLAYER:
		CG_Player(cent);
		break;
	case ET_ITEM:
		CG_Item(cent);
		break;
	case ET_MISSILE:
		CG_Missile(cent);
		break;
	case ET_MOVER:
		CG_Mover(cent);
		break;
	case ET_BEAM:
		CG_Beam(cent, 0);
		break;
	case ET_PORTAL:
		CG_Portal(cent);
		break;
	case ET_SPEAKER:
		if (cent->gent && cent->gent->soundSet && cent->gent->soundSet[0])
		{
			break;
		}
		CG_Speaker(cent);
		break;
	case ET_THINKER:
		CG_General(cent);
		CG_Think(cent);
		break;
	case ET_CLOUD: // dumb
		CG_Clouds(cent);
		break;
	case ET_GRAPPLE:
		CG_Grapple(cent);
		break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities(const qboolean is_portal)
{
	int num;
	centity_t* cent;

	if (is_portal)
	{
		for (num = 0; num < cg.snap->numEntities; num++)
		{
			cent = &cg_entities[cg.snap->entities[num].number];

			if (cent->currentState.isPortalEnt)
			{
				CG_AddCEntity(cent);
			}
		}
		return;
	}

	if (cg.nextSnap)
	{
		const int delta = cg.nextSnap->serverTime - cg.snap->serverTime;
		if (delta == 0)
		{
			cg.frameInterpolation = 0;
		}
		else
		{
			cg.frameInterpolation = static_cast<float>(cg.time - cg.snap->serverTime) / delta;
		}
	}
	else
	{
		cg.frameInterpolation = 0;
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 2047) * 360 / 2048.0f;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis(cg.autoAngles, cg.autoAxis);
	AnglesToAxis(cg.autoAnglesFast, cg.autoAxisFast);

	// Reset radar entities
	cg.radarEntityCount = 0;

	// generate and add the entity from the playerstate
	playerState_t* ps = &cg.predicted_player_state;

	PlayerStateToEntityState(ps, &cg_entities[ps->client_num].currentState);

	// add each entity sent over by the server
	for (num = 0; num < cg.snap->numEntities; num++)
	{
		cent = &cg_entities[cg.snap->entities[num].number];

		CG_AddCEntity(cent);
	}

	for (num = 0; num < cg_numpermanents; num++)
	{
		cent = cg_permanents[num];
		if (cent->currentValid)
		{
			CG_AddCEntity(cent);
		}
	}
}

//rww - This function is not currently called. Use it as the client-side ROFF
//callback once that's implemented fully.
void CG_ROFF_NotetrackCallback(const centity_t* cent, const char* notetrack)
{
	int i = 0, r = 0, object_id;
	char type[256]{};
	char argument[512]{};
	char addl_arg[512]{};
	char err_msg[256];
	int addl_args = 0;

	if (!cent || !notetrack)
	{
		return;
	}

	//notetrack = "effect effects/explosion1.efx 0+0+64 0-0-1";

	while (notetrack[i] && notetrack[i] != ' ')
	{
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	if (notetrack[i] != ' ')
	{
		//didn't pass in a valid notetrack type, or forgot the argument for it
		return;
	}

	i++;

	while (notetrack[i] && notetrack[i] != ' ')
	{
		argument[r] = notetrack[i];
		r++;
		i++;
	}
	argument[r] = '\0';

	if (!r)
	{
		return;
	}

	if (notetrack[i] == ' ')
	{
		//additional arguments...
		addl_args = 1;

		i++;
		r = 0;
		while (notetrack[i])
		{
			addl_arg[r] = notetrack[i];
			r++;
			i++;
		}
		addl_arg[r] = '\0';
	}

	if (strcmp(type, "effect") == 0)
	{
		char t[64]{};
		vec3_t parsed_offset;
		int posoffset_gathered = 0;
		if (!addl_args)
		{
			//sprintf(errMsg, "Offset position argument for 'effect' type is invalid.");
			//goto functionend;
			VectorClear(parsed_offset);
			goto defaultoffsetposition;
		}

		i = 0;

		while (posoffset_gathered < 3)
		{
			r = 0;
			while (addl_arg[i] && addl_arg[i] != '+' && addl_arg[i] != ' ')
			{
				t[r] = addl_arg[i];
				r++;
				i++;
			}
			t[r] = '\0';
			i++;
			if (!r)
			{
				//failure..
				//sprintf(errMsg, "Offset position argument for 'effect' type is invalid.");
				//goto functionend;
				VectorClear(parsed_offset);
				i = 0;
				goto defaultoffsetposition;
			}
			parsed_offset[posoffset_gathered] = atof(t);
			posoffset_gathered++;
		}

		if (posoffset_gathered < 3)
		{
			Q_strncpyz(err_msg, "Offset position argument for 'effect' type is invalid.", sizeof err_msg);
			goto functionend;
		}

		i--;

		if (addl_arg[i] != ' ')
		{
			addl_args = 0;
		}

	defaultoffsetposition:

		object_id = theFxScheduler.RegisterEffect(argument);

		if (object_id)
		{
			vec3_t up;
			vec3_t right;
			vec3_t forward;
			vec3_t use_origin;
			vec3_t use_angles;
			if (addl_args)
			{
				vec3_t parsed_angles{};
				int angles_gathered = 0;
				//if there is an additional argument for an effect it is expected to be XANGLE-YANGLE-ZANGLE
				i++;
				while (angles_gathered < 3)
				{
					r = 0;
					while (addl_arg[i] && addl_arg[i] != '-')
					{
						t[r] = addl_arg[i];
						r++;
						i++;
					}
					t[r] = '\0';
					i++;

					if (!r)
					{
						//failed to get a new part of the vector
						angles_gathered = 0;
						break;
					}

					parsed_angles[angles_gathered] = atof(t);
					angles_gathered++;
				}

				if (angles_gathered)
				{
					VectorCopy(parsed_angles, use_angles);
				}
				else
				{
					//failed to parse angles from the extra argument provided..
					VectorCopy(cent->lerpAngles, use_angles);
				}
			}
			else
			{
				//if no constant angles, play in direction entity is facing
				VectorCopy(cent->lerpAngles, use_angles);
			}

			AngleVectors(use_angles, forward, right, up);

			VectorCopy(cent->lerpOrigin, use_origin);

			//forward
			use_origin[0] += forward[0] * parsed_offset[0];
			use_origin[1] += forward[1] * parsed_offset[0];
			use_origin[2] += forward[2] * parsed_offset[0];

			//right
			use_origin[0] += right[0] * parsed_offset[1];
			use_origin[1] += right[1] * parsed_offset[1];
			use_origin[2] += right[2] * parsed_offset[1];

			//up
			use_origin[0] += up[0] * parsed_offset[2];
			use_origin[1] += up[1] * parsed_offset[2];
			use_origin[2] += up[2] * parsed_offset[2];

			theFxScheduler.PlayEffect(object_id, use_origin, use_angles);
		}
	}
	else if (strcmp(type, "sound") == 0)
	{
		object_id = cgi_S_RegisterSound(argument);
		cgi_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_BODY, object_id);
	}
	else if (strcmp(type, "loop") == 0)
	{
		//handled server-side
		return;
	}
	//else if ...
	else
	{
		if (type[0])
		{
			Com_Printf("^3Warning: \"%s\" is an invalid ROFF notetrack function\n", type);
		}
		else
		{
			Com_Printf("^3Warning: Notetrack is missing function and/or arguments\n");
		}
	}

	return;

functionend:
	Com_Printf("^3Type-specific notetrack error: %s\n", err_msg);
}