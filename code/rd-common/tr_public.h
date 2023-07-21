/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#include "tr_types.h"
#include "../qcommon/qcommon.h"

#include "../ghoul2/G2.h"
#include "../ghoul2/ghoul2_gore.h"

constexpr auto REF_API_VERSION = 18;

using refimport_t = struct
{
	void (QDECL* Printf)(int printLevel, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
	void (QDECL* Error)(int errorLevel, const char* fmt, ...) __attribute__((noreturn, format(printf, 2, 3)));

	// milliseconds should only be used for profiling, never for anything game related. Get time from the refdef
	int (*Milliseconds)();

	void (*Hunk_ClearToMark)();
	void* (*Malloc)(int iSize, memtag_t eTag, qboolean zeroIt, int iAlign);
	int (*Z_Free)(void* memory);
	int (*Z_MemSize)(memtag_t eTag);
	void (*Z_MorphMallocTag)(void* pvBuffer, memtag_t eDesiredTag);

	void (*Cmd_ExecuteString)(const char* text);
	int (*Cmd_Argc)();
	char* (*Cmd_Argv)(int arg);
	void (*Cmd_ArgsBuffer)(char* buffer, int bufferLength);
	void (*Cmd_AddCommand)(const char* cmd_name, xcommand_t function);
	void (*Cmd_RemoveCommand)(const char* cmd_name);
	void (*Cvar_Set)(const char* var_name, const char* value);
	cvar_t* (*Cvar_Get)(const char* var_name, const char* value, int flags);
	void (*Cvar_SetValue)(const char* name, float value);
	void (*Cvar_CheckRange)(cvar_t* cv, float minVal, float maxVal, qboolean shouldBeIntegral);
	void (*Cvar_VariableStringBuffer)(const char* var_name, char* buffer, int bufsize);
	char* (*Cvar_VariableString)(const char* var_name);
	float (*Cvar_VariableValue)(const char* var_name);
	int (*Cvar_VariableIntegerValue)(const char* var_name);

	qboolean(*LowPhysicalMemory)();
	const char* (*SE_GetString)(const char* reference);

	void (*FS_FreeFile)(void* buffer);
	void (*FS_FreeFileList)(char** fileList);
	int (*FS_Read)(void* buffer, int len, fileHandle_t f);
	long (*FS_ReadFile)(const char* qpath, void** buffer);
	void (*FS_FCloseFile)(fileHandle_t f);
	long (*FS_FOpenFileRead)(const char* qpath, fileHandle_t* file, qboolean uniqueFILE);
	fileHandle_t(*FS_FOpenFileWrite)(const char* qpath, qboolean safe);
	int (*FS_FOpenFileByMode)(const char* qpath, fileHandle_t* f, fsMode_t mode);
	qboolean(*FS_FileExists)(const char* file);
	int (*FS_FileIsInPAK)(const char* filename);
	char** (*FS_ListFiles)(const char* directory, const char* extension, int* numfiles);
	int (*FS_Write)(const void* buffer, int len, fileHandle_t f);
	void (*FS_WriteFile)(const char* qpath, const void* buffer, int size);

	void (*CM_DrawDebugSurface)(void (*drawPoly)(int color, int num_points, const float* points));
	bool (*CM_CullWorldBox)(const cplane_t* frustrum, const vec3pair_t bounds);
	byte* (*CM_ClusterPVS)(int cluster);
	int (*CM_PointContents)(const vec3_t p, clip_handle_t model);
	void (*S_RestartMusic)();
	qboolean(*SND_RegisterAudio_LevelLoadEnd)(qboolean bDeleteEverythingNotUsedThisLevel);

	e_status(*CIN_RunCinematic)(int handle);
	int (*CIN_PlayCinematic)(const char* arg0, int xpos, int ypos, int width, int height,
		int bits, const char* psAudioFile /* = NULL */);
	void (*CIN_UploadCinematic)(int handle);

	// window handling
	window_t(*WIN_Init)(const windowDesc_t* desc, glconfig_t* glConfig);
	void (*WIN_SetGamma)(const glconfig_t* glConfig, byte red[256], byte green[256], byte blue[256]);
	void (*WIN_Present)(const window_t* window);
	void (*WIN_Shutdown)();

	// OpenGL-specific
	void* (*GL_GetProcAddress)(const char* name);
	qboolean(*GL_ExtensionSupported)(const char* extension);

	CMiniHeap* (*GetG2VertSpaceServer)();

	// Persistent data store
	bool (*PD_Store)(const char* name, const void* data, size_t size);
	const void* (*PD_Load)(const char* name, size_t* size);

	// ============= NOT IN MP BEYOND THIS POINT
	void (*SV_Trace)(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
		int pass_entity_num, int contentmask,
		EG2_Collision e_g2_trace_type, int use_lod);

	ojk::ISavedGame* saved_game;

	int (*SV_PointContents)(const vec3_t p, clip_handle_t model);

	qboolean(*CM_DeleteCachedMap)(qboolean bGuaranteedOkToDelete); // NOT IN MP

	qboolean(*CL_IsRunningInGameCinematic)();

	void* (*gpvCachedMapDiskImage)();
	char* (*gsCachedMapDiskImage)();
	qboolean* (*gbUsingCachedMapDataRightNow)();
	qboolean* (*gbAlreadyDoingLoad)();
	int (*com_frameTime)();
};

extern refimport_t ri;

//
// these are the functions exported by the refresh module
//
using refexport_t = struct
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void (*Shutdown)(qboolean destroyWindow, qboolean restarting);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void (*BeginRegistration)(glconfig_t* config);
	qhandle_t(*RegisterModel)(const char* name);
	qhandle_t(*RegisterSkin)(const char* name);
	int (*GetAnimationCFG)(const char* psCFGFilename, char* psDest, int iDestSize);
	qhandle_t(*RegisterShader)(const char* name);
	qhandle_t(*RegisterShaderNoMip)(const char* name);
	void (*LoadWorld)(const char* name);
	void (*R_LoadImage)(const char* name, byte** pic, int* width, int* height);

	// these two functions added to help with the new model alloc scheme...
	//
	void (*RegisterMedia_LevelLoadBegin)(const char* psMapName, ForceReload_e eForceReload,
		qboolean bAllowScreenDissolve);
	void (*RegisterMedia_LevelLoadEnd)();
	int (*RegisterMedia_GetLevel)();
	qboolean(*RegisterModels_LevelLoadEnd)(qboolean bDeleteEverythingNotUsedThisLevel);
	qboolean(*RegisterImages_LevelLoadEnd)();

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void (*SetWorldVisData)(const byte* vis);

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void (*EndRegistration)();

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void (*ClearScene)();
	void (*AddRefEntityToScene)(const refEntity_t* re);
	void (*AddPolyToScene)(qhandle_t h_shader, int num_verts, const polyVert_t* verts);
	void (*AddLightToScene)(const vec3_t org, float intensity, float r, float g, float b);
	void (*RenderScene)(const refdef_t* fd);
	qboolean(*GetLighting)(const vec3_t org, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);

	void (*SetColor)(const float* rgba); // NULL = 1,1,1,1
	void (*DrawStretchPic)(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2, qhandle_t h_shader); // 0 = white
	void (*DrawRotatePic)(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2, float a1, qhandle_t h_shader); // 0 = white
	void (*DrawRotatePic2)(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2, float a1, qhandle_t h_shader); // 0 = white
	void (*LAGoggles)();
	void (*Scissor)(float x, float y, float w, float h); // 0 = white

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void (*DrawStretchRaw)(int x, int y, int w, int h, int cols, int rows, const byte* data, int client,
		qboolean dirty);
	void (*UploadCinematic)(int cols, int rows, const byte* data, int client, qboolean dirty);

	void (*BeginFrame)(stereoFrame_t stereoFrame);

	// if the pointers are not NULL, timing info will be returned
	void (*EndFrame)(int* frontEndMsec, int* backEndMsec);

	qboolean(*ProcessDissolve)();
	qboolean(*InitDissolve)(qboolean bForceCircularExtroWipe);

	// for use with save-games mainly...
	void (*GetScreenShot)(byte* data, int w, int h);

#ifdef JK2_MODE
	size_t(*SaveJPGToBuffer)(byte* buffer, size_t bufSize, int quality, int image_width, int image_height, byte* image_buffer, int padding, bool flip_vertical);
	void	(*LoadJPGFromBuffer)(byte* inputBuffer, size_t len, byte** pic, int* width, int* height);
#endif

	// this is so you can get access to raw pixels from a graphics format (TGA/JPG/BMP etc),
	//	currently only the save game uses it (to make raw shots for the autosaves)
	//
	byte* (*TempRawImage_ReadFromFile)(const char* psLocalFilename, int* pi_width, int* pi_height, byte* pb_re_sample_buffer,
		qboolean qbVertFlip);
	void (*TempRawImage_CleanUp)();

	//misc stuff
	int (*MarkFragments)(int num_points, const vec3_t* points, const vec3_t projection,
		int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer);

	//model stuff
	void (*LerpTag)(orientation_t* tag, qhandle_t model, int start_frame, int end_frame,
		float frac, const char* tagName);
	void (*ModelBounds)(qhandle_t model, vec3_t mins, vec3_t maxs);

	void (*GetLightStyle)(int style, color4ub_t color);
	void (*SetLightStyle)(int style, int color);

	void (*GetBModelVerts)(int bmodelIndex, vec3_t* vec, vec3_t normal);
	void (*WorldEffectCommand)(const char* command);
	void (*GetModelBounds)(const refEntity_t* ref_ent, vec3_t bounds1, vec3_t bounds2);

	int (*RegisterFont)(const char* name);

	int (*Font_HeightPixels)(int index, float scale);
	int (*Font_StrLenPixels)(const char* s, int index, float scale);
	void (*Font_DrawString)(int x, int y, const char* s, const float* rgba, int iFontHandle, int iMaxPixelWidth,
		float scale);
	int (*Font_StrLenChars)(const char* s);
	qboolean(*Language_IsAsian)();
	qboolean(*Language_UsesSpaces)();
	unsigned int (*AnyLanguage_ReadCharFromString)(char* psText, int* piAdvanceCount,
		qboolean* pbIsTrailingPunctuation /* = NULL */);
	unsigned int (*AnyLanguage_ReadCharFromString2)(char** psText, qboolean* pbIsTrailingPunctuation /* = NULL */);

	// Misc
	void (*R_InitWorldEffects)();
	void (*R_ClearStuffToStopGhoul2CrashingThings)();
	qboolean(*R_inPVS)(vec3_t p1, vec3_t p2);

	void (*SVModelInit)();

	// Distortion effects
	float* (*tr_distortionAlpha)();
	float* (*tr_distortionStretch)();
	qboolean* (*tr_distortionPrePost)();
	qboolean* (*tr_distortionNegate)();

	// Weather effects
	bool (*GetWindVector)(vec3_t windVector, vec3_t atPoint);
	bool (*GetWindGusting)(vec3_t atpoint);
	bool (*IsOutside)(vec3_t pos);
	float (*IsOutsideCausingPain)(vec3_t pos);
	float (*GetChanceOfSaberFizz)();
	bool (*IsShaking)(vec3_t pos);
	void (*AddWeatherZone)(vec3_t mins, vec3_t maxs);
	bool (*SetTempGlobalFogColor)(vec3_t color);

	void (*SetRangedFog)(float dist);

	// GHOUL 2
	IGhoul2InfoArray& (*TheGhoul2InfoArray)();

	// GHOUL 2 API
	int (*G2API_AddBolt)(CGhoul2Info* ghl_info, const char* boneName);
	int (*G2API_AddBoltSurfNum)(CGhoul2Info* ghl_info, int surf_index);
	int (*G2API_AddSurface)(CGhoul2Info* ghl_info, int surfaceNumber, int polyNumber,
		float BarycentricI, float BarycentricJ, int lod);
	void (*G2API_AnimateG2Models)(CGhoul2Info_v& ghoul2, int AcurrentTime, CRagDollUpdateParams* params);
	qboolean(*G2API_AttachEnt)(int* bolt_info, CGhoul2Info* ghlInfoTo, int toBoltIndex, int ent_num, int toModelNum);
	qboolean(*G2API_AttachG2Model)(CGhoul2Info* ghl_info, CGhoul2Info* ghlInfoTo, int toBoltIndex, int toModel);
	void (*G2API_CollisionDetect)(CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2, const vec3_t angles,
		const vec3_t position, int AframeNumber, int ent_num, vec3_t rayStart, vec3_t rayEnd,
		vec3_t scale,
		CMiniHeap*, EG2_Collision e_g2_trace_type, int use_lod, float fRadius);
	void (*G2API_CleanGhoul2Models)(CGhoul2Info_v& ghoul2);
	void (*G2API_CopyGhoul2Instance)(const CGhoul2Info_v& ghoul2_from, CGhoul2Info_v& ghoul2_to, int model_index);
	void (*G2API_DetachEnt)(int* bolt_info);
	qboolean(*G2API_DetachG2Model)(CGhoul2Info* ghl_info);
	qboolean(*G2API_GetAnimFileName)(CGhoul2Info* ghl_info, char** filename);
	char* (*G2API_GetAnimFileNameIndex)(qhandle_t model_index);
	char* (*G2API_GetAnimFileInternalNameIndex)(qhandle_t model_index);
	int (*G2API_GetAnimIndex)(const CGhoul2Info* ghl_info);
	qboolean(*G2API_GetAnimRange)(CGhoul2Info* ghl_info, const char* boneName, int* start_frame, int* end_frame);
	qboolean(*G2API_GetAnimRangeIndex)(CGhoul2Info* ghl_info, int bone_index, int* start_frame, int* end_frame);
	qboolean(*G2API_GetBoneAnim)(CGhoul2Info* ghl_info, const char* boneName, int AcurrentTime,
		float* current_frame, int* start_frame, int* end_frame, int* flags, float* animSpeed,
		int* model_list);
	qboolean(*G2API_GetBoneAnimIndex)(CGhoul2Info* ghl_info, int iBoneIndex, int AcurrentTime,
		float* current_frame, int* start_frame, int* end_frame, int* flags,
		float* animSpeed, int* model_list);
	int (*G2API_GetBoneIndex)(CGhoul2Info* ghl_info, const char* boneName, qboolean bAddIfNotFound);
	qboolean(*G2API_GetBoltMatrix)(CGhoul2Info_v& ghoul2, int model_index, int bolt_index, mdxaBone_t* matrix,
		const vec3_t angles, const vec3_t position, int AframeNum, qhandle_t* model_list,
		const vec3_t scale);
	int (*G2API_GetGhoul2ModelFlags)(CGhoul2Info* ghl_info);
	char* (*G2API_GetGLAName)(CGhoul2Info* ghl_info);
	int (*G2API_GetParentSurface)(CGhoul2Info* ghl_info, int index);
	qboolean(*G2API_GetRagBonePos)(CGhoul2Info_v& ghoul2, const char* boneName, vec3_t pos, vec3_t entAngles,
		vec3_t entPos, vec3_t entScale);
	int (*G2API_GetSurfaceIndex)(CGhoul2Info* ghl_info, const char* surface_name);
	char* (*G2API_GetSurfaceName)(CGhoul2Info* ghl_info, int surfNumber);
	int (*G2API_GetSurfaceRenderStatus)(CGhoul2Info* ghl_info, const char* surface_name);
	int (*G2API_GetTime)(int argTime);
	void (*G2API_GiveMeVectorFromMatrix)(mdxaBone_t& bolt_matrix, Eorientations flags, vec3_t& vec);
	qboolean(*G2API_HaveWeGhoul2Models)(const CGhoul2Info_v& ghoul2);
	qboolean(*G2API_IKMove)(CGhoul2Info_v& ghoul2, int time, sharedIKMoveParams_t* params);
	int (*G2API_InitGhoul2Model)(CGhoul2Info_v& ghoul2, const char* fileName, int model_index,
		qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias);
	qboolean(*G2API_IsPaused)(CGhoul2Info* ghl_info, const char* boneName);
	void (*G2API_ListBones)(CGhoul2Info* ghl_info, int frame);
	void (*G2API_ListSurfaces)(CGhoul2Info* ghl_info);
	void (*G2API_LoadGhoul2Models)(CGhoul2Info_v& ghoul2, const char* buffer);
	void (*G2API_LoadSaveCodeDestructGhoul2Info)(CGhoul2Info_v& ghoul2);
	qboolean(*G2API_PauseBoneAnim)(CGhoul2Info* ghl_info, const char* boneName, int AcurrentTime);
	qboolean(*G2API_PauseBoneAnimIndex)(CGhoul2Info* ghl_info, int bone_index, int AcurrentTime);
	qhandle_t(*G2API_PrecacheGhoul2Model)(const char* fileName);
	qboolean(*G2API_RagEffectorGoal)(CGhoul2Info_v& ghoul2, const char* boneName, vec3_t pos);
	qboolean(*G2API_RagEffectorKick)(CGhoul2Info_v& ghoul2, const char* boneName, vec3_t velocity);
	qboolean(*G2API_RagForceSolve)(CGhoul2Info_v& ghoul2, qboolean force);
	qboolean(*G2API_RagPCJConstraint)(CGhoul2Info_v& ghoul2, const char* boneName, vec3_t min, vec3_t max);
	qboolean(*G2API_RagPCJGradientSpeed)(CGhoul2Info_v& ghoul2, const char* boneName, float speed);
	qboolean(*G2API_RemoveBolt)(CGhoul2Info* ghl_info, int index);
	qboolean(*G2API_RemoveBone)(CGhoul2Info* ghl_info, const char* boneName);
	qboolean(*G2API_RemoveGhoul2Model)(CGhoul2Info_v& ghl_info, int model_index);
	qboolean(*G2API_RemoveSurface)(CGhoul2Info* ghl_info, int index);
	void (*G2API_SaveGhoul2Models)(CGhoul2Info_v& ghoul2);
	qboolean(*G2API_SetAnimIndex)(CGhoul2Info* ghl_info, int index);
	qboolean(*G2API_SetBoneAnim)(CGhoul2Info* ghl_info, const char* boneName, int start_frame, int end_frame,
		int flags, float animSpeed, int AcurrentTime, float setFrame, int blend_time);
	qboolean(*G2API_SetBoneAnimIndex)(CGhoul2Info* ghl_info, int index, int start_frame, int end_frame,
		int flags, float animSpeed, int AcurrentTime, float setFrame, int blend_time);
	qboolean(*G2API_SetBoneAngles)(CGhoul2Info* ghl_info, const char* boneName, const vec3_t angles, int flags,
		Eorientations up, Eorientations left, Eorientations forward, qhandle_t* model_list,
		int blend_time, int AcurrentTime);
	qboolean(*G2API_SetBoneAnglesIndex)(CGhoul2Info* ghl_info, int index, const vec3_t angles, int flags,
		Eorientations yaw, Eorientations pitch, Eorientations roll,
		qhandle_t* model_list,
		int blend_time, int AcurrentTime);
	qboolean(*G2API_SetBoneAnglesOffset)(CGhoul2Info* ghl_info, const char* boneName, const vec3_t angles, int flags,
		Eorientations up, Eorientations left, Eorientations forward,
		qhandle_t* model_list,
		int blend_time, int AcurrentTime, const vec3_t offset);
	qboolean(*G2API_SetBoneAnglesMatrix)(CGhoul2Info* ghl_info, const char* boneName, const mdxaBone_t& matrix,
		int flags, qhandle_t* model_list, int blend_time, int AcurrentTime);
	qboolean(*G2API_SetBoneAnglesMatrixIndex)(CGhoul2Info* ghl_info, int index, const mdxaBone_t& matrix,
		int flags, qhandle_t* model_list, int blandeTime, int AcurrentTime);
	qboolean(*G2API_SetBoneIKState)(CGhoul2Info_v& ghoul2, int time, const char* boneName, int ikState,
		sharedSetBoneIKStateParams_t* params);
	qboolean(*G2API_SetGhoul2ModelFlags)(CGhoul2Info* ghl_info, int flags);
	void (*G2API_SetGhoul2ModelIndexes)(CGhoul2Info_v& ghoul2, qhandle_t* model_list, const qhandle_t* skin_list);
	qboolean(*G2API_SetLodBias)(CGhoul2Info* ghl_info, int lodBias);
	qboolean(*G2API_SetNewOrigin)(CGhoul2Info* ghl_info, int bolt_index);
	void (*G2API_SetRagDoll)(CGhoul2Info_v& ghoul2, CRagDollParams* parms);
	qboolean(*G2API_SetRootSurface)(CGhoul2Info_v& ghl_info, int model_index, const char* surface_name);
	qboolean(*G2API_SetShader)(CGhoul2Info* ghl_info, qhandle_t customShader);
	qboolean(*G2API_SetSkin)(CGhoul2Info* ghl_info, qhandle_t customSkin, qhandle_t renderSkin);
	qboolean(*G2API_SetSurfaceOnOff)(CGhoul2Info* ghl_info, const char* surface_name, int flags);
	void (*G2API_SetTime)(int current_time, int clock);
	qboolean(*G2API_StopBoneAnim)(CGhoul2Info* ghl_info, const char* boneName);
	qboolean(*G2API_StopBoneAnimIndex)(CGhoul2Info* ghl_info, int index);
	qboolean(*G2API_StopBoneAngles)(CGhoul2Info* ghl_info, const char* boneName);
	qboolean(*G2API_StopBoneAnglesIndex)(CGhoul2Info* ghl_info, int index);

#ifdef _G2_GORE
	void (*G2API_AddSkinGore)(CGhoul2Info_v& ghoul2, SSkinGoreData& gore);
	void (*G2API_ClearSkinGore)(CGhoul2Info_v& ghoul2);
#endif

	// Performance analysis (perform anal)
	void (*G2Time_ResetTimers)();
	void (*G2Time_ReportTimers)();
};

// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.

using GetRefAPI_t = refexport_t * (QDECL*)(int apiVersion, refimport_t* rimp);
