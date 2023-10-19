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

// sv_game.c -- interface to the game dll

#include "../server/exe_headers.h"

#include "../qcommon/cm_local.h"

#include "server.h"
#include "../client/vmachine.h"
#include "../client/client.h"
#include "qcommon/ojk_saved_game.h"
/*#include "..\renderer\tr_local.h"
#include "..\renderer\tr_WorldEffects.h"*/
/*
Ghoul2 Insert Start
*/
#if !defined(G2_H_INC)
#include "../ghoul2/G2.h"
#endif

/*
Ghoul2 Insert End
*/

static void* gameLibrary;

//prototypes
extern void Com_WriteCam(const char* text);
extern void Com_FlushCamFile();

extern int s_entityWavVol[MAX_GENTITIES];

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
/*
int	SV_NumForGentity( gentity_t *ent ) {
	int		num;

	num = ( (byte *)ent - (byte *)ge->gentities ) / ge->gentitySize;

	return num;
}
*/
gentity_t* SV_Gentity_num(const int num)
{
	assert(num >= 0);
	const auto ent = (gentity_t*)((byte*)ge->gentities + ge->gentitySize * num);

	return ent;
}

svEntity_t* SV_SvEntityForGentity(gentity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
	}
	return &sv.svEntities[gEnt->s.number];
}

gentity_t* SV_GEntityForSvEntity(svEntity_t* svEnt)
{
	const int num = svEnt - sv.svEntities;
	return SV_Gentity_num(num);
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(const int client_num, const char* fmt, ...)
{
	char msg[8192];
	va_list argptr;

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof msg, fmt, argptr);
	va_end(argptr);

	if (client_num == -1)
	{
		SV_SendServerCommand(nullptr, "%s", msg);
	}
	else
	{
		if (client_num < 0 || client_num >= 1)
		{
			return;
		}
		SV_SendServerCommand(svs.clients + client_num, "%s", msg);
	}
}

/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(const int client_num, const char* reason)
{
	if (client_num < 0 || client_num >= 1)
	{
		return;
	}
	SV_DropClient(svs.clients + client_num, reason);
}

/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(gentity_t* ent, const char* name)
{
	clip_handle_t h;
	vec3_t mins, maxs;

	if (!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL model for ent number %d", ent->s.number);
	}

	if (name[0] == '*')
	{
		ent->s.modelindex = atoi(name + 1);

		if (sv.mLocalSubBSPIndex != -1)
		{
			ent->s.modelindex += sv.mLocalSubBSPModelOffset;
		}

		h = CM_InlineModel(ent->s.modelindex);

		if (sv.mLocalSubBSPIndex != -1)
		{
			CM_ModelBounds(SubBSP[sv.mLocalSubBSPIndex], h, mins, maxs);
		}
		else
		{
			CM_ModelBounds(cmg, h, mins, maxs);
		}

		//CM_ModelBounds( h, mins, maxs );

		VectorCopy(mins, ent->mins);
		VectorCopy(maxs, ent->maxs);
		ent->bmodel = qtrue;

		ent->contents = CM_ModelContents(h, -1);
	}
	else if (name[0] == '#')
	{
		ent->s.modelindex = CM_LoadSubBSP(va("maps/%s.bsp", name + 1), qfalse);
		CM_ModelBounds(SubBSP[CM_FindSubBSP(ent->s.modelindex)], ent->s.modelindex, mins, maxs);

		VectorCopy(mins, ent->mins);
		VectorCopy(maxs, ent->maxs);
		ent->bmodel = qtrue;

		//rwwNOTE: We don't ever want to set contents -1, it includes CONTENTS_LIGHTSABER.
		//Lots of stuff will explode if there's a brush with CONTENTS_LIGHTSABER that isn't attached to a client owner.
		//ent->contents = -1;		// we don't know exactly what is in the brushes
		h = CM_InlineModel(ent->s.modelindex);
		ent->contents = CM_ModelContents(h, CM_FindSubBSP(ent->s.modelindex));
		//	ent->contents = CONTENTS_SOLID;
	}
	else
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model (ent %d)", name, ent->s.number);
	}
}

const char* SV_SetActiveSubBSP(const int index)
{
	if (index >= 0)
	{
		sv.mLocalSubBSPIndex = CM_FindSubBSP(index);
		sv.mLocalSubBSPModelOffset = index;
		sv.mLocalSubBSPEntityParsePoint = CM_SubBSPEntityString(sv.mLocalSubBSPIndex);
		return sv.mLocalSubBSPEntityParsePoint;
	}
	sv.mLocalSubBSPIndex = -1;

	return nullptr;
}

