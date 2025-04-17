/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
** 
** 86    2/12/98 4:01p Atai
** change refresh rate if oemdll updated for tv out
 * 
 * 85    12/19/97 11:04a Atai
 * oeminit dll stuff
 * 
 * 84    12/03/97 9:36a Dow
 * Fixed bug in grSstIsBusy()
 * 
 * 83    9/19/97 12:38p Peter
 * asm rush trisetup vs alt fifo
 * 
 * 82    9/11/97 9:31a Peter
 * This is what happens when you don't listen to Chris
 * 
 * 81    9/10/97 10:15p Peter
 * fifo logic from GaryT
 * 
 * 80    9/08/97 3:24p Peter
 * fixed my fifo muckage
 * 
 * 79    8/19/97 8:54p Peter
 * lots of stuff, hopefully no muckage
 * 
 * 78    8/01/97 11:48a Dow
 * Made some macros use conventional FIFO accounting
 * 
 * 77    7/10/97 1:36p Dow
 * Modified Nudge of Love to work around likely hardware bug.
 * 
 * 76    7/09/97 10:18a Dow
 * Further Nudge Of Love adjustments
 * 
 * 75    7/08/97 8:55p Dow
 * Fixed muckage in the Nudge Of Love
 * 
 * 74    7/08/97 1:29p Jdt
 * Fixed watcom muckage
 * 
 * 73    7/07/97 8:33a Jdt
 * New tracing macros.
 * 
 * 72    7/04/97 12:07p Dow
 * Changed the DUMPGWH stuff, added const for triangle command packet
 * 
 * 71    6/29/97 11:28p Jdt
 * Added gwCommand
 * 
 * 70    6/26/97 3:08p Dow
 * New metrics for P6 stuff.
 * 
 * 69    6/21/97 1:05p Dow
 * Moved the Nudge of Love to a macro
 * 
 * 68    6/20/97 5:50p Dow
 * Changes for Chip Field
 * 
 * 67    6/19/97 7:35p Dow
 * More P6 Stuff
 * 
 * 66    6/18/97 6:07p Dow
 * Protected P6 Stuff
 * 
 * 65    6/18/97 5:54p Dow
 * P6 adjustments
 * 
 * 64    6/16/97 12:45p Dow
 * P6 Fixes
 * 
 * 63    6/08/97 11:06p Pgj
 * use Group Write for Texture Downloads
 * 
 * 62    5/28/97 2:08p Dow
 * Added checks for int10h when in debugging mode.
 * 
 * 61    5/27/97 11:37p Pgj
 * Fix for Bug report 545
 * 
 * 60    5/27/97 2:02p Dow
 * Fixed up some macros--GR_CHECK_FOR_ROOM & a call to assert()
 * 
 * 59    5/22/97 11:18a Dow
 * Changed GR_ASSERT to fix stack muckage
 * 
 * 58    5/19/97 7:35p Pgj
 * Print cogent error message if h/w not found
 * 
 * 57    5/15/97 12:20p Dow
 * Fixed improper definition of GR_SET
 * 
 * 56    5/04/97 12:47p Dow
 * Added direct write macro fro grSstControl
 * 
 * 55    5/02/97 2:07p Pgj
 * screen_width/height now FxU32
 * 
 * 54    5/02/97 9:34a Dow
 * Changed indentation of GrState to match the rest of file, modified
 * GR_ASSERT
 * 
 * 53    4/13/97 2:06p Pgj
 * eliminate all anonymous unions (use hwDep)
 * 
 * 52    3/24/97 2:00p Dow
 * Fixed some chip field problems
 * 
 * 51    3/21/97 12:42p Dow
 * Made STWHints not send the Bend Over Baby Packet to FBI Jr.
 * 
 * 50    3/19/97 10:43p Dow
 * windowsInit stuff
 * 
 * 49    3/19/97 1:37a Jdt
 * Added fbStride to gc
 * 
 * 48    3/18/97 9:08p Dow
 * Added FX_GLIDE_NO_DITHER_SUB environment variable
 * 
 * 47    3/17/97 6:25a Jdt
 * Added open flag to gc
 * 
 * 46    3/16/97 12:38a Jdt
 * Shouldn't have removed fifoData, duh..
 * 
 * 45    3/16/97 12:19a Jdt
 * Removed redundant data
 * 
 * 44    3/13/97 2:51a Jdt
 * Removed lockIdle flag
 * 
 * 43    3/13/97 1:18a Jdt
 * Added flag for sli lfb reads.
 * 
 * 42    3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
 * 
 * 41    3/04/97 9:11p Dow
 * Neutered mutiplatform multiheaded monster.
 * 
 * 40    2/26/97 2:18p Dow
 * moved all debug set functions to __cdecl
 * 
 * 39    2/26/97 11:54a Jdt
 * Added glide buffer locking and fixed bug in GR_SET
 * 
 * 38    2/19/97 4:42p Dow
 * Fixed debug build for Watcom
 * 
 * 37    2/14/97 12:55p Dow
 * moved vg96 fifo wrap into init code
 * 
 * 36    2/12/97 5:31p Dow
 * Fixed my ^$%^^&*^%^  error.
 * 
 * 35    2/12/97 2:09p Hanson
 * Hopefully removed the rest of my muckage. 
 * 
 * 34    2/11/97 6:58p Dow
 * Changes to support vid tiles and ser status
 * 
 * 33    1/18/97 11:44p Dow
 * Moved _curGCFuncs into _GlideRoot
 * Added support for GMT's register debugging
 * 
 * 32    1/16/97 3:41p Dow
 * Embedded fn protos in ifndef FX_GLIDE_NO_FUNC_PROTO
 * 
 * 31    12/23/96 1:37p Dow
 * chagnes for multiplatform glide
 * 
 * 30    11/14/96 1:04p Jdt
 * Test for keywords
