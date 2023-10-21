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

#include "rd-common/tr_types.h"
#include "ghoul2/G2.h"
#include "ghoul2/g2_local.h"
#include "tr_local.h"

class CConstructBoneList
{
public:
	int surface_num;
	int* boneUsedList;
	surfaceInfo_v& rootSList;
	model_t* current_model;
	boneInfo_v& boneList;

	CConstructBoneList(
		const int initsurfaceNum,
		int* initboneUsedList,
		surfaceInfo_v& initrootSList,
		model_t* initcurrent_model,
		boneInfo_v& initboneList) :

		surface_num(initsurfaceNum),
		boneUsedList(initboneUsedList),
		rootSList(initrootSList),
		current_model(initcurrent_model),
		boneList(initboneList)
	{
	}
};

extern void G2_ConstructUsedBoneList(CConstructBoneList& CBL);

//=====================================================================================================================
// Surface List handling routines - so entities can determine what surfaces attached to a model are operational or not.

// find a particular surface in the surface override list
surfaceInfo_t* G2_FindOverrideSurface(const int surface_num, surfaceInfo_v& surface_list)
{
	// look through entire list
	for (auto& i : surface_list)
	{
		if (i.surface == surface_num)
		{
			return &i;
		}
	}
	// didn't find it.
	return nullptr;
}

// given a surface name, lets see if it's legal in the model
int G2_IsSurfaceLegal(void* mod, const char* surface_name, int* flags)
{
	const model_t* mod_m = static_cast<model_t*>(mod);
	auto surf = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(mod_m->mdxm) + mod_m->mdxm->ofsSurfHierarchy);

	for (int i = 0; i < mod_m->mdxm->numSurfaces; i++)
	{
		if (!Q_stricmp(surface_name, surf->name))
		{
			*flags = surf->flags;
			return i;
		}
		// find the next surface
		surf = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(surf) + reinterpret_cast<size_t>(&static_cast<mdxmSurfHierarchy_t*>(nullptr)->childIndexes[
			surf->numChildren]));
	}
	return -1;
}

/************************************************************************************************
 * G2_FindSurface
 *    find a surface in a ghoul2 surface override list based on it's name
 *
 * Input
 *    filename of model, surface list of model instance, name of surface, int to be filled in
 * with the index of this surface (defaults to NULL)
 *
 * Output
 *    pointer to surface if successful, false otherwise
 *
 ************************************************************************************************/
mdxmSurface_t* G2_FindSurface(const CGhoul2Info* ghl_info, const surfaceInfo_v& slist, const char* surface_name, int* surf_index)
{
	// find the model we want
	const auto mod = const_cast<model_t*>(ghl_info->current_model);
	const auto surf_indexes = reinterpret_cast<mdxmHierarchyOffsets_t*>(reinterpret_cast<byte*>(mod->mdxm) + sizeof(mdxmHeader_t));

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		assert(0);
		if (surf_index)
		{
			*surf_index = -1;
		}
		return nullptr;
	}

	// first find if we already have this surface in the list
	for (int i = slist.size() - 1; i >= 0; i--)
	{
		if (slist[i].surface != 10000 && slist[i].surface != -1)
		{
			const auto surf = static_cast<mdxmSurface_t*>(G2_FindSurface(mod, slist[i].surface, 0));
			// back track and get the surfinfo struct for this surface
			const mdxmSurfHierarchy_t* surf_info = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(surf_indexes) + surf_indexes->offsets[surf->
				thisSurfaceIndex]);

			// are these the droids we're looking for?
			if (!Q_stricmp(surf_info->name, surface_name))
			{
				// yup
				if (surf_index)
				{
					*surf_index = i;
				}
				return surf;
			}
		}
	}
	// didn't find it
	if (surf_index)
	{
		*surf_index = -1;
	}
	return nullptr;
}