/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS(const vec3_t p1, const vec3_t p2)
{
	int start = 0;

	if (com_speeds->integer)
	{
		start = Sys_Milliseconds();
	}
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	const int area1 = CM_LeafArea(leafnum);
	const byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	const int area2 = CM_LeafArea(leafnum);
	if (mask && !(mask[cluster >> 3] & 1 << (cluster & 7)))
	{
		if (com_speeds->integer)
		{
			timeInPVSCheck += Sys_Milliseconds() - start;
		}
		return qfalse;
	}

	if (!CM_AreasConnected(area1, area2))
	{
		timeInPVSCheck += Sys_Milliseconds() - start;
		return qfalse; // a door blocks sight
	}

	if (com_speeds->integer)
	{
		timeInPVSCheck += Sys_Milliseconds() - start;
	}
	return qtrue;
}

/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int start = 0;

	if (com_speeds->integer)
	{
		start = Sys_Milliseconds();
	}

	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	const byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);

	if (mask && !(mask[cluster >> 3] & 1 << (cluster & 7)))
	{
		if (com_speeds->integer)
		{
			timeInPVSCheck += Sys_Milliseconds() - start;
		}
		return qfalse;
	}

	if (com_speeds->integer)
	{
		timeInPVSCheck += Sys_Milliseconds() - start;
	}
	return qtrue;
}

/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(gentity_t* ent, const qboolean open)
{
#ifndef JK2_MODE
	if (!(ent->contents & CONTENTS_OPAQUE))
	{
#ifndef FINAL_BUILD
		//		Com_Printf( "INFO: entity number %d not opaque: not affecting area portal!\n", ent->s.number );
#endif
		return;
	}
#endif

	const svEntity_t* svEnt = SV_SvEntityForGentity(ent);
	if (svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}

/*
==================
SV_GameAreaEntities
==================
*/
qboolean SV_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t* gEnt)
{
	trace_t trace;

	// check for exact collision
	const float* origin = gEnt->currentOrigin;
	const float* angles = gEnt->currentAngles;

	const clip_handle_t ch = SV_clip_handleForEntity(gEnt);
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles);

	return trace.startsolid;
}

/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char* buffer, const int buffer_size)
{
	if (buffer_size < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: buffer_size == %i", buffer_size);
	}
	Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO), buffer_size);
}

qboolean SV_GetEntityToken(char* buffer, const int buffer_size)
{
	char* s;

	if (sv.mLocalSubBSPIndex == -1)
	{
		s = COM_Parse((const char**)&sv.entityParsePoint);
		Q_strncpyz(buffer, s, buffer_size);
		if (!sv.entityParsePoint && !s[0])
		{
			return qfalse;
		}
		return qtrue;
	}
	s = COM_Parse((const char**)&sv.mLocalSubBSPEntityParsePoint);
	Q_strncpyz(buffer, s, buffer_size);
	if (!sv.mLocalSubBSPEntityParsePoint && !s[0])
	{
		return qfalse;
	}
	return qtrue;
}

//==============================================

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs(qboolean shutdownCin)
{
	if (!ge)
	{
		return;
	}
	ge->Shutdown();

	SCR_StopCinematic();
	CL_ShutdownCGame(); //we have cgame burried in here.

	Sys_UnloadDll(gameLibrary);

	ge = nullptr;
	cgvm.entryPoint = nullptr;
}

// this is a compile-helper function since Z_Malloc can now become a macro with __LINE__ etc
//
static void* G_ZMalloc_Helper(const int iSize, const memtag_t eTag, const qboolean bZeroit)
{
	return Z_Malloc(iSize, eTag, bZeroit);
}

static int SV_G2API_AddBolt(CGhoul2Info* ghl_info, const char* bone_name)
{
	return re.G2API_AddBolt(ghl_info, bone_name);
}

static int SV_G2API_AddBoltSurfNum(CGhoul2Info* ghl_info, const int surf_index)
{
	return re.G2API_AddBoltSurfNum(ghl_info, surf_index);
}

static int SV_G2API_AddSurface(CGhoul2Info* ghl_info, const int surface_number, const int polyNumber,
	const float BarycentricI,
	const float BarycentricJ, const int lod)
{
	return re.G2API_AddSurface(ghl_info, surface_number, polyNumber, BarycentricI, BarycentricJ, lod);
}

static void SV_G2API_AnimateG2Models(CGhoul2Info_v& ghoul2, const int acurrent_time, CRagDollUpdateParams* params)
{
	re.G2API_AnimateG2Models(ghoul2, acurrent_time, params);
}

static qboolean SV_G2API_AttachEnt(int* bolt_info, CGhoul2Info* ghlInfoTo, const int toBoltIndex, const int ent_num,
	const int toModelNum)
{
	return re.G2API_AttachEnt(bolt_info, ghlInfoTo, toBoltIndex, ent_num, toModelNum);
}

static qboolean SV_G2API_AttachG2Model(CGhoul2Info* ghl_info, CGhoul2Info* ghlInfoTo, const int toBoltIndex,
	const int toModel)
{
	return re.G2API_AttachG2Model(ghl_info, ghlInfoTo, toBoltIndex, toModel);
}

