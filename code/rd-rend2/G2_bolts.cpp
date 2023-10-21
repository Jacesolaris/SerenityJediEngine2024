#include "server/exe_headers.h"

#ifndef __Q_SHARED_H
#include "qcommon/q_shared.h"
#endif

#if !defined(TR_LOCAL_H)
#include "tr_local.h"
#endif

#if !defined(G2_H_INC)
#include "ghoul2/G2.h"
#endif

#define G2_MODEL_OK(g) ((g)&&(g)->mValid&&(g)->aHeader&&(g)->current_model&&(g)->animModel)

//=====================================================================================================================
// Bolt List handling routines - so entities can attach themselves to any part of the model in question

// Given a bone number, see if that bone is already in our bone list
int G2_Find_Bolt_Bone_Num(boltInfo_v& bltlist, const int bone_num)
{
	if (bone_num == -1)
	{
		return -1;
	}

	// look through entire list
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		if (bltlist[i].boneNumber == bone_num)
		{
			return i;
		}
	}

	// didn't find it
	return -1;
}

// Given a bone number, see if that surface is already in our surfacelist list
int G2_Find_Bolt_Surface_Num(boltInfo_v& bltlist, const int surfaceNum, const int flags)
{
	if (surfaceNum == -1)
	{
		return -1;
	}

	// look through entire list
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		if ((bltlist[i].surface_number == surfaceNum) && ((bltlist[i].surfaceType & flags) == flags))
		{
			return i;
		}
	}

	// didn't find it
	return -1;
}

//=========================================================================================
//// Public Bolt Routines
int G2_Add_Bolt_Surf_Num(const CGhoul2Info* ghl_info, boltInfo_v& bltlist, const surfaceInfo_v& slist, const int surf_num)
{
	assert(ghl_info && ghl_info->mValid);
	boltInfo_t			temp_bolt;

	assert(surf_num >= 0 && surf_num < (int)slist.size());
	// first up, make sure have a surface first
	if (surf_num >= (int)slist.size())
	{
		return -1;
	}

	// look through entire list - see if it's already there first
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// already there??
		if (bltlist[i].surface_number == surf_num)
		{
			// increment the usage count
			bltlist[i].boltUsed++;
			return i;
		}
	}

	// we have a surface
	// look through entire list - see if it's already there first
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// if this surface entry has info in it, bounce over it
		if (bltlist[i].boneNumber == -1 && bltlist[i].surface_number == -1)
		{
			// if we found an entry that had a -1 for the bone / surface number, then we hit a surface / bone slot that was empty
			bltlist[i].surface_number = surf_num;
			bltlist[i].surfaceType = G2SURFACEFLAG_GENERATED;
			bltlist[i].boltUsed = 1;
			return i;
		}
	}

	// ok, we didn't find an existing surface of that name, or an empty slot. Lets add an entry
	temp_bolt.surface_number = surf_num;
	temp_bolt.surfaceType = G2SURFACEFLAG_GENERATED;
	temp_bolt.boneNumber = -1;
	temp_bolt.boltUsed = 1;
	bltlist.push_back(temp_bolt);
	return bltlist.size() - 1;
}