**
*/
            
/*                                               
** fxglide.h
**  
** Internal declarations for use inside Glide.
**
** GLIDE_LIB:        Defined if building the Glide Library.  This macro
**                   should ONLY be defined by a makefile intended to build
**                   GLIDE.LIB or glide.a.
**
** GLIDE_HARDWARE:   Defined if GLIDE should use the actual SST hardware.  An
**                   application is responsible for defining this macro.
**
** GLIDE_NUM_TMU:    Number of physical TMUs installed.  Valid values are 1
**                   and 2.  If this macro is not defined by the application
**                   it is automatically set to the value 2.
**
*/

#ifndef __FXGLIDE_H__
#define __FXGLIDE_H__

/*
** -----------------------------------------------------------------------
** INCLUDE FILES
** -----------------------------------------------------------------------
*/
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "3dfx.h"
// #include <sst1init.h>
#include "sst.h"
// #include <gdebug.h>
#include <init.h>
#include "glide.h"

#define GR_CDECL

/*==========================================================================*/
/*
** GrState
**
** If something changes in here, then go into glide.h, and look for a 
** declaration of the following form:
**
** #define GLIDE_STATE_PAD_SIZE N
** #ifndef GLIDE_LIB
** typedef struct {
**   char pad[GLIDE_STATE_PAD_SIZE];
** }  GrState;
** #endif
** 
** Then change N to sizeof(GrState) AS DECLARED IN THIS FILE!
**
*/

struct _GrState_s 
{
  GrCullMode_t                 /* these go in front for cache hits */
    cull_mode;                 /* cull neg, cull pos, don't cull   */
  
  GrHint_t
    paramHints;                /* Tells us if we need to pointcast a
                                  parameter to a specific chip */
  FxI32
    fifoFree;                  /* # free entries in FIFO */
  FxU32
    paramIndex,                /* Index into array containing
                                  parameter indeces to be sent ot the
                                  triangle setup code    */
    tmuMask;                   /* Tells the paramIndex updater which
                                  TMUs need values */
  struct{
    FxU32   fbzColorPath;
    FxU32   fogMode;
    FxU32   alphaMode;
    FxU32   fbzMode;
    FxU32   lfbMode;
    FxU32   clipLeftRight;
    FxU32   clipBottomTop;
    
