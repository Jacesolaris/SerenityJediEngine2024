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

#include "../server/exe_headers.h"

#include <list>
#include <string>

#include "../qcommon/q_shared.h"
#include "tr_local.h"
#include "tr_common.h"
#include "../ghoul2/G2.h"
#include "../qcommon/MiniHeap.h"

#ifdef FINAL_BUILD
#define G2API_DEBUG (0) // please don't change this
#else
#if defined(_DEBUG)
#define G2API_DEBUG (1)
#else
#define G2API_DEBUG (0) // change this to test g2api in release
#endif
#endif

//rww - RAGDOLL_BEGIN
#include "../ghoul2/ghoul2_gore.h"
//rww - RAGDOLL_END

extern mdxaBone_t worldMatrix;
extern mdxaBone_t worldMatrixInv;

extern cvar_t* r_Ghoul2TimeBase;

extern refexport_t re;

#define G2_MODEL_OK(g) ((g)&&(g)->mValid&&(g)->aHeader&&(g)->current_model&&(g)->animModel)

#define G2_DEBUG_TIME (0)

static int G2TimeBases[NUM_G2T_TIME];

bool G2_TestModelPointers(CGhoul2Info* ghl_info);

#if G2API_DEBUG
#include <float.h> //for isnan

#define MAX_ERROR_PRINTS (3)
class ErrorReporter
{
	std::string mName;
	std::map<std::string, int> mErrors;
public:
	ErrorReporter(const std::string& name) :
		mName(name)
	{
	}
	~ErrorReporter()
	{
		char mess[1000];
		int total = 0;
		sprintf(mess, "****** %s Error Report Begin******\n", mName.c_str());
		Com_DPrintf(mess);

		std::map<std::string, int>::iterator i;
		for (i = mErrors.begin(); i != mErrors.end(); ++i)
		{
			total += (*i).second;
			sprintf(mess, "%s (hits %d)\n", (*i).first.c_str(), (*i).second);
			Com_DPrintf(mess);
		}

		sprintf(mess, "****** %s Error Report End   %d errors of %ld kinds******\n", mName.c_str(), total, mErrors.size());
		Com_DPrintf(mess);
	}
	int AnimTest(CGhoul2Info_v& ghoul2, const char* m, const char*, int line)
	{
		if (G2_SetupModelPointers(ghoul2))
		{
			int i;
			for (i = 0; i < ghoul2.size(); i++)
			{
				AnimTest(&ghoul2[i], m, 0, line);
			}
			return i;
		}
		return 666; //these return values are to saisfy the optimizer
	}
	int AnimTest(CGhoul2Info* ghl_info, const char* m, const char*, int line)
	{
		bool ok = G2_TestModelPointers(ghl_info);
		if (!ok)
		{
			return 5; // I guess this happens from time to time
		}
		size_t i;
		int ret = 0;

		char GLAName1[1000];
		char GLAName2[1000];
		char GLMName1[1000];
		char GLMName2[1000];

		strcpy(GLAName1, ghl_info->animModel->name);
		strcpy(GLAName2, ghl_info->aHeader->name);
		strcpy(GLMName1, ghl_info->mFileName);
		strcpy(GLMName2, ghl_info->current_model->name);

		int numFramesInFile = ghl_info->aHeader->num_frames;

		int numActiveBones = 0;
		for (i = 0; i < ghl_info->mBlist.size(); i++)
		{
			if (ghl_info->mBlist[i].boneNumber != -1) // slot used?
			{
				if (ghl_info->mBlist[i].flags & BONE_ANIM_TOTAL) // anim on this?
				{
					numActiveBones++;
					bool loop = !!(ghl_info->mBlist[i].flags & BONE_ANIM_OVERRIDE_LOOP);
					bool not_loop = !!(ghl_info->mBlist[i].flags & BONE_ANIM_OVERRIDE);

					if (loop == not_loop)
					{
						Error("Unusual animation flags, should have some sort of override, but not both", 1, 0, line);
					}

					bool freeze = (ghl_info->mBlist[i].flags & BONE_ANIM_OVERRIDE_FREEZE) == BONE_ANIM_OVERRIDE_FREEZE;

					if (loop && freeze)
					{
						Error("Unusual animation flags, loop and freeze", 1, 0, line);
					}
					bool no_lerp = !!(ghl_info->mBlist[i].flags & BONE_ANIM_NO_LERP);
					bool blend = !!(ghl_info->mBlist[i].flags & BONE_ANIM_BLEND);

					//comments according to jake
					int			start_frame = ghl_info->mBlist[i].start_frame;		// start frame for animation
					int			end_frame = ghl_info->mBlist[i].end_frame;		// end frame for animation NOTE anim actually ends on end_frame+1
					int			startTime = ghl_info->mBlist[i].startTime;		// time we started this animation
					int			pauseTime = ghl_info->mBlist[i].pauseTime;		// time we paused this animation - 0 if not paused
					float		anim_speed = ghl_info->mBlist[i].anim_speed;		// speed at which this anim runs. 1.0f means full speed of animation incoming - ie if anim is 20hrtz, we run at 20hrts. If 5hrts, we run at 5 hrts

					float		blendFrame = 0.0f;		// frame PLUS LERP value to blend
					int			blendLerpFrame = 0;	// frame to lerp the blend frame with.

					if (blend)
					{
						blendFrame = ghl_info->mBlist[i].blendFrame;
						blendLerpFrame = ghl_info->mBlist[i].blendLerpFrame;
						if (floor(blendFrame) < 0.0f)
						{
							Error("negative blendFrame", 1, 0, line);
						}
						if (ceil(blendFrame) >= float(numFramesInFile))
						{
							Error("blendFrame >= numFramesInFile", 1, 0, line);
						}
						if (blendLerpFrame < 0)
						{
							Error("negative blendLerpFrame", 1, 0, line);
						}
						if (blendLerpFrame >= numFramesInFile)
						{
							Error("blendLerpFrame >= numFramesInFile", 1, 0, line);
						}
					}
					if (start_frame < 0)
					{
						Error("negative start_frame", 1, 0, line);
					}
					if (start_frame >= numFramesInFile)
					{
						Error("start_frame >= numFramesInFile", 1, 0, line);
					}
					if (end_frame < 0)
					{
						Error("negative end_frame", 1, 0, line);
					}
					if (end_frame == 0 && anim_speed > 0.0f)
					{
						Error("Zero end_frame", 1, 0, line);
					}
					if (end_frame > numFramesInFile)
					{
						Error("end_frame > numFramesInFile", 1, 0, line);
					}
					// mikeg call out here for further checks.
					ret = (int)startTime + (int)pauseTime + (int)no_lerp; // quiet VC.
				}
			}
		}
		return ret;
	}
	int Error(const char* m, int kind, const char*, int line)
	{
		char mess[1000];
		assert(m);
		std::string full = mName;
		if (kind == 2)
		{
			full += ":NOTE:     ";
		}
		else if (kind == 1)
		{
			//			assert(!"G2API Warning");
			full += ":WARNING:  ";
		}
		else
		{
			//			assert(!"G2API Error");
			full += ":ERROR  :  ";
		}
		full += m;
		sprintf(mess, "  [line %d]", line);
		full += mess;

		// assert(0);
		int ret = 0; //place a breakpoint here
		std::map<std::string, int>::iterator f = mErrors.find(full);
		if (f == mErrors.end())
		{
			ret++; // or a breakpoint here for the first occurance
			mErrors.insert(std::make_pair(full, 0));
			f = mErrors.find(full);
		}
		assert(f != mErrors.end());
		(*f).second++;
		if ((*f).second == 1000)
		{
			ret *= -1; // breakpoint to find a specific occurance of an error
		}
		if ((*f).second <= MAX_ERROR_PRINTS && kind < 2)
		{
			Com_Printf("%s (hit # %d)\n", full.c_str(), (*f).second);
			if (1)
			{
				sprintf(mess, "%s (hit # %d)\n", full.c_str(), (*f).second);
				Com_DPrintf(mess);
			}
		}
		return ret;
	}
};

#include "assert.h"
ErrorReporter& G2APIError()
{
	static ErrorReporter singleton("G2API");
	return singleton;
}

#define G2ERROR(exp,m) (void)( (exp) || (G2APIError().Error(m,0,__FILE__,__LINE__), 0) )
#define G2WARNING(exp,m) (void)( (exp) || (G2APIError().Error(m,1,__FILE__,__LINE__), 0) )
#define G2NOTE(exp,m) (void)( (exp) || (G2APIError().Error(m,2,__FILE__,__LINE__), 0) )
#define G2ANIM(ghl_info,m) (void)((G2APIError().AnimTest(ghl_info,m,__FILE__,__LINE__), 0) )
#else

#define G2ERROR(exp,m)		((void)0)
#define G2WARNING(exp,m)     ((void)0)
#define G2NOTE(exp,m)     ((void)0)
#define G2ANIM(ghl_info,m) ((void)0)

#endif

#ifdef _DEBUG
void G2_Bone_Not_Found(const char* bone_name, const char* modName)
{
	G2ERROR(bone_name, "NULL Bone Name");
	G2ERROR(bone_name[0], "Empty Bone Name");
	if (bone_name)
	{
		G2NOTE(0, va("Bone Not Found (%s:%s)", bone_name, modName));
	}
}

void G2_Bolt_Not_Found(const char* bone_name)
{
	G2ERROR(bone_name, "NULL Bolt/Bone Name");
	G2ERROR(bone_name[0], "Empty Bolt/Bone Name");
	if (bone_name)
	{
		G2NOTE(0, va("Bolt/Bone Not Found (%s:%s)", bone_name, modName));
	}
}
#endif

void G2API_SetTime(const int current_time, const int clock)
{
	assert(clock >= 0 && clock < NUM_G2T_TIME);
#if G2_DEBUG_TIME
	Com_Printf("Set Time: before c%6d  s%6d", G2TimeBases[1], G2TimeBases[0]);
#endif
	G2TimeBases[clock] = current_time;
	if (G2TimeBases[1] > G2TimeBases[0] + 200)
	{
		G2TimeBases[1] = 0; // use server time instead
	}
#if G2_DEBUG_TIME
	Com_Printf(" after c%6d  s%6d\n", G2TimeBases[1], G2TimeBases[0]);
#endif
}