int G2_Add_Bolt(const CGhoul2Info* ghl_info, boltInfo_v& bltlist, const surfaceInfo_v& slist, const char* bone_name)
{
	assert(ghl_info && ghl_info->mValid);
	model_t* mod_m = (model_t*)ghl_info->current_model;
	model_t* mod_a = (model_t*)ghl_info->animModel;
	int x, surf_num = -1;
	mdxaSkel_t* skel;
	mdxaSkelOffsets_t* offsets;
	boltInfo_t temp_bolt;
	uint32_t flags;

	// first up, we'll search for that which this bolt names in all the surfaces
	surf_num = G2_IsSurfaceLegal(mod_m, bone_name, &flags);

	// did we find it as a surface?
	if (surf_num != -1)
	{
		// look through entire list - see if it's already there first
		for (size_t i = 0; i < bltlist.size(); i++)
		{
			// already there??
			if (bltlist[i].surface_number == surf_num)
			{
				// increment the usage count
				bltlist[i].boltUsed++;
				return i;
			}
		}

		// look through entire list - see if we can re-use one
		for (size_t i = 0; i < bltlist.size(); i++)
		{
			// if this surface entry has info in it, bounce over it
			if (bltlist[i].boneNumber == -1 && bltlist[i].surface_number == -1)
			{
				// if we found an entry that had a -1 for the bone / surface number, then we hit a surface / bone slot that was empty
				bltlist[i].surface_number = surf_num;
				bltlist[i].boltUsed = 1;
				bltlist[i].surfaceType = 0;
				return i;
			}
		}

		// ok, we didn't find an existing surface of that name, or an empty slot. Lets add an entry
		temp_bolt.surface_number = surf_num;
		temp_bolt.boneNumber = -1;
		temp_bolt.boltUsed = 1;
		temp_bolt.surfaceType = 0;
		bltlist.push_back(temp_bolt);
		return bltlist.size() - 1;
	}

	// no, check to see if it's a bone then

	mdxaHeader_t* mdxa = mod_a->data.gla;
	offsets = (mdxaSkelOffsets_t*)((byte*)mdxa + sizeof(mdxaHeader_t));

	// walk the entire list of bones in the gla file for this model and see if any match the name of the bone we want to find
	for (x = 0; x < mdxa->numBones; x++)
	{
		skel = (mdxaSkel_t*)((byte*)mdxa + sizeof(mdxaHeader_t) + offsets->offsets[x]);
		// if name is the same, we found it
		if (!Q_stricmp(skel->name, bone_name))
		{
			break;
		}
	}

	// check to see we did actually make a match with a bone in the model
	if (x == mdxa->numBones)
	{
		// didn't find it? Error
		//assert(0&&x == mod_a->mdxa->numBones);
#ifdef _DEBUG
//		Com_Printf("WARNING: %s not found on skeleton\n", bone_name);
#endif
		return -1;
	}

	// look through entire list - see if it's already there first
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// already there??
		if (bltlist[i].boneNumber == x)
		{
			// increment the usage count
			bltlist[i].boltUsed++;
			return i;
		}
	}

	// look through entire list - see if we can re-use it
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// if this bone entry has info in it, bounce over it
		if (bltlist[i].boneNumber == -1 && bltlist[i].surface_number == -1)
		{
			// if we found an entry that had a -1 for the bonenumber, then we hit a bone slot that was empty
			bltlist[i].boneNumber = x;
			bltlist[i].boltUsed = 1;
			bltlist[i].surfaceType = 0;
			return i;
		}
	}

	// ok, we didn't find an existing bone of that name, or an empty slot. Lets add an entry
	temp_bolt.boneNumber = x;
	temp_bolt.surface_number = -1;
	temp_bolt.boltUsed = 1;
	temp_bolt.surfaceType = 0;
	bltlist.push_back(temp_bolt);
	return bltlist.size() - 1;
}

// Given a model handle, and a bone name, we want to remove this bone from the bone override list
qboolean G2_Remove_Bolt(boltInfo_v& bltlist, int index)
{
	// did we find it?
	if (index != -1)
	{
		bltlist[index].boltUsed--;
		if (!bltlist[index].boltUsed)
		{
			// set this bone to not used
			bltlist[index].boneNumber = -1;
			bltlist[index].surface_number = -1;

			unsigned int newSize = bltlist.size();
			// now look through the list from the back and see if there is a block of -1's we can resize off the end of the list
			for (int i = bltlist.size() - 1; i > -1; i--)
			{
				if ((bltlist[i].surface_number == -1) && (bltlist[i].boneNumber == -1))
				{
					newSize = i;
				}
				// once we hit one that isn't a -1, we are done.
				else
				{
					break;
				}
			}
			// do we need to resize?
			if (newSize != bltlist.size())
			{
				// yes, so lets do it
				bltlist.resize(newSize);
			}
		}
		return qtrue;
	}

	assert(0);

	// no
	return qfalse;
}

// set the bolt list to all unused so the bone transformation routine ignores it.
void G2_Init_Bolt_List(boltInfo_v& bltlist)
{
	bltlist.clear();
}

// remove any bolts that reference original surfaces, generated surfaces, or bones that aren't active anymore
void G2_RemoveRedundantBolts(boltInfo_v& bltlist, surfaceInfo_v& slist, int* activeSurfaces, int* activeBones)
{
	// walk the bolt list
	for (size_t i = 0; i < bltlist.size(); i++)
	{
		// are we using this bolt?
		if ((bltlist[i].surface_number != -1) || (bltlist[i].boneNumber != -1))
		{
			// is this referenceing a surface?
			if (bltlist[i].surface_number != -1)
			{
				// is this bolt looking at a generated surface?
				if (bltlist[i].surfaceType)
				{
					// yes, so look for it in the surface list
					if (!G2_FindOverrideSurface(bltlist[i].surface_number, slist))
					{
						// no - we want to remove this bolt, regardless of how many people are using it
						bltlist[i].boltUsed = 1;
						G2_Remove_Bolt(bltlist, i);
					}
				}
				// no, it's an original, so look for it in the active surfaces list
				{
					if (!activeSurfaces[bltlist[i].surface_number])
					{
						// no - we want to remove this bolt, regardless of how many people are using it
						bltlist[i].boltUsed = 1;
						G2_Remove_Bolt(bltlist, i);
					}
				}
			}
			// no, must be looking at a bone then
			else
			{
				// is that bone active then?
				if (!activeBones[bltlist[i].boneNumber])
				{
					// no - we want to remove this bolt, regardless of how many people are using it
					bltlist[i].boltUsed = 1;
					G2_Remove_Bolt(bltlist, i);
				}
			}
		}
	}
}