    FxU32   fogColor;
    FxU32   zaColor;
    FxU32   chromaKey;
     
    FxU32   stipple;
    FxU32   color0;
    FxU32   color1;
  } fbi_config;                 /* fbi register shadow */
  
  struct {
    FxU32   textureMode;
    FxU32   tLOD;
    FxU32   tDetail;
    FxU32   texBaseAddr;
    FxU32   texBaseAddr_1;
    FxU32   texBaseAddr_2;
    FxU32   texBaseAddr_3_8;
    GrMipMapMode_t mmMode;      /* saved to allow MM en/dis */
    GrLOD_t        smallLod, largeLod; /* saved to allow MM en/dis */
    FxU32          evenOdd;
    GrNCCTable_t   nccTable;
  } tmu_config[GLIDE_NUM_TMU];  /* tmu register shadow           */
  
  FxBool                       /* Values needed to determine which */
    ac_requires_it_alpha,      /*   parameters need gradients computed */
    ac_requires_texture,       /*   when drawing triangles      */
    cc_requires_it_rgb,
    cc_requires_texture,
    cc_delta0mode,             /* Use constants for flat shading */
    allowLODdither,            /* allow LOD dithering            */
    checkFifo;                 /* Check fifo status as specified by hints */
  
  FxU16
    lfb_constant_depth;        /* Constant value for depth buffer (LFBs) */
  GrAlpha_t
    lfb_constant_alpha;        /* Constant value for alpha buffer (LFBs) */
  
  FxI32
    num_buffers;               /* 2 or 3 */
  
  GrColorFormat_t
    color_format;              /* ARGB, RGBA, etc. */
  
  GrMipMapId_t
    current_mm[GLIDE_NUM_TMU]; /* Which guTex** thing is the TMU set
                                  up for? THIS NEEDS TO GO!!! */
  
  float
    clipwindowf_xmin, clipwindowf_ymin, /* Clipping info */
    clipwindowf_xmax, clipwindowf_ymax;
  FxU32
    screen_width, screen_height; /* Screen width and height */
  float
    a, r, g, b;                /* Constant color values for Delta0 mode */
};

typedef struct datalist_s {
    int i;
    uint32_t addr;    // nand2mario: device sidebyte address of the data
} datalist_s;

typedef struct GrGC_s
{
  // nand2mario: these are device side pointers
  FxU32
    base_ptr,                  /* base address of SST */
    reg_ptr,                   /* pointer to base of SST registers */
    tex_ptr,                   /* texture memory address */
    lfb_ptr,                   /* linear frame buffer address */
    slave_ptr;                 /* Scanline Interleave Slave address */

  void *oemInit;

  datalist_s dataList[20+12*GLIDE_NUM_TMU+3];/* add 3 for:
                                       fbi-tmu0 trans
                                       tmu0-tmu1 trans
                                       tmu1-tmu2 trans
                                       */
  GrState
    state;                      /* state of Glide/SST */

  FxBool
    nopCMD;                     /* Have we placed a NOP in the FIFO ? */

  // InitFIFOData  
  //   fifoData;

  union hwDep_u {
    struct sst96Dep_s {
      FxU32
        writesSinceFence,       /* Writes since last fence */
        writesSinceHeader,      /* Grouped Writes since last header */
        paramMask,              /* Mask indicating required parameters */
        gwCommand,              /* Command for initiating a triangle gw packet */
        gwHeaders[4];           /* Group write headers for SST96 */
      FxU32
        *serialStatus,          /* address of serial status register */
        *fifoApertureBase;      /* base of fifo apurture (if different) */

    } sst96Dep;
    struct sst1Dep_s {
      FxU32
        /* fifoFree,               # Free entries in FIFO */
        swFifoLWM;              /* fudge factor */
    } sst1Dep;
    
  } hwDep;

  FxU32 lockPtrs[2];        /* pointers to locked buffers */
  FxU32 fbStride;

  struct {
    FxU32             freemem_base;
    FxU32             total_mem;
    FxU32             next_ncc_table;
    GrMipMapId_t      ncc_mmids[2];
    const GuNccTable *ncc_table[2];
  } tmu_state[GLIDE_NUM_TMU];

  int
    grSstRez,                   /* Video Resolution of board */
    grSstRefresh,               /* Video Refresh of board */
    fbuf_size,                  /* in MB */
    num_tmu;                    /* number of TMUs attached */

  FxBool
    scanline_interleaved;

  struct {
    GrMipMapInfo data[MAX_MIPMAPS_PER_SST];
    GrMipMapId_t free_mmid;
  } mm_table;                   /* mip map table */

  /* LFB Flags */
  FxU32 lfbSliOk;

  /* DEBUG and SANITY variables */
  FxI32 myLevel;                /* debug level */
  FxI32 counter;                /* counts bytes sent to HW */
  FxI32 expected_counter;       /* the number of bytes expected to be sent */

  sst1VideoTimingStruct         /* init code overrides */
    *vidTimings;

  FxBool open;                  /* Has GC Been Opened? */
  FxBool closedP;               /* Have we closed since an init call? (see grSstWinOpen) */
} GrGC;