// set a named surface off_flags - if it doesn't find a surface with this name in the list then it will add one.
qboolean G2_SetSurfaceOnOff(const CGhoul2Info* ghl_info, surfaceInfo_v& slist, const char* surface_name, const int off_flags)
{
	int					surf_index = -1;
	// find the model we want
	const auto mod = const_cast<model_t*>(ghl_info->current_model);

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		assert(0);
		return qfalse;
	}

	// first find if we already have this surface in the list
	const mdxmSurface_t* surf = G2_FindSurface(ghl_info, slist, surface_name, &surf_index);
	if (surf)
	{
		// set descendants value

		// slist[surf_index].off_flags = off_flags;
		// seems to me that we shouldn't overwrite the other flags.
		// the only bit we really care about in the incoming flags is the off bit
		slist[surf_index].off_flags &= ~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);
		slist[surf_index].off_flags |= off_flags & (G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);
		return qtrue;
	}
	// ok, not in the list already - in that case, lets verify this surface exists in the model mesh
	int	flags;
	const int surface_num = G2_IsSurfaceLegal(mod, surface_name, &flags);
	if (surface_num != -1)
	{
		int newflags = flags;
		// the only bit we really care about in the incoming flags is the off bit
		newflags &= ~(G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);
		newflags |= off_flags & (G2SURFACEFLAG_OFF | G2SURFACEFLAG_NODESCENDANTS);

		if (newflags != flags)
		{
			surfaceInfo_t temp_slist_entry;
			// insert here then because it changed, no need to add an override otherwise
			temp_slist_entry.off_flags = newflags;
			temp_slist_entry.surface = surface_num;

			slist.push_back(temp_slist_entry);
		}
		return qtrue;
	}
	return qfalse;
}

void G2_SetSurfaceOnOffFromSkin(CGhoul2Info* ghl_info, const qhandle_t render_skin)
{
	const skin_t* skin = R_GetSkinByHandle(render_skin);

	ghl_info->mSlist.clear(); //remove any overrides we had before.
	ghl_info->mMeshFrameNum = 0;

	for (int j = 0; j < skin->numSurfaces; j++)
	{
		// the names have both been lowercased
		//FIXME: why is this using the shader name and not the surface name?
		if (strcmp(static_cast<shader_t*>(skin->surfaces[j]->shader)->name, "*off") == 0)
		{
			G2_SetSurfaceOnOff(ghl_info, ghl_info->mSlist, skin->surfaces[j]->name, G2SURFACEFLAG_OFF);
		}
		else
		{
			int flags;
			const int surface_num = G2_IsSurfaceLegal((void*)ghl_info->current_model, skin->surfaces[j]->name, &flags);
			if (surface_num != -1 && !(flags & G2SURFACEFLAG_OFF)) //only turn on if it's not an "_off" surface
			{
				G2_SetSurfaceOnOff(ghl_info, ghl_info->mSlist, skin->surfaces[j]->name, 0);
			}
		}
	}
}

// return a named surfaces off flags - should tell you if this surface is on or off.
int G2_IsSurfaceOff(const CGhoul2Info* ghl_info, const surfaceInfo_v& slist, const char* surface_name)
{
	const model_t* mod = const_cast<model_t*>(ghl_info->current_model);
	int surf_index = -1;

	// did we find a ghoul 2 model or not?
	if (!mod->mdxm)
	{
		return 0;
	}

	// first find if we already have this surface in the list
	const mdxmSurface_t* surf = G2_FindSurface(ghl_info, slist, surface_name, &surf_index);
	if (surf)
	{
		// set descendants value
		return slist[surf_index].off_flags;
	}
	// ok, we didn't find it in the surface list. Lets look at the original surface then.

	auto surface = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(mod->mdxm) + mod->mdxm->ofsSurfHierarchy);

	for (int i = 0; i < mod->mdxm->numSurfaces; i++)
	{
		if (!Q_stricmp(surface_name, surface->name))
		{
			return surface->flags;
		}
		// find the next surface
		surface = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(surface) + reinterpret_cast<intptr_t>(&static_cast<mdxmSurfHierarchy_t*>(nullptr)->
			childIndexes[surface->numChildren]));
	}

	assert(0);
	return 0;
}