int G2API_GetTime(int arg_time) // this may or may not return arg depending on ghoul2_time cvar
{
	int ret = G2TimeBases[1];
	if (!ret)
	{
		ret = G2TimeBases[0];
	}
	return ret;
}

// must be a power of two
#define MAX_G2_MODELS (1024) //jacesolaris was 512
#define G2_MODEL_BITS (10)   //jacesolaris was 9
#define G2_INDEX_MASK (MAX_G2_MODELS-1)

void RemoveBoneCache(const CBoneCache* bone_cache);

static size_t GetSizeOfGhoul2Info(const CGhoul2Info& g2_info)
{
	size_t size = 0;

	// This is pretty ugly, but we don't want to save everything in the CGhoul2Info object.
	size += offsetof(CGhoul2Info, mTransformedVertsArray) - offsetof(CGhoul2Info, mmodel_index);

	// Surface vector + size
	size += sizeof(int);
	size += g2_info.mSlist.size() * sizeof(surfaceInfo_t);

	// Bone vector + size
	size += sizeof(int);
	size += g2_info.mBlist.size() * sizeof(boneInfo_t);

	// Bolt vector + size
	size += sizeof(int);
	size += g2_info.mBltlist.size() * sizeof(boltInfo_t);

	return size;
}

static size_t SerializeGhoul2Info(char* buffer, const CGhoul2Info& g2_info)
{
	const char* base = buffer;

	// Oh the ugliness...
	size_t block_size = offsetof(CGhoul2Info, mTransformedVertsArray) - offsetof(CGhoul2Info, mmodel_index);
	memcpy(buffer, &g2_info.mmodel_index, block_size);
	buffer += block_size;

	// Surfaces vector + size
	*reinterpret_cast<int*>(buffer) = g2_info.mSlist.size();
	buffer += sizeof(int);

	block_size = g2_info.mSlist.size() * sizeof(surfaceInfo_t);
	memcpy(buffer, g2_info.mSlist.data(), g2_info.mSlist.size() * sizeof(surfaceInfo_t));
	buffer += block_size;

	// Bones vector + size
	*reinterpret_cast<int*>(buffer) = g2_info.mBlist.size();
	buffer += sizeof(int);

	block_size = g2_info.mBlist.size() * sizeof(boneInfo_t);
	memcpy(buffer, g2_info.mBlist.data(), g2_info.mBlist.size() * sizeof(boneInfo_t));
	buffer += block_size;

	// Bolts vector + size
	*reinterpret_cast<int*>(buffer) = g2_info.mBltlist.size();
	buffer += sizeof(int);

	block_size = g2_info.mBltlist.size() * sizeof(boltInfo_t);
	memcpy(buffer, g2_info.mBltlist.data(), g2_info.mBltlist.size() * sizeof(boltInfo_t));
	buffer += block_size;

	return static_cast<size_t>(buffer - base);
}

static size_t DeserializeGhoul2Info(const char* buffer, CGhoul2Info& g2_info) // vanilla sp
{
	const char* base = buffer;

	size_t size = offsetof(CGhoul2Info, mTransformedVertsArray) - offsetof(CGhoul2Info, mmodel_index);
	memcpy(&g2_info.mmodel_index, buffer, size);
	buffer += size;

	// Surfaces vector
	size = *(int*)buffer;
	buffer += sizeof(int);

	g2_info.mSlist.assign((surfaceInfo_t*)buffer, (surfaceInfo_t*)buffer + size);
	buffer += sizeof(surfaceInfo_t) * size;

	// Bones vector
	size = *(int*)buffer;
	buffer += sizeof(int);

	g2_info.mBlist.assign((boneInfo_t*)buffer, (boneInfo_t*)buffer + size);
	buffer += sizeof(boneInfo_t) * size;

	// Bolt vector
	size = *(int*)buffer;
	buffer += sizeof(int);

	g2_info.mBltlist.assign((boltInfo_t*)buffer, (boltInfo_t*)buffer + size);
	buffer += sizeof(boltInfo_t) * size;

	return static_cast<size_t>(buffer - base);
}

class Ghoul2InfoArray : public IGhoul2InfoArray
{
	std::vector<CGhoul2Info> m_infos_[MAX_G2_MODELS];
	int m_ids_[MAX_G2_MODELS]{};
	std::list<int> m_free_indecies_;

	void DeleteLow(const int idx)
	{
		{
			for (auto& model : m_infos_[idx])
			{
				RemoveBoneCache(model.mBoneCache);
				model.mBoneCache = nullptr;
			}
		}
		m_infos_[idx].clear();

		if ((m_ids_[idx] >> G2_MODEL_BITS) > (1 << (31 - G2_MODEL_BITS)))
		{
			m_ids_[idx] = MAX_G2_MODELS + idx; //rollover reset id to minimum value
			m_free_indecies_.push_back(idx);
		}
		else
		{
			m_ids_[idx] += MAX_G2_MODELS;
			m_free_indecies_.push_front(idx);
		}
	}

public:
	Ghoul2InfoArray()
	{
		for (size_t i = 0; i < MAX_G2_MODELS; i++)
		{
			m_ids_[i] = MAX_G2_MODELS + i;
			m_free_indecies_.push_back(i);
		}
	}

	size_t GetSerializedSize() const
	{
		size_t size = 0;

		size += sizeof(int); // size of m_free_indecies_ linked list
		size += m_free_indecies_.size() * sizeof(int);

		size += sizeof m_ids_;

		for (const auto& m_info : m_infos_)
		{
			size += sizeof(int); // size of the m_infos_[i] vector

			for (const auto& j : m_info)
			{
				size += GetSizeOfGhoul2Info(j);
			}
		}

		return size;
	}

	size_t Serialize(char* buffer) const
	{
		const char* base = buffer;

		// Free indices
		*reinterpret_cast<int*>(buffer) = m_free_indecies_.size();
		buffer += sizeof(int);

		std::copy(m_free_indecies_.begin(), m_free_indecies_.end(), reinterpret_cast<int*>(buffer));
		buffer += sizeof(int) * m_free_indecies_.size();

		// IDs
		memcpy(buffer, m_ids_, sizeof m_ids_);
		buffer += sizeof m_ids_;

		// Ghoul2 infos
		for (const auto& m_info : m_infos_)
		{
			*reinterpret_cast<int*>(buffer) = m_info.size();
			buffer += sizeof(int);

			for (const auto& j : m_info)
			{
				buffer += SerializeGhoul2Info(buffer, j);
			}
		}

		return static_cast<size_t>(buffer - base);
	}

	size_t Deserialize(const char* buffer, size_t size)
	{
		const char* base = buffer;
		size_t count;

		// Free indices
		count = *(int*)buffer;
		buffer += sizeof(int);

		m_free_indecies_.assign((int*)buffer, (int*)buffer + count);
		buffer += sizeof(int) * count;

		// IDs
		memcpy(m_ids_, buffer, sizeof(m_ids_));
		buffer += sizeof(m_ids_);

		// Ghoul2 infos
		for (size_t i = 0; i < MAX_G2_MODELS; i++)
		{
			m_infos_[i].clear();

			count = *(int*)buffer;
			buffer += sizeof(int);

			m_infos_[i].resize(count);

			for (size_t j = 0; j < count; j++)
			{
				buffer += DeserializeGhoul2Info(buffer, m_infos_[i][j]);
			}
		}

		return static_cast<size_t>(buffer - base);
	}

	~Ghoul2InfoArray() override
	{
		if (m_free_indecies_.size() < MAX_G2_MODELS)
		{
#if G2API_DEBUG
			char mess[1000];
			sprintf(mess, "************************\nLeaked %ld ghoul2info slots\n", MAX_G2_MODELS - m_free_indecies_.size());
			Com_DPrintf(mess);
#endif
			for (int i = 0; i < MAX_G2_MODELS; i++)
			{
				std::list<int>::iterator j;
				for (j = m_free_indecies_.begin(); j != m_free_indecies_.end(); ++j)
				{
					if (*j == i)
						break;
				}
				if (j == m_free_indecies_.end())
				{
#if G2API_DEBUG
					sprintf(mess, "Leaked Info idx=%d id=%d sz=%ld\n", i, m_ids_[i], m_infos_[i].size());
					Com_DPrintf(mess);
					if (m_infos_[i].size())
					{
						sprintf(mess, "%s\n", m_infos_[i][0].mFileName);
						Com_DPrintf(mess);
					}
#endif
					DeleteLow(i);
				}
			}
		}
#if G2API_DEBUG
		else
		{
			Com_DPrintf("No ghoul2 info slots leaked\n");
		}
#endif
	}

	int New() override
	{
		if (m_free_indecies_.empty())
		{
			assert(0);
			Com_Error(ERR_FATAL, "Out of ghoul2 info slots");
		}
		// gonna pull from the front, doing a
		const int idx = *m_free_indecies_.begin();
		m_free_indecies_.erase(m_free_indecies_.begin());
		return m_ids_[idx];
	}

	bool IsValid(const int handle) const override
	{
		if (!handle)
		{
			return false;
		}
		assert(handle > 0); //negative handle???
		assert((handle & G2_INDEX_MASK) >= 0 && (handle & G2_INDEX_MASK) < MAX_G2_MODELS); //junk handle
		if (m_ids_[handle & G2_INDEX_MASK] != handle) // not a valid handle, could be old
		{
			return false;
		}
		return true;
	}

	void Delete(const int handle) override
	{
		if (!handle)
		{
			return;
		}
		assert(handle > 0); //null handle
		assert((handle & G2_INDEX_MASK) >= 0 && (handle & G2_INDEX_MASK) < MAX_G2_MODELS); //junk handle
		assert(m_ids_[handle & G2_INDEX_MASK] == handle); // not a valid handle, could be old or garbage
		if (m_ids_[handle & G2_INDEX_MASK] == handle)
		{
			DeleteLow(handle & G2_INDEX_MASK);
		}
	}

