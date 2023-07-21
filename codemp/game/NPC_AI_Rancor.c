/*
===========================================================================
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

#include "b_local.h"

extern void G_GetBoltPosition(gentity_t* self, int bolt_index, vec3_t pos, int model_index);

// These define the working combat range for these suckers
#define MIN_DISTANCE		128
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		1024
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1

void Rancor_SetBolts(const gentity_t* self)
{
	if (self && self->client)
	{
		renderInfo_t* ri = &self->client->renderInfo;

		ri->handRBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*r_hand");
		ri->handLBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*l_hand");
		ri->headBolt = trap->G2API_AddBolt(self->ghoul2, 0, "*head_eyes");
		ri->torsoBolt = trap->G2API_AddBolt(self->ghoul2, 0, "jaw_bone");
	}
}

/*
-------------------------
NPC_Rancor_Precache
-------------------------
*/
void NPC_Rancor_Precache(void)
{
	for (int i = 1; i < 3; i++)
	{
		G_SoundIndex(va("sound/chars/rancor/snort_%d.wav", i));
	}
	G_SoundIndex("sound/chars/rancor/swipehit.wav");
	G_SoundIndex("sound/chars/rancor/chomp.wav");
}

/*
-------------------------
Rancor_Idle
-------------------------
*/
void Rancor_Idle(void)
{
	NPCS.NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}
}

