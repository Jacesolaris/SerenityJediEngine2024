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

/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///
///																																///
///																																///
///													SERENITY JEDI ENGINE														///
///										          LIGHTSABER COMBAT SYSTEM													    ///
///																																///
///						      System designed by Serenity and modded by JaceSolaris. (c) 2023 SJE   		                    ///
///								    https://www.moddb.com/mods/serenityjediengine-20											///
///																																///
/// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ///

//NPC_senses.cpp

#include "b_local.h"
#include "../cgame/cg_local.h"
#include "g_navigator.h"
#ifdef _DEBUG
#include <float.h>
#endif

extern int eventClearTime;
/*
qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask)

returns true if can see from point 1 to 2, even through glass (1 pane)- doesn't work with portals
*/
qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, const int ignore, const int clipmask)
{
	trace_t tr;

	gi.trace(&tr, point1, nullptr, nullptr, point2, ignore, clipmask, static_cast<EG2_Collision>(0), 0);
	if (tr.fraction == 1.0)
	{
		return qtrue;
	}

	const gentity_t* hit = &g_entities[tr.entityNum];
	if (EntIsGlass(hit))
	{
		vec3_t newpoint1;
		VectorCopy(tr.endpos, newpoint1);
		gi.trace(&tr, newpoint1, nullptr, nullptr, point2, hit->s.number, clipmask, static_cast<EG2_Collision>(0), 0);

		if (tr.fraction == 1.0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
CanSee
determine if NPC can see an entity

This is a straight line trace check.  This function does not look at PVS or FOV,
or take any AI related factors (for example, the NPC's reaction time) into account

FIXME do we need fat and thin version of this?
*/
qboolean CanSee(const gentity_t* ent)
{
	trace_t tr;
	vec3_t eyes;
	vec3_t spot;

	CalcEntitySpot(NPC, SPOT_HEAD_LEAN, eyes);

	CalcEntitySpot(ent, SPOT_ORIGIN, spot);
	gi.trace(&tr, eyes, nullptr, nullptr, spot, NPC->s.number, MASK_OPAQUE, static_cast<EG2_Collision>(0), 0);
	ShotThroughGlass(&tr, ent, spot, MASK_OPAQUE);
	if (tr.fraction == 1.0)
	{
		return qtrue;
	}

	CalcEntitySpot(ent, SPOT_HEAD, spot);
	gi.trace(&tr, eyes, nullptr, nullptr, spot, NPC->s.number, MASK_OPAQUE, static_cast<EG2_Collision>(0), 0);
	ShotThroughGlass(&tr, ent, spot, MASK_OPAQUE);
	if (tr.fraction == 1.0)
	{
		return qtrue;
	}

	CalcEntitySpot(ent, SPOT_LEGS, spot);
	gi.trace(&tr, eyes, nullptr, nullptr, spot, NPC->s.number, MASK_OPAQUE, static_cast<EG2_Collision>(0), 0);
	ShotThroughGlass(&tr, ent, spot, MASK_OPAQUE);
	if (tr.fraction == 1.0)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean in_front(vec3_t spot, vec3_t from, vec3_t from_angles, const float thresh_hold = 0.0f)
{
	vec3_t dir, forward, angles;

	VectorSubtract(spot, from, dir);
	dir[2] = 0;
	VectorNormalize(dir);

	VectorCopy(from_angles, angles);
	angles[0] = 0;
	AngleVectors(angles, forward, nullptr, nullptr);

	const float dot = DotProduct(dir, forward);

	return static_cast<qboolean>(dot > thresh_hold);
}

float DotToSpot(vec3_t spot, vec3_t from, vec3_t fromAngles)
{
	vec3_t dir, forward, angles;

	VectorSubtract(spot, from, dir);
	dir[2] = 0;
	VectorNormalize(dir);

	VectorCopy(fromAngles, angles);
	angles[0] = 0;
	AngleVectors(angles, forward, nullptr, nullptr);

	const float dot = DotProduct(dir, forward);

	return dot;
}

/*
InFOV

IDEA: further off to side of FOV range, higher chance of failing even if technically in FOV,
	keep core of 50% to sides as always succeeding
*/

//Position compares

qboolean InFOV(vec3_t spot, vec3_t from, vec3_t fromAngles, const int hFOV, const int vFOV)
{
	vec3_t deltaVector, angles, deltaAngles;

	VectorSubtract(spot, from, deltaVector);
	vectoangles(deltaVector, angles);

	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);

	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	return qfalse;
}

//NPC to position

qboolean InFOV(vec3_t origin, const gentity_t* from, const int hFOV, const int vFOV)
{
	vec3_t fromAngles, eyes;

	if (from->client)
	{
		VectorCopy(from->client->ps.viewangles, fromAngles);
	}
	else
	{
		VectorCopy(from->s.angles, fromAngles);
	}

	CalcEntitySpot(from, SPOT_HEAD, eyes);

	return InFOV(origin, eyes, fromAngles, hFOV, vFOV);
}

//Entity to entity
qboolean InFOVFromPlayerView(const gentity_t* ent, const int hFOV, const int vFOV)
{
	vec3_t eyes;
	vec3_t spot;
	vec3_t deltaVector;
	vec3_t angles, fromAngles;
	vec3_t deltaAngles{};

	if (!player || !player->client)
	{
		return qfalse;
	}
	if (cg.time)
	{
		VectorCopy(cg.refdefViewAngles, fromAngles);
	}
	else
	{
		VectorCopy(player->client->ps.viewangles, fromAngles);
	}

	if (cg.time)
	{
		VectorCopy(cg.refdef.vieworg, eyes);
	}
	else
	{
		CalcEntitySpot(player, SPOT_HEAD_LEAN, eyes);
	}

	CalcEntitySpot(ent, SPOT_ORIGIN, spot);
	VectorSubtract(spot, eyes, deltaVector);

	vectoangles(deltaVector, angles);
	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);
	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	CalcEntitySpot(ent, SPOT_HEAD, spot);
	VectorSubtract(spot, eyes, deltaVector);
	vectoangles(deltaVector, angles);
	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);
	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	CalcEntitySpot(ent, SPOT_LEGS, spot);
	VectorSubtract(spot, eyes, deltaVector);
	vectoangles(deltaVector, angles);
	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);
	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean InFOV(const gentity_t* ent, const gentity_t* from, const int hFOV, const int vFOV)
{
	vec3_t eyes;
	vec3_t spot;
	vec3_t deltaVector;
	vec3_t angles, fromAngles;
	vec3_t deltaAngles{};

	if (from->client)
	{
		if (from->client->NPC_class != CLASS_RANCOR
			&& from->client->NPC_class != CLASS_WAMPA
			&& !VectorCompare(from->client->renderInfo.eyeAngles, vec3_origin))
		{
			//Actual facing of tag_head!
			//NOTE: Stasis aliens may have a problem with this?
			VectorCopy(from->client->renderInfo.eyeAngles, fromAngles);
		}
		else
		{
			VectorCopy(from->client->ps.viewangles, fromAngles);
		}
	}
	else
	{
		VectorCopy(from->s.angles, fromAngles);
	}

	CalcEntitySpot(from, SPOT_HEAD_LEAN, eyes);

	CalcEntitySpot(ent, SPOT_ORIGIN, spot);
	VectorSubtract(spot, eyes, deltaVector);

	vectoangles(deltaVector, angles);
	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);
	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	CalcEntitySpot(ent, SPOT_HEAD, spot);
	VectorSubtract(spot, eyes, deltaVector);
	vectoangles(deltaVector, angles);
	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);
	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	CalcEntitySpot(ent, SPOT_LEGS, spot);
	VectorSubtract(spot, eyes, deltaVector);
	vectoangles(deltaVector, angles);
	deltaAngles[PITCH] = AngleDelta(fromAngles[PITCH], angles[PITCH]);
	deltaAngles[YAW] = AngleDelta(fromAngles[YAW], angles[YAW]);
	if (fabs(deltaAngles[PITCH]) <= vFOV && fabs(deltaAngles[YAW]) <= hFOV)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean InVisrange(const gentity_t* ent)
{
	//FIXME: make a calculate visibility for ents that takes into account
	//lighting, movement, turning, crouch/stand up, other anims, hide brushes, etc.
	vec3_t eyes;
	vec3_t spot;
	vec3_t deltaVector;
	const float visrange = NPCInfo->stats.visrange * NPCInfo->stats.visrange * 2;

	CalcEntitySpot(NPC, SPOT_HEAD_LEAN, eyes);

	CalcEntitySpot(ent, SPOT_ORIGIN, spot);
	VectorSubtract(spot, eyes, deltaVector);

	if (VectorLengthSquared(deltaVector) > visrange)
	{
		return qfalse;
	}

	return qtrue;
}

/*
NPC_CheckVisibility
*/

visibility_t NPC_CheckVisibility(const gentity_t* ent, const int flags)
{
	// flags should never be 0
	if (!flags)
	{
		return VIS_NOT;
	}

	// check PVS
	if (flags & CHECK_PVS)
	{
		if (!gi.inPVS(ent->currentOrigin, NPC->currentOrigin))
		{
			return VIS_NOT;
		}
	}
	if (!(flags & (CHECK_360 | CHECK_FOV | CHECK_SHOOT)))
	{
		return VIS_PVS;
	}

	// check within visrange
	if (flags & CHECK_VISRANGE)
	{
		if (!InVisrange(ent))
		{
			return VIS_PVS;
		}
	}

	// check 360 degree visibility
	//Meaning has to be a direct line of site
	if (flags & CHECK_360)
	{
		if (!CanSee(ent))
		{
			return VIS_PVS;
		}
	}
	if (!(flags & (CHECK_FOV | CHECK_SHOOT)))
	{
		return VIS_360;
	}

	// check FOV
	if (flags & CHECK_FOV)
	{
		if (!InFOV(ent, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov))
		{
			return VIS_360;
		}
	}

	if (!(flags & CHECK_SHOOT))
	{
		return VIS_FOV;
	}

	// check shootability
	if (flags & CHECK_SHOOT)
	{
		if (!CanShoot(ent, NPC))
		{
			return VIS_FOV;
		}
	}

	return VIS_SHOOT;
}

/*
-------------------------
NPC_CheckSoundEvents
-------------------------
*/
static int G_CheckSoundEvents(gentity_t* self, float max_hear_dist, const int ignoreAlert, const qboolean mustHaveOwner,
	const int minAlertLevel, const qboolean onGroundOnly)
{
	int best_event = -1;
	int best_alert = -1;
	int best_time = -1;

	max_hear_dist *= max_hear_dist;

	for (int i = 0; i < level.numAlertEvents; i++)
	{
		//are we purposely ignoring this alert?
		if (level.alertEvents[i].ID == ignoreAlert)
			continue;
		//We're only concerned about sounds
		if (level.alertEvents[i].type != AET_SOUND)
			continue;
		//must be at least this noticable
		if (level.alertEvents[i].level < minAlertLevel)
			continue;
		//must have an owner?
		if (mustHaveOwner && !level.alertEvents[i].owner)
			continue;
		//must be on the ground?
		if (onGroundOnly && !level.alertEvents[i].onGround)
			continue;

		//Must be within range
		const float dist = DistanceSquared(level.alertEvents[i].position, self->currentOrigin);

		//can't hear it
		if (dist > max_hear_dist)
			continue;

		if (self->client && self->client->NPC_class != CLASS_SAND_CREATURE)
		{
			//sand creatures hear all in within their earshot, regardless of quietness and alert sound radius!
			const float radius = level.alertEvents[i].radius * level.alertEvents[i].radius;
			if (dist > radius)
				continue;

			if (level.alertEvents[i].addLight)
			{
				//a quiet sound, must have LOS to hear it
				if (G_ClearLOS(self, level.alertEvents[i].position) == qfalse)
				{
					//no LOS, didn't hear it
					continue;
				}
			}
		}

		//See if this one takes precedence over the previous one
		if (level.alertEvents[i].level >= best_alert //higher alert level
			|| level.alertEvents[i].level == best_alert && level.alertEvents[i].timestamp >= best_time)
			//same alert level, but this one is newer
		{
			//NOTE: equal is better because it's later in the array
			best_event = i;
			best_alert = level.alertEvents[i].level;
			best_time = level.alertEvents[i].timestamp;
		}
	}

	return best_event;
}

float G_GetLightLevel(vec3_t pos, vec3_t from_dir)
{
	vec3_t ambient = { 0 }, directed, lightDir;

	cgi_R_GetLighting(pos, ambient, directed, lightDir);

	const float light_level = VectorLength(ambient) + VectorLength(directed) * DotProduct(lightDir, from_dir);

	return light_level;
}

/*
-------------------------
NPC_CheckSightEvents
-------------------------
*/
static int G_CheckSightEvents(gentity_t* self, const int hFOV, const int vFOV, float max_see_dist,
	const int ignoreAlert,
	const qboolean mustHaveOwner, const int minAlertLevel)
{
	int best_event = -1;
	int best_alert = -1;
	int best_time = -1;

	max_see_dist *= max_see_dist;
	for (int i = 0; i < level.numAlertEvents; i++)
	{
		//are we purposely ignoring this alert?
		if (level.alertEvents[i].ID == ignoreAlert)
			continue;
		//We're only concerned about sounds
		if (level.alertEvents[i].type != AET_SIGHT)
			continue;
		//must be at least this noticable
		if (level.alertEvents[i].level < minAlertLevel)
			continue;
		//must have an owner?
		if (mustHaveOwner && !level.alertEvents[i].owner)
			continue;

		//Must be within range
		const float dist = DistanceSquared(level.alertEvents[i].position, self->currentOrigin);

		//can't see it
		if (dist > max_see_dist)
			continue;

		const float radius = level.alertEvents[i].radius * level.alertEvents[i].radius;
		if (dist > radius)
			continue;

		//Must be visible
		if (InFOV(level.alertEvents[i].position, self, hFOV, vFOV) == qfalse)
			continue;

		if (G_ClearLOS(self, level.alertEvents[i].position) == qfalse)
			continue;

		//FIXME: possibly have the light level at this point affect the
		//			visibility/alert level of this event?  Would also
		//			need to take into account how bright the event
		//			itself is.  A lightsaber would stand out more
		//			in the dark... maybe pass in a light level that
		//			is added to the actual light level at this position?

		//See if this one takes precedence over the previous one
		if (level.alertEvents[i].level >= best_alert //higher alert level
			|| level.alertEvents[i].level == best_alert && level.alertEvents[i].timestamp >= best_time)
			//same alert level, but this one is newer
		{
			//NOTE: equal is better because it's later in the array
			best_event = i;
			best_alert = level.alertEvents[i].level;
			best_time = level.alertEvents[i].timestamp;
		}
	}

	return best_event;
}

qboolean G_RememberAlertEvent(gentity_t* self, const int alertIndex)
{
	if (!self || !self->NPC)
	{
		//not a valid ent
		return qfalse;
	}

	if (alertIndex == -1)
	{
		//not a valid event
		return qfalse;
	}

	// Get The Event Struct
	//----------------------
	const alertEvent_t& at = level.alertEvents[alertIndex];

	if (at.ID == self->NPC->lastAlertID)
	{
		//already know this one
		return qfalse;
	}

	if (at.owner == self)
	{
		//don't care about events that I made
		return qfalse;
	}

	self->NPC->lastAlertID = at.ID;

	// Now, If It Is Dangerous Enough, We Want To Register This With The Pathfinding System
	//--------------------------------------------------------------------------------------
	const bool is_dangerous = at.level >= AEL_DANGER;
	const bool is_from_npc = at.owner && at.owner->client;
	const bool is_from_enemy = is_from_npc && at.owner->client->playerTeam != self->client->playerTeam;

	if (is_dangerous && (!is_from_npc || is_from_enemy))
	{
		NAV::RegisterDangerSense(self, alertIndex);
	}

	return qtrue;
}

/*
-------------------------
NPC_CheckAlertEvents

	NOTE: Should all NPCs create alertEvents too so they can detect each other?
-------------------------
*/

int G_CheckAlertEvents(gentity_t* self, const qboolean checkSight, const qboolean checkSound, const float maxSeeDist,
	const float maxHearDist,
	const int ignoreAlert, const qboolean mustHaveOwner, const int minAlertLevel,
	const qboolean onGroundOnly)
{
	if (g_entities[0].health <= 0)
	{
		//player is dead
		return -1;
	}

	int best_sound_event = -1;
	int best_sound_alert = -1;
	int best_sight_alert = -1;

	if (checkSound)
	{
		//get sound event
		best_sound_event = G_CheckSoundEvents(self, maxHearDist, ignoreAlert, mustHaveOwner, minAlertLevel,
			onGroundOnly);
		//get sound event alert level
		if (best_sound_event >= 0)
		{
			best_sound_alert = level.alertEvents[best_sound_event].level;
		}
	}

	if (checkSight)
	{
		int best_sight_event;
		//get sight event
		if (self->NPC)
		{
			best_sight_event = G_CheckSightEvents(self, self->NPC->stats.hfov, self->NPC->stats.vfov, maxSeeDist,
				ignoreAlert, mustHaveOwner, minAlertLevel);
		}
		else
		{
			best_sight_event = G_CheckSightEvents(self, 80, 80, maxSeeDist, ignoreAlert, mustHaveOwner, minAlertLevel);
			//FIXME: look at cg_view to get more accurate numbers?
		}
		//get sight event alert level
		if (best_sight_event >= 0)
		{
			best_sight_alert = level.alertEvents[best_sight_event].level;
		}

		//return the one that has a higher alert (or sound if equal)
		//FIXME:	This doesn't take the distance of the event into account

		if (best_sight_event >= 0 && best_sight_alert > best_sound_alert)
		{
			//valid best sight event, more important than the sound event
			//get the light level of the alert event for this checker
			vec3_t eyePoint, sightDir;
			//get eye point
			CalcEntitySpot(self, SPOT_HEAD_LEAN, eyePoint);
			VectorSubtract(level.alertEvents[best_sight_event].position, eyePoint, sightDir);
			level.alertEvents[best_sight_event].light = level.alertEvents[best_sight_event].addLight + G_GetLightLevel(
				level.alertEvents[best_sight_event].position, sightDir);
			//return the sight event
			if (G_RememberAlertEvent(self, best_sight_event))
			{
				return best_sight_event;
			}
		}
	}
	//return the sound event
	if (G_RememberAlertEvent(self, best_sound_event))
	{
		return best_sound_event;
	}
	//no event or no new event
	return -1;
}

int NPC_CheckAlertEvents(const qboolean checkSight, const qboolean checkSound, const int ignoreAlert,
	const qboolean mustHaveOwner,
	const int minAlertLevel, const qboolean onGroundOnly)
{
	return G_CheckAlertEvents(NPC, checkSight, checkSound, NPCInfo->stats.visrange, NPCInfo->stats.earshot, ignoreAlert,
		mustHaveOwner, minAlertLevel, onGroundOnly);
}

extern void WP_ForcePowerStop(gentity_t* self, forcePowers_t force_power);

qboolean G_CheckForDanger(const gentity_t* self, const int alert_event)
{
	//FIXME: more bStates need to call this?
	if (alert_event == -1)
	{
		return qfalse;
	}

	if (level.alertEvents[alert_event].level >= AEL_DANGER)
	{
		//run away!
		if (!level.alertEvents[alert_event].owner || !level.alertEvents[alert_event].owner->client || level.alertEvents[
			alert_event].owner != self && level.alertEvents[alert_event].owner->client->playerTeam != self->client->
				playerTeam)
		{
			if (self->NPC)
			{
				if (self->NPC->scriptFlags & SCF_DONT_FLEE)
				{
					//can't flee
					return qfalse;
				}
				if (level.alertEvents[alert_event].level >= AEL_DANGER_GREAT || self->s.weapon == WP_NONE || self->s.
					weapon == WP_MELEE)
				{
					//flee for a longer period of time
					NPC_StartFlee(level.alertEvents[alert_event].owner, level.alertEvents[alert_event].position,
						level.alertEvents[alert_event].level, 3000, 6000);
				}
				else if (!Q_irand(0, 10)) //FIXME: base on rank?  aggression?
				{
					//just normal danger and I have a weapon, so just a 25% chance of fleeing only for a few seconds
					//FIXME: used to just find a better combat point, need that functionality back
					NPC_StartFlee(level.alertEvents[alert_event].owner, level.alertEvents[alert_event].position,
						level.alertEvents[alert_event].level, 1000, 3000);
				}
				else
				{
					//didn't flee
					TIMER_Set(NPC, "duck", 2000); // something dangerous going on...
					return qfalse;
				}
				return qtrue;
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean NPC_CheckForDanger(const int alert_event)
{
	//FIXME: more bStates need to call this?
	return G_CheckForDanger(NPC, alert_event);
}

/*
-------------------------
AddSoundEvent
-------------------------
*/
qboolean RemoveOldestAlert();

void AddSoundEvent(gentity_t* owner, vec3_t position, const float radius, const alertEventLevel_e alertLevel, const qboolean needLOS, const qboolean onGround)
{
	if (level.numAlertEvents >= MAX_ALERT_EVENTS)
	{
		if (!RemoveOldestAlert())
		{
			//how could that fail?
			return;
		}
	}

	if (owner == nullptr && alertLevel < AEL_DANGER) //allows un-owned danger alerts
		return;

	if (owner && owner->client && owner->client->NPC_class == CLASS_SAND_CREATURE)
	{
		return;
	}

	//FIXME: if owner is not a player or player ally, and there are no player allies present,
	//			perhaps we don't need to store the alert... unless we want the player to
	//			react to enemy alert events in some way?

#ifdef _DEBUG
	assert(!Q_isnan(position[0]) && !Q_isnan(position[1]) && !Q_isnan(position[2]));
#endif
	VectorCopy(position, level.alertEvents[level.numAlertEvents].position);

	level.alertEvents[level.numAlertEvents].radius = radius;
	level.alertEvents[level.numAlertEvents].level = alertLevel;
	level.alertEvents[level.numAlertEvents].type = AET_SOUND;
	level.alertEvents[level.numAlertEvents].owner = owner;
	if (needLOS)
	{
		//a very low-level sound, when check this sound event, check for LOS
		level.alertEvents[level.numAlertEvents].addLight = 1; //will force an LOS trace on this sound
	}
	else
	{
		level.alertEvents[level.numAlertEvents].addLight = 0; //will force an LOS trace on this sound
	}
	level.alertEvents[level.numAlertEvents].onGround = onGround;
	level.alertEvents[level.numAlertEvents].ID = ++level.curAlertID;
	level.alertEvents[level.numAlertEvents].timestamp = level.time;
	level.numAlertEvents++;
}

/*
-------------------------
AddSightEvent
-------------------------
*/

void AddSightEvent(gentity_t* owner, vec3_t position, const float radius, const alertEventLevel_e alertLevel,
	const float addLight)
{
	//FIXME: Handle this in another manner?
	if (level.numAlertEvents >= MAX_ALERT_EVENTS)
	{
		if (!RemoveOldestAlert())
		{
			//how could that fail?
			return;
		}
	}

	if (owner == nullptr && alertLevel < AEL_DANGER) //allows un-owned danger alerts
		return;

	//FIXME: if owner is not a player or player ally, and there are no player allies present,
	//			perhaps we don't need to store the alert... unless we want the player to
	//			react to enemy alert events in some way?

#ifdef _DEBUG
	assert(!Q_isnan(position[0]) && !Q_isnan(position[1]) && !Q_isnan(position[2]));
#endif
	VectorCopy(position, level.alertEvents[level.numAlertEvents].position);

	level.alertEvents[level.numAlertEvents].radius = radius;
	level.alertEvents[level.numAlertEvents].level = alertLevel;
	level.alertEvents[level.numAlertEvents].type = AET_SIGHT;
	level.alertEvents[level.numAlertEvents].owner = owner;
	level.alertEvents[level.numAlertEvents].addLight = addLight;
	//will get added to actual light at that point when it's checked
	level.alertEvents[level.numAlertEvents].ID = level.curAlertID++;
	level.alertEvents[level.numAlertEvents].timestamp = level.time;

	level.numAlertEvents++;
}

/*
-------------------------
ClearPlayerAlertEvents
-------------------------
*/

void ClearPlayerAlertEvents()
{
	const int curNumAlerts = level.numAlertEvents;
	//loop through them all (max 32)
	for (int i = 0; i < curNumAlerts; i++)
	{
		//see if the event is old enough to delete
		if (level.alertEvents[i].timestamp && level.alertEvents[i].timestamp + ALERT_CLEAR_TIME < level.time)
		{
			//this event has timed out
			//drop the count
			level.numAlertEvents--;
			//shift the rest down
			if (level.numAlertEvents > 0)
			{
				//still have more in the array
				if (i + 1 < MAX_ALERT_EVENTS)
				{
					memmove(&level.alertEvents[i], &level.alertEvents[i + 1],
						sizeof(alertEvent_t) * (MAX_ALERT_EVENTS - (i + 1)));
				}
			}
			else
			{
				//just clear this one... or should we clear the whole array?
				memset(&level.alertEvents[i], 0, sizeof(alertEvent_t));
			}
		}
	}
	//make sure this never drops below zero... if it does, something very very bad happened
	assert(level.numAlertEvents >= 0);

	if (eventClearTime < level.time)
	{
		//this is just a 200ms debouncer so things that generate constant alerts (like corpses and missiles) add an alert every 200 ms
		eventClearTime = level.time + ALERT_CLEAR_TIME;
	}
}

qboolean RemoveOldestAlert()
{
	int oldestEvent = -1, oldestTime = Q3_INFINITE;
	//loop through them all (max 32)
	for (int i = 0; i < level.numAlertEvents; i++)
	{
		//see if the event is old enough to delete
		if (level.alertEvents[i].timestamp < oldestTime)
		{
			oldestEvent = i;
			oldestTime = level.alertEvents[i].timestamp;
		}
	}
	if (oldestEvent != -1)
	{
		//drop the count
		level.numAlertEvents--;
		//shift the rest down
		if (level.numAlertEvents > 0)
		{
			//still have more in the array
			if (oldestEvent + 1 < MAX_ALERT_EVENTS)
			{
				memmove(&level.alertEvents[oldestEvent], &level.alertEvents[oldestEvent + 1],
					sizeof(alertEvent_t) * (MAX_ALERT_EVENTS - (oldestEvent + 1)));
			}
		}
		else
		{
			//just clear this one... or should we clear the whole array?
			memset(&level.alertEvents[oldestEvent], 0, sizeof(alertEvent_t));
		}
	}
	//make sure this never drops below zero... if it does, something very very bad happened
	assert(level.numAlertEvents >= 0);
	//return true is have room for one now
	return static_cast<qboolean>(level.numAlertEvents < MAX_ALERT_EVENTS);
}

/*
-------------------------
G_ClearLOS
-------------------------
*/

// Position to position
qboolean G_ClearLOS(gentity_t* self, const vec3_t start, const vec3_t end)
{
	trace_t tr;
	int traceCount = 0;

	//FIXME: ENTITYNUM_NONE ok?
	gi.trace(&tr, start, nullptr, nullptr, end, ENTITYNUM_NONE,
		CONTENTS_OPAQUE/*CONTENTS_SOLID*//*(CONTENTS_SOLID|CONTENTS_MONSTERCLIP)*/, static_cast<EG2_Collision>(0),
		0);
	while (tr.fraction < 1.0 && traceCount < 3)
	{
		//can see through 3 panes of glass
		if (tr.entityNum < ENTITYNUM_WORLD)
		{
			if (&g_entities[tr.entityNum] != nullptr && g_entities[tr.entityNum].svFlags & SVF_GLASS_BRUSH)
			{
				//can see through glass, trace again, ignoring me
				gi.trace(&tr, tr.endpos, nullptr, nullptr, end, tr.entityNum, MASK_OPAQUE,
					static_cast<EG2_Collision>(0), 0);
				traceCount++;
				continue;
			}
		}
		return qfalse;
	}

	if (tr.fraction == 1.0)
		return qtrue;

	return qfalse;
}

//Entity to position
qboolean G_ClearLOS(gentity_t* self, const gentity_t* ent, const vec3_t end)
{
	vec3_t eyes;

	CalcEntitySpot(ent, SPOT_HEAD_LEAN, eyes);

	return G_ClearLOS(self, eyes, end);
}

//Position to entity
qboolean G_ClearLOS(gentity_t* self, const vec3_t start, const gentity_t* ent)
{
	vec3_t spot;

	//Look for the chest first
	CalcEntitySpot(ent, SPOT_ORIGIN, spot);

	if (G_ClearLOS(self, start, spot))
		return qtrue;

	//Look for the head next
	CalcEntitySpot(ent, SPOT_HEAD_LEAN, spot);

	if (G_ClearLOS(self, start, spot))
		return qtrue;

	return qfalse;
}

//NPC's eyes to entity
qboolean G_ClearLOS(gentity_t* self, const gentity_t* ent)
{
	vec3_t eyes;

	//Calculate my position
	CalcEntitySpot(self, SPOT_HEAD_LEAN, eyes);

	return G_ClearLOS(self, eyes, ent);
}

//NPC's eyes to position
qboolean G_ClearLOS(gentity_t* self, const vec3_t end)
{
	vec3_t eyes;

	//Calculate the my position
	CalcEntitySpot(self, SPOT_HEAD_LEAN, eyes);

	return G_ClearLOS(self, eyes, end);
}

/*
-------------------------
NPC_GetFOVPercentage
-------------------------
*/

float NPC_GetHFOVPercentage(vec3_t spot, vec3_t from, vec3_t facing, const float hFOV)
{
	vec3_t deltaVector, angles;

	VectorSubtract(spot, from, deltaVector);

	vectoangles(deltaVector, angles);

	const float delta = fabs(AngleDelta(facing[YAW], angles[YAW]));

	if (delta > hFOV)
		return 0.0f;

	return (hFOV - delta) / hFOV;
}

/*
-------------------------
NPC_GetVFOVPercentage
-------------------------
*/

float NPC_GetVFOVPercentage(vec3_t spot, vec3_t from, vec3_t facing, const float vFOV)
{
	vec3_t deltaVector, angles;

	VectorSubtract(spot, from, deltaVector);

	vectoangles(deltaVector, angles);

	const float delta = fabs(AngleDelta(facing[PITCH], angles[PITCH]));

	if (delta > vFOV)
		return 0.0f;

	return (vFOV - delta) / vFOV;
}

constexpr auto MAX_INTEREST_DIST = 256 * 256;
/*
-------------------------
NPC_FindLocalInterestPoint
-------------------------
*/

int G_FindLocalInterestPoint(gentity_t* self)
{
	int bestPoint = ENTITYNUM_NONE;
	float bestDist = Q3_INFINITE;
	vec3_t eyes;

	CalcEntitySpot(self, SPOT_HEAD_LEAN, eyes);
	for (int i = 0; i < level.numInterestPoints; i++)
	{
		//Don't ignore portals?  If through a portal, need to look at portal!
		if (gi.inPVS(level.interestPoints[i].origin, eyes))
		{
			vec3_t diffVec;
			VectorSubtract(level.interestPoints[i].origin, eyes, diffVec);
			if ((fabs(diffVec[0]) + fabs(diffVec[1])) / 2 < 48 &&
				fabs(diffVec[2]) > (fabs(diffVec[0]) + fabs(diffVec[1])) / 2)
			{
				//Too close to look so far up or down
				continue;
			}
			const float dist = VectorLengthSquared(diffVec);
			//Some priority to more interesting points
			//dist -= ((int)level.interestPoints[i].lookMode * 5) * ((int)level.interestPoints[i].lookMode * 5);
			if (dist < MAX_INTEREST_DIST && dist < bestDist)
			{
				if (G_ClearLineOfSight(eyes, level.interestPoints[i].origin, self->s.number, MASK_OPAQUE))
				{
					bestDist = dist;
					bestPoint = i;
				}
			}
		}
	}
	if (bestPoint != ENTITYNUM_NONE && level.interestPoints[bestPoint].target)
	{
		G_UseTargets2(self, self, level.interestPoints[bestPoint].target);
	}
	return bestPoint;
}

/*QUAKED target_interest (1 0.8 0.5) (-4 -4 -4) (4 4 4)
A point that a squadmate will look at if standing still

target - thing to fire when someone looks at this thing
*/

void SP_target_interest(gentity_t* self)
{
	//FIXME: rename point_interest
	if (level.numInterestPoints >= MAX_INTEREST_POINTS)
	{
		gi.Printf("ERROR:  Too many interest points, limit is %d\n", MAX_INTEREST_POINTS);
		G_FreeEntity(self);
		return;
	}

	VectorCopy(self->currentOrigin, level.interestPoints[level.numInterestPoints].origin);

	if (self->target && self->target[0])
	{
		level.interestPoints[level.numInterestPoints].target = G_NewString(self->target);
	}

	level.numInterestPoints++;

	G_FreeEntity(self);
}