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

// Concussion Rifle Weapon

#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"

/*
---------------------------
FX_ConcProjectileThink
---------------------------
*/

void FX_ConcProjectileThink(centity_t* cent, const weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	theFxScheduler.PlayEffect("concussion/shot", cent->lerpOrigin, forward);
}

/*
---------------------------
FX_ConcHitWall
---------------------------
*/

void FX_ConcHitWall(vec3_t origin, vec3_t normal)
{
	theFxScheduler.PlayEffect("concussion/explosion", origin, normal);
}

/*
---------------------------
FX_ConcHitPlayer
---------------------------
*/

void FX_ConcHitPlayer(gentity_t* hit, vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		if (hit && hit->client && hit->ghoul2.size())
		{
			CG_AddGhoul2Mark(cgs.media.bdecal_bodyburn1, flrand(3.5, 4.0), origin, normal, hit->s.number,
				hit->client->ps.origin, hit->client->renderInfo.legsYaw, hit->ghoul2, hit->s.modelScale,
				Q_irand(10000, 13000));
		}
		theFxScheduler.PlayEffect("concussion/explosion", origin, normal);
	}
	else
	{
		theFxScheduler.PlayEffect(cgs.effects.blasterDroidImpactEffect, origin, normal);
	}
}

/*
---------------------------
FX_ConcAltShot
---------------------------
*/
static vec3_t WHITE = { 1.0f, 1.0f, 1.0f };

void FX_ConcAltShot(vec3_t start, vec3_t end)
{
	//"concussion/beam"
	FX_AddLine(-1, start, end, 0.1f, 10.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		WHITE, WHITE, 0.0f,
		675, cgi_R_RegisterShader("gfx/effects/blueLine"),
		0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR);

	vec3_t BRIGHT = { 0.75f, 0.5f, 1.0f };

	// add some beef
	FX_AddLine(-1, start, end, 0.1f, 7.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		BRIGHT, BRIGHT, 0.0f,
		650, cgi_R_RegisterShader("gfx/misc/whiteline2"),
		0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR);
}

/*
---------------------------
FX_ConcAltMiss
---------------------------
*/

void FX_ConcAltMiss(vec3_t origin, vec3_t normal)
{
	vec3_t pos, c1, c2;

	VectorMA(origin, 4.0f, normal, c1);
	VectorCopy(c1, c2);
	c1[2] += 4;
	c2[2] += 12;

	VectorAdd(origin, normal, pos);
	pos[2] += 28;

	FX_AddBezier(origin, pos, c1, vec3_origin, c2, vec3_origin, 6.0f, 6.0f, 0.0f, 0.0f, 0.2f, 0.5f, WHITE, WHITE, 0.0f,
		4000, cgi_R_RegisterShader("gfx/effects/smokeTrail"), FX_ALPHA_WAVE);

	theFxScheduler.PlayEffect("concussion/alt_miss", origin, normal);
}