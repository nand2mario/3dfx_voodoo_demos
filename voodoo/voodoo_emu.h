 /*
 *  Copyright (C) 2002-2021  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef DOSBOX_VOODOO_EMU_H
#define DOSBOX_VOODOO_EMU_H

#include <stdlib.h>

// #include "dosbox.h"

#include "voodoo_types.h"
#include "voodoo_data.h"


extern voodoo_state *v;

// SST-1 address space:
// 0x000000 - 0x3fffff: registers (offset: 0 .. 0x100000)
// 0x400000 - 0x7fffff: linear frame buffer    
// 0x800000 - 0xffffff: texture memory
// offset: 32-bit word address
void voodoo_w(UINT32 offset, UINT32 data, UINT32 mask);
void voodoo_w_float(UINT32 offset, float data);

// Voodoo memory and register reads
UINT32 voodoo_r(UINT32 offset);
float voodoo_r_float(UINT32 offset);

void voodoo_init(int type);
void voodoo_shutdown();
void voodoo_leave(void);

void voodoo_activate(void);
void voodoo_update_dimensions(void);
void voodoo_set_window(void);

void voodoo_vblank_flush(void);
void voodoo_swap_buffers(voodoo_state *v);


// Machine interface
extern void Voodoo_UpdateScreenStart();
extern void Voodoo_Output_Enable(bool enabled);
extern bool Voodoo_GetRetrace();
extern double Voodoo_GetVRetracePosition();
extern double Voodoo_GetHRetracePosition();

extern void Voodoo_Initialize(const char *title);
extern void Voodoo_frame_done();     

extern void CPU_Core_Dyn_X86_SaveDHFPUState(void);
extern void CPU_Core_Dyn_X86_RestoreDHFPUState(void);

// SDL related
extern void sdl_process_events();

#endif