static void SV_G2API_CleanGhoul2Models(CGhoul2Info_v& ghoul2)
{
	return re.G2API_CleanGhoul2Models(ghoul2);
}

static void SV_G2API_CollisionDetect(
	CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2, const vec3_t angles, const vec3_t position,
	const int AframeNumber, const int ent_num, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, CMiniHeap* miniHeap,
	const EG2_Collision e_g2_trace_type, const int use_lod, const float fRadius)
{
	re.G2API_CollisionDetect(collRecMap, ghoul2, angles, position, AframeNumber,
		ent_num, rayStart, rayEnd, scale, miniHeap, e_g2_trace_type, use_lod, fRadius);
}

static void SV_G2API_CopyGhoul2Instance(const CGhoul2Info_v& ghoul2_from, CGhoul2Info_v& ghoul2_to, const int model_index)
{
	re.G2API_CopyGhoul2Instance(ghoul2_from, ghoul2_to, model_index);
}

static void SV_G2API_DetachEnt(int* bolt_info)
{
	re.G2API_DetachEnt(bolt_info);
}

static qboolean SV_G2API_DetachG2Model(CGhoul2Info* ghl_info)
{
	return re.G2API_DetachG2Model(ghl_info);
}

static qboolean SV_G2API_GetAnimFileName(CGhoul2Info* ghl_info, char** filename)
{
	return re.G2API_GetAnimFileName(ghl_info, filename);
}

static char* SV_G2API_GetAnimFileNameIndex(const qhandle_t model_index)
{
	return re.G2API_GetAnimFileNameIndex(model_index);
}

static char* SV_G2API_GetAnimFileInternalNameIndex(const qhandle_t model_index)
{
	return re.G2API_GetAnimFileInternalNameIndex(model_index);
}

static int SV_G2API_GetAnimIndex(const CGhoul2Info* ghl_info)
{
	return re.G2API_GetAnimIndex(ghl_info);
}

static qboolean SV_G2API_GetAnimRange(CGhoul2Info* ghl_info, const char* bone_name, int* start_frame, int* end_frame)
{
	return re.G2API_GetAnimRange(ghl_info, bone_name, start_frame, end_frame);
}

static qboolean SV_G2API_GetAnimRangeIndex(CGhoul2Info* ghl_info, const int bone_index, int* start_frame, int* end_frame)
{
	return re.G2API_GetAnimRangeIndex(ghl_info, bone_index, start_frame, end_frame);
}

static qboolean SV_G2API_GetBoneAnim(
	CGhoul2Info* ghl_info, const char* bone_name, const int acurrent_time,
	float* current_frame, int* start_frame, int* end_frame, int* flags, float* anim_speed, int* model_list)
{
	return re.G2API_GetBoneAnim(ghl_info, bone_name, acurrent_time, current_frame,
		start_frame, end_frame, flags, anim_speed, model_list);
}

static qboolean SV_G2API_GetBoneAnimIndex(CGhoul2Info* ghl_info, const int iBoneIndex, const int acurrent_time,
	float* current_frame, int* start_frame, int* end_frame, int* flags,
	float* anim_speed, int* model_list)
{
	return re.G2API_GetBoneAnimIndex(ghl_info, iBoneIndex, acurrent_time, current_frame,
		start_frame, end_frame, flags, anim_speed, model_list);
}

static int SV_G2API_GetBoneIndex(CGhoul2Info* ghl_info, const char* bone_name, const qboolean bAddIfNotFound)
{
	return re.G2API_GetBoneIndex(ghl_info, bone_name, bAddIfNotFound);
}

static qboolean SV_G2API_GetBoltMatrix(
	CGhoul2Info_v& ghoul2, const int model_index, const int bolt_index, mdxaBone_t* matrix, const vec3_t angles,
	const vec3_t position, const int AframeNum, qhandle_t* model_list, const vec3_t scale)
{
	return re.G2API_GetBoltMatrix(ghoul2, model_index, bolt_index, matrix, angles,
		position, AframeNum, model_list, scale);
}

static int SV_G2API_GetGhoul2ModelFlags(CGhoul2Info* ghl_info)
{
	return re.G2API_GetGhoul2ModelFlags(ghl_info);
}

static char* SV_G2API_GetGLAName(CGhoul2Info* ghl_info)
{
	return re.G2API_GetGLAName(ghl_info);
}

static int SV_G2API_GetParentSurface(CGhoul2Info* ghl_info, const int index)
{
	return re.G2API_GetParentSurface(ghl_info, index);
}

static qboolean SV_G2API_GetRagBonePos(
	CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale)
{
	return re.G2API_GetRagBonePos(ghoul2, bone_name, pos, entAngles, entPos, entScale);
}

