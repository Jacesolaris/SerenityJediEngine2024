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

#pragma once
#if !defined(G2_H_INC)
#define G2_H_INC

class CMiniHeap;

// defines to setup the
constexpr auto ENTITY_WIDTH = 12;
constexpr auto MODEL_WIDTH = 10;
constexpr auto BOLT_WIDTH = 10;

#define		MODEL_AND	((1<<MODEL_WIDTH)-1)
#define		BOLT_AND	((1<<BOLT_WIDTH)-1)
#define		ENTITY_AND	((1<<ENTITY_WIDTH)-1)

constexpr auto BOLT_SHIFT = 0;
#define		MODEL_SHIFT	(BOLT_SHIFT + BOLT_WIDTH)
#define		ENTITY_SHIFT (MODEL_SHIFT + MODEL_WIDTH)

//rww - RAGDOLL_BEGIN
class CRagDollUpdateParams;
//rww - RAGDOLL_END

struct model_s;
// internal surface calls  G2_surfaces.cpp
qboolean G2_SetSurfaceOnOff(CGhoul2Info* ghl_info, const char* surface_name, int off_flags);
qboolean G2_SetRootSurface(CGhoul2Info_v& ghoul2, int model_index, const char* surface_name);
int G2_AddSurface(CGhoul2Info* ghoul2, int surface_number, int poly_number, float barycentric_i, float barycentric_j,
	int lod);
qboolean G2_RemoveSurface(surfaceInfo_v& slist, int index);
const surfaceInfo_t* G2_FindOverrideSurface(int surface_num, const surfaceInfo_v& surface_list);
int G2_IsSurfaceLegal(const model_s*, const char* surface_name, uint32_t* flags);
int G2_GetParentSurface(const CGhoul2Info* ghl_info, int index);
int G2_GetSurfaceIndex(const CGhoul2Info* ghl_info, const char* surface_name);
int G2_IsSurfaceRendered(const CGhoul2Info* ghl_info, const char* surface_name, const surfaceInfo_v& slist);

// internal bone calls - G2_Bones.cpp
qboolean G2_Set_Bone_Angles(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, const float* angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, const int blend_time, const int current_time, const vec3_t offset);
qboolean G2_Remove_Bone(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name);
qboolean G2_Remove_Bone_Index(boneInfo_v& blist, int index);

qboolean G2_Set_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, const int start_frame, const int end_frame, const int flags, const float anim_speed, const int current_time, const float set_frame, const int blend_time);

qboolean G2_Get_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, const int current_time, float* current_frame, int* start_frame, int* end_frame, int* flags, float* ret_anim_speed);

qboolean G2_Get_Bone_Anim_Range(const CGhoul2Info* ghl_info, const boneInfo_v& blist, const char* bone_name, int* start_frame, int* end_frame);

qboolean G2_Get_Bone_Anim_Range_Index(const boneInfo_v& blist, const int bone_index, int* start_frame, int* end_frame);
qboolean G2_Pause_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, const int current_time);
qboolean G2_Pause_Bone_Anim_Index(boneInfo_v& blist, int bone_index, int current_time, int num_frames);
qboolean G2_IsPaused(const CGhoul2Info* ghl_info, const boneInfo_v& blist, const char* bone_name);
qboolean G2_Stop_Bone_Anim(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name);
qboolean G2_Stop_Bone_Angles(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name);
//rww - RAGDOLL_BEGIN
void G2_Animate_Bone_List(CGhoul2Info_v& ghoul2, int current_time, int index, CRagDollUpdateParams* params);
//rww - RAGDOLL_END

void G2_Init_Bone_List(boneInfo_v& blist, int numBones);
int G2_Find_Bone_In_List(const boneInfo_v& blist, const int bone_num);
qboolean G2_Set_Bone_Angles_Matrix(const CGhoul2Info* ghl_info, boneInfo_v& blist, const char* bone_name, const mdxaBone_t& matrix, const int flags, const int blend_time, const int current_time);
int G2_Get_Bone_Index(CGhoul2Info* ghoul2, const char* bone_name, qboolean b_add_if_not_found);
qboolean G2_Set_Bone_Angles_Index(CGhoul2Info* ghl_info, boneInfo_v& blist, const int index, const float* angles, const int flags, const Eorientations yaw, const Eorientations pitch, const Eorientations roll, const int blend_time, const int current_time, const vec3_t offset);
qboolean G2_Set_Bone_Angles_Matrix_Index(boneInfo_v& blist, int index,
	const mdxaBone_t& matrix, int flags,
	int blend_time, int current_time);