	std::vector<CGhoul2Info>& Get(const int handle) override
	{
		assert(handle > 0); //null handle
		assert((handle & G2_INDEX_MASK) >= 0 && (handle & G2_INDEX_MASK) < MAX_G2_MODELS); //junk handle
		assert(m_ids_[handle & G2_INDEX_MASK] == handle); // not a valid handle, could be old or garbage
		assert(
			!(handle <= 0 || (handle & G2_INDEX_MASK) < 0 || (handle & G2_INDEX_MASK) >= MAX_G2_MODELS || m_ids_[handle &
				G2_INDEX_MASK] != handle));

		return m_infos_[handle & G2_INDEX_MASK];
	}

	const std::vector<CGhoul2Info>& Get(const int handle) const override
	{
		assert(handle > 0);
		assert(m_ids_[handle & G2_INDEX_MASK] == handle); // not a valid handle, could be old or garbage
		return m_infos_[handle & G2_INDEX_MASK];
	}

#if G2API_DEBUG
	std::vector<CGhoul2Info>& GetDebug(int handle)
	{
		assert(!(handle <= 0 || (handle & G2_INDEX_MASK) < 0 || (handle & G2_INDEX_MASK) >= MAX_G2_MODELS || m_ids_[handle & G2_INDEX_MASK] != handle));

		return m_infos_[handle & G2_INDEX_MASK];
	}
	void TestAllAnims()
	{
		for (size_t j = 0; j < MAX_G2_MODELS; j++)
		{
			std::vector<CGhoul2Info>& ghoul2 = m_infos_[j];
			for (size_t i = 0; i < ghoul2.size(); i++)
			{
				if (G2_SetupModelPointers(&ghoul2[i]))
				{
					G2ANIM(&ghoul2[i], "Test All");
				}
			}
		}
	}

#endif
};
static Ghoul2InfoArray* singleton = NULL;
IGhoul2InfoArray& TheGhoul2InfoArray()
{
	if (!singleton) {
		singleton = new Ghoul2InfoArray;
	}
	return *singleton;
}

#if G2API_DEBUG
std::vector<CGhoul2Info>& DebugG2Info(int handle)
{
	return ((Ghoul2InfoArray*)(&TheGhoul2InfoArray()))->GetDebug(handle);
}

CGhoul2Info& DebugG2InfoI(int handle, int item)
{
	return ((Ghoul2InfoArray*)(&TheGhoul2InfoArray()))->GetDebug(handle)[item];
}

void TestAllGhoul2Anims()
{
	((Ghoul2InfoArray*)(&TheGhoul2InfoArray()))->TestAllAnims();
}

#endif

#define PERSISTENT_G2DATA "g2infoarray"

void RestoreGhoul2InfoArray()
{
	if (singleton == NULL)
	{
		// Create the ghoul2 info array
		TheGhoul2InfoArray();

		size_t size;
		const void* data = ri.PD_Load(PERSISTENT_G2DATA, &size);
		if (data == NULL)
		{
			return;
		}

#ifdef _DEBUG
		size_t read =
#endif // _DEBUG
			singleton->Deserialize((const char*)data, size);
		R_Free((void*)data);
#ifdef _DEBUG
		assert(read == size);
#endif
	}
}

void SaveGhoul2InfoArray()
{
	const size_t size = singleton->GetSerializedSize();
	void* data = R_Malloc(size, TAG_GHOUL2, qfalse);
#ifdef _DEBUG
	const size_t written =
#endif // _DEBUG
		singleton->Serialize(static_cast<char*>(data));
#ifdef _DEBUG
	assert(written == size);
#endif // _DEBUG
	if (!ri.PD_Store(PERSISTENT_G2DATA, data, size))
	{
		Com_Printf(S_COLOR_RED "ERROR: Failed to store persistent renderer data.\n");
	}
}

// this is the ONLY function to read entity states directly
void G2API_CleanGhoul2Models(CGhoul2Info_v& ghoul2)
{
#ifdef _G2_GORE
	G2API_ClearSkinGore(ghoul2);
#endif
	ghoul2.~CGhoul2Info_v();
}

qhandle_t G2API_PrecacheGhoul2Model(const char* file_name)
{
	return RE_RegisterModel(file_name);
}

// initialise all that needs to be on a new Ghoul II model
int G2API_InitGhoul2Model(CGhoul2Info_v& ghoul2, const char* file_name, const int model_index, const qhandle_t custom_skin, const qhandle_t custom_shader, const int model_flags, const int lod_bias)
{
	int model;

	G2ERROR(file_name && file_name[0], "NULL filename");

	if (!file_name || !file_name[0])
	{
		assert(file_name[0]);
		return -1;
	}

	// find a free spot in the list
	for (model = 0; model < ghoul2.size(); model++)
	{
		if (ghoul2[model].mmodel_index == -1)
		{
			ghoul2[model] = CGhoul2Info();
			break;
		}
	}
	if (model == ghoul2.size())
	{
		assert(model < 8); //arb, just catching run-away models
		CGhoul2Info info;
		Q_strncpyz(info.mFileName, file_name, sizeof info.mFileName);
		info.mmodel_index = 0;
		if (G2_TestModelPointers(&info))
		{
			ghoul2.push_back(CGhoul2Info());
		}
		else
		{
			return -1;
		}
	}

	Q_strncpyz(ghoul2[model].mFileName, file_name, sizeof ghoul2[model].mFileName);
	ghoul2[model].mmodel_index = model;
	if (!G2_TestModelPointers(&ghoul2[model]))
	{
		ghoul2[model].mFileName[0] = 0;
		ghoul2[model].mmodel_index = -1;
	}
	else
	{
		G2_Init_Bone_List(ghoul2[model].mBlist, ghoul2[model].aHeader->numBones);
		G2_Init_Bolt_List(ghoul2[model].mBltlist);
		ghoul2[model].mCustomShader = custom_shader;
		ghoul2[model].mCustomSkin = custom_skin;
		ghoul2[model].mLodBias = lod_bias;
		ghoul2[model].mAnimFrameDefault = 0;
		ghoul2[model].mFlags = 0;

		ghoul2[model].mModelBoltLink = -1;
	}
	return ghoul2[model].mmodel_index;
}

qboolean G2API_SetLodBias(CGhoul2Info* ghl_info, const int lod_bias)
{
	G2ERROR(ghl_info, "NULL ghl_info");
	if (G2_SetupModelPointers(ghl_info))
	{
		ghl_info->mLodBias = lod_bias;
		return qtrue;
	}
	return qfalse;
}

extern void G2API_SetSurfaceOnOffFromSkin(CGhoul2Info* ghl_info, qhandle_t render_skin); //tr_ghoul2.cpp
qboolean G2API_SetSkin(CGhoul2Info* ghl_info, const qhandle_t custom_skin, const qhandle_t render_skin)
{
	G2ERROR(ghl_info, "NULL ghl_info");
#ifdef JK2_MODE
	if (G2_SetupModelPointers(ghl_info))
	{
		ghl_info->mCustomSkin = custom_skin;
		return qtrue;
	}
	return qfalse;
#else
	if (G2_SetupModelPointers(ghl_info))
	{
		ghl_info->mCustomSkin = custom_skin;
		if (render_skin)
		{
			//this is going to set the surfs on/off matching the skin file
			G2API_SetSurfaceOnOffFromSkin(ghl_info, render_skin);
		}
		return qtrue;
	}
#endif
	return qfalse;
}

qboolean G2API_SetShader(CGhoul2Info* ghl_info, const qhandle_t custom_shader)
{
	G2ERROR(ghl_info, "NULL ghl_info");
	if (G2_SetupModelPointers(ghl_info))
	{
		ghl_info->mCustomShader = custom_shader;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetSurfaceOnOff(CGhoul2Info* ghl_info, const char* surface_name, const int flags)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(!(flags & ~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS)), "G2API_SetSurfaceOnOff Illegal Flags");
		// ensure we flush the cache
		ghl_info->mMeshFrameNum = 0;
		return G2_SetSurfaceOnOff(ghl_info, surface_name, flags);
	}
	return qfalse;
}

qboolean G2API_SetRootSurface(CGhoul2Info_v& ghl_info, const int model_index, const char* surface_name)
{
	G2ERROR(ghl_info.IsValid(), "Invalid ghl_info");
	G2ERROR(surface_name, "Invalid surface_name");
	if (G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(model_index >= 0 && model_index < ghl_info.size(), "Bad Model Index");
		if (model_index >= 0 && model_index < ghl_info.size())
		{
			return G2_SetRootSurface(ghl_info, model_index, surface_name);
		}
	}
	return qfalse;
}

int G2API_AddSurface(CGhoul2Info* ghl_info, const int surface_number, const int poly_number, const float barycentric_i, const float barycentric_j, const int lod)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		// ensure we flush the cache
		ghl_info->mMeshFrameNum = 0;
		return G2_AddSurface(ghl_info, surface_number, poly_number, barycentric_i, barycentric_j, lod);
	}
	return -1;
}

qboolean G2API_RemoveSurface(CGhoul2Info* ghl_info, const int index)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		// ensure we flush the cache
		ghl_info->mMeshFrameNum = 0;
		return G2_RemoveSurface(ghl_info->mSlist, index);
	}
	return qfalse;
}

int G2API_GetParentSurface(CGhoul2Info* ghl_info, const int index)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		return G2_GetParentSurface(ghl_info, index);
	}
	return -1;
}

int G2API_GetSurfaceRenderStatus(CGhoul2Info* ghl_info, const char* surface_name)
{
	G2ERROR(surface_name, "Invalid surface_name");
	if (G2_SetupModelPointers(ghl_info))
	{
		return G2_IsSurfaceRendered(ghl_info, surface_name, ghl_info->mSlist);
	}
	return -1;
}