/* if we are debugging, call a routine so we can trace fences */
#define GR_P6FENCE P6FENCE

/*
**  The root of all Glide data, all global data is in here
**  stuff near the top is accessed a lot
*/
struct _GlideRoot_s {
  int p6Fencer;                 /* xchg to here to keep this in cache!!! */
  int current_sst;
  FxU32 CPUType;
  GrGC *curGC;                  /* point to the current GC      */
  FxI32 curTriSize;             /* the size in bytes of the current triangle */
  FxI32 curTriSizeNoGradient;   /* special for _trisetup_nogradients */
  FxU32 packerFixAddress;       /* address to write packer fix to */
  FxBool    windowsInit;        /* Is the Windows part of glide initialized? */

  int initialized;

  struct {                      /* constant pool (minimizes cache misses) */
    float f0;
    float fHalf;
    float f1;
    float f255;
    float f256;
    float ftemp1, ftemp2;       /* temps to convert floats to ints */
  } pool;

  struct {                      /* environment data             */
    FxBool ignoreReopen;
    FxBool triBoundsCheck;      /* check triangle bounds        */
    FxBool noSplash;            /* don't draw it                */
    FxBool shamelessPlug;       /* translucent 3Dfx logo in lower right */
    FxU32  rsrchFlags;           
    FxI32  swapInterval;        /* swapinterval override        */
    FxI32  swFifoLWM;
    FxU32  snapshot;            /* register trace snapshot      */
    FxBool disableDitherSub;    /* Turn off dither subtraction? */
  } environment;

  struct {
    FxU32       minMemFIFOFree;
    FxU32       minPciFIFOFree;

    FxU32       fifoSpins;      /* number of times we spun on fifo */

    FxU32       bufferSwaps;    /* number of buffer swaps       */
    FxU32       pointsDrawn;
    FxU32       linesDrawn;
    FxU32       trisProcessed;
    FxU32       trisDrawn;

    FxU32       texDownloads;   /* number of texDownload calls   */
    FxU32       texBytes;       /* number of texture bytes downloaded   */

    FxU32       palDownloads;   /* number of palette download calls     */
    FxU32       palBytes;       /* number of palette bytes downloaded   */
  } stats;

  GrHwConfiguration     hwConfig;
  
  GrGC                  GCs[MAX_NUM_SST];       /* one GC per board     */
};


extern struct _GlideRoot_s GR_CDECL _GlideRoot;
/*==========================================================================*/
/* Macros for declaring functions */
#define GR_DDFUNC(name, type, args) \
type FX_CSTYLE name args

#define GR_ENTRY(name, type, args) \
FX_ENTRY type FX_CSTYLE name args

#define GR_DIENTRY(name, type, args) \
FX_ENTRY type FX_CSTYLE name args

/*==========================================================================*/

