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

// Blaster Weapon

#include "cg_local.h"

/*
-------------------------
FX_BlasterProjectileThink
-------------------------
*/

void FX_BlasterProjectileThink(centity_t* cent, const struct weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID(cgs.effects.blasterShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse);
}

/*
-------------------------
FX_BlasterAltFireThink
-------------------------
*/
void FX_BlasterAltFireThink(centity_t* cent, const struct weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID(cgs.effects.blasterShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse);
}

/*
-------------------------
FX_BlasterWeaponHitWall
-------------------------
*/
void FX_BlasterWeaponHitWall(vec3_t origin, vec3_t normal)
{
	trap->FX_PlayEffectID(cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1, qfalse);
}

/*
-------------------------
FX_BlasterWeaponHitPlayer
-------------------------
*/
void FX_BlasterWeaponHitPlayer(vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		trap->FX_PlayEffectID(cgs.effects.blasterFleshImpactEffect, origin, normal, -1, -1, qfalse);
	}
	else
	{
		trap->FX_PlayEffectID(cgs.effects.blasterDroidImpactEffect, origin, normal, -1, -1, qfalse);
	}
}

/////////////// eweb

/*
-------------------------
FX_ewebProjectileThink
-------------------------
*/

void FX_EwebProjectileThink(centity_t* cent, const struct weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID(cgs.effects.ewebShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse);
}

/*
-------------------------
FX_ewebAltFireThink
-------------------------
*/
void FX_EwebAltFireThink(centity_t* cent, const struct weaponInfo_s* weapon)
{
	vec3_t forward;

	if (VectorNormalize2(cent->currentState.pos.trDelta, forward) == 0.0f)
	{
		forward[2] = 1.0f;
	}

	trap->FX_PlayEffectID(cgs.effects.ewebShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse);
}

/*
-------------------------
FX_ewebWeaponHitWall
-------------------------
*/
void FX_EwebWeaponHitWall(vec3_t origin, vec3_t normal)
{
	trap->FX_PlayEffectID(cgs.effects.ewebWallImpactEffect, origin, normal, -1, -1, qfalse);
}

/*
-------------------------
FX_ewebWeaponHitPlayer
-------------------------
*/
void FX_EwebWeaponHitPlayer(vec3_t origin, vec3_t normal, const qboolean humanoid)
{
	if (humanoid)
	{
		trap->FX_PlayEffectID(cgs.effects.ewebFleshImpactEffect, origin, normal, -1, -1, qfalse);
	}
	else
	{
		trap->FX_PlayEffectID(cgs.effects.ewebDroidImpactEffect, origin, normal, -1, -1, qfalse);
	}
}