qboolean G2API_RemoveGhoul2Model(CGhoul2Info_v& ghl_info, const int model_index)
{
	// sanity check
	if (!ghl_info.size() || ghl_info.size() <= model_index || model_index < 0 || ghl_info[model_index].mmodel_index < 0)
	{
		// if we hit this assert then we are trying to delete a ghoul2 model on a ghoul2 instance that
		// one way or another is already gone.
		G2ERROR(0, "Remove Nonexistant Model");
		assert(0 && "remove non existing model");
		return qfalse;
	}

#ifdef _G2_GORE
	// Cleanup the gore attached to this model
	if (ghl_info[model_index].mGoreSetTag)
	{
		DeleteGoreSet(ghl_info[model_index].mGoreSetTag);
		ghl_info[model_index].mGoreSetTag = 0;
	}
#endif

	RemoveBoneCache(ghl_info[model_index].mBoneCache);
	ghl_info[model_index].mBoneCache = nullptr;

	// set us to be the 'not active' state
	ghl_info[model_index].mmodel_index = -1;
	ghl_info[model_index].mFileName[0] = 0;

	ghl_info[model_index] = CGhoul2Info();
	return qtrue;
}

//rww - RAGDOLL_BEGIN
#define		GHOUL2_RAG_STARTED						0x0010
#define		GHOUL2_RAG_FORCESOLVE					0x1000		//api-override, determine if ragdoll should be forced to continue solving even if it thinks it is settled

//rww - RAGDOLL_END

int G2API_GetAnimIndex(const CGhoul2Info* ghl_info)
{
	if (ghl_info)
	{
		return ghl_info->animModelIndexOffset;
	}
	return 0;
}

qboolean G2API_SetAnimIndex(CGhoul2Info* ghl_info, const int index)
{
	// Is This A Valid G2 Model?
	//---------------------------
	if (ghl_info)
	{
		// Is This A New Anim Index?
		//---------------------------
		if (ghl_info->animModelIndexOffset != index)
		{
			ghl_info->animModelIndexOffset = index;
			ghl_info->currentAnimModelSize = 0; // Clear anim size so SetupModelPointers recalcs

			//			RemoveBoneCache(ghl_info[0].mBoneCache);
			//			ghl_info[0].mBoneCache=0;

			// Kill All Existing Animation, Blending, Etc.
			//---------------------------------------------
			for (auto& info : ghl_info->mBlist)
			{
				info.flags &= ~(BONE_ANIM_TOTAL);
				info.flags &= ~(BONE_ANGLES_TOTAL);
			}
		}
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_SetBoneAnimIndex(CGhoul2Info* ghl_info, const int index, const int start_frame, const int end_frame, const int flags, const float anim_speed, const int acurrent_time, const float set_frame, const int blend_time)
{
	//rww - RAGDOLL_BEGIN
	if (ghl_info && ghl_info->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(start_frame >= 0, "startframe<0");
		G2ERROR(start_frame < ghl_info->aHeader->num_frames, "startframe>=numframes");
		G2ERROR(end_frame > 0, "endframe<=0");
		G2ERROR(end_frame <= ghl_info->aHeader->num_frames, "endframe>numframes");
		G2ERROR(set_frame < ghl_info->aHeader->num_frames, "setframe>=numframes");
		G2ERROR(set_frame == -1.0f || set_frame >= 0.0f, "setframe<0 but not -1");
		if (start_frame < 0 || start_frame >= ghl_info->aHeader->num_frames)
		{
			*const_cast<int*>(&start_frame) = 0; // cast away const
		}
		if (end_frame <= 0 || end_frame > ghl_info->aHeader->num_frames)
		{
			*const_cast<int*>(&end_frame) = 1;
		}
		if (set_frame != -1.0f && (set_frame < 0.0f || set_frame >= static_cast<float>(ghl_info->aHeader->num_frames)))
		{
			*const_cast<float*>(&set_frame) = 0.0f;
		}
		ghl_info->mSkelFrameNum = 0;
		G2ERROR(index >= 0 && index < (int)ghl_info->mBlist.size(),
			va("Out of Range Bone Index (%s)", ghl_info->mFileName));
		if (index >= 0 && index < static_cast<int>(ghl_info->mBlist.size()))
		{
			G2ERROR(ghl_info->mBlist[index].boneNumber >= 0, va("Bone Index is not Active (%s)", ghl_info->mFileName));
			const int current_time = G2API_GetTime(acurrent_time);
#if 0
			/*
			if ( ge->ValidateAnimRange( start_frame, end_frame, anim_speed ) == -1 )
			{
				int wtf = 1;
			}
			*/
#endif
			ret = G2_Set_Bone_Anim_Index(ghl_info->mBlist, index, start_frame, end_frame, flags, anim_speed, current_time,
				set_frame, blend_time, ghl_info->aHeader->num_frames);
			G2ANIM(ghl_info, "G2API_SetBoneAnimIndex");
		}
	}
	G2WARNING(ret, va("G2API_SetBoneAnimIndex Failed (%s)", ghl_info->mFileName));
	return ret;
}

qboolean G2API_SetBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int start_frame, const int end_frame, const int flags, const float anim_speed, const int acurrent_time, const float set_frame, const int blend_time)
{
	if (ghl_info && ghl_info->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}

	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(start_frame >= 0, "startframe<0");
		G2ERROR(start_frame < ghl_info->aHeader->num_frames, "startframe>=numframes");
		G2ERROR(end_frame > 0, "endframe<=0");
		G2ERROR(end_frame <= ghl_info->aHeader->num_frames, "endframe>numframes");
		G2ERROR(set_frame < ghl_info->aHeader->num_frames, "setframe>=numframes");
		G2ERROR(set_frame == -1.0f || set_frame >= 0.0f, "setframe<0 but not -1");
		if (start_frame < 0 || start_frame >= ghl_info->aHeader->num_frames)
		{
			*const_cast<int*>(&start_frame) = 0; // cast away const
		}
		if (end_frame <= 0 || end_frame > ghl_info->aHeader->num_frames)
		{
			*const_cast<int*>(&end_frame) = 1;
		}
		if (set_frame != -1.0f && (set_frame < 0.0f || set_frame >= static_cast<float>(ghl_info->aHeader->num_frames)))
		{
			*const_cast<float*>(&set_frame) = 0.0f;
		}
		ghl_info->mSkelFrameNum = 0;
		const int current_time = G2API_GetTime(acurrent_time);
		ret = G2_Set_Bone_Anim(ghl_info, ghl_info->mBlist, bone_name, start_frame, end_frame, flags, anim_speed, current_time, set_frame, blend_time);
		G2ANIM(ghl_info, "G2API_SetBoneAnim");
	}
	G2WARNING(ret, "G2API_SetBoneAnim Failed");
	return ret;
}

qboolean G2API_GetBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int acurrent_time, float* current_frame, int* start_frame, int* end_frame, int* flags, float* anim_speed, qhandle_t* model_list)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		ret = G2_Get_Bone_Anim(ghl_info, ghl_info->mBlist, bone_name, current_time, current_frame, start_frame, end_frame, flags, anim_speed);
	}
	G2WARNING(ret, "G2API_GetBoneAnim Failed");
	return ret;
}

qboolean G2API_GetBoneAnimIndex(CGhoul2Info* ghl_info, const int iBoneIndex, const int acurrent_time, float* current_frame, int* start_frame, int* end_frame, int* flags, float* anim_speed, qhandle_t* model_list)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		G2NOTE(iBoneIndex >= 0 && iBoneIndex < (int)ghl_info->mBlist.size(), va("Bad Bone Index (%d:%s)", iBoneIndex, ghl_info->mFileName));

		if (iBoneIndex >= 0 && iBoneIndex < static_cast<int>(ghl_info->mBlist.size()))
		{
			G2NOTE(ghl_info->mBlist[iBoneIndex].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE),
				"GetBoneAnim on non-animating bone.");
			if (ghl_info->mBlist[iBoneIndex].flags & (BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE))
			{
				int sf, ef;
				ret = G2_Get_Bone_Anim_Index(ghl_info->mBlist, // boneInfo_v &blist,
					iBoneIndex, // const int index,
					current_time, // const int current_time,
					current_frame, // float *current_frame,
					&sf, // int *start_frame,
					&ef, // int *end_frame,
					flags, // int *flags,
					anim_speed, // float *ret_anim_speed,
					ghl_info->aHeader->num_frames
				);
				G2ERROR(sf >= 0, "returning startframe<0");
				G2ERROR(sf < ghl_info->aHeader->num_frames, "returning startframe>=numframes");
				G2ERROR(ef > 0, "returning endframe<=0");
				G2ERROR(ef <= ghl_info->aHeader->num_frames, "returning endframe>numframes");
				if (current_frame)
				{
					G2ERROR(*current_frame >= 0.0f, "returning currentframe<0");
					G2ERROR(((int)(*current_frame)) < ghl_info->aHeader->num_frames, "returning currentframe>=numframes");
				}
				if (end_frame)
				{
					*end_frame = ef;
				}
				if (start_frame)
				{
					*start_frame = sf;
				}
				G2ANIM(ghl_info, "G2API_GetBoneAnimIndex");
			}
		}
	}
	if (!ret)
	{
		if (end_frame) *end_frame = 1;
		if (start_frame) *start_frame = 0;
		if (flags) *flags = 0;
		if (current_frame) *current_frame = 0.0f;
		if (anim_speed) *anim_speed = 1.0f;
	}
	G2NOTE(ret, "G2API_GetBoneAnimIndex Failed");
	return ret;
}

qboolean G2API_GetAnimRange(CGhoul2Info* ghl_info, const char* bone_name, int* start_frame, int* end_frame)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		ret = G2_Get_Bone_Anim_Range(ghl_info, ghl_info->mBlist, bone_name, start_frame, end_frame);
		G2ANIM(ghl_info, "G2API_GetAnimRange");
	}
	//	looks like the game checks the return value
	//	G2WARNING(ret,"G2API_GetAnimRange Failed");
	return ret;
}

qboolean G2API_GetAnimRangeIndex(CGhoul2Info* ghl_info, const int bone_index, int* start_frame, int* end_frame)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(bone_index >= 0 && bone_index < (int)ghl_info->mBlist.size(), "Bad Bone Index");
		if (bone_index >= 0 && bone_index < static_cast<int>(ghl_info->mBlist.size()))
		{
			ret = G2_Get_Bone_Anim_Range_Index(ghl_info->mBlist, bone_index, start_frame, end_frame);
			G2ANIM(ghl_info, "G2API_GetAnimRange");
		}
	}
	//	looks like the game checks the return value
	//	G2WARNING(ret,"G2API_GetAnimRangeIndex Failed");
	return ret;
}