#define STATE_REQUIRES_IT_DRGB  FXBIT(0)
#define STATE_REQUIRES_IT_ALPHA FXBIT(1)
#define STATE_REQUIRES_OOZ      FXBIT(2)
#define STATE_REQUIRES_OOW_FBI  FXBIT(3)
#define STATE_REQUIRES_W_TMU0   FXBIT(4)
#define STATE_REQUIRES_ST_TMU0  FXBIT(5)
#define STATE_REQUIRES_W_TMU1   FXBIT(6)
#define STATE_REQUIRES_ST_TMU1  FXBIT(7)
#define STATE_REQUIRES_W_TMU2   FXBIT(8)
#define STATE_REQUIRES_ST_TMU2  FXBIT(9)

#define GR_TMUMASK_TMU0 FXBIT(GR_TMU0)
#define GR_TMUMASK_TMU1 FXBIT(GR_TMU1)
#define GR_TMUMASK_TMU2 FXBIT(GR_TMU2)

/*
**  Parameter gradient offsets
**
**  These are the offsets (in bytes)of the DPDX and DPDY registers from
**  from the P register
*/
#ifdef GLIDE_USE_ALT_REGMAP
#define DPDX_OFFSET 0x4
#define DPDY_OFFSET 0x8
#else
#define DPDX_OFFSET 0x20
#define DPDY_OFFSET 0x40
#endif

/*==========================================================================*/
/*
**  Here's the infamous Packer Bug Check and Workaround:   
**  XOR the two addresses together to find out which bits are different.
**  AND the result with the bits that represent the chip field of the 
**  SST address.  If ANY of them are different, then do the packer hack.
**  Save this address as the last with which we compared.
*/  

#define SST_CHIP_MASK 0x3C00

#if   ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
#define GLIDE_DRIVER_NAME "Voodoo Graphics"
#elif   ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
#define GLIDE_DRIVER_NAME "Voodoo Rush"
#else
#define GLIDE_DRIVER_NAME "Unknown"
#endif

// nand2mario: remove packer workaround for SST-1
  #define PACKER_WORKAROUND_SIZE 0
  #define PACKER_WORKAROUND
  #define PACKER_BUGCHECK(a)

/* GMT: a very useful macro for making sure that SST commands are properly
   fenced on a P6, e.g. P6FENCH_CMD( GR_SET(hw->nopCMD,1) );
*/
/* #if (GLIDE_PLATFORM & GLIDE_HW_SST1) */
#if 1
#define P6FENCE_CMD( cmd ) \
    if (_GlideRoot.CPUType == 6) {     /* if it's a p6 */ \
      GR_P6FENCE;                      /* then fence off the cmd */ \
      cmd; \
      GR_P6FENCE;                      /* to ensure write order  */ \
    } \
    else \
      cmd
#else
#define P6FENCE_CMD( cmd ) cmd
#endif

/*==========================================================================*/
#ifndef FX_GLIDE_NO_FUNC_PROTO

#define FX_CSTYLE

void _grMipMapInit(void);
FxI32 FX_CSTYLE
_trisetup_asm(const GrVertex *va, const GrVertex *vb, const GrVertex *vc );
FxI32 FX_CSTYLE
_trisetup(const GrVertex *va, const GrVertex *vb, const GrVertex *vc );
FxI32 FX_CSTYLE
_trisetup_nogradients(const GrVertex *va, const GrVertex *vb, const GrVertex *vc );

#endif /* FX_GLIDE_NO_FUNC_PROTO */

/* GMT: BUG need to make this dynamically switchable */
#if defined(GLIDE_USE_C_TRISETUP)
  #define TRISETUP _trisetup
#else
  #define TRISETUP _trisetup_asm
#endif /* GLIDE_USE_C_TRISETUP */

/*==========================================================================*/
/* 
**  Function Prototypes
*/
#ifdef GLIDE_DEBUG
FxBool
_grCanSupportDepthBuffer( void );
#endif

void
_grClipNormalizeAndGenerateRegValues(FxU32 minx, FxU32 miny, FxU32 maxx,
                                     FxU32 maxy, FxU32 *clipLeftRight,
                                     FxU32 *clipBottomTop);

