//------------------------------------------------------------------------
//  MAIN Program
//------------------------------------------------------------------------
//
//  DEH_EDGE  Copyright (C) 2004-2005  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License (in COPYING.txt) for more details.
//
//------------------------------------------------------------------------
//
//  DEH_EDGE is based on:
//
//  +  DeHackEd source code, by Greg Lewis.
//  -  DOOM source code (C) 1993-1996 id Software, Inc.
//  -  Linux DOOM Hack Editor, by Sam Lantinga.
//  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//
//------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "deh_i_defs.h"
#include "deh_edge.h"

#include "deh_ammo.h"
#include "deh_attacks.h"
#include "deh_buffer.h"
#include "deh_frames.h"
#include "deh_info.h"
#include "deh_misc.h"
#include "deh_patch.h"
#include "deh_rscript.h"
#include "deh_sounds.h"
#include "deh_storage.h"
#include "deh_system.h"
#include "deh_things.h"
#include "deh_text.h"
#include "deh_util.h"
#include "deh_wad.h"
#include "deh_weapons.h"

namespace Deh_Edge
{

class input_buffer_c
{
public:
	parse_buffer_api *buf;
	const char *infoname;
	bool is_lump;

	input_buffer_c(parse_buffer_api *_buf, const char *_info, bool _lump) :
		buf(_buf), is_lump(_lump)
	{
		infoname = StringDup(_info);
	}

	~input_buffer_c()
	{
		delete buf;
		buf = NULL;

		free((void*)infoname); 
	}
};

#define MAX_INPUTS  32

input_buffer_c *input_bufs[MAX_INPUTS];
int num_inputs = 0;

bool quiet_mode;
bool all_mode;

const dehconvfuncs_t *cur_funcs = NULL;


void Startup(void)
{
	System_Startup();

	Ammo::Startup();
	Frames::Startup();
	Misc::Startup();
	Rscript::Startup();
	Sounds::Startup();
	TextStr::Startup();
	Things::Startup();
	Weapons::Startup();

	Storage::Startup();

	/* reset parameters */

	num_inputs = 0;

	quiet_mode = false;
	all_mode = false;
}

dehret_e AddFile(const char *filename)
{
	if (num_inputs >= MAX_INPUTS)
	{
		SetErrorMsg("Too many input files !!\n");
		return DEH_E_BadArgs;
	}

	if (strlen(ReplaceExtension(filename, NULL)) == 0)
	{
		SetErrorMsg("Illegal input filename: %s\n", filename);
		return DEH_E_BadArgs;
	}

	if (CheckExtension(filename, "wad") || CheckExtension(filename, "hwa"))
	{
		SetErrorMsg("Input filename cannot be a WAD file.\n");
		return DEH_E_BadArgs;
	}

	if (CheckExtension(filename, NULL))
	{
		// memory management here is "optimised" (i.e. a bit dodgy),
		// since the result of ReplaceExtension() is an internal static
		// buffer.

		const char *bex_name = ReplaceExtension(filename, "bex");

		if (FileExists(bex_name))
		{
			parse_buffer_api *buf = Buffer::OpenFile(bex_name);

			if (! buf) return DEH_E_NoFile;  // normally won't happen

			input_bufs[num_inputs++] = new input_buffer_c(buf,
				FileBaseName(bex_name), false);

			return DEH_OK;
		}

		const char *deh_name = ReplaceExtension(filename, "deh");

		if (FileExists(deh_name))
		{
			parse_buffer_api *buf = Buffer::OpenFile(deh_name);

			if (! buf) return DEH_E_NoFile;  // normally won't happen

			input_bufs[num_inputs++] = new input_buffer_c(buf,
				FileBaseName(deh_name), false);

			return DEH_OK;
		}
	}

	parse_buffer_api *buf = Buffer::OpenFile(filename);

	if (! buf)
		return DEH_E_NoFile;

	input_bufs[num_inputs++] = new input_buffer_c(buf,
		FileBaseName(filename), false);

	return DEH_OK;
}

void FreeInputBuffers(void)
{
	for (int j = 0; j < num_inputs; j++)
	{
		if (input_bufs[j])
		{
			delete input_bufs[j];
			input_bufs[j] = NULL;
		}
	}
}

dehret_e Convert(void)
{
	dehret_e result;

	// load DEH patch file(s)
	for (int j = 0; j < num_inputs; j++)
	{
		char temp_text[256];
		sprintf(temp_text, "Parsing %s", input_bufs[j]->infoname);

		ProgressText(temp_text);
		ProgressMajor(j * 70 / num_inputs, (j+1) * 70 / num_inputs);

		PrintMsg("Loading patch file: %s\n", input_bufs[j]->infoname);

		result = Patch::Load(input_bufs[j]->buf);

		if (result != DEH_OK)
			return result;
	}

	FreeInputBuffers();

	ProgressText("Converting DEH");
	ProgressMajor(70, 80);

	Storage::ApplyAll();

	// do conversions into DDF...

	TextStr::SpriteDependencies();
	Frames::StateDependencies();
	Ammo::AmmoDependencies();

	Things::FixHeights();

	Sounds::ConvertSFX();
	Sounds::ConvertMUS();
	Attacks::ConvertATK();
	Things::ConvertTHING();
	Weapons::ConvertWEAP();
	TextStr::ConvertLDF();
	Rscript::ConvertRAD();

	Storage::RestoreAll();

	PrintMsg("\n");

	return DEH_OK;
}

void Shutdown(void)
{
	System_Shutdown();
}

}  // Deh_Edge

//------------------------------------------------------------------------

void DehEdgeStartup(const dehconvfuncs_t *funcs)
{
	Deh_Edge::Startup();
	Deh_Edge::cur_funcs = funcs;

	Deh_Edge::PrintMsg("*** DeHackEd -> EDGE Conversion ***\n");
}

const char *DehEdgeGetError(void)
{
	return Deh_Edge::GetErrorMsg();
}

dehret_e DehEdgeSetQuiet(int quiet)
{
	Deh_Edge::quiet_mode = (quiet != 0);

	return DEH_OK;
}

dehret_e DehEdgeAddFile(const char *filename)
{
	return Deh_Edge::AddFile(filename);
}

dehret_e DehEdgeAddLump(const char *data, int length, const char *infoname)
{
	if (Deh_Edge::num_inputs >= MAX_INPUTS)
	{
		Deh_Edge::SetErrorMsg("Too many input lumps !!\n");
		return DEH_E_BadArgs;
	}

	Deh_Edge::input_bufs[Deh_Edge::num_inputs++] =
		new Deh_Edge::input_buffer_c(
			Deh_Edge::Buffer::OpenLump(data, length), infoname, true);

	return DEH_OK;
}

dehret_e DehEdgeRunConversion(deh_container_c *dest)
{
	Deh_Edge::WAD::dest_container = dest;

	return Deh_Edge::Convert();
}

void DehEdgeShutdown(void)
{
	Deh_Edge::WAD::dest_container = NULL;

	Deh_Edge::Shutdown();
	Deh_Edge::cur_funcs = NULL;
}