qboolean G2API_PauseBoneAnim(CGhoul2Info* ghl_info, const char* bone_name, const int acurrent_time)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		ret = G2_Pause_Bone_Anim(ghl_info, ghl_info->mBlist, bone_name, current_time);
		G2ANIM(ghl_info, "G2API_PauseBoneAnim");
	}
	G2NOTE(ret, "G2API_PauseBoneAnim Failed");
	return ret;
}

qboolean G2API_PauseBoneAnimIndex(CGhoul2Info* ghl_info, const int bone_index, const int acurrent_time)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		G2ERROR(bone_index >= 0 && bone_index < (int)ghl_info->mBlist.size(), "Bad Bone Index");
		if (bone_index >= 0 && bone_index < static_cast<int>(ghl_info->mBlist.size()))
		{
			ret = G2_Pause_Bone_Anim_Index(ghl_info->mBlist, bone_index, current_time, ghl_info->aHeader->num_frames);
			G2ANIM(ghl_info, "G2API_PauseBoneAnimIndex");
		}
	}
	G2WARNING(ret, "G2API_PauseBoneAnimIndex Failed");
	return ret;
}

qboolean G2API_IsPaused(CGhoul2Info* ghl_info, const char* bone_name)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		ret = G2_IsPaused(ghl_info, ghl_info->mBlist, bone_name);
	}
	G2WARNING(ret, "G2API_IsPaused Failed");
	return ret;
}

qboolean G2API_StopBoneAnimIndex(CGhoul2Info* ghl_info, const int index)
{
	qboolean ret = qfalse;
	G2ERROR(ghl_info, "NULL ghl_info");
	if (G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(index >= 0 && index < (int)ghl_info->mBlist.size(), "Bad Bone Index");
		if (index >= 0 && index < static_cast<int>(ghl_info->mBlist.size()))
		{
			ret = G2_Stop_Bone_Anim_Index(ghl_info->mBlist, index);
			G2ANIM(ghl_info, "G2API_StopBoneAnimIndex");
		}
	}
	return ret;
}

qboolean G2API_StopBoneAnim(CGhoul2Info* ghl_info, const char* bone_name)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		ret = G2_Stop_Bone_Anim(ghl_info, ghl_info->mBlist, bone_name);
		G2ANIM(ghl_info, "G2API_StopBoneAnim");
	}
	G2WARNING(ret, "G2API_StopBoneAnim Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesOffsetIndex(CGhoul2Info* ghl_info, const int index, const vec3_t angles, const int flags, const Eorientations yaw, const Eorientations pitch, const Eorientations roll, qhandle_t* model_list, const int blend_time, const int acurrent_time, const vec3_t offset)
{
	//rww - RAGDOLL_BEGIN
	if (ghl_info && ghl_info->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		G2ERROR(index >= 0 && index < (int)ghl_info->mBlist.size(), "G2API_SetBoneAnglesIndex:Invalid bone index");
		if (index >= 0 && index < static_cast<int>(ghl_info->mBlist.size()))
		{
			ret = G2_Set_Bone_Angles_Index(ghl_info, ghl_info->mBlist, index, angles, flags, yaw, pitch, roll, blend_time, current_time, offset);
		}
	}
	G2WARNING(ret, "G2API_SetBoneAnglesIndex Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesIndex(CGhoul2Info* ghl_info, const int index, const vec3_t angles, const int flags, const Eorientations yaw, const Eorientations pitch, const Eorientations roll, qhandle_t* model_list, const int blend_time, const int acurrent_time)
{
	return G2API_SetBoneAnglesOffsetIndex(ghl_info, index, angles, flags, yaw, pitch, roll, 0, blend_time, acurrent_time, nullptr);
}

qboolean G2API_SetBoneAnglesOffset(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t* model_list, const int blend_time, const int acurrent_time, const vec3_t offset)
{
	//rww - RAGDOLL_BEGIN
	if (ghl_info && ghl_info->mFlags & GHOUL2_RAG_STARTED)
	{
		return qfalse;
	}
	//rww - RAGDOLL_END

	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		ret = G2_Set_Bone_Angles(ghl_info, ghl_info->mBlist, bone_name, angles, flags, up, left, forward, blend_time, current_time, offset);
	}
	G2WARNING(ret, "G2API_SetBoneAngles Failed");
	return ret;
}

qboolean G2API_SetBoneAngles(CGhoul2Info* ghl_info, const char* bone_name, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t* model_list, const int blend_time, const int acurrent_time)
{
	return G2API_SetBoneAnglesOffset(ghl_info, bone_name, angles, flags, up, left, forward, nullptr, blend_time, acurrent_time, nullptr);
}

qboolean G2API_SetBoneAnglesMatrixIndex(CGhoul2Info* ghl_info, const int index, const mdxaBone_t& matrix, const int flags, qhandle_t* model_list, const int blend_time, const int acurrent_time)
{
	qboolean ret = qfalse;

	if (G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		G2ERROR(index >= 0 && index < (int)ghl_info->mBlist.size(), "Bad Bone Index");
		if (index >= 0 && index < static_cast<int>(ghl_info->mBlist.size()))
		{
			ret = G2_Set_Bone_Angles_Matrix_Index(ghl_info->mBlist, index, matrix, flags, blend_time, current_time);
		}
	}
	G2WARNING(ret, "G2API_SetBoneAnglesMatrixIndex Failed");
	return ret;
}

qboolean G2API_SetBoneAnglesMatrix(CGhoul2Info* ghl_info, const char* bone_name, const mdxaBone_t& matrix, const int flags, qhandle_t* model_list, const int blend_time, const int acurrent_time)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		const int current_time = G2API_GetTime(acurrent_time);
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		ret = G2_Set_Bone_Angles_Matrix(ghl_info, ghl_info->mBlist, bone_name, matrix, flags, blend_time, current_time);
	}
	G2WARNING(ret, "G2API_SetBoneAnglesMatrix Failed");
	return ret;
}

qboolean G2API_StopBoneAnglesIndex(CGhoul2Info* ghl_info, const int index)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		G2ERROR(index >= 0 && index < (int)ghl_info->mBlist.size(), "Bad Bone Index");
		if (index >= 0 && index < static_cast<int>(ghl_info->mBlist.size()))
		{
			ret = G2_Stop_Bone_Angles_Index(ghl_info->mBlist, index);
		}
	}
	G2WARNING(ret, "G2API_StopBoneAnglesIndex Failed");
	return ret;
}

qboolean G2API_StopBoneAngles(CGhoul2Info* ghl_info, const char* bone_name)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		ret = G2_Stop_Bone_Angles(ghl_info, ghl_info->mBlist, bone_name);
	}
	G2WARNING(ret, "G2API_StopBoneAngles Failed");
	return ret;
}

//rww - RAGDOLL_BEGIN
class CRagDollParams;
void G2_SetRagDoll(CGhoul2Info_v& ghoul2_v, CRagDollParams* parms);

void G2API_SetRagDoll(CGhoul2Info_v& ghoul2, CRagDollParams* parms)
{
	G2_SetRagDoll(ghoul2, parms);
}

//rww - RAGDOLL_END

qboolean G2API_RemoveBone(CGhoul2Info* ghl_info, const char* bone_name)
{
	qboolean ret = qfalse;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		// ensure we flush the cache
		ghl_info->mSkelFrameNum = 0;
		ret = G2_Remove_Bone(ghl_info, ghl_info->mBlist, bone_name);
		G2ANIM(ghl_info, "G2API_RemoveBone");
	}
	G2WARNING(ret, "G2API_RemoveBone Failed");
	return ret;
}

//rww - RAGDOLL_BEGIN
#ifdef _DEBUG
extern int ragTraceTime;
extern int ragSSCount;
extern int ragTraceCount;
#endif

void G2API_AnimateG2Models(CGhoul2Info_v& ghoul2, const int acurrent_time, CRagDollUpdateParams* params)
{
	const int current_time = G2API_GetTime(acurrent_time);

#ifdef _DEBUG
	ragTraceTime = 0;
	ragSSCount = 0;
	ragTraceCount = 0;
#endif

	// Walk the list and find all models that are active
	for (int model = 0; model < ghoul2.size(); model++)
	{
		if (ghoul2[model].mModel)
		{
			G2_Animate_Bone_List(ghoul2, current_time, model, params);
		}
	}
#ifdef _DEBUG
	//	Com_Printf("Rag trace time: %i (%i STARTSOLID, %i TOTAL)\n", ragTraceTime, ragSSCount, ragTraceCount);

	//	assert(ragTraceTime < 15);
	//assert(ragTraceCount < 600);
#endif
}

//rww - RAGDOLL_END

int G2_Find_Bone_Rag(const CGhoul2Info* ghl_info, const boneInfo_v& blist, const char* bone_name);
#define RAG_PCJ						(0x00001)
#define RAG_EFFECTOR				(0x00100)

static boneInfo_t* G2_GetRagBoneConveniently(CGhoul2Info_v& ghoul2, const char* bone_name)
{
	assert(ghoul2.size());
	CGhoul2Info* ghl_info = &ghoul2[0];

	if (!(ghl_info->mFlags & GHOUL2_RAG_STARTED))
	{
		//can't do this if not in ragdoll
		return nullptr;
	}

	const int bone_index = G2_Find_Bone_Rag(ghl_info, ghl_info->mBlist, bone_name);

	if (bone_index < 0)
	{
		//bad bone specification
		return nullptr;
	}

	boneInfo_t* bone = &ghl_info->mBlist[bone_index];

	if (!(bone->flags & BONE_ANGLES_RAGDOLL))
	{
		//only want to return rag bones
		return nullptr;
	}

	return bone;
}

qboolean G2API_RagPCJConstraint(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t min, vec3_t max)
{
	boneInfo_t* bone = G2_GetRagBoneConveniently(ghoul2, bone_name);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_PCJ))
	{
		//this function is only for PCJ bones
		return qfalse;
	}

	VectorCopy(min, bone->minAngles);
	VectorCopy(max, bone->maxAngles);

	return qtrue;
}