void 
_grSwizzleColor( GrColor_t *color );

void
_grDisplayStats(void);

void
_GlideInitEnvironment(void);

void FX_CSTYLE
_grColorCombineDelta0Mode( FxBool delta0Mode );

void
_doGrErrorCallback( const char *name, const char *msg, FxBool fatal );

void _grErrorDefaultCallback( const char *s, FxBool fatal );

#ifdef __WIN32__
void _grErrorWindowsCallback( const char *s, FxBool fatal );
#endif /* __WIN32__ */

extern void
(*GrErrorCallback)( const char *string, FxBool fatal );

void GR_CDECL
_grFence( void );

int
_guHeapCheck( void );

void FX_CSTYLE
_grRebuildDataList( void );

void
_grReCacheFifo( FxI32 n );

FxI32 GR_CDECL
_grSpinFifo( FxI32 n );

void
_grShamelessPlug(void);

FxBool
_grSstDetectResources(void);

FxU16
_grTexFloatLODToFixedLOD( float value );

void FX_CSTYLE
_grTexDetailControl( GrChipID_t tmu, FxU32 detail );

void FX_CSTYLE
_grTexDownloadNccTable( GrChipID_t tmu, FxU32 which,
                        const GuNccTable *ncc_table,
                        int start, int end );
void FX_CSTYLE
_grTexDownloadPalette( GrChipID_t   tmu, 
                       GuTexPalette *pal,
                       int start, int end );

FxU32
_grTexCalcBaseAddress(
                      FxU32 start_address, GrLOD_t largeLod,
                      GrAspectRatio_t aspect, GrTextureFormat_t fmt,
                      FxU32 odd_even_mask ); 

void
_grTexForceLod( GrChipID_t tmu, int value );

FxU32
_grTexTextureMemRequired( GrLOD_t small_lod, GrLOD_t large_lod, 
                           GrAspectRatio_t aspect, GrTextureFormat_t format,
                           FxU32 evenOdd );
void FX_CSTYLE
_grUpdateParamIndex( void );

#if 0
FX_ENTRY void FX_CALL
grSstConfigPipeline(GrChipID_t chip, GrSstRegister reg, FxU32 value);
#endif

/*==========================================================================*/
/*  GMT: have to figure out when to include this and when not to
*/
#if defined(GLIDE_DEBUG) || defined(GLIDE_ASSERT) || \
	defined(GLIDE_SANITY_ASSERT) || defined(GLIDE_SANITY_SIZE) || \
	defined(GDBG_INFO_ON)
#define DEBUG_MODE 1
#include <assert.h>

#if SST96_FIFO
/* sst96.c */
extern void
_grSst96CheckFifoData(void);

#define GLIDE_FIFO_CHECK() _grSst96CheckFifoData()
#else
#define GLIDE_FIFO_CHECK()
#endif
#else
#define GLIDE_FIFO_CHECK()
#endif

#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
/* NOTE: fifoFree is the number of entries, each is 8 bytes */
#define GR_CHECK_FOR_ROOM(n) \
{ \
  FxI32 fifoFree = gc->state.fifoFree - (n); \
  if (fifoFree < 0) { \
    _grReCacheFifo(0);  \
    fifoFree = gc->state.fifoFree - (n); \
  } \
  if (fifoFree < 0)          \
    printf("PANIC: fifoFree < 0\n"); \
  gc->state.fifoFree = fifoFree;\
}

// fifoFree = _grSpinFifo(n); \

#elif (GLIDE_PLATFORM & GLIDE_HW_SST96)
/* NOTE: fifoSize is in bytes, and each fifo entry is 8 bytes.  Since
   the fifoSize element of the sst96Dep data structure must be
   accurate, we subtract after we write, instead of at the beginning
   as above. */
#if (GLIDE_PLATFORM & GLIDE_OS_DOS32) && defined(GLIDE_DEBUG10)
#define GR_CHECKINT10 if (gc->hwDep.sst96Dep.int10Called)_doGrErrorCallback("Glide Error:", "Application called Int 10 between grSstWinOpen and Close.\n", FXTRUE)
#else
#define GR_CHECKINT10
#endif