static int SV_G2API_GetSurfaceIndex(CGhoul2Info* ghl_info, const char* surface_name)
{
	return re.G2API_GetSurfaceIndex(ghl_info, surface_name);
}

static char* SV_G2API_GetSurfaceName(CGhoul2Info* ghl_info, const int surfNumber)
{
	return re.G2API_GetSurfaceName(ghl_info, surfNumber);
}

static int SV_G2API_GetSurfaceRenderStatus(CGhoul2Info* ghl_info, const char* surface_name)
{
	return re.G2API_GetSurfaceRenderStatus(ghl_info, surface_name);
}

static void SV_G2API_GiveMeVectorFromMatrix(mdxaBone_t& bolt_matrix, const Eorientations flags, vec3_t& vec)
{
	re.G2API_GiveMeVectorFromMatrix(bolt_matrix, flags, vec);
}

static qboolean SV_G2API_HaveWeGhoul2Models(const CGhoul2Info_v& ghoul2)
{
	return re.G2API_HaveWeGhoul2Models(ghoul2);
}

static qboolean SV_G2API_IKMove(CGhoul2Info_v& ghoul2, const int time, sharedIKMoveParams_t* params)
{
	return re.G2API_IKMove(ghoul2, time, params);
}

static int SV_G2API_InitGhoul2Model(CGhoul2Info_v& ghoul2, const char* file_name, const int model_index,
	const qhandle_t customSkin, const qhandle_t customShader, const int modelFlags,
	const int lodBias)
{
	return re.G2API_InitGhoul2Model(ghoul2, file_name, model_index, customSkin, customShader, modelFlags, lodBias);
}

static qboolean SV_G2API_IsPaused(CGhoul2Info* ghl_info, const char* bone_name)
{
	return re.G2API_IsPaused(ghl_info, bone_name);
}

static void SV_G2API_ListBones(CGhoul2Info* ghl_info, const int frame)
{
	return re.G2API_ListBones(ghl_info, frame);
}

static void SV_G2API_ListSurfaces(CGhoul2Info* ghl_info)
{
	return re.G2API_ListSurfaces(ghl_info);
}

static void SV_G2API_LoadGhoul2Models(CGhoul2Info_v& ghoul2, const char* buffer)
{
	return re.G2API_LoadGhoul2Models(ghoul2, buffer);
}

static void SV_G2API_LoadSaveCodeDestructGhoul2Info(CGhoul2Info_v& ghoul2)
{
	return re.G2API_LoadSaveCodeDestructGhoul2Info(ghoul2);
}

static qboolean SV_G2API_PauseBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int acurrent_time)
{
	return re.G2API_PauseBoneAnim(ghl_info, bone_name, acurrent_time);
}

static qboolean SV_G2API_PauseBoneAnimIndex(CGhoul2Info* ghl_info, const int bone_index, const int acurrent_time)
{
	return re.G2API_PauseBoneAnimIndex(ghl_info, bone_index, acurrent_time);
}

static qhandle_t SV_G2API_PrecacheGhoul2Model(const char* file_name)
{
	return re.G2API_PrecacheGhoul2Model(file_name);
}

static qboolean SV_G2API_RagEffectorGoal(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t pos)
{
	return re.G2API_RagEffectorGoal(ghoul2, bone_name, pos);
}

static qboolean SV_G2API_RagEffectorKick(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t velocity)
{
	return re.G2API_RagEffectorKick(ghoul2, bone_name, velocity);
}

static qboolean SV_G2API_RagForceSolve(CGhoul2Info_v& ghoul2, const qboolean force)
{
	return re.G2API_RagForceSolve(ghoul2, force);
}

static qboolean SV_G2API_RagPCJConstraint(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t min, vec3_t max)
{
	return re.G2API_RagPCJConstraint(ghoul2, bone_name, min, max);
}

static qboolean SV_G2API_RagPCJGradientSpeed(CGhoul2Info_v& ghoul2, const char* bone_name, const float speed)
{
	return re.G2API_RagPCJGradientSpeed(ghoul2, bone_name, speed);
}

static qboolean SV_G2API_RemoveBolt(CGhoul2Info* ghl_info, const int index)
{
	return re.G2API_RemoveBolt(ghl_info, index);
}

static qboolean SV_G2API_RemoveBone(CGhoul2Info* ghl_info, const char* bone_name)
{
	return re.G2API_RemoveBone(ghl_info, bone_name);
}

static qboolean SV_G2API_RemoveGhoul2Model(CGhoul2Info_v& ghl_info, const int model_index)
{
	return re.G2API_RemoveGhoul2Model(ghl_info, model_index);
}

static qboolean SV_G2API_RemoveSurface(CGhoul2Info* ghl_info, const int index)
{
	return re.G2API_RemoveSurface(ghl_info, index);
}