qboolean G2API_RagPCJGradientSpeed(CGhoul2Info_v& ghoul2, const char* bone_name, const float speed)
{
	boneInfo_t* bone = G2_GetRagBoneConveniently(ghoul2, bone_name);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_PCJ))
	{
		//this function is only for PCJ bones
		return qfalse;
	}

	bone->overGradSpeed = speed;

	return qtrue;
}

qboolean G2API_RagEffectorGoal(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t pos)
{
	boneInfo_t* bone = G2_GetRagBoneConveniently(ghoul2, bone_name);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_EFFECTOR))
	{
		//this function is only for effectors
		return qfalse;
	}

	if (!pos)
	{
		//go back to none in case we have one then
		bone->hasOverGoal = false;
	}
	else
	{
		VectorCopy(pos, bone->overGoalSpot);
		bone->hasOverGoal = true;
	}
	return qtrue;
}

qboolean G2API_GetRagBonePos(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t pos, vec3_t ent_angles, vec3_t entPos,
	vec3_t ent_scale)
{
	//do something?
	return qfalse;
}

qboolean G2API_RagEffectorKick(CGhoul2Info_v& ghoul2, const char* bone_name, vec3_t velocity)
{
	boneInfo_t* bone = G2_GetRagBoneConveniently(ghoul2, bone_name);

	if (!bone)
	{
		return qfalse;
	}

	if (!(bone->RagFlags & RAG_EFFECTOR))
	{
		//this function is only for effectors
		return qfalse;
	}

	bone->epVelocity[2] = 0;
	VectorAdd(bone->epVelocity, velocity, bone->epVelocity);
	bone->physicsSettled = false;

	return qtrue;
}

qboolean G2API_RagForceSolve(CGhoul2Info_v& ghoul2, const qboolean force)
{
	assert(ghoul2.size());
	CGhoul2Info* ghl_info = &ghoul2[0];

	if (!(ghl_info->mFlags & GHOUL2_RAG_STARTED))
	{
		//can't do this if not in ragdoll
		return qfalse;
	}

	if (force)
	{
		ghl_info->mFlags |= GHOUL2_RAG_FORCESOLVE;
	}
	else
	{
		ghl_info->mFlags &= ~GHOUL2_RAG_FORCESOLVE;
	}

	return qtrue;
}

qboolean G2_SetBoneIKState(CGhoul2Info_v& ghoul2, int time, const char* bone_name, int ik_state,
	sharedSetBoneIKStateParams_t* params);

qboolean G2API_SetBoneIKState(CGhoul2Info_v& ghoul2, const int time, const char* bone_name, const int ik_state,
	sharedSetBoneIKStateParams_t* params)
{
	return G2_SetBoneIKState(ghoul2, time, bone_name, ik_state, params);
}

qboolean G2_IKMove(CGhoul2Info_v& ghoul2, int time, sharedIKMoveParams_t* params);

qboolean G2API_IKMove(CGhoul2Info_v& ghoul2, const int time, sharedIKMoveParams_t* params)
{
	return G2_IKMove(ghoul2, time, params);
}

qboolean G2API_RemoveBolt(CGhoul2Info* ghl_info, const int index)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		ret = G2_Remove_Bolt(ghl_info->mBltlist, index);
	}
	G2WARNING(ret, "G2API_RemoveBolt Failed");
	return ret;
}

int G2API_AddBolt(CGhoul2Info* ghl_info, const char* bone_name)
{
	int ret = -1;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		ret = G2_Add_Bolt(ghl_info, ghl_info->mBltlist, ghl_info->mSlist, bone_name);
		G2NOTE(ret >= 0, va("G2API_AddBolt Failed (%s:%s)", bone_name, ghl_info->mFileName));
	}
	return ret;
}

int G2API_AddBoltSurfNum(CGhoul2Info* ghl_info, const int surf_index)
{
	int ret = -1;
	if (G2_SetupModelPointers(ghl_info))
	{
		ret = G2_Add_Bolt_Surf_Num(ghl_info, ghl_info->mBltlist, ghl_info->mSlist, surf_index);
	}
	G2WARNING(ret >= 0, "G2API_AddBoltSurfNum Failed");
	return ret;
}

qboolean G2API_AttachG2Model(CGhoul2Info* ghl_info, CGhoul2Info* ghl_info_to, int to_bolt_index, int to_model)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info) && G2_SetupModelPointers(ghl_info_to))
	{
		G2ERROR(toBoltIndex >= 0 && toBoltIndex < (int)ghlInfoTo->mBltlist.size(), "Invalid Bolt Index");
		G2ERROR(ghlInfoTo->mBltlist.size() > 0, "Empty Bolt List");
		assert(to_bolt_index >= 0);
		if (to_bolt_index >= 0 && ghl_info_to->mBltlist.size())
		{
			// make sure we have a model to attach, a model to attach to, and a bolt on that model
			if (ghl_info_to->mBltlist[to_bolt_index].boneNumber != -1 || ghl_info_to->mBltlist[to_bolt_index].surface_number != -
				1)
			{
				// encode the bolt address into the model bolt link
				to_model &= MODEL_AND;
				to_bolt_index &= BOLT_AND;
				ghl_info->mModelBoltLink = to_model << MODEL_SHIFT | to_bolt_index << BOLT_SHIFT;
				ret = qtrue;
			}
		}
	}
	G2WARNING(ret, "G2API_AttachG2Model Failed");
	return ret;
}

qboolean G2API_DetachG2Model(CGhoul2Info* ghl_info)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		ghl_info->mModelBoltLink = -1;
		return qtrue;
	}
	return qfalse;
}

qboolean G2API_AttachEnt(int* bolt_info, CGhoul2Info* ghl_info_to, int to_bolt_index, int ent_num, int to_model_num)
{
	qboolean ret = qfalse;
	G2ERROR(bolt_info, "NULL bolt_info");
	if (bolt_info && G2_SetupModelPointers(ghl_info_to))
	{
		// make sure we have a model to attach, a model to attach to, and a bolt on that model
		if (ghl_info_to->mBltlist.size() && (ghl_info_to->mBltlist[to_bolt_index].boneNumber != -1 || ghl_info_to->mBltlist[
			to_bolt_index].surface_number != -1))
		{
			// encode the bolt address into the model bolt link
			to_model_num &= MODEL_AND;
			to_bolt_index &= BOLT_AND;
			ent_num &= ENTITY_AND;
			*bolt_info = to_bolt_index << BOLT_SHIFT | to_model_num << MODEL_SHIFT | ent_num << ENTITY_SHIFT;
			ret = qtrue;
		}
	}
	G2WARNING(ret, "G2API_AttachEnt Failed");
	return ret;
}

void G2API_DetachEnt(int* bolt_info)
{
	G2ERROR(bolt_info, "NULL bolt_info");
	if (bolt_info)
	{
		*bolt_info = 0;
	}
}

bool G2_NeedsRecalc(CGhoul2Info* ghl_info, int frame_num);

qboolean G2API_GetBoltMatrix(CGhoul2Info_v& ghoul2, const int model_index, const int bolt_index, mdxaBone_t* matrix, const vec3_t angles, const vec3_t position, const int aframe_num, qhandle_t* model_list, const vec3_t scale)
{
	G2ERROR(ghoul2.IsValid(), "Invalid ghl_info");
	G2ERROR(matrix, "NULL matrix");
	G2ERROR(model_index >= 0 && model_index < ghoul2.size(), "Invalid ModelIndex");
	constexpr static mdxaBone_t identity_matrix =
	{
		{
			{0.0f, -1.0f, 0.0f, 0.0f},
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f}
		}
	};

	G2_GenerateWorldMatrix(angles, position);
	if (G2_SetupModelPointers(ghoul2))
	{
		if (matrix && model_index >= 0 && model_index < ghoul2.size())
		{
			const int frame_num = G2API_GetTime(aframe_num);
			CGhoul2Info* ghl_info = &ghoul2[model_index];
			G2ERROR(bolt_index >= 0 && (bolt_index < (int)ghl_info->mBltlist.size()), va("Invalid Bolt Index (%d:%s)", bolt_index, ghl_info->mFileName));

			if (bolt_index >= 0 && ghl_info && bolt_index < static_cast<int>(ghl_info->mBltlist.size()))
			{
				mdxaBone_t bolt;

				if (G2_NeedsRecalc(ghl_info, frame_num))
				{
					G2_ConstructGhoulSkeleton(ghoul2, frame_num, true, scale);
				}

				G2_GetBoltMatrixLow(*ghl_info, bolt_index, scale, bolt);
				// scale the bolt position by the scale factor for this model since at this point its still in model space
				if (scale[0])
				{
					bolt.matrix[0][3] *= scale[0];
				}
				if (scale[1])
				{
					bolt.matrix[1][3] *= scale[1];
				}
				if (scale[2])
				{
					bolt.matrix[2][3] *= scale[2];
				}
				VectorNormalize(reinterpret_cast<float*>(&bolt.matrix[0]));
				VectorNormalize(reinterpret_cast<float*>(&bolt.matrix[1]));
				VectorNormalize(reinterpret_cast<float*>(&bolt.matrix[2]));

				Multiply_3x4Matrix(matrix, &worldMatrix, &bolt);
#if G2API_DEBUG
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 4; j++)
					{
						assert(!Q_isnan(matrix->matrix[i][j]));
					}
				}
#endif // _DEBUG
				G2ANIM(ghl_info, "G2API_GetBoltMatrix");
				return qtrue;
			}
		}
	}
	else
	{
		G2WARNING(0, "G2API_GetBoltMatrix Failed on empty or bad model");
	}
	Multiply_3x4Matrix(matrix, &worldMatrix, (mdxaBone_t*)&identity_matrix);
	return qfalse;
}

void G2API_ListSurfaces(CGhoul2Info* ghl_info)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		G2_List_Model_Surfaces(ghl_info->mFileName);
	}
}

void G2API_ListBones(CGhoul2Info* ghl_info, const int frame)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		G2_List_Model_Bones(ghl_info->mFileName, frame);
	}
}

// decide if we have Ghoul2 models associated with this ghoul list or not
qboolean G2API_HaveWeGhoul2Models(const CGhoul2Info_v& ghoul2)
{
	return static_cast<qboolean>(ghoul2.IsValid());
}