qboolean G2_Stop_Bone_Anim_Index(boneInfo_v& blist, int index);
qboolean G2_Stop_Bone_Angles_Index(boneInfo_v& blist, int index);
qboolean G2_Set_Bone_Anim_Index(boneInfo_v& blist, int index, int start_frame,
	int end_frame, int flags, float anim_speed, int current_time, float set_frame,
	int ablend_time, int num_frames);
qboolean G2_Get_Bone_Anim_Index(boneInfo_v& blist, int index, int current_time,
	float* retcurrent_frame, int* start_frame, int* end_frame, int* flags, float* ret_anim_speed,
	int num_frames);

// misc functions G2_misc.cpp
void G2_List_Model_Surfaces(const char* file_name);
void G2_List_Model_Bones(const char* file_name, int frame);
qboolean G2_GetAnimFileName(const char* file_name, char** filename);

#ifdef _G2_GORE
void G2_TraceModels(CGhoul2Info_v& ghoul2, vec3_t rayStart, vec3_t rayEnd, CCollisionRecord* collRecMap, int ent_num, EG2_Collision eG2TraceType, int use_lod, float fRadius, float ssize, float tsize, float theta, int shader, SSkinGoreData* gore, qboolean skipIfLODNotMatch);
#else
void G2_TraceModels(CGhoul2Info_v& ghoul2, vec3_t rayStart, vec3_t rayEnd, CCollisionRecord* collRecMap, int ent_num, EG2_Collision eG2TraceType, int use_lod, float fRadius);
#endif

void TransformAndTranslatePoint(const vec3_t in, vec3_t out, const mdxaBone_t* mat);

#ifdef _G2_GORE
void G2_TransformModel(CGhoul2Info_v& ghoul2, const int frame_num, vec3_t scale, CMiniHeap* G2VertSpace, int use_lod, const bool ApplyGore, const SSkinGoreData* gore = nullptr);
#else
void G2_TransformModel(CGhoul2Info_v& ghoul2, const int frame_num, vec3_t scale, CMiniHeap* G2VertSpace, int use_lod);
#endif

void G2_GenerateWorldMatrix(const vec3_t angles, const vec3_t origin);
void TransformPoint(const vec3_t in, vec3_t out, const mdxaBone_t* mat);
void Inverse_Matrix(const mdxaBone_t* src, mdxaBone_t* dest);
void* G2_FindSurface(const model_s* mod, const int index, const int lod);
void G2_SaveGhoul2Models(CGhoul2Info_v& ghoul2);
void G2_LoadGhoul2Model(CGhoul2Info_v& ghoul2, const char* buffer);

// internal bolt calls. G2_bolts.cpp
int G2_Add_Bolt(const CGhoul2Info* ghl_info, boltInfo_v& bltlist, surfaceInfo_v& slist, const char* bone_name);
qboolean G2_Remove_Bolt(boltInfo_v& bltlist, int index);
void G2_Init_Bolt_List(boltInfo_v& bltlist);
int G2_Find_Bolt_Bone_Num(const boltInfo_v& bltlist, int bone_num);
int G2_Find_Bolt_Surface_Num(const boltInfo_v& bltlist, const int surface_num, const int flags);
int G2_Add_Bolt_Surf_Num(const CGhoul2Info* ghl_info, boltInfo_v& bltlist, const surfaceInfo_v& slist, const int surf_num);

// API calls - G2_API.cpp
void RestoreGhoul2InfoArray();
void SaveGhoul2InfoArray();

qhandle_t G2API_PrecacheGhoul2Model(const char* file_name);

int G2API_InitGhoul2Model(CGhoul2Info_v& ghoul2, const char* file_name, int model_index,
	qhandle_t custom_skin = NULL_HANDLE,
	qhandle_t custom_shader = NULL_HANDLE, int model_flags = 0, int lod_bias = 0);