static void SV_G2API_SaveGhoul2Models(CGhoul2Info_v& ghoul2)
{
	return re.G2API_SaveGhoul2Models(ghoul2);
}

static qboolean SV_G2API_SetAnimIndex(CGhoul2Info* ghl_info, const int index)
{
	return re.G2API_SetAnimIndex(ghl_info, index);
}

static qboolean SV_G2API_SetBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int start_frame,const int end_frame,const int flags, const float anim_speed, const int acurrent_time,const float set_frame, const int blend_time)
{
	return re.G2API_SetBoneAnim(ghl_info, bone_name, start_frame, end_frame, flags,	anim_speed, acurrent_time, set_frame, blend_time);
}

static qboolean SV_G2API_SetBoneAnimIndex(CGhoul2Info* ghl_info, const int index, const int start_frame,
	const int end_frame,
	const int flags, const float anim_speed, const int acurrent_time,
	const float set_frame, const int blend_time)
{
	return re.G2API_SetBoneAnimIndex(ghl_info, index, start_frame, end_frame, flags,
		anim_speed, acurrent_time, set_frame, blend_time);
}

static qboolean SV_G2API_SetBoneAngles(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, const int flags,
	const Eorientations up, const Eorientations left, const Eorientations forward,
	qhandle_t* model_list,
	const int blend_time, const int acurrent_time)
{
	return re.G2API_SetBoneAngles(ghl_info, bone_name, angles, flags, up, left, forward,
		model_list, blend_time, acurrent_time);
}

static qboolean SV_G2API_SetBoneAnglesOffset(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t* model_list, const int blend_time, const int acurrent_time, const vec3_t offset)
{
	return re.G2API_SetBoneAnglesOffset(ghl_info, bone_name, angles, flags, up, left, forward, nullptr, blend_time, acurrent_time, nullptr);
}

static qboolean SV_G2API_SetBoneAnglesIndex(CGhoul2Info* ghl_info, const int index, const vec3_t angles, const int flags,
	const Eorientations yaw, const Eorientations pitch,
	const Eorientations roll, qhandle_t* model_list,
	const int blend_time, const int acurrent_time)
{
	return re.G2API_SetBoneAnglesIndex(ghl_info, index, angles, flags, yaw, pitch, roll,
		model_list, blend_time, acurrent_time);
}

static qboolean SV_G2API_SetBoneAnglesMatrix(CGhoul2Info* ghl_info, const char* bone_name, const mdxaBone_t& matrix,
	const int flags, qhandle_t* model_list, const int blend_time,
	const int acurrent_time)
{
	return re.G2API_SetBoneAnglesMatrix(ghl_info, bone_name, matrix, flags, model_list, blend_time, acurrent_time);
}

static qboolean SV_G2API_SetBoneAnglesMatrixIndex(CGhoul2Info* ghl_info, const int index, const mdxaBone_t& matrix,
	const int flags, qhandle_t* model_list, const int blandeTime,
	const int acurrent_time)
{
	return re.G2API_SetBoneAnglesMatrixIndex(ghl_info, index, matrix, flags, model_list, blandeTime, acurrent_time);
}

static qboolean SV_G2API_SetBoneIKState(CGhoul2Info_v& ghoul2, const int time, const char* bone_name, const int ikState,
	sharedSetBoneIKStateParams_t* params)
{
	return re.G2API_SetBoneIKState(ghoul2, time, bone_name, ikState, params);
}

static qboolean SV_G2API_SetGhoul2ModelFlags(CGhoul2Info* ghl_info, const int flags)
{
	return re.G2API_SetGhoul2ModelFlags(ghl_info, flags);
}

static qboolean SV_G2API_SetLodBias(CGhoul2Info* ghl_info, const int lodBias)
{
	return re.G2API_SetLodBias(ghl_info, lodBias);
}

static qboolean SV_G2API_SetNewOrigin(CGhoul2Info* ghl_info, const int bolt_index)
{
	return re.G2API_SetNewOrigin(ghl_info, bolt_index);
}

static void SV_G2API_SetRagDoll(CGhoul2Info_v& ghoul2, CRagDollParams* parms)
{
	return re.G2API_SetRagDoll(ghoul2, parms);
}

static qboolean SV_G2API_SetRootSurface(CGhoul2Info_v& ghl_info, const int model_index, const char* surface_name)
{
	return re.G2API_SetRootSurface(ghl_info, model_index, surface_name);
}

static qboolean SV_G2API_SetShader(CGhoul2Info* ghl_info, const qhandle_t customShader)
{
	return re.G2API_SetShader(ghl_info, customShader);
}

static qboolean SV_G2API_SetSkin(CGhoul2Info* ghl_info, const qhandle_t customSkin, const qhandle_t renderSkin)
{
	return re.G2API_SetSkin(ghl_info, customSkin, renderSkin);
}