// run through the Ghoul2 models and set each of the mModel values to the correct one from the cgs.gameModel offset lsit
void G2API_SetGhoul2ModelIndexes(CGhoul2Info_v& ghoul2, qhandle_t* model_list, const qhandle_t* skin_list)
{
	G2ERROR(ghoul2.IsValid(), "Invalid ghl_info");
	for (int i = 0; i < ghoul2.size(); i++)
	{
		if (ghoul2[i].mmodel_index != -1)
		{
			ghoul2[i].mSkin = skin_list[ghoul2[i].mCustomSkin];
		}
	}
}

char* G2API_GetAnimFileNameIndex(const qhandle_t model_index)
{
	const model_t* mod_m = R_GetModelByHandle(model_index);
	G2ERROR(mod_m && mod_m->mdxm, "Bad Model");
	if (mod_m && mod_m->mdxm)
	{
		return mod_m->mdxm->animName;
	}
	return "";
}

// as above, but gets the internal embedded name, not the name of the disk file.
// This is needed for some unfortunate jiggery-hackery to do with frameskipping & the animevents.cfg file
//
char* G2API_GetAnimFileInternalNameIndex(const qhandle_t model_index)
{
	const model_t* mod_a = R_GetModelByHandle(model_index);
	G2ERROR(mod_a && mod_a->mdxa, "Bad Model");
	if (mod_a && mod_a->mdxa)
	{
		return mod_a->mdxa->name;
	}
	return "";
}

/************************************************************************************************
 * G2API_GetAnimFileName
 *    obtains the name of a model's .gla (animation) file
 *
 * Input
 *    pointer to list of CGhoul2Info's, WraithID of specific model in that list
 *
 * Output
 *    true if a good filename was obtained, false otherwise
 *
 ************************************************************************************************/
qboolean G2API_GetAnimFileName(CGhoul2Info* ghl_info, char** filename)
{
	qboolean ret = qfalse;
	if (G2_SetupModelPointers(ghl_info))
	{
		ret = G2_GetAnimFileName(ghl_info->mFileName, filename);
	}
	G2WARNING(ret, "G2API_GetAnimFileName Failed");
	return ret;
}

/*
=======================
SV_Qsortentity_numbers
=======================
*/
static int QDECL QsortDistance(const void* a, const void* b)
{
	const float& ea = ((CCollisionRecord*)a)->mDistance;
	const float& eb = ((CCollisionRecord*)b)->mDistance;

	if (ea < eb)
	{
		return -1;
	}
	return 1;
}

void G2API_CollisionDetect(CCollisionRecord* collRecMap, CGhoul2Info_v& ghoul2, const vec3_t angles,
	const vec3_t position,
	const int aframe_number, int ent_num, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, CMiniHeap*,
	EG2_Collision e_g2_trace_type, int use_lod, float fRadius)
{
	G2ERROR(ghoul2.IsValid(), "Invalid ghl_info");
	G2ERROR(collRecMap, "NULL Collision Rec");
	if (G2_SetupModelPointers(ghoul2) && collRecMap)
	{
		int frameNumber = G2API_GetTime(aframe_number);

		vec3_t transRayStart, transRayEnd;

		// make sure we have transformed the whole skeletons for each model
		G2_ConstructGhoulSkeleton(ghoul2, frameNumber, true, scale);

		// pre generate the world matrix - used to transform the incoming ray
		G2_GenerateWorldMatrix(angles, position);

		ri.GetG2VertSpaceServer()->ResetHeap();

		// now having done that, time to build the model
#ifdef _G2_GORE
		G2_TransformModel(ghoul2, frameNumber, scale, ri.GetG2VertSpaceServer(), use_lod, false);
#else
		G2_TransformModel(ghoul2, frameNumber, scale, ri.GetG2VertSpaceServer(), use_lod);
#endif

		// model is built. Lets check to see if any triangles are actually hit.
		// first up, translate the ray to model space
		TransformAndTranslatePoint(rayStart, transRayStart, &worldMatrixInv);
		TransformAndTranslatePoint(rayEnd, transRayEnd, &worldMatrixInv);

		// now walk each model and check the ray against each poly - sigh, this is SO expensive. I wish there was a better way to do this.
#ifdef _G2_GORE
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, ent_num, e_g2_trace_type, use_lod, fRadius, 0, 0, 0,
			0, nullptr, qfalse);
#else
		G2_TraceModels(ghoul2, transRayStart, transRayEnd, collRecMap, ent_num, e_g2_trace_type, use_lod, fRadius);
#endif

		ri.GetG2VertSpaceServer()->ResetHeap();
		// now sort the resulting array of collision records so they are distance ordered
		qsort(collRecMap, MAX_G2_COLLISIONS,
			sizeof(CCollisionRecord), QsortDistance);
		G2ANIM(ghoul2, "G2API_CollisionDetect");
	}
}

qboolean G2API_SetGhoul2ModelFlags(CGhoul2Info* ghl_info, const int flags)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		ghl_info->mFlags &= GHOUL2_NEWORIGIN;
		ghl_info->mFlags |= flags;
		return qtrue;
	}
	return qfalse;
}

int G2API_GetGhoul2ModelFlags(CGhoul2Info* ghl_info)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		return ghl_info->mFlags & ~GHOUL2_NEWORIGIN;
	}
	return 0;
}

// given a boltmatrix, return in vec a normalised vector for the axis requested in flags
void G2API_GiveMeVectorFromMatrix(mdxaBone_t& bolt_matrix, const Eorientations flags, vec3_t& vec)
{
	switch (flags)
	{
	case ORIGIN:
		vec[0] = bolt_matrix.matrix[0][3];
		vec[1] = bolt_matrix.matrix[1][3];
		vec[2] = bolt_matrix.matrix[2][3];
		break;
	case POSITIVE_Y:
		vec[0] = bolt_matrix.matrix[0][1];
		vec[1] = bolt_matrix.matrix[1][1];
		vec[2] = bolt_matrix.matrix[2][1];
		break;
	case POSITIVE_X:
		vec[0] = bolt_matrix.matrix[0][0];
		vec[1] = bolt_matrix.matrix[1][0];
		vec[2] = bolt_matrix.matrix[2][0];
		break;
	case POSITIVE_Z:
		vec[0] = bolt_matrix.matrix[0][2];
		vec[1] = bolt_matrix.matrix[1][2];
		vec[2] = bolt_matrix.matrix[2][2];
		break;
	case NEGATIVE_Y:
		vec[0] = -bolt_matrix.matrix[0][1];
		vec[1] = -bolt_matrix.matrix[1][1];
		vec[2] = -bolt_matrix.matrix[2][1];
		break;
	case NEGATIVE_X:
		vec[0] = -bolt_matrix.matrix[0][0];
		vec[1] = -bolt_matrix.matrix[1][0];
		vec[2] = -bolt_matrix.matrix[2][0];
		break;
	case NEGATIVE_Z:
		vec[0] = -bolt_matrix.matrix[0][2];
		vec[1] = -bolt_matrix.matrix[1][2];
		vec[2] = -bolt_matrix.matrix[2][2];
		break;
	default:;
	}
}

// copy a model from one ghoul2 instance to another, and reset the root surface on the new model if need be
// NOTE if model_index = -1 then copy all the models
void G2API_CopyGhoul2Instance(const CGhoul2Info_v& ghoul2_from, CGhoul2Info_v& ghoul2_to, int model_index)
{
	//Ensiform: I'm commenting this out because model_index appears unused and legitimately set in gamecode
	//assert(model_index==-1); // copy individual bolted parts is not used in jk2 and I didn't want to deal with it
	// if ya want it, we will add it back correctly

	G2ERROR(ghoul2_from.IsValid(), "Invalid ghl_info");
	if (ghoul2_from.IsValid())
	{
		ghoul2_to.DeepCopy(ghoul2_from);
#ifdef _G2_GORE //check through gore stuff then, as well.
		int model = 0;

		//(since we are sharing this gore set with the copied instance we will have to increment
		//the reference count - if the goreset is "removed" while the refcount is > 0, the refcount
		//is decremented to avoid giving other instances an invalid pointer -rww)
		while (model < ghoul2_to.size())
		{
			if (ghoul2_to[model].mGoreSetTag)
			{
				CGoreSet* gore = FindGoreSet(ghoul2_to[model].mGoreSetTag);
				assert(gore);
				if (gore)
				{
					gore->mRefCount++;
				}
			}

			model++;
		}
#endif
		G2ANIM(ghoul2_from, "G2API_CopyGhoul2Instance (source)");
		G2ANIM(ghoul2_to, "G2API_CopyGhoul2Instance (dest)");
	}
}

char* G2API_GetSurfaceName(CGhoul2Info* ghl_info, const int surfNumber)
{
	static char noSurface[1] = "";
	if (G2_SetupModelPointers(ghl_info))
	{
		const mdxmSurface_t* surf = static_cast<mdxmSurface_t*>(G2_FindSurface(ghl_info->current_model, surfNumber, 0));
		if (surf)
		{
			assert(G2_MODEL_OK(ghl_info));
			const auto surf_indexes = reinterpret_cast<mdxmHierarchyOffsets_t*>(reinterpret_cast<byte*>(ghl_info->current_model->mdxm) + sizeof(mdxmHeader_t));
			const auto surf_info = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(surf_indexes) + surf_indexes->offsets[surf->
				thisSurfaceIndex]);
			return surf_info->name;
		}
	}
	G2WARNING(0, "Surface Not Found");
	return noSurface;
}

int G2API_GetSurfaceIndex(CGhoul2Info* ghl_info, const char* surface_name)
{
	int ret = -1;
	G2ERROR(surface_name, "NULL surface_name");
	if (surface_name && G2_SetupModelPointers(ghl_info))
	{
		ret = G2_GetSurfaceIndex(ghl_info, surface_name);
	}
	G2WARNING(ret >= 0, "G2API_GetSurfaceIndex Failed");
	return ret;
}

char* G2API_GetGLAName(CGhoul2Info* ghl_info)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		assert(G2_MODEL_OK(ghl_info));
		return const_cast<char*>(ghl_info->aHeader->name);
		//return ghl_info->current_model->mdxm->animName;
	}
	return nullptr;
}