qboolean G2API_SetLodBias(CGhoul2Info* ghl_info, int lod_bias);
qboolean G2API_SetSkin(CGhoul2Info* ghl_info, qhandle_t custom_skin, qhandle_t render_skin = 0);
qboolean G2API_SetShader(CGhoul2Info* ghl_info, qhandle_t custom_shader);
qboolean G2API_RemoveGhoul2Model(CGhoul2Info_v& ghl_info, int model_index);
qboolean G2API_SetSurfaceOnOff(CGhoul2Info* ghl_info, const char* surface_name, int flags);
qboolean G2API_SetRootSurface(CGhoul2Info_v& ghl_info, int model_index, const char* surface_name);
qboolean G2API_RemoveSurface(CGhoul2Info* ghl_info, int index);
int G2API_AddSurface(CGhoul2Info* ghl_info, int surface_number, int poly_number, float barycentric_i, float barycentric_j, int lod);

qboolean G2API_SetBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int start_frame, const int end_frame, const int flags, const float anim_speed, const int acurrent_time, const float set_frame = -1, const int blend_time = -1);

qboolean G2API_GetBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int acurrent_time, float* current_frame, int* start_frame, int* end_frame, int* flags, float* anim_speed, qhandle_t* model_list);

qboolean G2API_GetBoneAnimIndex(CGhoul2Info* ghl_info, const int iBoneIndex, const int acurrent_time, float* current_frame, int* start_frame, int* end_frame, int* flags, float* anim_speed, qhandle_t* model_list);

qboolean G2API_GetAnimRange(CGhoul2Info* ghl_info, const char* bone_name, int* start_frame, int* end_frame);
qboolean G2API_GetAnimRangeIndex(CGhoul2Info* ghl_info, const int bone_index, int* start_frame, int* end_frame);

qboolean G2API_PauseBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int acurrent_time);
qboolean G2API_PauseBoneAnimIndex(CGhoul2Info* ghl_info, const int bone_index, const int acurrent_time);

qboolean G2API_IsPaused(CGhoul2Info* ghl_info, const char* bone_name);
qboolean G2API_StopBoneAnim(CGhoul2Info* ghl_info, const char* bone_name);

qboolean G2API_SetBoneAngles(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t* model_list, const int blend_time = 0, const int acurrent_time = 0);

qboolean G2API_SetBoneAnglesOffset(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t* model_list, const int blend_time = 0, const int acurrent_time = 0, const vec3_t offset = nullptr);

qboolean G2API_StopBoneAngles(CGhoul2Info* ghl_info, const char* bone_name);
qboolean G2API_RemoveBone(CGhoul2Info* ghl_info, const char* bone_name);
qboolean G2API_RemoveBolt(CGhoul2Info* ghl_info, int index);
int G2API_AddBolt(CGhoul2Info* ghl_info, const char* bone_name);
int G2API_AddBoltSurfNum(CGhoul2Info* ghl_info, int surf_index);
qboolean G2API_AttachG2Model(CGhoul2Info* ghl_info, CGhoul2Info* ghl_info_to, int to_bolt_index, int to_model);
qboolean G2API_DetachG2Model(CGhoul2Info* ghl_info);
qboolean G2API_AttachEnt(int* bolt_info, CGhoul2Info* ghl_info_to, int to_bolt_index, int ent_num, int to_model_num);
void G2API_DetachEnt(int* bolt_info);

qboolean G2API_GetBoltMatrix(CGhoul2Info_v& ghoul2, const int model_index, const int bolt_index, mdxaBone_t* matrix, const vec3_t angles, const vec3_t position, const int aframe_num, qhandle_t* model_list, const vec3_t scale);

void G2API_ListSurfaces(CGhoul2Info* ghl_info);
void G2API_ListBones(CGhoul2Info* ghl_info, int frame);
qboolean G2API_HaveWeGhoul2Models(const CGhoul2Info_v& ghoul2);
void G2API_SetGhoul2ModelIndexes(CGhoul2Info_v& ghoul2, qhandle_t* model_list, const qhandle_t* skin_list);
qboolean G2API_SetGhoul2ModelFlags(CGhoul2Info* ghl_info, int flags);
int G2API_GetGhoul2ModelFlags(CGhoul2Info* ghl_info);