static qboolean SV_G2API_SetSurfaceOnOff(CGhoul2Info* ghl_info, const char* surface_name, const int flags)
{
	return re.G2API_SetSurfaceOnOff(ghl_info, surface_name, flags);
}

static qboolean SV_G2API_StopBoneAnim(CGhoul2Info* ghl_info, const char* bone_name)
{
	return re.G2API_StopBoneAnim(ghl_info, bone_name);
}

static qboolean SV_G2API_StopBoneAnimIndex(CGhoul2Info* ghl_info, const int index)
{
	return re.G2API_StopBoneAnimIndex(ghl_info, index);
}

static qboolean SV_G2API_StopBoneAngles(CGhoul2Info* ghl_info, const char* bone_name)
{
	return re.G2API_StopBoneAngles(ghl_info, bone_name);
}

static qboolean SV_G2API_StopBoneAnglesIndex(CGhoul2Info* ghl_info, const int index)
{
	return re.G2API_StopBoneAnglesIndex(ghl_info, index);
}

#ifdef _G2_GORE
static void SV_G2API_AddSkinGore(CGhoul2Info_v& ghoul2, SSkinGoreData& gore)
{
	return re.G2API_AddSkinGore(ghoul2, gore);
}

static void SV_G2API_ClearSkinGore(CGhoul2Info_v& ghoul2)
{
	return re.G2API_ClearSkinGore(ghoul2);
}
#else
static void SV_G2API_AddSkinGore(
	CGhoul2Info_v& ghoul2,
	SSkinGoreData& gore)
{
	static_cast<void>(ghoul2);
	static_cast<void>(gore);
}

static void SV_G2API_ClearSkinGore(
	CGhoul2Info_v& ghoul2)
{
	static_cast<void>(ghoul2);
}
#endif

static IGhoul2InfoArray& SV_TheGhoul2InfoArray()
{
	return re.TheGhoul2InfoArray();
}

static qhandle_t SV_RE_RegisterSkin(const char* name)
{
	return re.RegisterSkin(name);
}

static int SV_RE_GetAnimationCFG(const char* psCFGFilename, char* psDest, const int iDestSize)
{
	return re.GetAnimationCFG(psCFGFilename, psDest, iDestSize);
}

static bool SV_WE_GetWindVector(vec3_t windVector, vec3_t atPoint)
{
	return re.GetWindVector(windVector, atPoint);
}

static bool SV_WE_GetWindGusting(vec3_t atpoint)
{
	return re.GetWindGusting(atpoint);
}

static bool SV_WE_IsOutside(vec3_t pos)
{
	return re.IsOutside(pos);
}

static float SV_WE_IsOutsideCausingPain(vec3_t pos)
{
	return re.IsOutsideCausingPain(pos);
}

static float SV_WE_GetChanceOfSaberFizz()
{
	return re.GetChanceOfSaberFizz();
}

static bool SV_WE_IsShaking(vec3_t pos)
{
	return re.IsShaking(pos);
}

static void SV_WE_AddWeatherZone(vec3_t mins, vec3_t maxs)
{
	return re.AddWeatherZone(mins, maxs);
}

static bool SV_WE_SetTempGlobalFogColor(vec3_t color)
{
	return re.SetTempGlobalFogColor(color);
}