qboolean G2API_SetNewOrigin(CGhoul2Info* ghl_info, const int bolt_index)
{
	if (G2_SetupModelPointers(ghl_info))
	{
		G2ERROR(bolt_index >= 0 && bolt_index < (int)ghl_info->mBltlist.size(), "invalid bolt_index");

		if (bolt_index >= 0 && bolt_index < static_cast<int>(ghl_info->mBltlist.size()))
		{
			ghl_info->mNewOrigin = bolt_index;
			ghl_info->mFlags |= GHOUL2_NEWORIGIN;
		}
		return qtrue;
	}
	return qfalse;
}

int G2API_GetBoneIndex(CGhoul2Info* ghl_info, const char* bone_name, const qboolean bAddIfNotFound)
{
	int ret = -1;
	G2ERROR(bone_name, "NULL bone_name");
	if (bone_name && G2_SetupModelPointers(ghl_info))
	{
		ret = G2_Get_Bone_Index(ghl_info, bone_name, bAddIfNotFound);
		G2ANIM(ghl_info, "G2API_GetBoneIndex");
	}
	G2NOTE(ret >= 0, "G2API_GetBoneIndex Failed");
	return ret;
}

void G2API_SaveGhoul2Models(CGhoul2Info_v& ghoul2)
{
	G2ANIM(ghoul2, "G2API_SaveGhoul2Models");
	G2_SaveGhoul2Models(ghoul2);
}

void G2API_LoadGhoul2Models(CGhoul2Info_v& ghoul2, const char* buffer)
{
	G2_LoadGhoul2Model(ghoul2, buffer);
	G2ANIM(ghoul2, "G2API_LoadGhoul2Models");
}

// this is kinda sad, but I need to call the destructor in this module (exe), not the game.dll...
//
void G2API_LoadSaveCodeDestructGhoul2Info(CGhoul2Info_v& ghoul2)
{
	ghoul2.~CGhoul2Info_v(); // so I can load junk over it then memset to 0 without orphaning
}

#ifdef _G2_GORE
void ResetGoreTag(); // put here to reduce coupling

void G2API_ClearSkinGore(CGhoul2Info_v& ghoul2)
{
	for (int i = 0; i < ghoul2.size(); i++)
	{
		if (ghoul2[i].mGoreSetTag)
		{
			DeleteGoreSet(ghoul2[i].mGoreSetTag);
			ghoul2[i].mGoreSetTag = 0;
		}
	}
}

extern int G2_DecideTraceLod(const CGhoul2Info& ghoul2, int use_lod);

void G2API_AddSkinGore(CGhoul2Info_v& ghoul2, SSkinGoreData& gore)
{
	if (VectorLength(gore.rayDirection) < .1f)
	{
		assert(0); // can't add gore without a shot direction
		return;
	}

	// make sure we have transformed the whole skeletons for each model
	//G2_ConstructGhoulSkeleton(ghoul2, gore.current_time, NULL, true, gore.angles, gore.position, gore.scale, false);
	G2_ConstructGhoulSkeleton(ghoul2, gore.current_time, true, gore.scale);

	// pre generate the world matrix - used to transform the incoming ray
	G2_GenerateWorldMatrix(gore.angles, gore.position);

	// first up, translate the ray to model space
	vec3_t transRayDirection, transHitLocation;
	TransformAndTranslatePoint(gore.hitLocation, transHitLocation, &worldMatrixInv);
	TransformPoint(gore.rayDirection, transRayDirection, &worldMatrixInv);
	if (!gore.useTheta)
	{
		vec3_t t;
		VectorCopy(gore.uaxis, t);
		TransformPoint(t, gore.uaxis, &worldMatrixInv);
	}

	ResetGoreTag();
	const int lodbias = Com_Clamp(0, 2, G2_DecideTraceLod(ghoul2[0], r_lodbias->integer));
	const int maxLod = Com_Clamp(0, ghoul2[0].current_model->numLods, 3);
	//limit to the number of lods the main model has
	for (int lod = lodbias; lod < maxLod; lod++)
	{
		// now having done that, time to build the model
		ri.GetG2VertSpaceServer()->ResetHeap();

		G2_TransformModel(ghoul2, gore.current_time, gore.scale, ri.GetG2VertSpaceServer(), lod, true, &gore);

		// now walk each model and compute new texture coordinates
		G2_TraceModels(ghoul2, transHitLocation, transRayDirection, nullptr, gore.ent_num, G2_NOCOLLIDE, lod, 1.0f,
			gore.SSize, gore.TSize, gore.theta, gore.shader, &gore, qtrue);
	}
}
#else
void G2API_ClearSkinGore(CGhoul2Info_v& ghoul2)
{
}

void G2API_AddSkinGore(CGhoul2Info_v& ghoul2, SSkinGoreData& gore)
{
}
#endif

extern model_t* R_GetAnimModelByHandle(const CGhoul2Info* ghl_info, qhandle_t index);

bool G2_TestModelPointers(CGhoul2Info* ghl_info) // returns true if the model is properly set up
{
	G2ERROR(ghl_info, "NULL ghl_info");
	if (!ghl_info)
	{
		return false;
	}
	ghl_info->mValid = false;
	if (ghl_info->mmodel_index != -1)
	{
		ghl_info->mModel = RE_RegisterModel(ghl_info->mFileName);
		ghl_info->current_model = R_GetModelByHandle(ghl_info->mModel);
		if (ghl_info->current_model)
		{
			if (ghl_info->current_model->mdxm)
			{
				if (ghl_info->currentModelSize)
				{
					if (ghl_info->currentModelSize != ghl_info->current_model->mdxm->ofsEnd)
					{
						Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
					}
				}
				ghl_info->currentModelSize = ghl_info->current_model->mdxm->ofsEnd;
				ghl_info->animModel = R_GetModelByHandle(
					ghl_info->current_model->mdxm->animIndex + ghl_info->animModelIndexOffset);
				if (ghl_info->animModel)
				{
					ghl_info->aHeader = ghl_info->animModel->mdxa;
					G2ERROR(ghl_info->aHeader, va("Model has no mdxa (gla) %s", ghl_info->mFileName));
					if (!ghl_info->aHeader)
					{
						Com_Error(ERR_DROP, "Ghoul2 (test)Model has no mdxa (gla) %s", ghl_info->mFileName);
					}
					if (ghl_info->currentAnimModelSize)
					{
						if (ghl_info->currentAnimModelSize != ghl_info->aHeader->ofsEnd)
						{
							Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
						}
					}
					ghl_info->currentAnimModelSize = ghl_info->aHeader->ofsEnd;
					ghl_info->mValid = true;
				}
			}
		}
	}
	if (!ghl_info->mValid)
	{
		ghl_info->current_model = nullptr;
		ghl_info->currentModelSize = 0;
		ghl_info->animModel = nullptr;
		ghl_info->currentAnimModelSize = 0;
		ghl_info->aHeader = nullptr;
	}
	return ghl_info->mValid;
}

bool G2_SetupModelPointers(CGhoul2Info* ghl_info) // returns true if the model is properly set up
{
	G2ERROR(ghl_info, "NULL ghl_info");
	if (!ghl_info)
	{
		return false;
	}
	ghl_info->mValid = false;
	if (ghl_info->mmodel_index != -1)
	{
		G2ERROR(ghl_info->mFileName[0], "empty ghl_info->mFileName");
		ghl_info->mModel = RE_RegisterModel(ghl_info->mFileName);
		ghl_info->current_model = R_GetModelByHandle(ghl_info->mModel);
		G2ERROR(ghl_info->current_model, va("NULL Model (glm) %s", ghl_info->mFileName));
		if (ghl_info->current_model)
		{
			G2ERROR(ghl_info->current_model->mdxm, va("Model has no mdxm (glm) %s", ghl_info->mFileName));
			if (ghl_info->current_model->mdxm)
			{
				if (ghl_info->currentModelSize)
				{
					if (ghl_info->currentModelSize != ghl_info->current_model->mdxm->ofsEnd)
					{
						Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
					}
				}
				ghl_info->currentModelSize = ghl_info->current_model->mdxm->ofsEnd;
				G2ERROR(ghl_info->currentModelSize, va("Zero sized Model? (glm) %s", ghl_info->mFileName));

				ghl_info->animModel = R_GetAnimModelByHandle(
					ghl_info, ghl_info->current_model->mdxm->animIndex + ghl_info->animModelIndexOffset);
				G2ERROR(ghl_info->animModel, va("NULL Model (gla) %s", ghl_info->mFileName));
				if (ghl_info->animModel)
				{
					ghl_info->aHeader = ghl_info->animModel->mdxa;
					G2ERROR(ghl_info->aHeader, va("Model has no mdxa (gla) %s", ghl_info->mFileName));
					if (!ghl_info->aHeader)
					{
						Com_Error(ERR_DROP, "Ghoul2 (set up model poiinters)Model has no mdxa (gla) %s",
							ghl_info->mFileName);
					}
					if (ghl_info->currentAnimModelSize)
					{
						if (ghl_info->currentAnimModelSize != ghl_info->aHeader->ofsEnd)
						{
							Com_Error(ERR_DROP, "Ghoul2 model was reloaded and has changed, map must be restarted.\n");
						}
					}
					ghl_info->currentAnimModelSize = ghl_info->aHeader->ofsEnd;
					G2ERROR(ghl_info->currentAnimModelSize, va("Zero sized Model? (gla) %s", ghl_info->mFileName));
					ghl_info->mValid = true;
				}
			}
		}
	}
	if (!ghl_info->mValid)
	{
		ghl_info->current_model = nullptr;
		ghl_info->currentModelSize = 0;
		ghl_info->animModel = nullptr;
		ghl_info->currentAnimModelSize = 0;
		ghl_info->aHeader = nullptr;
	}
	return ghl_info->mValid;
}

bool G2_SetupModelPointers(CGhoul2Info_v& ghoul2) // returns true if any model is properly set up
{
	bool ret = false;
	for (int i = 0; i < ghoul2.size(); i++)
	{
		const bool r = G2_SetupModelPointers(&ghoul2[i]);
		ret = ret || r;
	}
	return ret;
}