qboolean G2API_GetAnimFileName(CGhoul2Info* ghl_info, char** filename);
void G2API_CollisionDetect(CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2, const vec3_t angles,
	const vec3_t position,
	int aframe_number, int ent_num, vec3_t rayStart, vec3_t rayEnd, vec3_t scale,
	CMiniHeap* G2VertSpace,
	EG2_Collision e_g2_trace_type, int use_lod, float fRadius);
void G2API_GiveMeVectorFromMatrix(mdxaBone_t& bolt_matrix, Eorientations flags, vec3_t& vec);
void G2API_CopyGhoul2Instance(const CGhoul2Info_v& ghoul2_from, CGhoul2Info_v& ghoul2_to, int model_index = -1);
void G2API_CleanGhoul2Models(CGhoul2Info_v& ghoul2);
int G2API_GetParentSurface(CGhoul2Info* ghl_info, int index);
int G2API_GetSurfaceIndex(CGhoul2Info* ghl_info, const char* surface_name);
char* G2API_GetSurfaceName(CGhoul2Info* ghl_info, int surfNumber);
char* G2API_GetGLAName(CGhoul2Info* ghl_info);

qboolean G2API_SetBoneAnglesMatrix(CGhoul2Info* ghl_info, const char* bone_name, const mdxaBone_t& matrix, const int flags, qhandle_t* model_list, const int blend_time = 0, const int acurrent_time = 0);

qboolean G2API_SetNewOrigin(CGhoul2Info* ghl_info, int bolt_index);
int G2API_GetBoneIndex(CGhoul2Info* ghl_info, const char* bone_name, qboolean bAddIfNotFound);
qboolean G2API_StopBoneAnglesIndex(CGhoul2Info* ghl_info, int index);
qboolean G2API_StopBoneAnimIndex(CGhoul2Info* ghl_info, const int index);

qboolean G2API_SetBoneAnglesIndex(CGhoul2Info* ghl_info, const int index, const vec3_t angles, const int flags, const Eorientations yaw, const Eorientations pitch, const Eorientations roll, qhandle_t* model_list, const int blend_time, const int acurrent_time);

qboolean G2API_SetBoneAnglesMatrixIndex(CGhoul2Info* ghl_info, const int index, const mdxaBone_t& matrix, const int flags, qhandle_t* model_list, const int blend_time, const int acurrent_time);

qboolean G2API_SetBoneAnimIndex(CGhoul2Info* ghl_info, const int index, const int start_frame, const int end_frame, const int flags, const float anim_speed, const int acurrent_time, const float set_frame, const int blend_time);

qboolean G2API_SetAnimIndex(CGhoul2Info* ghl_info, int index);
int G2API_GetAnimIndex(const CGhoul2Info* ghl_info);
void G2API_SaveGhoul2Models(CGhoul2Info_v& ghoul2);
void G2API_LoadGhoul2Models(CGhoul2Info_v& ghoul2, const char* buffer);
void G2API_LoadSaveCodeDestructGhoul2Info(CGhoul2Info_v& ghoul2);
char* G2API_GetAnimFileNameIndex(qhandle_t model_index);
char* G2API_GetAnimFileInternalNameIndex(qhandle_t model_index);
int G2API_GetSurfaceRenderStatus(CGhoul2Info* ghl_info, const char* surface_name);

// From tr_ghoul2.cpp
void G2_ConstructGhoulSkeleton(CGhoul2Info_v& ghoul2, int frame_num, bool checkForNewOrigin, const vec3_t scale);
void G2_GetBoltMatrixLow(CGhoul2Info& ghoul2, int boltNum, const vec3_t scale, mdxaBone_t& retMatrix);
void G2_TimingModel(boneInfo_t& bone, int current_time, int num_frames_in_file, int& current_frame, int& new_frame, float& lerp);

bool G2_SetupModelPointers(CGhoul2Info_v& ghoul2); // returns true if any model is properly set up
bool G2_SetupModelPointers(CGhoul2Info* ghl_info); // returns true if the model is properly set up

//#ifdef _G2_GORE	// These exist regardless, non-gore versions are empty
void G2API_AddSkinGore(CGhoul2Info_v& ghoul2, SSkinGoreData& gore);
void G2API_ClearSkinGore(CGhoul2Info_v& ghoul2);
//#endif

#endif // G2_H_INC