#if SST96_ALT_FIFO_WRAP

#define GR_CHECK_FOR_ROOM(n) \
{\
  FxI32 fifoSize = gc->fifoData.hwDep.vg96FIFOData.fifoSize - ((n) << 1);\
  if (fifoSize < 0) {\
    gc->fifoData.hwDep.vg96FIFOData.blockSize = ((n) << 1); \
    initWrapFIFO(&gc->fifoData); \
     GR_CHECKINT10;\
  }\
}
#else /* !SST96_ALT_FIFO_WRAP */
#define GR_CHECK_FOR_ROOM(n) \
{\
  FxI32 fifoSize = gc->fifoData.hwDep.vg96FIFOData.fifoSize - ((n) << 1);\
  if (fifoSize < 0) {\
    _grSst96FifoMakeRoom();\
    GR_ASSERT(!(gc->fifoData.hwDep.vg96FIFOData.fifoSize < 0x1000));\
    GR_CHECKINT10;\
  }\
}
#endif /* !SST96_ALT_FIFO_WRAP */
#endif

#ifdef GLIDE_SANITY_SIZE

#define FIFO_SUB_VAL ((FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoPtr - \
                      (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoVirt)
   
#define GR_CHECK_SIZE() \
   if(gc->counter != gc->expected_counter) \
   GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be %d but is %d\n", \
              gc->expected_counter,gc->counter); \
   ASSERT(gc->counter == gc->expected_counter); \
   gc->counter = gc->expected_counter = 0
#define GR_CHECK_SIZE_SLOPPY() \
   if(gc->counter > gc->expected_counter) \
     GDBG_ERROR("GR_ASSERT_SIZE","byte counter should be < %d but is %d\n", \
                gc->expected_counter,gc->counter); \
   ASSERT(gc->counter <= gc->expected_counter); \
   gc->counter = gc->expected_counter = 0