/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/
void SV_InitGameProgs()
{
	game_import_t import;
	int i;

	// unload anything we have now
	if (ge)
	{
		SV_ShutdownGameProgs(qtrue);
	}

	// load a new game dll
import.Printf = Com_Printf;
import.WriteCam = Com_WriteCam;
import.FlushCamFile = Com_FlushCamFile;
import.Error = Com_Error;

import.Milliseconds = Sys_Milliseconds2;

import.DropClient = SV_GameDropClient;

import.SendServerCommand = SV_GameSendServerCommand;

import.linkentity = SV_LinkEntity;
import.unlinkentity = SV_UnlinkEntity;
import.EntitiesInBox = SV_AreaEntities;
import.EntityContact = SV_EntityContact;
import.trace = SV_Trace;
import.pointcontents = SV_PointContents;
import.totalMapContents = CM_TotalMapContents;
import.SetBrushModel = SV_SetBrushModel;

import.inPVS = SV_inPVS;
import.inPVSIgnorePortals = SV_inPVSIgnorePortals;

import.SetConfigstring = SV_SetConfigstring;
import.GetConfigstring = SV_GetConfigstring;

import.SetUserinfo = SV_SetUserinfo;
import.GetUserinfo = SV_GetUserinfo;

import.GetServerinfo = SV_GetServerinfo;

import.cvar = Cvar_Get;
import.cvar_set = Cvar_Set;
import.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
import.Cvar_VariableStringBuffer = Cvar_VariableStringBuffer;

import.argc = Cmd_Argc;
import.argv = Cmd_Argv;
import.SendConsoleCommand = Cbuf_AddText;

import.FS_FOpenFile = FS_FOpenFileByMode;
import.FS_Read = FS_Read;
import.FS_Write = FS_Write;
import.FS_FCloseFile = FS_FCloseFile;
import.FS_ReadFile = FS_ReadFile;
import.FS_FreeFile = FS_FreeFile;
import.FS_GetFileList = FS_GetFileList;

import.saved_game = &ojk::SavedGame::get_instance();

import.AdjustAreaPortalState = SV_AdjustAreaPortalState;
import.AreasConnected = CM_AreasConnected;

import.VoiceVolume = s_entityWavVol;

import.Malloc = G_ZMalloc_Helper;
import.Free = Z_Free;
import.bIsFromZone = Z_IsFromZone;

import.G2API_AddBolt = SV_G2API_AddBolt;
import.G2API_AttachEnt = SV_G2API_AttachEnt;
import.G2API_AttachG2Model = SV_G2API_AttachG2Model;
import.G2API_CollisionDetect = SV_G2API_CollisionDetect;
import.G2API_DetachEnt = SV_G2API_DetachEnt;
import.G2API_DetachG2Model = SV_G2API_DetachG2Model;
import.G2API_GetAnimFileName = SV_G2API_GetAnimFileName;
import.G2API_GetBoltMatrix = SV_G2API_GetBoltMatrix;
import.G2API_GetBoneAnim = SV_G2API_GetBoneAnim;
import.G2API_GetBoneAnimIndex = SV_G2API_GetBoneAnimIndex;
import.G2API_AddSurface = SV_G2API_AddSurface;
import.G2API_HaveWeGhoul2Models = SV_G2API_HaveWeGhoul2Models;
import.G2API_InitGhoul2Model = SV_G2API_InitGhoul2Model;
import.G2API_SetBoneAngles = SV_G2API_SetBoneAngles;
import.G2API_SetBoneAnglesOffset = SV_G2API_SetBoneAnglesOffset;
import.G2API_SetBoneAnglesMatrix = SV_G2API_SetBoneAnglesMatrix;
import.G2API_SetBoneAnim = SV_G2API_SetBoneAnim;
import.G2API_SetSkin = SV_G2API_SetSkin;
import.G2API_CopyGhoul2Instance = SV_G2API_CopyGhoul2Instance;
import.G2API_SetBoneAnglesIndex = SV_G2API_SetBoneAnglesIndex;
import.G2API_SetBoneAnimIndex = SV_G2API_SetBoneAnimIndex;
import.G2API_IsPaused = SV_G2API_IsPaused;
import.G2API_ListBones = SV_G2API_ListBones;
import.G2API_ListSurfaces = SV_G2API_ListSurfaces;
import.G2API_PauseBoneAnim = SV_G2API_PauseBoneAnim;
import.G2API_PauseBoneAnimIndex = SV_G2API_PauseBoneAnimIndex;
import.G2API_PrecacheGhoul2Model = SV_G2API_PrecacheGhoul2Model;
import.G2API_RemoveBolt = SV_G2API_RemoveBolt;
import.G2API_RemoveBone = SV_G2API_RemoveBone;
import.G2API_RemoveGhoul2Model = SV_G2API_RemoveGhoul2Model;
import.G2API_SetLodBias = SV_G2API_SetLodBias;
import.G2API_SetRootSurface = SV_G2API_SetRootSurface;
import.G2API_SetShader = SV_G2API_SetShader;
import.G2API_SetSurfaceOnOff = SV_G2API_SetSurfaceOnOff;
import.G2API_StopBoneAngles = SV_G2API_StopBoneAngles;
import.G2API_StopBoneAnim = SV_G2API_StopBoneAnim;
import.G2API_SetGhoul2ModelFlags = SV_G2API_SetGhoul2ModelFlags;
import.G2API_AddBoltSurfNum = SV_G2API_AddBoltSurfNum;
import.G2API_RemoveSurface = SV_G2API_RemoveSurface;
import.G2API_GetAnimRange = SV_G2API_GetAnimRange;
import.G2API_GetAnimRangeIndex = SV_G2API_GetAnimRangeIndex;
import.G2API_GiveMeVectorFromMatrix = SV_G2API_GiveMeVectorFromMatrix;
import.G2API_GetGhoul2ModelFlags = SV_G2API_GetGhoul2ModelFlags;
import.G2API_CleanGhoul2Models = SV_G2API_CleanGhoul2Models;
import.TheGhoul2InfoArray = SV_TheGhoul2InfoArray;
import.G2API_GetParentSurface = SV_G2API_GetParentSurface;
import.G2API_GetSurfaceIndex = SV_G2API_GetSurfaceIndex;
import.G2API_GetSurfaceName = SV_G2API_GetSurfaceName;
import.G2API_GetGLAName = SV_G2API_GetGLAName;
import.G2API_SetNewOrigin = SV_G2API_SetNewOrigin;
import.G2API_GetBoneIndex = SV_G2API_GetBoneIndex;
import.G2API_StopBoneAnglesIndex = SV_G2API_StopBoneAnglesIndex;
import.G2API_StopBoneAnimIndex = SV_G2API_StopBoneAnimIndex;
import.G2API_SetBoneAnglesMatrixIndex = SV_G2API_SetBoneAnglesMatrixIndex;
import.G2API_SetAnimIndex = SV_G2API_SetAnimIndex;
import.G2API_GetAnimIndex = SV_G2API_GetAnimIndex;

import.G2API_SaveGhoul2Models = SV_G2API_SaveGhoul2Models;
import.G2API_LoadGhoul2Models = SV_G2API_LoadGhoul2Models;
import.G2API_LoadSaveCodeDestructGhoul2Info = SV_G2API_LoadSaveCodeDestructGhoul2Info;
import.G2API_GetAnimFileNameIndex = SV_G2API_GetAnimFileNameIndex;
import.G2API_GetAnimFileInternalNameIndex = SV_G2API_GetAnimFileInternalNameIndex;
import.G2API_GetSurfaceRenderStatus = SV_G2API_GetSurfaceRenderStatus;

import.G2API_SetRagDoll = SV_G2API_SetRagDoll;
import.G2API_AnimateG2Models = SV_G2API_AnimateG2Models;

import.G2API_RagPCJConstraint = SV_G2API_RagPCJConstraint;
import.G2API_RagPCJGradientSpeed = SV_G2API_RagPCJGradientSpeed;
import.G2API_RagEffectorGoal = SV_G2API_RagEffectorGoal;
import.G2API_GetRagBonePos = SV_G2API_GetRagBonePos;
import.G2API_RagEffectorKick = SV_G2API_RagEffectorKick;
import.G2API_RagForceSolve = SV_G2API_RagForceSolve;

import.G2API_SetBoneIKState = SV_G2API_SetBoneIKState;
import.G2API_IKMove = SV_G2API_IKMove;

import.G2API_AddSkinGore = SV_G2API_AddSkinGore;
import.G2API_ClearSkinGore = SV_G2API_ClearSkinGore;

import.SetActiveSubBSP = SV_SetActiveSubBSP;

import.RE_RegisterSkin = SV_RE_RegisterSkin;
import.RE_GetAnimationCFG = SV_RE_GetAnimationCFG;

import.WE_GetWindVector = SV_WE_GetWindVector;
import.WE_GetWindGusting = SV_WE_GetWindGusting;
import.WE_IsOutside = SV_WE_IsOutside;
import.WE_IsOutsideCausingPain = SV_WE_IsOutsideCausingPain;
import.WE_GetChanceOfSaberFizz = SV_WE_GetChanceOfSaberFizz;
import.WE_IsShaking = SV_WE_IsShaking;
import.WE_AddWeatherZone = SV_WE_AddWeatherZone;
import.WE_SetTempGlobalFogColor = SV_WE_SetTempGlobalFogColor;

#ifdef JK2_MODE
	const char* gamename = "SerenityJediEngine2024-JKO-game";
#else
	auto gamename = "SerenityJediEngine2024-game";
#endif

	GetGameAPIProc* GetGameAPI;
	gameLibrary = Sys_LoadSPGameDll(gamename, &GetGameAPI);
	if (!gameLibrary)
		Com_Error(ERR_DROP, "Failed to load %s library", gamename);

	ge = static_cast<game_export_t*>(GetGameAPI(&import));
	if (!ge)
	{
		Sys_UnloadDll(gameLibrary);
		Com_Error(ERR_DROP, "Failed to load %s library", gamename);
	}

	if (ge->apiversion != GAME_API_VERSION)
	{
		int apiVersion = ge->apiversion;
		Sys_UnloadDll(gameLibrary);
		Com_Error(ERR_DROP, "game is version %i, not %i", apiVersion, GAME_API_VERSION);
	}

	//hook up the client while we're here
	if (!CL_InitCGameVM(gameLibrary))
	{
		Sys_UnloadDll(gameLibrary);
		Com_Error(ERR_DROP, "Failed to load client game functions");
	}

	sv.entityParsePoint = CM_EntityString();

	// use the current msec count for a random seed
	Z_TagFree(TAG_G_ALLOC);
	ge->Init(sv_mapname->string, sv_spawntarget->string, sv_mapChecksum->integer, CM_EntityString(), sv.time,
		com_frameTime, Com_Milliseconds(), e_saved_game_just_loaded, qbLoadTransition);

	// clear all gentity pointers that might still be set from
	// a previous level
	for (i = 0; i < 1; i++)
	{
		svs.clients[i].gentity = nullptr;
	}
}

/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand()
{
	if (sv.state != SS_GAME)
	{
		return qfalse;
	}

	return ge->ConsoleCommand();
}