qboolean Rancor_CheckRoar(gentity_t* self)
{
	if (!self->wait)
	{
		//haven't ever gotten mad yet
		self->wait = 1; //do this only once
		self->client->ps.eFlags2 |= EF2_ALERTED;
		NPC_SetAnim(self, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		TIMER_Set(self, "rageTime", self->client->ps.legsTimer);
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
Rancor_Patrol
-------------------------
*/
void Rancor_Patrol(void)
{
	NPCS.NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if (UpdateGoal())
	{
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal(qtrue);
	}
	else
	{
		if (TIMER_Done(NPCS.NPC, "patrolTime"))
		{
			TIMER_Set(NPCS.NPC, "patrolTime", Q_flrand(-1.0f, 1.0f) * 5000 + 5000);
		}
	}

	if (NPC_CheckEnemyExt(qtrue) == qfalse)
	{
		Rancor_Idle();
		return;
	}
	Rancor_CheckRoar(NPCS.NPC);
	TIMER_Set(NPCS.NPC, "lookForNewEnemy", Q_irand(5000, 15000));
}

/*
-------------------------
Rancor_Move
-------------------------
*/
void Rancor_Move()
{
	if (NPCS.NPCInfo->localState != LSTATE_WAITING)
	{
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		if (!NPC_MoveToGoal(qtrue))
		{
			NPCS.NPCInfo->consecutiveBlockedMoves++;
		}
		else
		{
			NPCS.NPCInfo->consecutiveBlockedMoves = 0;
		}
		NPCS.NPCInfo->goalRadius = MAX_DISTANCE; // just get us within combat range
	}
}

//---------------------------------------------------------
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t push_dir, float strength,
	qboolean break_saber_lock);
extern void G_Dismember(const gentity_t* ent, const gentity_t* enemy, vec3_t point, int limb_type);
extern float NPC_EntRangeFromBolt(const gentity_t* targ_ent, int bolt_index);
extern int NPC_GetEntsNearBolt(int* radius_ents, float radius, int bolt_index, vec3_t bolt_org);

void Rancor_DropVictim(gentity_t* self)
{
	//FIXME: if Rancor dies, it should drop its victim.
	//FIXME: if Rancor is removed, it must remove its victim.
	if (self->activator)
	{
		if (self->activator->client)
		{
			self->activator->client->ps.eFlags2 &= ~EF2_HELD_BY_MONSTER;
			self->activator->client->ps.hasLookTarget = qfalse;
			self->activator->client->ps.lookTarget = ENTITYNUM_NONE;
			self->activator->client->ps.viewangles[ROLL] = 0;
			SetClientViewAngle(self->activator, self->activator->client->ps.viewangles);
			self->activator->r.currentAngles[PITCH] = self->activator->r.currentAngles[ROLL] = 0;
			G_SetAngles(self->activator, self->activator->r.currentAngles);
		}
		if (self->activator->health <= 0)
		{
			//if ( self->activator->s.number )
			{
				//never free player
				if (self->count == 1)
				{
					//in my hand, just drop them
					if (self->activator->client)
					{
						self->activator->client->ps.legsTimer = self->activator->client->ps.torsoTimer = 0;
						//FIXME: ragdoll?
					}
				}
				else
				{
					if (self->activator->client)
					{
						self->activator->client->ps.eFlags |= EF_NODRAW; //so his corpse doesn't drop out of me...
					}
					//G_FreeEntity( self->activator );
				}
			}
		}
		else
		{
			if (self->activator->NPC)
			{
				//start thinking again
				self->activator->NPC->nextBStateThink = level.time;
			}
			//clear their anim and let them fall
			self->activator->client->ps.legsTimer = self->activator->client->ps.torsoTimer = 0;
		}
		if (self->enemy == self->activator)
		{
			self->enemy = NULL;
		}
		self->activator = NULL;
	}
	self->count = 0; //drop him
}

void Rancor_Swing(const qboolean try_grab)
{
	int radius_ent_nums[128];
	const float radius = 88;
	const float radius_squared = radius * radius;
	vec3_t bolt_org;

	const int num_ents = NPC_GetEntsNearBolt(radius_ent_nums, radius, NPCS.NPC->client->renderInfo.handRBolt, bolt_org);

	for (int i = 0; i < num_ents; i++)
	{
		gentity_t* radius_ent = &g_entities[radius_ent_nums[i]];
		if (!radius_ent->inuse)
		{
			continue;
		}

		if (radius_ent == NPCS.NPC)
		{
			//Skip the rancor ent
			continue;
		}

		if (radius_ent->client == NULL)
		{
			//must be a client
			continue;
		}

		if (radius_ent->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
		{
			//can't be one already being held
			continue;
		}

		if (DistanceSquared(radius_ent->r.currentOrigin, bolt_org) <= radius_squared)
		{
			if (try_grab
				&& NPCS.NPC->count != 1
				//don't have one in hand or in mouth already - FIXME: allow one in hand and any number in mouth!
				&& radius_ent->client->NPC_class != CLASS_RANCOR
				&& radius_ent->client->NPC_class != CLASS_GALAKMECH
				&& radius_ent->client->NPC_class != CLASS_ATST
				&& radius_ent->client->NPC_class != CLASS_GONK
				&& radius_ent->client->NPC_class != CLASS_R2D2
				&& radius_ent->client->NPC_class != CLASS_R5D2
				&& radius_ent->client->NPC_class != CLASS_MARK1
				&& radius_ent->client->NPC_class != CLASS_MARK2
				&& radius_ent->client->NPC_class != CLASS_MOUSE
				&& radius_ent->client->NPC_class != CLASS_PROBE
				&& radius_ent->client->NPC_class != CLASS_SEEKER
				&& radius_ent->client->NPC_class != CLASS_REMOTE
				&& radius_ent->client->NPC_class != CLASS_SENTRY
				&& radius_ent->client->NPC_class != CLASS_INTERROGATOR
				&& radius_ent->client->NPC_class != CLASS_DROIDEKA
				&& radius_ent->client->NPC_class != CLASS_VEHICLE)
			{
				//grab
				if (NPCS.NPC->count == 2)
				{
					//have one in my mouth, remove him
					TIMER_Remove(NPCS.NPC, "clearGrabbed");
					Rancor_DropVictim(NPCS.NPC);
				}
				NPCS.NPC->enemy = radius_ent; //make him my new best friend
				radius_ent->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;
				//FIXME: this makes it so that the victim can't hit us with shots!  Just use activator or something
				radius_ent->client->ps.hasLookTarget = qtrue;
				radius_ent->client->ps.lookTarget = NPCS.NPC->s.number;
				NPCS.NPC->activator = radius_ent; //remember him
				NPCS.NPC->count = 1; //in my hand
				//wait to attack
				TIMER_Set(NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer + Q_irand(500, 2500));
				if (radius_ent->health > 0 && radius_ent->pain)
				{
					//do pain on enemy
					radius_ent->pain(radius_ent, NPCS.NPC, 100);
					//GEntity_PainFunc( radius_ent, NPC, NPC, radius_ent->r.currentOrigin, 0, MOD_CRUSH );
				}
				else if (radius_ent->client)
				{
					radius_ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
					radius_ent->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(radius_ent, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else
			{
				//smack
				vec3_t push_dir;
				vec3_t angs;

				G_Sound(radius_ent, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/swipehit.wav"));
				VectorCopy(NPCS.NPC->client->ps.viewangles, angs);
				angs[YAW] += flrand(25, 50);
				angs[PITCH] = flrand(-25, -15);
				AngleVectors(angs, push_dir, NULL, NULL);
				if (radius_ent->client->NPC_class != CLASS_RANCOR
					&& radius_ent->client->NPC_class != CLASS_ATST)
				{
					G_Damage(radius_ent, NPCS.NPC, NPCS.NPC, vec3_origin, radius_ent->r.currentOrigin, Q_irand(25, 40),
						DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);
					g_throw(radius_ent, push_dir, 250);
					if (radius_ent->health > 0)
					{
						//do pain on enemy
						G_Knockdown(radius_ent, NPCS.NPC, push_dir, 100, qtrue);
					}
				}
			}
		}
	}
}

void Rancor_Smash(void)
{
	int radius_ent_nums[128];
	const float radius = 128;
	const float half_rad_squared = radius / 2 * (radius / 2);
	const float radius_squared = radius * radius;
	vec3_t bolt_org;

	AddSoundEvent(NPCS.NPC, NPCS.NPC->r.currentOrigin, 512, AEL_DANGER, qfalse, qtrue);

	const int num_ents = NPC_GetEntsNearBolt(radius_ent_nums, radius, NPCS.NPC->client->renderInfo.handLBolt, bolt_org);

	for (int i = 0; i < num_ents; i++)
	{
		gentity_t* radius_ent = &g_entities[radius_ent_nums[i]];
		if (!radius_ent->inuse)
		{
			continue;
		}

		if (radius_ent == NPCS.NPC)
		{
			//Skip the rancor ent
			continue;
		}

		if (radius_ent->client == NULL)
		{
			//must be a client
			continue;
		}

		if (radius_ent->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
		{
			//can't be one being held
			continue;
		}

		const float dist_sq = DistanceSquared(radius_ent->r.currentOrigin, bolt_org);
		if (dist_sq <= radius_squared)
		{
			G_Sound(radius_ent, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/swipehit.wav"));
			if (dist_sq < half_rad_squared)
			{
				//close enough to do damage, too
				G_Damage(radius_ent, NPCS.NPC, NPCS.NPC, vec3_origin, radius_ent->r.currentOrigin, Q_irand(10, 25),
					DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);
			}
			if (radius_ent->health > 0
				&& radius_ent->client
				&& radius_ent->client->NPC_class != CLASS_RANCOR
				&& radius_ent->client->NPC_class != CLASS_ATST)
			{
				if (dist_sq < half_rad_squared
					|| radius_ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//within range of my fist or withing ground-shaking range and not in the air
					G_Knockdown(radius_ent, NPCS.NPC, vec3_origin, 100, qtrue);
				}
			}
		}
	}
}

void Rancor_Bite(void)
{
	int radius_ent_nums[128];
	const float radius = 100;
	const float radius_squared = radius * radius;
	vec3_t bolt_org;

	const int num_ents = NPC_GetEntsNearBolt(radius_ent_nums, radius, NPCS.NPC->client->renderInfo.crotchBolt, bolt_org);
	//was gutBolt?

	for (int i = 0; i < num_ents; i++)
	{
		gentity_t* radius_ent = &g_entities[radius_ent_nums[i]];
		if (!radius_ent->inuse)
		{
			continue;
		}

		if (radius_ent == NPCS.NPC)
		{
			//Skip the rancor ent
			continue;
		}

		if (radius_ent->client == NULL)
		{
			//must be a client
			continue;
		}

		if (radius_ent->client->ps.eFlags2 & EF2_HELD_BY_MONSTER)
		{
			//can't be one already being held
			continue;
		}

		if (DistanceSquared(radius_ent->r.currentOrigin, bolt_org) <= radius_squared)
		{
			G_Damage(radius_ent, NPCS.NPC, NPCS.NPC, vec3_origin, radius_ent->r.currentOrigin, Q_irand(15, 30),
				DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);
			if (radius_ent->health <= 0 && radius_ent->client)
			{
				//killed them, chance of dismembering
				if (!Q_irand(0, 1))
				{
					//bite something off
					const int hit_loc = Q_irand(G2_MODELPART_HEAD, G2_MODELPART_RLEG);
					if (hit_loc == G2_MODELPART_HEAD)
					{
						NPC_SetAnim(radius_ent, SETANIM_BOTH, BOTH_DEATH17, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (hit_loc == G2_MODELPART_WAIST)
					{
						NPC_SetAnim(radius_ent, SETANIM_BOTH, BOTH_DEATHBACKWARD2,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					G_Dismember(radius_ent, NPCS.NPC, radius_ent->r.currentOrigin, hit_loc);
				}
			}
			G_Sound(radius_ent, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/chomp.wav"));
		}
	}
}

//------------------------------
extern void TossClientItems(gentity_t* self);

void Rancor_Attack(const float distance, const qboolean do_charge)
{
	if (!TIMER_Exists(NPCS.NPC, "attacking"))
	{
		if (NPCS.NPC->count == 2 && NPCS.NPC->activator)
		{
		}
		else if (NPCS.NPC->count == 1 && NPCS.NPC->activator)
		{
			//holding enemy
			if (NPCS.NPC->activator->health > 0 && Q_irand(0, 1))
			{
				//quick bite
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attack_dmg", 450);
			}
			else
			{
				//full eat
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_ATTACK3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				TIMER_Set(NPCS.NPC, "attack_dmg", 900);
				//Make victim scream in fright
				if (NPCS.NPC->activator->health > 0 && NPCS.NPC->activator->client)
				{
					G_AddEvent(NPCS.NPC->activator, Q_irand(EV_DEATH1, EV_DEATH3), 0);
					NPC_SetAnim(NPCS.NPC->activator, SETANIM_TORSO, BOTH_FALLDEATH1,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					if (NPCS.NPC->activator->NPC)
					{
						//no more thinking for you
						TossClientItems(NPCS.NPC);
						NPCS.NPC->activator->NPC->nextBStateThink = Q3_INFINITE;
					}
				}
			}
		}
		else if (NPCS.NPC->enemy->health > 0 && do_charge)
		{
			//charge
			vec3_t fwd, yaw_ang;
			VectorSet(yaw_ang, 0, NPCS.NPC->client->ps.viewangles[YAW], 0);
			AngleVectors(yaw_ang, fwd, NULL, NULL);
			VectorScale(fwd, distance * 1.5f, NPCS.NPC->client->ps.velocity);
			NPCS.NPC->client->ps.velocity[2] = 150;
			NPCS.NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;

			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_MELEE2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(NPCS.NPC, "attack_dmg", 1250);
		}
		else if (!Q_irand(0, 1))
		{
			//smash
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_MELEE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(NPCS.NPC, "attack_dmg", 1000);
		}
		else
		{
			//try to grab
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_ATTACK2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			TIMER_Set(NPCS.NPC, "attack_dmg", 1000);
		}

		TIMER_Set(NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer + Q_flrand(0.0f, 1.0f) * 200);
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks

	if (TIMER_Done2(NPCS.NPC, "attack_dmg", qtrue))
	{
		vec3_t shake_pos;
		switch (NPCS.NPC->client->ps.legsAnim)
		{
		case BOTH_MELEE1:
			Rancor_Smash();
			G_GetBoltPosition(NPCS.NPC, NPCS.NPC->client->renderInfo.handLBolt, shake_pos, 0);
			G_ScreenShake(shake_pos, NULL, 4.0f, 1000, qfalse);
			//CGCam_Shake( 1.0f*playerDist/128.0f, 1000 );
			break;
		case BOTH_MELEE2:
			Rancor_Bite();
			TIMER_Set(NPCS.NPC, "attack_dmg2", 450);
			break;
		case BOTH_ATTACK1:
			if (NPCS.NPC->count == 1 && NPCS.NPC->activator)
			{
				G_Damage(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC, vec3_origin, NPCS.NPC->activator->r.currentOrigin,
					Q_irand(25, 40), DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK, MOD_MELEE);
				if (NPCS.NPC->activator->health <= 0)
				{
					//killed him
					//make it look like we bit his head off
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC->activator->r.currentOrigin, G2_MODELPART_HEAD);
					//G_DoDismemberment( NPC->activator, NPC->activator->r.currentOrigin, MOD_SABER, 1000, HL_HEAD, qtrue );
					NPCS.NPC->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					NPCS.NPC->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(NPCS.NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				G_Sound(NPCS.NPC->activator, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/chomp.wav"));
			}
			break;
		case BOTH_ATTACK2:
			//try to grab
			Rancor_Swing(qtrue);
			break;
		case BOTH_ATTACK3:
			if (NPCS.NPC->count == 1 && NPCS.NPC->activator)
			{
				//cut in half
				if (NPCS.NPC->activator->client)
				{
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC->activator->r.currentOrigin,
						G2_MODELPART_WAIST);
					//G_DoDismemberment( NPC->activator, NPC->enemy->r.currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
				}
				//KILL
				G_Damage(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC, vec3_origin, NPCS.NPC->activator->r.currentOrigin,
					NPCS.NPC->enemy->health + 10,
					DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC,
					MOD_MELEE); //, HL_NONE );//
				if (NPCS.NPC->activator->client)
				{
					NPCS.NPC->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					NPCS.NPC->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(NPCS.NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				TIMER_Set(NPCS.NPC, "attack_dmg2", 1350);
				G_Sound(NPCS.NPC->activator, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/swipehit.wav"));
				G_AddEvent(NPCS.NPC->activator, EV_JUMP, NPCS.NPC->activator->health);
			}
			break;
		default:;
		}
	}
	else if (TIMER_Done2(NPCS.NPC, "attack_dmg2", qtrue))
	{
		switch (NPCS.NPC->client->ps.legsAnim)
		{
		case BOTH_MELEE1:
			break;
		case BOTH_MELEE2:
			Rancor_Bite();
			break;
		case BOTH_ATTACK1:
			break;
		case BOTH_ATTACK2:
			break;
		case BOTH_ATTACK3:
			if (NPCS.NPC->count == 1 && NPCS.NPC->activator)
			{
				//swallow victim
				G_Sound(NPCS.NPC->activator, CHAN_AUTO, G_SoundIndex("sound/chars/rancor/chomp.wav"));
				//FIXME: sometimes end up with a live one in our mouths?
				//just make sure they're dead
				if (NPCS.NPC->activator->health > 0)
				{
					//cut in half
					//NPC->activator->client->dismembered = qfalse;
					G_Dismember(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC->activator->r.currentOrigin,
						G2_MODELPART_WAIST);
					//G_DoDismemberment( NPC->activator, NPC->enemy->r.currentOrigin, MOD_SABER, 1000, HL_WAIST, qtrue );
					//KILL
					G_Damage(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC, vec3_origin, NPCS.NPC->activator->r.currentOrigin,
						NPCS.NPC->enemy->health + 10,
						DAMAGE_NO_PROTECTION | DAMAGE_NO_ARMOR | DAMAGE_NO_KNOCKBACK | DAMAGE_NO_HIT_LOC,
						MOD_MELEE); //, HL_NONE );
					NPCS.NPC->activator->client->ps.forceHandExtend = HANDEXTEND_NONE;
					NPCS.NPC->activator->client->ps.forceHandExtendTime = 0;
					NPC_SetAnim(NPCS.NPC->activator, SETANIM_BOTH, BOTH_SWIM_IDLE1,
						SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_AddEvent(NPCS.NPC->activator, EV_JUMP, NPCS.NPC->activator->health);
				}
				if (NPCS.NPC->activator->client)
				{
					//*sigh*, can't get tags right, just remove them?
					NPCS.NPC->activator->client->ps.eFlags |= EF_NODRAW;
				}
				NPCS.NPC->count = 2;
				TIMER_Set(NPCS.NPC, "clearGrabbed", 2600);
			}
			break;
		default:;
		}
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_ATTACK2)
	{
		if (NPCS.NPC->client->ps.legsTimer >= 1200 && NPCS.NPC->client->ps.legsTimer <= 1350)
		{
			if (Q_irand(0, 2))
			{
				Rancor_Swing(qfalse);
			}
			else
			{
				Rancor_Swing(qtrue);
			}
		}
		else if (NPCS.NPC->client->ps.legsTimer >= 1100 && NPCS.NPC->client->ps.legsTimer <= 1550)
		{
			Rancor_Swing(qtrue);
		}
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2(NPCS.NPC, "attacking", qtrue);
}

//----------------------------------
void Rancor_Combat(void)
{
	if (NPCS.NPC->count)
	{
		//holding my enemy
		if (TIMER_Done2(NPCS.NPC, "takingPain", qtrue))
		{
			NPCS.NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			Rancor_Attack(0, qfalse);
		}
		NPC_UpdateAngles(qtrue, qtrue);
		return;
	}
	// If we cannot see our target or we have somewhere to go, then do that
	if (!NPC_ClearLOS4(NPCS.NPC->enemy)) //|| UpdateGoal( ))
	{
		NPCS.NPCInfo->combatMove = qtrue;
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = MIN_DISTANCE; //MAX_DISTANCE;	// just get us within combat range

		if (!NPC_MoveToGoal(qtrue))
		{
			//couldn't go after him?  Look for a new one
			TIMER_Set(NPCS.NPC, "lookForNewEnemy", 0);
			NPCS.NPCInfo->consecutiveBlockedMoves++;
		}
		else
		{
			NPCS.NPCInfo->consecutiveBlockedMoves = 0;
		}
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy(qtrue);

	{
		const float distance = Distance(NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin);
		qboolean advance = distance > NPCS.NPC->r.maxs[0] + MIN_DISTANCE ? qtrue : qfalse;
		qboolean do_charge = qfalse;

		if (advance)
		{
			//have to get closer
			vec3_t yawOnlyAngles;
			VectorSet(yawOnlyAngles, 0, NPCS.NPC->r.currentAngles[YAW], 0);
			if (NPCS.NPC->enemy->health > 0
				&& fabs(distance - 250) <= 80
				&& InFOV3(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, yawOnlyAngles, 30, 30))
			{
				if (!Q_irand(0, 9))
				{
					//go for the charge
					do_charge = qtrue;
					advance = qfalse;
				}
			}
		}

		if (advance /*|| NPCInfo->localState == LSTATE_WAITING*/ && TIMER_Done(NPCS.NPC, "attacking"))
			// waiting monsters can't attack
		{
			if (TIMER_Done2(NPCS.NPC, "takingPain", qtrue))
			{
				NPCS.NPCInfo->localState = LSTATE_CLEAR;
			}
			else
			{
				Rancor_Move();
			}
		}
		else
		{
			Rancor_Attack(distance, do_charge);
		}
	}
}

/*
-------------------------
NPC_Rancor_Pain
-------------------------
*/
void NPC_Rancor_Pain(gentity_t* self, gentity_t* attacker, const int damage)
{
	qboolean hit_by_rancor = qfalse;

	if (self->NPC && self->NPC->ignorePain)
	{
		return;
	}
	if (!TIMER_Done(self, "breathAttack"))
	{
		//nothing interrupts breath attack
		return;
	}

	TIMER_Remove(self, "confusionTime");

	if (attacker && attacker->client && attacker->client->NPC_class == CLASS_RANCOR)
	{
		hit_by_rancor = qtrue;
	}
	if (attacker
		&& attacker->inuse
		&& attacker != self->enemy
		&& !(attacker->flags & FL_NOTARGET))
	{
		if (!self->count)
		{
			if (attacker->s.number < MAX_CLIENTS && !Q_irand(0, 3)
				|| !self->enemy
				|| self->enemy->health <= 0
				|| self->enemy->client && self->enemy->client->NPC_class == CLASS_RANCOR
				|| !Q_irand(0, 4) && DistanceSquared(attacker->r.currentOrigin, self->r.currentOrigin) <
				DistanceSquared(self->enemy->r.currentOrigin, self->r.currentOrigin))
			{
				//if my enemy is dead (or attacked by player) and I'm not still holding/eating someone, turn on the attacker
				//FIXME: if can't nav to my enemy, take this guy if I can nav to him
				self->lastEnemy = self->enemy;
				G_SetEnemy(self, attacker);

				TIMER_Set(self, "lookForNewEnemy", Q_irand(5000, 15000));
				if (hit_by_rancor)
				{
					//stay mad at this Rancor for 2-5 secs before looking for attacker enemies
					TIMER_Set(self, "rancorInfight", Q_irand(2000, 5000));
				}
			}
		}
	}
	if ((hit_by_rancor || self->count == 1 && self->activator && !Q_irand(0, 4) || Q_irand(0, 200) < damage)
		//hit by rancor, hit while holding live victim, or took a lot of damage
		&& self->client->ps.legsAnim != BOTH_STAND1TO2
		&& TIMER_Done(self, "takingPain"))
	{
		if (!Rancor_CheckRoar(self))
		{
			if (self->client->ps.legsAnim != BOTH_MELEE1
				&& self->client->ps.legsAnim != BOTH_MELEE2
				&& self->client->ps.legsAnim != BOTH_ATTACK2
				&& self->client->ps.legsAnim != BOTH_ATTACK10
				&& self->client->ps.legsAnim != BOTH_ATTACK11)
			{
				//cant interrupt one of the big attack anims
				/*
				if ( self->count != 1
					|| attacker == self->activator
					|| (self->client->ps.legsAnim != BOTH_ATTACK1&&self->client->ps.legsAnim != BOTH_ATTACK3) )
				*/
				{
					//if going to bite our victim, only victim can interrupt that anim
					if (self->health > 100 || hit_by_rancor)
					{
						TIMER_Remove(self, "attacking");

						VectorCopy(self->NPC->lastPathAngles, self->s.angles);

						if (self->count == 1)
						{
							NPC_SetAnim(self, SETANIM_BOTH, BOTH_PAIN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						TIMER_Set(self, "takingPain",
							self->client->ps.legsTimer + Q_irand(0, 500 * (2 - g_npcspskill.integer)));

						if (self->NPC)
						{
							self->NPC->localState = LSTATE_WAITING;
						}
					}
				}
			}
		}
	}
}

void Rancor_CheckDropVictim(void)
{
	vec3_t mins, maxs;
	vec3_t start, end;
	trace_t trace;

	VectorSet(mins, NPCS.NPC->activator->r.mins[0] - 1, NPCS.NPC->activator->r.mins[1] - 1, 0);
	VectorSet(maxs, NPCS.NPC->activator->r.maxs[0] + 1, NPCS.NPC->activator->r.maxs[1] + 1, 1);
	VectorSet(start, NPCS.NPC->activator->r.currentOrigin[0], NPCS.NPC->activator->r.currentOrigin[1],
		NPCS.NPC->activator->r.absmin[2]);
	VectorSet(end, NPCS.NPC->activator->r.currentOrigin[0], NPCS.NPC->activator->r.currentOrigin[1],
		NPCS.NPC->activator->r.absmax[2] - 1);

	trap->Trace(&trace, start, mins, maxs, end, NPCS.NPC->activator->s.number, NPCS.NPC->activator->clipmask, qfalse, 0,
		0);
	if (!trace.allsolid && !trace.startsolid && trace.fraction >= 1.0f)
	{
		Rancor_DropVictim(NPCS.NPC);
	}
}

//if he's stepping on things then crush them -rww
void Rancor_Crush(void)
{
	if (!NPCS.NPC ||
		!NPCS.NPC->client ||
		NPCS.NPC->client->ps.groundEntityNum >= ENTITYNUM_WORLD)
	{
		//nothing to crush
		return;
	}

	gentity_t* crush = &g_entities[NPCS.NPC->client->ps.groundEntityNum];
	if (crush->inuse && crush->client && !crush->localAnimIndex)
	{
		//a humanoid, smash them good.
		G_Damage(crush, NPCS.NPC, NPCS.NPC, NULL, NPCS.NPC->r.currentOrigin, 200, 0, MOD_CRUSH);
	}
}

/*
-------------------------
NPC_BSRancor_Default
-------------------------
*/
void NPC_BSRancor_Default(void)
{
	AddSightEvent(NPCS.NPC, NPCS.NPC->r.currentOrigin, 1024, AEL_DANGER_GREAT, 50);

	Rancor_Crush();

	NPCS.NPC->client->ps.eFlags2 &= ~(EF2_USE_ALT_ANIM | EF2_GENERIC_NPC_FLAG);
	if (NPCS.NPC->count)
	{
		//holding someone
		NPCS.NPC->client->ps.eFlags2 |= EF2_USE_ALT_ANIM;
		if (NPCS.NPC->count == 2)
		{
			//in my mouth
			NPCS.NPC->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;
		}
	}
	else
	{
		NPCS.NPC->client->ps.eFlags2 &= ~(EF2_USE_ALT_ANIM | EF2_GENERIC_NPC_FLAG);
	}

	if (TIMER_Done2(NPCS.NPC, "clearGrabbed", qtrue))
	{
		Rancor_DropVictim(NPCS.NPC);
	}
	else if (NPCS.NPC->client->ps.legsAnim == BOTH_PAIN2
		&& NPCS.NPC->count == 1
		&& NPCS.NPC->activator)
	{
		if (!Q_irand(0, 3))
		{
			Rancor_CheckDropVictim();
		}
	}
	if (!TIMER_Done(NPCS.NPC, "rageTime"))
	{
		//do nothing but roar first time we see an enemy
		AddSoundEvent(NPCS.NPC, NPCS.NPC->r.currentOrigin, 1024, AEL_DANGER_GREAT, qfalse, qfalse);
		NPC_FaceEnemy(qtrue);
		return;
	}
	if (NPCS.NPC->enemy)
	{
		if (TIMER_Done(NPCS.NPC, "angrynoise"))
		{
			G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex(va("sound/chars/rancor/misc/anger%d.wav", Q_irand(1, 3))));

			TIMER_Set(NPCS.NPC, "angrynoise", Q_irand(5000, 10000));
		}
		else
		{
			AddSoundEvent(NPCS.NPC, NPCS.NPC->r.currentOrigin, 512, AEL_DANGER_GREAT, qfalse, qfalse);
		}
		if (NPCS.NPC->count == 2 && NPCS.NPC->client->ps.legsAnim == BOTH_ATTACK3)
		{
			//we're still chewing our enemy up
			NPC_UpdateAngles(qtrue, qtrue);
			return;
		}
		//else, if he's in our hand, we eat, else if he's on the ground, we keep attacking his dead body for a while
		if (NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_RANCOR)
		{
			//got mad at another Rancor, look for a valid enemy
			if (TIMER_Done(NPCS.NPC, "rancorInfight"))
			{
				NPC_CheckEnemyExt(qtrue);
			}
		}
		else if (!NPCS.NPC->count)
		{
			if (ValidEnemy(NPCS.NPC->enemy) == qfalse)
			{
				TIMER_Remove(NPCS.NPC, "lookForNewEnemy"); //make them look again right now
				if (!NPCS.NPC->enemy->inuse || level.time - NPCS.NPC->enemy->s.time > Q_irand(10000, 15000))
				{
					//it's been a while since the enemy died, or enemy is completely gone, get bored with him
					NPCS.NPC->enemy = NULL;
					Rancor_Patrol();
					NPC_UpdateAngles(qtrue, qtrue);
					return;
				}
			}
			if (TIMER_Done(NPCS.NPC, "lookForNewEnemy"))
			{
				gentity_t* sav_enemy = NPCS.NPC->enemy; //FIXME: what about NPC->lastEnemy?
				NPCS.NPC->enemy = NULL;
				gentity_t* newEnemy = NPC_CheckEnemy(NPCS.NPCInfo->confusionTime < level.time, qfalse, qfalse);
				NPCS.NPC->enemy = sav_enemy;
				if (newEnemy && newEnemy != sav_enemy)
				{
					//picked up a new enemy!
					NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
					G_SetEnemy(NPCS.NPC, newEnemy);
					//hold this one for at least 5-15 seconds
					TIMER_Set(NPCS.NPC, "lookForNewEnemy", Q_irand(5000, 15000));
				}
				else
				{
					//look again in 2-5 secs
					TIMER_Set(NPCS.NPC, "lookForNewEnemy", Q_irand(2000, 5000));
				}
			}
		}
		Rancor_Combat();
	}
	else
	{
		if (TIMER_Done(NPCS.NPC, "idlenoise"))
		{
			G_Sound(NPCS.NPC, CHAN_AUTO, G_SoundIndex(va("sound/chars/rancor/snort_%d.wav", Q_irand(1, 2))));

			TIMER_Set(NPCS.NPC, "idlenoise", Q_irand(2000, 4000));
			AddSoundEvent(NPCS.NPC, NPCS.NPC->r.currentOrigin, 384, AEL_DANGER, qfalse, qfalse);
		}
		if (NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES)
		{
			Rancor_Patrol();
		}
		else
		{
			Rancor_Idle();
		}
	}

	NPC_UpdateAngles(qtrue, qtrue);
}