#define GR_SET_EXPECTED_SIZE(n) \
   GLIDE_FIFO_CHECK(); \
   GDBG_INFO((gc->myLevel, \
              "FIFO: 0x%X 0x%X (0x%X)\n", \
              (n), \
              (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoPtr, \
              FIFO_SUB_VAL)); \
   GDBG_INFO((gc->myLevel, \
              "FIFOSize: 0x%X\n", \
              (FxU32)gc->fifoData.hwDep.vg96FIFOData.fifoSize));\
   ASSERT(gc->counter == 0); \
   ASSERT(gc->expected_counter == 0); \
   GR_CHECK_FOR_ROOM((n)); \
   gc->expected_counter = n
#define GR_INC_SIZE(n) gc->counter += n
#else /* !GLIDE_SANITY_SIZE */
   /* define to do nothing */
#define GR_CHECK_SIZE_SLOPPY()
#define GR_CHECK_SIZE()
#define GR_SET_EXPECTED_SIZE(n) GR_CHECK_FOR_ROOM((n))
#define GR_INC_SIZE(n)
#endif /* !GLIDE_SANITY_SIZE */

#define GR_DCL_GC GrGC *gc = _GlideRoot.curGC
// #define GR_DCL_HW Sstregs *hw = (Sstregs *)gc->reg_ptr

#ifdef DEBUG_MODE
  #define ASSERT(exp) GR_ASSERT(exp)

  #define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;      \
                GR_DCL_HW;      \
                static char myName[] = name;  \
                GR_ASSERT(gc != NULL);  \
                GR_ASSERT(hw != NULL);  \
                gc->myLevel = level;    \
                GDBG_INFO((gc->myLevel,"%s\n", myName)); \
                FXUNUSED( hw )

#else
  #define ASSERT(exp)

  #define GR_BEGIN_NOFIFOCHECK(name,level) \
                GR_DCL_GC;
                // GR_DCL_HW;    

#endif

#define GR_BEGIN(name,level,size) \
                GR_BEGIN_NOFIFOCHECK(name,level); \
                GR_SET_EXPECTED_SIZE(size)

#define GR_EXIT_TRACE   GDBG_INFO((gc->myLevel,  "%s Done\n", myName))

#define GR_END()        {GR_CHECK_SIZE(); GR_EXIT_TRACE;}
#define GR_END_SLOPPY() {GR_CHECK_SIZE_SLOPPY(); GR_EXIT_TRACE;}
#define GR_RETURN(val) \
                if ( GDBG_GET_DEBUGLEVEL(gc->myLevel) ) { \
                  GR_CHECK_SIZE(); \
                } \
                else \
                  GR_END(); \
                GDBG_INFO((gc->myLevel,"%s() => 0x%x---------------------\n",myName,val,val)); \
                return val

#ifndef GR_ASSERT               
#if defined(GLIDE_SANITY_ASSERT)
#  define GR_ASSERT(exp) if (!(exp)) _grAssert(#exp,  __FILE__, __LINE__)
#else
#  define GR_ASSERT(exp) 
#endif
#endif

#if defined(GLIDE_DEBUG)
  #define GR_CHECK_F(name,condition,msg) \
    if ( condition ) _doGrErrorCallback( name, msg, FXTRUE )
  #define GR_CHECK_W(name,condition,msg) \
    if ( condition ) _doGrErrorCallback( name, msg, FXFALSE )
#else
  #define GR_CHECK_F(name,condition,msg)
  #define GR_CHECK_W(name,condition,msg)
#endif

/* macro define some basic and common GLIDE debug checks */
#define GR_CHECK_TMU(name,tmu) \
  GR_CHECK_F(name, tmu < GR_TMU0 || tmu >= gc->num_tmu , "invalid TMU specified")


FxBool
_grSst96PCIFifoEmpty(void);
FxU32
_grSst96Load32(FxU32 *s);
void
_grSst96Store32(FxU32 *d, FxU32 s);
void
_grSst96Store16(FxU16 *d, FxU16 s);
void 
_grSst96Store32F(float *d, float s);
void
_grAssert(char *, char *, int);

#include <stdint.h>
void voodoo_w(uint32_t offset, uint32_t data, uint32_t mask);
void voodoo_w_float(uint32_t offset, float data);
uint32_t voodoo_r(uint32_t offset);
float voodoo_r_float(uint32_t offset);

#define GR_GET(s)     voodoo_r(s)
#define GR_GETF(s)    voodoo_r_float(s)
#define GR_SET(d,s)   voodoo_w(d,s,0xFFFFFFFF)
#define GR_SETF(d,s)  voodoo_w_float(d,s)
#define GR_SET16(d,s) voodoo_w(d,s,0x0000FFFF)

#define VG96_REGISTER_OFFSET    0x400000
#define VG96_TEXTURE_OFFSET     0x600000

void rle_decode_line_asm(FxU16 *tlut,FxU8 *src,FxU16 *dest);

extern FxU16 rle_line[256];
extern FxU16 *rle_line_end;

#define RLE_CODE                        0xE0
#define NOT_RLE_CODE            31


#define P6FENCE 

void _grErrorDefaultCallback( const char *s, FxBool fatal );

FxBool initSetVideo( FxU32                hWnd,
                     GrScreenResolution_t sRes,
                     GrScreenRefresh_t    vRefresh,
                     InitColorFormat_t    cFormat,
                     InitOriginLocation_t yOrigin,
                     int                  nColBuffers,
                     int                  nAuxBuffers,
                     int                  *xres,
                     int                  *yres,
                     int                  *fbStride,
                     sst1VideoTimingStruct *vidTimings);

FxBool sst1InitVideo(FxU32 *sstbase,
  GrScreenResolution_t screenResolution, GrScreenRefresh_t screenRefresh,
  sst1VideoTimingStruct *altVideoTiming);

FxBool initEnableTransport( InitFIFOData *info );

void _grReCacheFifo( FxI32 n );

#endif /* __FXGLIDE_H__ */

