/*
===========================================================================
Copyright (C) 2010 James Canete (use.less01@gmail.com)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_subs.c - common function replacements for modular renderer

#include "tr_local.h"

void QDECL Com_Printf(const char* msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

void QDECL Com_OPrintf(const char* msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);
#ifndef REND2_SP
	ri.OPrintf("%s", text);
#else
	ri.Printf(PRINT_ALL, "%s", text);
#endif
}

void QDECL Com_Error(int level, const char* error, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}

// ZONE
void* Z_Malloc(const int i_size, const memtag_t e_tag, const qboolean b_zeroit, const int iUnusedAlign)
{
	return ri.Malloc(i_size, e_tag, b_zeroit, iUnusedAlign);
}

void* R_Malloc(const int i_size, const memtag_t e_tag)
{
	return ri.Malloc(i_size, e_tag, qtrue, 4);
}

void* R_Malloc(const int i_size, const memtag_t e_tag, const qboolean b_zeroit)
{
	return ri.Malloc(i_size, e_tag, b_zeroit, 4);
}

#ifdef REND2_SP
void* R_Malloc(const int i_size, const memtag_t e_tag, const qboolean b_zeroit, const int iAlign)
{
	return ri.Malloc(i_size, e_tag, b_zeroit, iAlign);
}

int Z_Free(void* ptr) {
	return ri.Z_Free(ptr);
}

void R_Free(void* ptr)
{
	ri.Z_Free(ptr);
}
#else
void Z_Free(void* ptr) {
	ri.Z_Free(ptr);
}
#endif

int Z_MemSize(memtag_t e_tag) {
	return ri.Z_MemSize(e_tag);
}

void Z_MorphMallocTag(void* pvBuffer, memtag_t eDesiredTag) {
	ri.Z_MorphMallocTag(pvBuffer, eDesiredTag);
}

// HUNK
#ifdef REND2_SP
//void* Hunk_Alloc(int i_size, ha_pref preferences)
//{
//	return Hunk_Alloc(i_size, qtrue);
//}

void* Hunk_Alloc(int size, ha_pref preference) {
	return R_Malloc(size, TAG_HUNKALLOC, qtrue);
}

void* Hunk_AllocateTempMemory(int size) {
	// don't bother clearing, because we are going to load a file over it
	return R_Malloc(size, TAG_TEMP_HUNKALLOC, qfalse);
}

void Hunk_FreeTempMemory(void* buf)
{
	ri.Z_Free(buf);
}
#else

void* Hunk_AllocateTempMemory(int size) {
	return ri.Hunk_AllocateTempMemory(size);
}

void Hunk_FreeTempMemory(void* buf) {
	ri.Hunk_FreeTempMemory(buf);
}

void* Hunk_Alloc(int size, ha_pref preference) {
	return ri.Hunk_Alloc(size, preference);
}

int Hunk_MemoryRemaining(void) {
	return ri.Hunk_MemoryRemaining();
}
#endif