void G2_FindRecursiveSurface(model_t* current_model, int surface_num, surfaceInfo_v& root_list, int* active_surfaces)
{
	const mdxmSurface_t* surface = static_cast<mdxmSurface_t*>(G2_FindSurface(current_model, surface_num, 0));
	const auto surf_indexes = reinterpret_cast<mdxmHierarchyOffsets_t*>(reinterpret_cast<byte*>(current_model->mdxm) + sizeof(mdxmHeader_t));
	const mdxmSurfHierarchy_t* surf_info = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(surf_indexes) + surf_indexes->offsets[surface->
		thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	const surfaceInfo_t* surf_override = G2_FindOverrideSurface(surface_num, root_list);

	// really, we should use the default flags for this surface unless it's been overriden
	int off_flags = surf_info->flags;

	// set the off flags if we have some
	if (surf_override)
	{
		off_flags = surf_override->off_flags;
	}

	// if this surface is not off, indicate as such in the active surface list
	if (!(off_flags & G2SURFACEFLAG_OFF))
	{
		active_surfaces[surface_num] = 1;
	}
	else
		// if we are turning off all descendants, then stop this recursion now
		if (off_flags & G2SURFACEFLAG_NODESCENDANTS)
		{
			return;
		}

	// now recursively call for the children
	for (int i = 0; i < surf_info->numChildren; i++)
	{
		surface_num = surf_info->childIndexes[i];
		G2_FindRecursiveSurface(current_model, surface_num, root_list, active_surfaces);
	}
}

void G2_RemoveRedundantGeneratedSurfaces(surfaceInfo_v& slist, const int* active_surfaces)
{
	// walk the surface list, removing surface overrides or generated surfaces that are pointing at surfaces that aren't active anymore
	for (size_t i = 0; i < slist.size(); i++)
	{
		if (slist[i].surface != -1)
		{
			// is this a generated surface?
			if (slist[i].off_flags & G2SURFACEFLAG_GENERATED)
			{
				// if it's not in the list, remove it
				if (!active_surfaces[slist[i].genPolySurfaceIndex & 0xffff])
				{
					G2_RemoveSurface(slist, i);
				}
			}
			// no, so it does point back at a legal surface
			else
			{
				// if it's not in the list, remove it
				if (!active_surfaces[slist[i].surface])
				{
					G2_RemoveSurface(slist, i);
				}
			}
		}
	}
}

qboolean G2_SetRootSurface(CGhoul2Info_v& ghoul2, const int model_index, const char* surface_name)
{
	int flags;

	assert(ghoul2[model_index].current_model && ghoul2[model_index].animModel);

	const auto mod_m = const_cast<model_t*>(ghoul2[model_index].current_model);
	const model_t* mod_a = const_cast<model_t*>(ghoul2[model_index].animModel);

	// did we find a ghoul 2 model or not?
	if (!mod_m->mdxm)
	{
		return qfalse;
	}

	// first find if we already have this surface in the list
	const int surf = G2_IsSurfaceLegal(mod_m, surface_name, &flags);
	if (surf != -1)
	{
		// first see if this ghoul2 model already has this as a root surface
		if (ghoul2[model_index].mSurfaceRoot == surf)
		{
			return qtrue;
		}

		// set the root surface
		ghoul2[model_index].mSurfaceRoot = surf;

		// ok, now the tricky bits.
		// firstly, generate a list of active / on surfaces below the root point

		// gimme some space to put this list into
		const auto active_surfaces = static_cast<int*>(Z_Malloc(mod_m->mdxm->numSurfaces * 4, TAG_GHOUL2, qtrue));
		memset(active_surfaces, 0, mod_m->mdxm->numSurfaces * 4);
		const auto active_bones = static_cast<int*>(Z_Malloc(mod_a->mdxa->numBones * 4, TAG_GHOUL2, qtrue));
		memset(active_bones, 0, mod_a->mdxa->numBones * 4);

		G2_FindRecursiveSurface(mod_m, surf, ghoul2[model_index].mSlist, active_surfaces);

		// now generate the used bone list
		CConstructBoneList cbl(ghoul2[model_index].mSurfaceRoot,
			active_bones,
			ghoul2[model_index].mSlist,
			mod_m,
			ghoul2[model_index].mBlist);

		G2_ConstructUsedBoneList(cbl);

		// now remove all procedural or override surfaces that refer to surfaces that arent on this list
		G2_RemoveRedundantGeneratedSurfaces(ghoul2[model_index].mSlist, active_surfaces);

		// now remove all bones that are pointing at bones that aren't active
		G2_RemoveRedundantBoneOverrides(ghoul2[model_index].mBlist, active_bones);

		// then remove all bolts that point at surfaces or bones that *arent* active.
		G2_RemoveRedundantBolts(ghoul2[model_index].mBltlist, ghoul2[model_index].mSlist, active_surfaces, active_bones);

		// then remove all models on this ghoul2 instance that use those bolts that are being removed.
		for (int i = 0; i < ghoul2.size(); i++)
		{
			// are we even bolted to anything?
			if (ghoul2[i].mModelBoltLink != -1)
			{
				const int bolt_mod = ghoul2[i].mModelBoltLink >> MODEL_SHIFT & MODEL_AND;
				const int bolt_num = ghoul2[i].mModelBoltLink >> BOLT_SHIFT & BOLT_AND;
				// if either the bolt list is too small, or the bolt we are pointing at references nothing, remove this model
				if (static_cast<int>(ghoul2[bolt_mod].mBltlist.size()) <= bolt_num ||
					ghoul2[bolt_mod].mBltlist[bolt_num].boneNumber == -1 &&
					ghoul2[bolt_mod].mBltlist[bolt_num].surface_number == -1)
				{
					CGhoul2Info_v* g2i = &ghoul2;
					G2API_RemoveGhoul2Model(&g2i, i);
				}
			}
		}
		//No support for this, for now.

		// remember to free what we used
		Z_Free(active_surfaces);
		Z_Free(active_bones);

		return qtrue;
	}
	/*
	//g2r	if (entstate->ghoul2)
		{
			CGhoul2Info_v &ghoul2 = *((CGhoul2Info_v *)entstate->ghoul2);
			model_t				*mod_m = R_GetModelByHandle(RE_RegisterModel(ghoul2[model_index].mFileName));
			model_t				*mod_a = R_GetModelByHandle(mod_m->mdxm->animIndex);
			int					surf;
			int					flags;
			int					*activeSurfaces, *active_bones;

			// did we find a ghoul 2 model or not?
			if (!mod_m->mdxm)
			{
				return qfalse;
			}

			// first find if we already have this surface in the list
			surf = G2_IsSurfaceLegal(mod_m, surface_name, &flags);
			if (surf != -1)
			{
				// first see if this ghoul2 model already has this as a root surface
				if (ghoul2[model_index].mSurfaceRoot == surf)
				{
					return qtrue;
				}

				// set the root surface
				ghoul2[model_index].mSurfaceRoot = surf;

				// ok, now the tricky bits.
				// firstly, generate a list of active / on surfaces below the root point

				// gimme some space to put this list into
				activeSurfaces = (int *)Z_Malloc(mod_m->mdxm->numSurfaces * 4, TAG_GHOUL2, qtrue);
				memset(activeSurfaces, 0, (mod_m->mdxm->numSurfaces * 4));
				active_bones = (int *)Z_Malloc(mod_a->mdxa->numBones * 4, TAG_GHOUL2, qtrue);
				memset(active_bones, 0, (mod_a->mdxa->numBones * 4));

				G2_FindRecursiveSurface(mod_m, surf, ghoul2[model_index].mSlist, activeSurfaces);

				// now generate the used bone list
				CConstructBoneList	CBL(ghoul2[model_index].mSurfaceRoot,
									active_bones,
									ghoul2[model_index].mSlist,
									mod_m,
									ghoul2[model_index].mBlist);

				G2_ConstructUsedBoneList(CBL);

				// now remove all procedural or override surfaces that refer to surfaces that arent on this list
				G2_RemoveRedundantGeneratedSurfaces(ghoul2[model_index].mSlist, activeSurfaces);

				// now remove all bones that are pointing at bones that aren't active
				G2_RemoveRedundantBoneOverrides(ghoul2[model_index].mBlist, active_bones);

				// then remove all bolts that point at surfaces or bones that *arent* active.
				G2_RemoveRedundantBolts(ghoul2[model_index].mBltlist, ghoul2[model_index].mSlist, activeSurfaces, active_bones);

				// then remove all models on this ghoul2 instance that use those bolts that are being removed.
				for (int i=0; i<ghoul2.size(); i++)
				{
					// are we even bolted to anything?
					if (ghoul2[i].mModelBoltLink != -1)
					{
						int	boltMod = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
						int	boltNum = (ghoul2[i].mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;
						// if either the bolt list is too small, or the bolt we are pointing at references nothing, remove this model
						if ((ghoul2[boltMod].mBltlist.size() <= boltNum) ||
							((ghoul2[boltMod].mBltlist[boltNum].boneNumber == -1) &&
							 (ghoul2[boltMod].mBltlist[boltNum].surface_number == -1)))
						{
							G2API_RemoveGhoul2Model(entstate, i);
						}
					}
				}

				// remember to free what we used
				Z_Free(activeSurfaces);
				Z_Free(active_bones);

				return (qtrue);
			}
		}
		assert(0);*/
	return qfalse;
}

extern int G2_DecideTraceLod(const CGhoul2Info& ghoul2, int use_lod);

int G2_AddSurface(CGhoul2Info* ghoul2, const int surface_number, const int poly_number, const float barycentric_i, const float barycentric_j, int lod)
{
	surfaceInfo_t temp_slist_entry;

	// decide if LOD is legal
	lod = G2_DecideTraceLod(*ghoul2, lod);

	// first up, see if we have a free one already set up  - look only from the end of the constant surfaces onwards
	for (size_t i = 0; i < ghoul2->mSlist.size(); i++)
	{
		// is the surface count -1? That would indicate it's free
		if (ghoul2->mSlist[i].surface == -1)
		{
			ghoul2->mSlist[i].off_flags = G2SURFACEFLAG_GENERATED;
			ghoul2->mSlist[i].surface = 10000; // no model will ever have 10000 surfaces
			ghoul2->mSlist[i].genBarycentricI = barycentric_i;
			ghoul2->mSlist[i].genBarycentricJ = barycentric_j;
			ghoul2->mSlist[i].genPolySurfaceIndex = (poly_number & 0xffff) << 16 | surface_number & 0xffff;
			ghoul2->mSlist[i].genLod = lod;
			return i;
		}
	}

	// ok, didn't find one. Better create one

	temp_slist_entry.off_flags = G2SURFACEFLAG_GENERATED;
	temp_slist_entry.surface = 10000;
	temp_slist_entry.genBarycentricI = barycentric_i;
	temp_slist_entry.genBarycentricJ = barycentric_j;
	temp_slist_entry.genPolySurfaceIndex = (poly_number & 0xffff) << 16 | surface_number & 0xffff;
	temp_slist_entry.genLod = lod;

	ghoul2->mSlist.push_back(temp_slist_entry);

	return ghoul2->mSlist.size() - 1;
}

qboolean G2_RemoveSurface(surfaceInfo_v& slist, const int index)
{
	// did we find it?
	if (index != -1)
	{
		// set us to be the 'not active' state
		slist[index].surface = -1;

		unsigned int new_size = slist.size();
		// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
		for (int i = slist.size() - 1; i > -1; i--)
		{
			if (slist[i].surface == -1)
			{
				new_size = i;
			}
			// once we hit one that isn't a -1, we are done.
			else
			{
				break;
			}
		}
		// do we need to resize?
		if (new_size != slist.size())
		{
			// yes, so lets do it
			slist.resize(new_size);
		}

		return qtrue;
	}

	assert(0);

	// no
	return qfalse;
}

int G2_GetParentSurface(const CGhoul2Info* ghl_info, const int index)
{
	const auto mod = const_cast<model_t*>(ghl_info->current_model);
	const auto surf_indexes = reinterpret_cast<mdxmHierarchyOffsets_t*>(reinterpret_cast<byte*>(mod->mdxm) + sizeof(mdxmHeader_t));

	// walk each surface and see if this index is listed in it's children
	const mdxmSurface_t* surf = static_cast<mdxmSurface_t*>(G2_FindSurface(mod, index, 0));
	const mdxmSurfHierarchy_t* surf_info = reinterpret_cast<mdxmSurfHierarchy_t*>(reinterpret_cast<byte*>(surf_indexes) + surf_indexes->offsets[surf->
		thisSurfaceIndex]);

	return surf_info->parentIndex;
}

int G2_GetSurfaceIndex(const CGhoul2Info* ghl_info, const char* surface_name)
{
	const auto mod = const_cast<model_t*>(ghl_info->current_model);
	int flags;

	return G2_IsSurfaceLegal(mod, surface_name, &flags);
}

int G2_IsSurfaceRendered(const CGhoul2Info* ghl_info, const char* surface_name, const surfaceInfo_v& slist)
{
	int flags = 0; //, surfFlags = 0;
	int surf_index = 0;
	assert(ghl_info->current_model);
	assert(ghl_info->current_model->mdxm);
	if (!ghl_info->current_model->mdxm)
	{
		return -1;
	}

	// now travel up the skeleton to see if any of it's ancestors have a 'no descendants' turned on

	// find the original surface in the surface list
	int surf_num = G2_IsSurfaceLegal(const_cast<model_t*>(ghl_info->current_model), surface_name, &flags);
	if (surf_num != -1)
	{
		//must be legal
		const mdxmHierarchyOffsets_t* surf_indexes = reinterpret_cast<mdxmHierarchyOffsets_t*>(reinterpret_cast<byte*>(ghl_info->current_model->mdxm) +
			sizeof(mdxmHeader_t));
		const mdxmSurfHierarchy_t* surf_info = reinterpret_cast<mdxmSurfHierarchy_t*>((byte*)surf_indexes + surf_indexes->offsets[
			surf_num]);
		surf_num = surf_info->parentIndex;
		// walk the surface hierarchy up until we hit the root
		while (surf_num != -1)
		{
			int parent_flags = 0;

			const mdxmSurfHierarchy_t* parent_surf_info = reinterpret_cast<mdxmSurfHierarchy_t*>((byte*)surf_indexes + surf_indexes->offsets
				[
					surf_num]);

			// find the original surface in the surface list
			//G2 was error, above comment was accurate, but we don't want the original flags, we want the parent flags
			G2_IsSurfaceLegal(const_cast<model_t*>(ghl_info->current_model), parent_surf_info->name, &parent_flags);

			// now see if we already have overriden this surface in the slist
			const mdxmSurface_t* parent_surf = G2_FindSurface(ghl_info, slist, parent_surf_info->name, &surf_index);
			if (parent_surf)
			{
				// set descendants value
				parent_flags = slist[surf_index].off_flags;
			}
			// now we have the parent flags, lets see if any have the 'no descendants' flag set
			if (parent_flags & G2SURFACEFLAG_NODESCENDANTS)
			{
				flags |= G2SURFACEFLAG_OFF;
				break;
			}
			// set up scan of next parent
			surf_num = parent_surf_info->parentIndex;
		}
	}
	else
	{
		return -1;
	}
	if (flags == 0)
	{
		//it's not being overridden by a parent
		// now see if we already have overriden this surface in the slist
		const mdxmSurface_t* surf = G2_FindSurface(ghl_info, slist, surface_name, &surf_index);
		if (surf)
		{
			// set descendants value
			flags = slist[surf_index].off_flags;
		}
		// ok, at this point in flags we have what this surface is set to, and the index of the surface itself
	}
	return flags;
}