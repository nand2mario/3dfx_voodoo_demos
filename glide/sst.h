/* 
 * THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
 * PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
 * TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
 * INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
 * DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
 * THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
 * FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
 * 
 * USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
 * RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
 * TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
 * AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
 * SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
 * THE UNITED STATES.  
 * 
 * COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
 */

#ifndef __SST_H__
#define __SST_H__

// #include <ncc.h>
// #include "fx64.h"

#ifdef __cplusplus
extern "C" {
#endif

// this crazy macro fixes the binary point within a floating point
// number so that it has 'fracbits' to the right of the binary point
// it also adds a '1' bit to the MSB so that slightly negative numbers
// end up looking like twos complement numbers (they carry out from the '1')
#define FLOAT_FIX(f,fracbits) ((f)+(float)(3<<(22-(fracbits))))

#define SST_FLOAT(f,scale,shift) (unsigned long)((f)*((scale)*(float)(1<<shift)))
#define SST_TLOD_MINMAX(lodmin,lodmax) \
                (((lodmin) << (SST_LODMIN_SHIFT)) | \
                ((lodmax) << (SST_LODMAX_SHIFT)))
#define SST_TLOD_MINMAX_INT(lodmin,lodmax) \
                (((lodmin) << (SST_LOD_FRACBITS+SST_LODMIN_SHIFT)) | \
                ((lodmax) << (SST_LOD_FRACBITS+SST_LODMAX_SHIFT)))

// this crazy macro tests the sign bit of a float by loading it into
// an integer register and then testing the sign bit of the integer
#define FLOAT_ISNEG(f) ((*(int *)(&(f))) < 0)

// these crazy macros returns the sign of a number (1 if >= 0; -1 if < 0)
#define ISIGN(x) (((x) | 0x40000000) >> 30)
#define FSIGN(f) ISIGN(*(long *)&f)

typedef struct vertex_Rec {
    unsigned long x;            // 12.4 format
    unsigned long y;            // 12.4
} vtxRec;

// 256 color palette tables
typedef struct {
    unsigned long argb[256];
} Pal256;

#define BIT(n)  (1<<(n))
#define SST_MASK(n) (0xFFFFFFFF >> (32-(n)))
#define SST_MASK64(n) FX_MASK64(n)

//----------------- SST binary point locations  ---------------
#define SST_LOD_SIZE     6
#define SST_LOD_FRACBITS 2
#define SST_XY_SIZE     16
#define SST_XY_INTBITS  12
#define SST_XY_FRACBITS  4
#define SST_RGBA_SIZE   24
#define SST_RGBA_INTBITS 8
#define SST_RGBA_FRACBITS 12
#define SST_Z_SIZE      32
#define SST_Z_INTBITS   16
#define SST_Z_FRACBITS  12
// NOTE: below are ONLY for the fixed point registers
#define SST_ST_SIZE     32
#define SST_ST_INTBITS  14
#define SST_ST_FRACBITS 18
#define SST_W_SIZE      32
#define SST_W_INTBITS    2
#define SST_W_FRACBITS  30

//----------------- SST lfbMode bits    -----------------------
#define SST_LFB_FORMAT_SHIFT    0
#define SST_LFB_FORMAT          (0xF<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_565              (0<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_555              (1<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_1555             (2<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u1               (3<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_888              (4<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_8888             (5<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u2               (6<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u3               (7<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z565             (12<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z555             (13<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z1555            (14<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_ZZ               (15<<SST_LFB_FORMAT_SHIFT)
#define SST_LFB_WRITEBUFSELECT_SHIFT    4
#define SST_LFB_WRITEBUFSELECT  (0x3<<SST_LFB_WRITEBUFSELECT_SHIFT)
#       define SST_LFB_WRITEFRONTBUFFER (0<<SST_LFB_WRITEBUFSELECT_SHIFT)
#       define SST_LFB_WRITEBACKBUFFER  (1<<SST_LFB_WRITEBUFSELECT_SHIFT)
#   define SST_LFB_WRITEAUXBUFFER   (2<<SST_LFB_WRITEBUFSELECT_SHIFT)
#define SST_LFB_READBUFSELECT_SHIFT     6
#define SST_LFB_READBUFSELECT   (0x3<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READFRONTBUFFER  (0<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READBACKBUFFER   (1<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READDEPTHABUFFER (2<<SST_LFB_READBUFSELECT_SHIFT)
#   define SST_LFB_READAUXBUFFER    SST_LFB_READDEPTHABUFFER
#define SST_LFB_ENPIXPIPE       BIT(8)
#define SST_LFB_RGBALANES_SHIFT         9
#define SST_LFB_RGBALANES       (0x3<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_ARGB (0<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_ABGR (1<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_RGBA (2<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_BGRA (3<<SST_LFB_RGBALANES_SHIFT)
#define SST_LFB_WRITE_SWAP16    BIT(11)
#define SST_LFB_WRITE_BYTESWAP  BIT(12)
#define SST_LFB_YORIGIN         BIT(13)
#define SST_LFB_WSELECT         BIT(14)
#define SST_LFB_READ_SWAP16     BIT(15)
#define SST_LFB_READ_BYTESWAP   BIT(16)

//----------------- SST videoFilterRgbThreshold
#define SST_VIDEOFILTER_BLUE_SHIFT      0
#define SST_VIDEOFILTER_BLUE    (0xff << SST_VIDEOFILTER_BLUE_SHIFT)
#define SST_VIDEOFILTER_GREEN_SHIFT     8
#define SST_VIDEOFILTER_GREEN   (0xff << SST_VIDEOFILTER_GREEN_SHIFT)
#define SST_VIDEOFILTER_RED_SHIFT 16            
#define SST_VIDEOFILTER_RED     (0xff << SST_VIDEOFILTER_RED_SHIFT)

//----------------- SST textureMode bits -----------------------
#define SST_TPERSP_ST           BIT(0)
#define SST_TMINFILTER          BIT(1)
#define SST_TMAGFILTER          BIT(2)
#define SST_TCLAMPW             BIT(3)
#define SST_TLODDITHER          BIT(4)
#define SST_TNCCSELECT          BIT(5)  // selects which table
#define SST_TCLAMPS             BIT(6)
#define SST_TCLAMPT             BIT(7)
#define SST_T8BIT(mode) ((mode&SST_TFORMAT)<SST_ARGB8332)
#define SST_TFORMAT_SHIFT       8
#define SST_TFORMAT             (0xF<<SST_TFORMAT_SHIFT)
#       define SST_RGB332               (0<<SST_TFORMAT_SHIFT)
#       define SST_YIQ422               (1<<SST_TFORMAT_SHIFT)
#       define SST_A8                   (2<<SST_TFORMAT_SHIFT)
#       define SST_I8                   (3<<SST_TFORMAT_SHIFT)
#       define SST_AI44                 (4<<SST_TFORMAT_SHIFT)
#       define SST_P8                   (5<<SST_TFORMAT_SHIFT)
#       define SST_P8_RGBA6666          (6<<SST_TFORMAT_SHIFT)
#       define SST_ARGB8332             (8<<SST_TFORMAT_SHIFT)
#       define SST_AYIQ8422             (9<<SST_TFORMAT_SHIFT)
#       define SST_RGB565               (10<<SST_TFORMAT_SHIFT)
#       define SST_ARGB1555             (11<<SST_TFORMAT_SHIFT)
#       define SST_ARGB4444             (12<<SST_TFORMAT_SHIFT)
#       define SST_AI88                 (13<<SST_TFORMAT_SHIFT)
#       define SST_AP88                 (14<<SST_TFORMAT_SHIFT)
#define SST_TC_ZERO_OTHER       BIT(12)
#define SST_TC_SUB_CLOCAL       BIT(13)
#define SST_TC_MSELECT_SHIFT    14
#define SST_TC_MSELECT          (0x7<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MONE              (0<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MCLOCAL           (1<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MAOTHER           (2<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MALOCAL           (3<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MLOD              (4<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MLODFRAC          (5<<SST_TC_MSELECT_SHIFT)
#define SST_TC_REVERSE_BLEND    BIT(17)
#define SST_TC_ADD_CLOCAL       BIT(18)
#define SST_TC_ADD_ALOCAL       BIT(19)
#define SST_TC_INVERT_OUTPUT    BIT(20)
#define SST_TCA_ZERO_OTHER      BIT(21)
#define SST_TCA_SUB_CLOCAL      BIT(22)
#define SST_TCA_MSELECT_SHIFT   23
#define SST_TCA_MSELECT         (0x7<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MONE             (0<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MCLOCAL          (1<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MAOTHER          (2<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MALOCAL          (3<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MLOD             (4<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MLODFRAC         (5<<SST_TCA_MSELECT_SHIFT)
#define SST_TCA_REVERSE_BLEND   BIT(26)
#define SST_TCA_ADD_CLOCAL      BIT(27)
#define SST_TCA_ADD_ALOCAL      BIT(28)
#define SST_TCA_INVERT_OUTPUT   BIT(29)
#define SST_TRILINEAR           BIT(30)
#define SST_SEQ_8_DOWNLD        BIT(31)

// here are some abstract constants that most people will be using
// TC stands for Texture Combine (RGB channels)
// TCA stands for Texture Combine Alpha (just the Alpha channel)
// BLEND_ATT blends on the Alpha channel of the color passed from TMU to TMU
// BLEND_ALOCAL blends on the Alpha channel of the local texture color
// NOTE: and of the BLEND modes can be modified using SST_TC_REVERSE_BLEND
//       and/or SST_TCA_REVERSE_BLEND, these modifiers reverse the "polarity"
//       of the blend
#define SST_TC_MZERO  (SST_TC_MONE  | SST_TC_REVERSE_BLEND)
#define SST_TCA_MZERO (SST_TCA_MONE | SST_TCA_REVERSE_BLEND)

#define SST_TCOMBINE_SHIFT      12       // RGB COMBINE MODES
#define SST_TCOMBINE            (0x1FF<<SST_TCOMBINE_SHIFT)
#       define SST_TC_REPLACE    (SST_TC_ZERO_OTHER | SST_TC_ADD_CLOCAL)
#       define SST_TC_PASS       (SST_TC_MONE)
#       define SST_TC_ADD        (SST_TC_MONE  | SST_TC_ADD_CLOCAL)
#       define SST_TC_SUB        (SST_TC_SUB_CLOCAL | SST_TC_MONE)
#       define SST_TC_MULT       (SST_TC_MCLOCAL | SST_TC_REVERSE_BLEND)
#       define SST_TC_ZERO       (SST_TC_MZERO)
#       define SST_TC_ONE        (SST_TC_MZERO | SST_TC_INVERT_OUTPUT)
#       define SST_TC_BLEND      (SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL)
#       define SST_TC_BLEND_LOD         (SST_TC_BLEND | SST_TC_MLOD)
#       define SST_TC_BLEND_LODFRAC     (SST_TC_BLEND | SST_TC_MLODFRAC)
#       define SST_TC_BLEND_ATT         (SST_TC_BLEND | SST_TC_MAOTHER)
#       define SST_TC_BLEND_ALOCAL      (SST_TC_BLEND | SST_TC_MALOCAL)
#define SST_TACOMBINE_SHIFT     21      // ALPHA COMBINE MODES
#define SST_TACOMBINE           (0x1FF<<SST_TACOMBINE_SHIFT)
#       define SST_TCA_REPLACE   (SST_TCA_ZERO_OTHER | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_PASS      (SST_TCA_MONE)
#       define SST_TCA_ADD       (SST_TCA_MONE  | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_SUB       (SST_TCA_SUB_CLOCAL | SST_TCA_MONE)
#       define SST_TCA_MULT      (SST_TCA_MCLOCAL | SST_TCA_REVERSE_BLEND)
#       define SST_TCA_ONE       (SST_TCA_MZERO | SST_TCA_INVERT_OUTPUT)
#       define SST_TCA_ZERO      (SST_TCA_MZERO)
#       define SST_TCA_BLEND     (SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_BLEND_LOD        (SST_TCA_BLEND | SST_TCA_MLOD)
#       define SST_TCA_BLEND_LODFRAC    (SST_TCA_BLEND | SST_TCA_MLODFRAC)
#       define SST_TCA_BLEND_ATT        (SST_TCA_BLEND | SST_TCA_MAOTHER)
#       define SST_TCA_BLEND_ALOCAL     (SST_TCA_BLEND | SST_TCA_MALOCAL)
#define SST_TMU_ACTIVE(texMode) \
        ((texMode & (SST_TCOMBINE|SST_TACOMBINE)) != (SST_TC_PASS|SST_TCA_PASS))
#define SST_TREX_ACTIVE(texMode) SST_TMU_ACTIVE(texMode)

//----------------- SST tLOD bits       -----------------------
#define SST_LODMIN_SHIFT        0       // 4.2 format
#define SST_LODMIN              (0x3F<<SST_LODMIN_SHIFT)
#define SST_LODMAX_SHIFT        6
#define SST_LODMAX              (0x3F<<SST_LODMAX_SHIFT)
#define SST_LODBIAS_SHIFT       12
#define SST_LODBIAS             (0x3F<<SST_LODBIAS_SHIFT)
#define SST_LOD_ODD             BIT(18)
#define SST_LOD_TSPLIT          BIT(19)
#define SST_LOD_S_IS_WIDER      BIT(20)
#define SST_LOD_ASPECT_SHIFT    21
#define SST_LOD_ASPECT          (0x3<<SST_LOD_ASPECT_SHIFT)
#define SST_LOD_ZEROFRAC        BIT(23)
#define SST_TMULTIBASEADDR      BIT(24)
#define SST_TDATA_SWIZZLE       BIT(25)
#define SST_TDATA_SWAP          BIT(26)
#define SST_TDIRECT_WRITE       BIT(27)

//----------------- SST tDetail bits    -----------------------
#define SST_DETAIL_MAX_SHIFT    0       // 8.0 format
#define SST_DETAIL_MAX          (0xFF<<SST_DETAIL_MAX_SHIFT)
#define SST_DETAIL_BIAS_SHIFT   8       // 6.0 format signed
#define SST_DETAIL_BIAS         (0x3F<<SST_DETAIL_BIAS_SHIFT)
#define SST_DETAIL_SCALE_SHIFT  14      // 3.0 format
#define SST_DETAIL_SCALE        (0x7<<SST_DETAIL_SCALE_SHIFT)

//----------------- SST fbzMode bits    -----------------------
#define SST_ENRECTCLIP          BIT(0)
#define SST_ENCHROMAKEY         BIT(1)
#define SST_ENSTIPPLE           BIT(2)
#define SST_WBUFFER             BIT(3)
#define SST_ENDEPTHBUFFER       BIT(4)
#       define SST_ZFUNC_LT     BIT(5)
#       define SST_ZFUNC_EQ     BIT(6)
#       define SST_ZFUNC_GT     BIT(7)
#       define SST_ZFUNC_SHIFT  5
#       define SST_ZFUNC        ( 0x7 << SST_ZFUNC_SHIFT )
#define SST_ENDITHER            BIT(8)
#define SST_RGBWRMASK           BIT(9)
#define SST_ZAWRMASK            BIT(10)
#define SST_DITHER2x2           BIT(11)
#define SST_ENSTIPPLEPATTERN    BIT(12)
#define SST_ENALPHAMASK         BIT(13)
#define SST_DRAWBUFFER_SHIFT    14
#define SST_DRAWBUFFER          (0x3<<SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_FRONT            (0<<SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_BACK             (1<<SST_DRAWBUFFER_SHIFT)
#define SST_ENZBIAS             BIT(16)
#define SST_YORIGIN             BIT(17)
#define SST_ENALPHABUFFER       BIT(18)
#define SST_ENDITHERSUBTRACT    BIT(19)
#define SST_ABLEND_DITHER_SUB_EN SST_ENDITHERSUBTRACT
#define SST_ENDITHERSUBTRACT    BIT(19)
#define SST_ZCOMPARE_TO_ZACOLOR BIT(20)

//----------------- SST fbzColorPath bits       -----------------------
#define SST_RGBSELECT_SHIFT     0
#define SST_RGBSELECT           (0x3<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_RGBA          (0<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_TMUOUT        (1<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_TREXOUT       SST_RGBSEL_TMUOUT
#       define SST_RGBSEL_C1            (2<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_LFB           (3<<SST_RGBSELECT_SHIFT)
#define SST_ASELECT_SHIFT       2
#define SST_ASELECT             (0x3<<SST_ASELECT_SHIFT)
#       define SST_ASEL_RGBA            (0<<SST_ASELECT_SHIFT)
#       define SST_ASEL_TMUOUT          (1<<SST_ASELECT_SHIFT)
#       define SST_ASEL_TREXOUT         SST_ASEL_TMUOUT
#       define SST_ASEL_C1              (2<<SST_ASELECT_SHIFT)
#       define SST_ASEL_LFB             (3<<SST_ASELECT_SHIFT)
#define SST_LOCALSELECT_SHIFT   4
#define SST_LOCALSELECT         BIT(4)
#define SST_ALOCALSELECT_SHIFT  5
#define SST_ALOCALSELECT        (0x3<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_ITERATOR             (0<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_C0                   (1<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_Z                    (2<<SST_ALOCALSELECT_SHIFT)
#define SST_LOCALSELECT_OVERRIDE_WITH_ATEX      BIT(7)
#define SST_CC_ZERO_OTHER       BIT(8)
#define SST_CC_SUB_CLOCAL       BIT(9)
#define SST_CC_MSELECT_SHIFT    10
#define SST_CC_MSELECT          (0x7<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MONE              (0<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MCLOCAL           (1<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MAOTHER           (2<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MALOCAL           (3<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MATMU             (4<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MATREX            SST_CC_MATMU
#define SST_CC_REVERSE_BLEND    BIT(13)
#define SST_CC_ADD_CLOCAL       BIT(14)
#define SST_CC_ADD_ALOCAL       BIT(15)
#define SST_CC_INVERT_OUTPUT    BIT(16)
#define SST_CCA_ZERO_OTHER      BIT(17)
#define SST_CCA_SUB_CLOCAL      BIT(18)
#define SST_CCA_MSELECT_SHIFT   19
#define SST_CCA_MSELECT         (0x7<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MONE             (0<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MCLOCAL          (1<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MAOTHER          (2<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MALOCAL          (3<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MATMU            (4<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MATREX           SST_CCA_MATMU
#define SST_CCA_REVERSE_BLEND   BIT(22)
#define SST_CCA_ADD_CLOCAL      BIT(23)
#define SST_CCA_ADD_ALOCAL      BIT(24)
#define SST_CCA_INVERT_OUTPUT   BIT(25)
#define SST_PARMADJUST          BIT(26)
#define SST_ENTEXTUREMAP        BIT(27)

// here are some abstract constants that most people will be using
// CC stands for Color Combine (RGB channels)
// CCA stands for Color Combine Alpha (just the Alpha channel)
// BLEND_ATT blends on the Alpha channel of the color passed from TMU to TMU
// BLEND_ALOCAL blends on the Alpha channel of the local texture color
// NOTE: and of the BLEND modes can be modified using SST_CC_REVERSE_BLEND
//       and/or SST_CCA_REVERSE_BLEND, these modifiers reverse the "polarity"
//       of the blend
#define SST_CC_MZERO  (SST_CC_MONE  | SST_CC_REVERSE_BLEND)
#define SST_CCA_MZERO (SST_CCA_MONE | SST_CCA_REVERSE_BLEND)

#define SST_CCOMBINE_SHIFT      8       // RGB COMBINE MODES
#define SST_CCOMBINE            (0x1FF<<SST_CCOMBINE_SHIFT)
#       define SST_CC_REPLACE    (SST_CC_ZERO_OTHER | SST_CC_ADD_CLOCAL)
#       define SST_CC_PASS       (SST_CC_MONE)
#       define SST_CC_ADD        (SST_CC_MONE  | SST_CC_ADD_CLOCAL)
#       define SST_CC_SUB        (SST_CC_SUB_CLOCAL | SST_CC_MONE)
#       define SST_CC_MULT       (SST_CC_MCLOCAL | SST_CC_REVERSE_BLEND)
#       define SST_CC_ZERO       (SST_CC_MZERO)
#       define SST_CC_ONE        (SST_CC_MZERO | SST_CC_INVERT_OUTPUT)
#       define SST_CC_BLEND      (SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL)
#       define SST_CC_BLEND_ATT         (SST_CC_BLEND | SST_CC_MAOTHER)
#       define SST_CC_BLEND_ATEX        (SST_CC_BLEND | SST_CC_MATMU)
#       define SST_CC_BLEND_ATEX_REV    (SST_CC_BLEND | SST_CC_MATMU | SST_CC_REVERSE_BLEND)
#       define SST_CC_BLEND_ATT_REV     (SST_CC_BLEND | SST_CC_MAOTHER | SST_CC_REVERSE_BLEND)
#       define SST_CC_BLEND_ALOCAL      (SST_CC_BLEND | SST_CC_MALOCAL)
#define SST_CACOMBINE_SHIFT     17      // ALPHA COMBINE MODES
#define SST_CACOMBINE           (0x1FF<<SST_CACOMBINE_SHIFT)
#       define SST_CCA_REPLACE   (SST_CCA_ZERO_OTHER | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_PASS      (SST_CCA_MONE)
#       define SST_CCA_ADD       (SST_CCA_MONE  | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_SUB       (SST_CCA_SUB_CLOCAL | SST_CCA_MONE)
#       define SST_CCA_MULT      (SST_CCA_MCLOCAL | SST_CCA_REVERSE_BLEND)
#       define SST_CCA_ONE       (SST_CCA_MZERO | SST_CCA_INVERT_OUTPUT)
#       define SST_CCA_ZERO      (SST_CCA_MZERO)
#       define SST_CCA_BLEND     (SST_CCA_SUB_CLOCAL | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_BLEND_ATT        (SST_CCA_BLEND | SST_CCA_MAOTHER)
#       define SST_CCA_BLEND_ATEX       (SST_CCA_BLEND | SST_CCA_MATMU)
#       define SST_CCA_BLEND_ATEX_REV   (SST_CCA_BLEND | SST_CCA_MATMU | SST_CCA_REVERSE_BLEN)
#       define SST_CCA_BLEND_ATT_REV    (SST_CCA_BLEND | SST_CCA_MAOTHER | SST_CCA_REVERSE_BLEND)
#       define SST_CCA_BLEND_ALOCAL     (SST_CCA_BLEND | SST_CCA_MALOCAL)

//----------------- SSTfogMode bits     -----------------------
#define SST_ENFOGGING           BIT(0)  // Afog*Cfog + (1-Afog)*Color
#define SST_FOGADD              BIT(1)  // generates (1-Afog)*Color
#define SST_FOGMULT             BIT(2)  // generates Afog*Cfog
#define SST_FOG_ALPHA           BIT(3)  // use Alpha iterator to control fog
#define SST_FOG_Z               BIT(4)  // use Z iterator to control fog
#define SST_FOG_CONSTANT        BIT(5)  // simply add Cfog to color and clamp

//----------------- SST alphaMode bits  -----------------------
#define SST_ENALPHAFUNC         BIT(0)
#define SST_ALPHAFNC_SHIFT      1
#define SST_ALPHAFUNC_SHIFT      1
#       define SST_ALPHAFUNC_LT BIT(1)
#       define SST_ALPHAFUNC_EQ BIT(2)
#       define SST_ALPHAFUNC_GT BIT(3)
#   define SST_ALPHAFNC     ( 0x7 << SST_ALPHAFNC_SHIFT )
#   define SST_ALPHAFUNC    ( 0x7 << SST_ALPHAFUNC_SHIFT )
#define SST_ENALPHABLEND        BIT(4)
#define SST_ENANTIALIAS         BIT(5)
#define SST_RGBSRCFACT_SHIFT    8       // RGB source factor
#define SST_RGBSRCFACT          (0xF<<SST_RGBSRCFACT_SHIFT)
#       define SST_A_ZERO       0
#       define SST_A_SRCALPHA   1
#       define SST_A_COLOR      2
#       define SST_A_DSTALPHA   3
#       define SST_A_ONE        4
#       define SST_AOM_SRCALPHA 5
#       define SST_AOM_COLOR    6
#       define SST_AOM_DSTALPHA 7
#       define SST_A_SATURATE   0xF     // for SRC factor only
#       define SST_A_COLORBEFOREFOG 0xF // for DST factor only
#define SST_RGBDSTFACT_SHIFT    12      // RGB destination factor
#define SST_RGBDSTFACT          (0xF<<SST_RGBDSTFACT_SHIFT)
#define SST_ASRCFACT_SHIFT      16      // A source factor
#define SST_ASRCFACT            (0xF<<SST_ASRCFACT_SHIFT)
#define SST_ADSTFACT_SHIFT      20      // A destination factor
#define SST_ADSTFACT            (0xF<<SST_ADSTFACT_SHIFT)
#define SST_ALPHAREF_SHIFT      24
#define SST_ALPHAREF            (0xFF<<SST_ALPHAREF_SHIFT)

//---------------- SST zaColor bits ----------------------
#define SST_ZACOLOR_DEPTH_SHIFT   0
#define SST_ZACOLOR_DEPTH         ( 0xFFFF << SST_ZACOLOR_DEPTH_SHIFT )
#define SST_ZACOLOR_ALPHA_SHIFT   24
#define SST_ZACOLOR_ALPHA         ( 0xFF << SST_ZACOLOR_ALPHA_SHIFT )

//----------------- SST clip bits ------------------------
//clipLeftRight
#define SST_CLIPLEFT_SHIFT        16
#define SST_CLIPRIGHT_SHIFT       0
//clipBottomTop
#define SST_CLIPBOTTOM_SHIFT      16
#define SST_CLIPTOP_SHIFT         0

//----------------- SST status bits     -----------------------
#define SST_FIFOLEVEL           0x3F
#define SST_VRETRACE            BIT(6)
#define SST_FBI_BUSY            BIT(7)
#define SST_TMU_BUSY            BIT(8)
#define SST_TREX_BUSY           SST_TMU_BUSY
#define SST_BUSY                BIT(9)
#define SST_DISPLAYED_BUFFER_SHIFT 10
#define SST_DISPLAYED_BUFFER    (0x3<<SST_DISPLAYED_BUFFER_SHIFT)
#define SST_MEMFIFOLEVEL_SHIFT  12
#define SST_MEMFIFOLEVEL        (0xFFFF<<SST_MEMFIFOLEVEL_SHIFT)
#define SST_SWAPBUFPENDING_SHIFT 28
#define SST_SWAPBUFPENDING      (0x7<<SST_SWAPBUFPENDING_SHIFT)
#define SST_PCIINTERRUPTED      BIT(31)
//----------------- SST fbiInit*        -----------------------
// fbiInit0
#define SST_EN_VGA_PASSTHRU     BIT(0)
#define SST_PCI_FIFO_LWM_SHIFT  6
#define SST_PCI_FIFO_LWM        (0x1F<<SST_PCI_FIFO_LWM_SHIFT)
#define SST_EN_LFB_MEMFIFO      BIT(11)
#define SST_EN_TEX_MEMFIFO      BIT(12)
#define SST_MEM_FIFO_EN         BIT(13)
#define SST_MEM_FIFO_HWM_SHIFT  14
#define SST_MEM_FIFO_HWM        (0x7FF<<SST_MEM_FIFO_HWM_SHIFT)
#define SST_MEM_FIFO_BURST_HWM_SHIFT    25
#define SST_MEM_FIFO_BURST_HWM  (0x3F<<SST_MEM_FIFO_BURST_HWM_SHIFT)
// fbiInit1
#define SST_PCI_WRWS_0                  0x0
#define SST_PCI_WRWS_1                  BIT(1)
#define SST_LFB_READ_EN                 BIT(3)
#define SST_VIDEO_TILES_IN_X_SHIFT      4
#define SST_VIDEO_TILES_IN_X            (0xF<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_448         (7<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_512         (8<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_640         (10<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_832         (13<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_800 SST_VIDEO_TILES_IN_X_832
#define SST_VIDEO_RESET                 BIT(8)
#define SST_VIDEO_RUN                   0
#define SST_VIDEO_STOP                  BIT(8)
#define SST_HVSYNC_OVERRIDE             BIT(9)
#define SST_HSYNC_OVERRIDE_VAL          BIT(10)
#define SST_VSYNC_OVERRIDE_VAL          BIT(11)
#define SST_VIDEO_BLANK_EN              BIT(12)
#define SST_VIDEO_DATA_OE_EN            BIT(13)
#define SST_VIDEO_BLANK_OE_EN           BIT(14)
#define SST_VIDEO_HVSYNC_OE_EN          BIT(15)
#define SST_VIDEO_DCLK_OE_EN            BIT(16)
#define SST_VIDEO_VID_CLK_SEL           BIT(17)
#define SST_VIDEO_VID_CLK_2X            0x0
#define SST_VIDEO_VID_CLK_SLAVE         BIT(17)
#define SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT 18
#define SST_VIDEO_VCLK_2X_INPUT_DEL     (0x3<<SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT)
#define SST_VIDEO_VCLK_SEL_SHIFT        20
#       define SST_VIDEO_VCLK_SEL               (0x3<<SST_VIDEO_VCLK_SEL_SHIFT)
#       define SST_VIDEO_VCLK_DIV2              (0x1<<SST_VIDEO_VCLK_SEL_SHIFT)
#define SST_VIDEO_24BPP_EN              BIT(22)
#define SST_EN_SCANLINE_INTERLEAVE      BIT(23)
#define SST_VIDEO_FILTER_EN             BIT(25)
#define SST_VIDEO_INVERT_VID_CLK_2X     BIT(26)
#define SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT 27
#       define SST_VIDEO_VCLK_2X_OUTPUT_DEL     (0x3<<SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT)
#define SST_VIDEO_VCLK_DEL_SHIFT        29
#       define SST_VIDEO_VCLK_DEL               (0x3<<SST_VIDEO_VCLK_DEL_SHIFT)
#define SST_DISEN_RD_AHEAD_WR_RD        BIT(31)
#define SST_VIDEO_MASK                  0x7e7ffe00
// fbiInit2
#define SST_VIDEO_DITHER_SUB_EN         BIT(0)
#define SST_DRAM_BANKING_CONFIG         BIT(1)
#define SST_EN_TRIPLE_BUFFERING         BIT(4)
#define SST_EN_FAST_RAS_READ            BIT(5)
#define SST_EN_OE_GEN                   BIT(6)
#define SST_EN_FAST_RD_AHEAD_WR         BIT(7)
#define SST_EN_DITHER_PASSTHRU          BIT(8)
#define SST_SWAP_ALGORITHM_SHIFT        9
#define SST_SWAP_ALGORITHM              (0x3<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_VSYNC                   (0<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_DACDATA0                (1<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_FIFOSTALL               (2<<SST_SWAP_ALGORITHM_SHIFT)
#define SST_VIDEO_BUFFER_OFFSET_SHIFT   11
#define SST_VIDEO_BUFFER_OFFSET         (0x1FF<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_448      (77<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_512      (96<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_640      (150<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_832      (247<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#define SST_VIDEO_BUFFER_OFFSET_800     SST_VIDEO_BUFFER_OFFSET_832
#define SST_EN_DRAM_BANKED              BIT(20)
#define SST_EN_DRAM_RD_AHEAD_FIFO       BIT(21)
#define SST_EN_DRAM_REFRESH             BIT(22)
#define SST_DRAM_REFRESH_CNTR_SHIFT     23
#       define SST_DRAM_REFRESH_CNTR            (0x1FF<<SST_DRAM_REFRESH_CNTR_SHIFT)
// fbiInit3
#define SST_ALT_REGMAPPING              BIT(0)
#define SST_VIDEO_FIFO_THRESH_SHIFT     1
#       define SST_VIDEO_FIFO_THRESH            (0x1F<<SST_VIDEO_FIFO_THRESH_SHIFT)
#define SST_YORIGIN_TOP_SHIFT           22
#       define SST_YORIGIN_TOP                  (0x3FF<<SST_YORIGIN_TOP_SHIFT)
#define SST_YORIGIN_SUBVAL_SHIFT        SST_YORIGIN_TOP_SHIFT
#define SST_YORIGIN_SUBVAL                      (0x3FF<<SST_YORIGIN_SUBVAL_SHIFT)

//----------------- SST dacData bits    -----------------------
#define SST_DACDATA_DATA                (0xFF)
#define SST_DACDATA_ADDR_SHIFT          8
#define SST_DACDATA_ADDR                (0x7<<SST_DACDATA_ADDR_SHIFT)
#define SST_DACDATA_RD                  BIT(11)
#define SST_DACDATA_WR                  0x0

//----------------- swapBufferCMD bits    ---------------------
#define SST_SWAP_EN_WAIT_ON_VSYNC       BIT(0)
#define SST_SWAP_BUFFER_INTERVAL_SHIFT  1
#define SST_SWAP_BUFFER_INTERVAL        (0xff << SST_SWAP_BUFFER_INTERVAL_SHIFT)

//----------------- SST Configuration Registers ---------------
#define SST_MEMBASE_ADDR                0x10
#define SST_INIT_ENABLE                 0x40
#define SST_BUS_SNOOP0                  0x44
#define SST_BUS_SNOOP1                  0x48
#define SST_CFG_STATUS                  0x4c

//----------------- SST Remapped Registers --------------------
#define SST_DAC_RD_DATA                 0x00000218
#define SST_VIDEO_CHECKSUM              0x0000021c

//----------------- SST initEnable (Config register) bits   ---
#define SST_INITWR_EN                   BIT(0)
#define SST_PCI_FIFOWR_EN               BIT(1)
#define SST_FBIINIT23_REMAP             BIT(2)
#define SST_SNOOP0_EN                   BIT(4)
#define SST_SNOOP0_IO                   BIT(5)
#define SST_SNOOP0_RD                   BIT(6)
#define SST_SNOOP1_EN                   BIT(7)
#define SST_SNOOP1_IO                   BIT(8)
#define SST_SNOOP1_RD                   BIT(9)
#define SST_SCANLINE_SLV_OWNPCI         BIT(10)
#define SST_SCANLINE_SLI_SLV            BIT(11)

//----------------- Other---------------------------------------
#define SST_LFB_ADDR            BIT(22)
#define SST_TEX_ADDR            BIT(23)

//----------------- SST vRetrace bits ---------------------
#define SST_VRETRACE_VSYNC_OFF  0xff

//----------------- SST hSync bits ------------------------
#define SST_HSYNC_ON            0xff
#define SST_HSYNC_OFF_SHIFT     16
#define SST_HSYNC_OFF           (0x3ff << SST_HSYNCOFF_SHIFT)

//----------------- SST vSync bits ------------------------
#define SST_VSYNC_ON            0xff
#define SST_VSYNC_OFF_SHIFT     16
#define SST_VSYNC_OFF           (0xfff << SST_VSYNC_OFF_SHIFT)

//----------------- SST backPorch bits
#define SST_HBACKPORCH          0xff
#define SST_VBACKPORCH_SHIFT    16
#define SST_VBACKPORCH          (0xff << SST_VBACKPORCH_SHIFT)

//----------------- SST videoDimensionS Register
#define SST_VIDEODIMENSIONS_X   0x3ff
#define SST_VIDEODIMENSIONS_Y_SHIFT 16
#define SST_VIDEODIMENSIONS_Y   0x3ff

//----------------- SST chip layout     -----------------------
// nand2mario: removed Sstregs struct and replaced with OFFSET_*
// registers are in groups of 8 for easy decode
#define OFFSET_status 0x0       // chip status, Read Only
#define OFFSET_reserved0 0x1
#define OFFSET_vA 0x2           // vtxRec vA
#define OFFSET_vB 0x4           // vtxRec vB
#define OFFSET_vC 0x6           // vtxRec vC

#define OFFSET_r 0x8            // long
#define OFFSET_g 0x9
#define OFFSET_b 0xA
#define OFFSET_z 0xB
#define OFFSET_a 0xC
#define OFFSET_s 0xD
#define OFFSET_t 0xE
#define OFFSET_w 0xF

#define OFFSET_drdx 0x10        // long: X gradients
#define OFFSET_dgdx 0x11
#define OFFSET_dbdx 0x12
#define OFFSET_dzdx 0x13
#define OFFSET_dadx 0x14
#define OFFSET_dsdx 0x15
#define OFFSET_dtdx 0x16
#define OFFSET_dwdx 0x17

#define OFFSET_drdy 0x18        // long: Y gradients
#define OFFSET_dgdy 0x19
#define OFFSET_dbdy 0x1A
#define OFFSET_dzdy 0x1B
#define OFFSET_dady 0x1C
#define OFFSET_dsdy 0x1D
#define OFFSET_dtdy 0x1E
#define OFFSET_dwdy 0x1F

#define OFFSET_triangleCMD 0x20 // execute a triangle command (float)
#define OFFSET_reserved1 0x21
#define OFFSET_FvA 0x22         // vtxRec, floating point version
#define OFFSET_FvB 0x24
#define OFFSET_FvC 0x26

#define OFFSET_Fr 0x28          // floating point version
#define OFFSET_Fg 0x29
#define OFFSET_Fb 0x2A
#define OFFSET_Fz 0x2B
#define OFFSET_Fa 0x2C
#define OFFSET_Fs 0x2D
#define OFFSET_Ft 0x2E
#define OFFSET_Fw 0x2F

#define OFFSET_Fdrdx 0x30
#define OFFSET_Fdgdx 0x31
#define OFFSET_Fdbdx 0x32
#define OFFSET_Fdzdx 0x33
#define OFFSET_Fdadx 0x34
#define OFFSET_Fdsdx 0x35
#define OFFSET_Fdtdx 0x36
#define OFFSET_Fdwdx 0x37

#define OFFSET_Fdrdy 0x38
#define OFFSET_Fdgdy 0x39
#define OFFSET_Fdbdy 0x3A
#define OFFSET_Fdzdy 0x3B
#define OFFSET_Fdady 0x3C
#define OFFSET_Fdsdy 0x3D
#define OFFSET_Fdtdy 0x3E
#define OFFSET_Fdwdy 0x3F

#define OFFSET_FtriangleCMD 0x40    // execute a triangle command (float)
#define OFFSET_fbzColorPath 0x41    // color select and combine
#define OFFSET_fogMode 0x42         // fog Mode
#define OFFSET_alphaMode 0x43       // alpha Mode
#define OFFSET_fbzMode 0x44         // framebuffer and Z mode
#define OFFSET_lfbMode 0x45         // linear framebuffer Mode
#define OFFSET_clipLeftRight 0x46   // (6)10(6)10
#define OFFSET_clipBottomTop 0x47   // (6)10(6)10

#define OFFSET_nopCMD 0x48          // execute a nop command
#define OFFSET_fastfillCMD 0x49     // execute a fast fill command
#define OFFSET_swapbufferCMD 0x4A   // execute a swapbuffer command
#define OFFSET_fogColor 0x4B        // (8)888
#define OFFSET_zaColor 0x4C         // 8(8)16
#define OFFSET_chromaKey 0x4D       // (8)888
#define OFFSET_reserved2 0x4E
#define OFFSET_reserved3 0x4F

#define OFFSET_stipple 0x50         // 32 bits, MSB masks pixels
#define OFFSET_c0 0x51              // 8.8.8.8 (ARGB)
#define OFFSET_c1 0x52              // 8.8.8.8 (ARGB)
#define OFFSET_fbiPixelsIn 0x53     // statistic gathering variables
#define OFFSET_fbiChromaFail 0x54
#define OFFSET_fbiZfuncFail 0x55
#define OFFSET_fbiAfuncFail 0x56
#define OFFSET_fbiPixelsOut 0x57

#define OFFSET_fogTable 0x58        // 64 entries, 2 per word, 2 bytes each

#define OFFSET_fbiInit4 0x80
#define OFFSET_vRetrace 0x81
#define OFFSET_backPorch 0x82
#define OFFSET_videoDimensions 0x83
#define OFFSET_fbiInit0 0x84
#define OFFSET_fbiInit1 0x85
#define OFFSET_fbiInit2 0x86
#define OFFSET_fbiInit3 0x87

#define OFFSET_hSync 0x88
#define OFFSET_vSync 0x89
#define OFFSET_clutData 0x8A
#define OFFSET_dacData 0x8B
#define OFFSET_videoFilterRgbThreshold 0x8C

#define OFFSET_textureMode 0xC0
#define OFFSET_tLOD 0xC1
#define OFFSET_tDetail 0xC2
#define OFFSET_texBaseAddr 0xC3
#define OFFSET_texBaseAddr1 0xC4
#define OFFSET_texBaseAddr2 0xC5
#define OFFSET_texBaseAddr38 0xC6
#define OFFSET_trexInit0 0xC7
#define OFFSET_trexInit1 0xC8

#define OFFSET_nccTable0 0xC9       // 0x324 - 0x350
#define OFFSET_nccTable1 0xD5       // 0x354 - 0x380


// Here are the defines for the hardware
#define GET(s) s
#define GET16(s) s
#define SET(d,s) d = s
#define SET16(d,s) d = s
#define SETF(d,s) (*(float *)&(d)) = s

// SET macros for FBI
#define SET_FBI(d,s)    SET (*(&(d)+0x100),s)
#define SET_FBIF(d,s)   SETF(*(&(d)+0x100),s)

// SET macros for TMU0
#define SET_0(d,s)      SET (*(&(d)+0x200),s)
#define SET_0F(d,s)     SETF(*(&(d)+0x200),s)

// SET macros for FBI+TMU0
#define SET_FBI_0(d,s)  SET (*(&(d)+0x300),s)
#define SET_FBI_0F(d,s) SETF(*(&(d)+0x300),s)

// SET macros for TMU1
#define SET_1(d,s)      SET (*(&(d)+0x400),s)
#define SET_1F(d,s)     SETF(*(&(d)+0x400),s)

// SET macros for FBI+TMU1
#define SET_FBI_1(d,s)  SET (*(&(d)+0x500),s)
#define SET_FBI_1F(d,s) SETF(*(&(d)+0x500),s)

// SET macros for TMU2
#define SET_2(d,s)      SET (*(&(d)+0x800),s)
#define SET_2F(d,s)     SETF(*(&(d)+0x800),s)

// #define SST_WRAP(sst,n) ((Sstregs *)((n)*0x4000+(long)(sst)))
// #define SST_CHIP(sst,n) ((Sstregs *)((n)*0x400+(long)(sst)))
// #define SST_TMU(sst,n)  ((Sstregs *)((0x800<<(n))+(long)(sst)))
// #define SST_TREX(sst,n) SST_TMU(sst,n)
#define SST_LFB_ADDRESS(sst)    (0x400000+(long)(sst))
#define SST_TEX_ADDRESS(sst)    (0x800000+(long)(sst))

extern unsigned long sstMipMapSize[4][16];
extern unsigned long sstMipMapOffset[4][16];
extern unsigned long sstMipMapOffset_Tsplit[4][16];


extern void sstMemoryConfig(int fbiMem, int trex0Mem,int trex1Mem,int trex2Mem);
// extern Sstregs *sstGinit (char *, int);
extern void sstTrexChips(int);

#ifdef __cplusplus
}
#endif

#endif /* !__SST_H__ */
