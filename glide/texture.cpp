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
** Revision 1.1.1.1.2.1  2004/12/23 20:56:08  koolsmoky
** builds without asm optimizations (USE_X86=1)
**
** Revision 1.1.1.1  1999/12/07 21:48:52  joseph
** Initial checkin into SourceForge.
**
 * 
 * 6     3/09/97 10:31a Dow
 * Added GR_DIENTRY for di glide functions
**
*/
#include <stdio.h>
#include <string.h>

#include <3dfx.h>
// #define FX_DLL_DEFINITION
// #include <fxdll.h>
#include <glide.h>
#include "fxglide.h"

#ifdef DEBUG_MODE
#define GDBG_INFO(a) gdbg_info a
#define GDBG_INFO_MORE(a) gdbg_info a

int gdbg_info(int level, const char *format, ...) {
    va_list args;
    va_start(args, format); 
    vprintf(format, args);
    va_end(args);
    return 1;
}
#else
#define GDBG_INFO(a)
#define GDBG_INFO_MORE(a)
#endif


extern const int _grMipMapHostWH[GR_ASPECT_1x8+1][GR_LOD_1+1][2];
extern FxU32 _gr_aspect_index_table[];
extern FxU32 _grMipMapHostSize[4][16];

static FxBool ReadDataShort(FILE *, FxU16 *data);
static FxBool ReadDataLong(FILE *, FxU32 *data);
static FxBool Read8Bit(FxU8 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);
static FxBool Read16Bit(FxU16 *dst, FILE *image, int small_lod, int large_lod, GrAspectRatio_t aspect);

#if ((GLIDE_PLATFORM & (GLIDE_OS_DOS32 | GLIDE_OS_WIN32)) != 0)
static const char *openmode = "rb";
#else
static const char *openmode = "r";
#endif

typedef struct
{
  const char        *name;
  GrTextureFormat_t  fmt;
  FxBool             valid;
} CfTableEntry;


static FxBool 
_grGet3dfHeader(FILE* stream, char* const buffer, const FxU32 bufSize)
{
  int numLines = 0;
  FxU32 bufPos = 0;
  
  while(numLines < 4) {
    /* Handle stream errors */
    if (fgets(buffer + bufPos, bufSize - bufPos, stream) == NULL) break;
    bufPos += strlen(buffer + bufPos);
    
    /* fgets includes the '\n' in the buffer. If this is not there
     * then the buffer is too small so fail.
     */
    if (*(buffer + bufPos - sizeof(char)) != '\n') break;
    numLines++;
  }

  return (numLines == 4);
}

/*---------------------------------------------------------------------------
** gu3dfGetInfo
*/
GR_DIENTRY(gu3dfGetInfo, FxBool,
           ( const char *FileName, Gu3dfInfo *Info ))
{
  FILE *image_file;
  FxU32 i;
  char  version[5];
  char  color_format[10];
  int   aspect_width, aspect_height;
  char  buffer[100];
  int   small_lod, large_lod;
  FxBool ratio_found = FXFALSE;
  FxBool format_found = FXFALSE;
  GrAspectRatio_t wh_aspect_table[] =
  {
    GR_ASPECT_1x1,
    GR_ASPECT_1x2,
    GR_ASPECT_1x4,
    GR_ASPECT_1x8
  };
  GrAspectRatio_t hw_aspect_table[] =
  {
    GR_ASPECT_1x1,
    GR_ASPECT_2x1,
    GR_ASPECT_4x1,
    GR_ASPECT_8x1
  };
  CfTableEntry cftable[] = 
  {
      { "I8",       GR_TEXFMT_INTENSITY_8,        FXTRUE },
      { "A8",       GR_TEXFMT_ALPHA_8,            FXTRUE },
      { "AI44",     GR_TEXFMT_ALPHA_INTENSITY_44, FXTRUE },
      { "YIQ",      GR_TEXFMT_YIQ_422,            FXTRUE },
      { "RGB332",   GR_TEXFMT_RGB_332,            FXTRUE },
      { "RGB565",   GR_TEXFMT_RGB_565,            FXTRUE },
      { "ARGB8332", GR_TEXFMT_ARGB_8332,          FXTRUE },
      { "ARGB1555", GR_TEXFMT_ARGB_1555,          FXTRUE },
      { "AYIQ8422", GR_TEXFMT_AYIQ_8422,          FXTRUE },
      { "ARGB4444", GR_TEXFMT_ARGB_4444,          FXTRUE },
      { "AI88",     GR_TEXFMT_ALPHA_INTENSITY_88, FXTRUE },
      { "P8",       GR_TEXFMT_P_8,                FXTRUE },
      { "AP88",     GR_TEXFMT_AP_88,              FXTRUE },
      { 0, 0, FXFALSE }
  };

  GDBG_INFO((81,"gu3dfGetInfo(%s,0x%x)\n",FileName,Info));

  if ((image_file = fopen(FileName, openmode)) == NULL) return FXFALSE;
  if (!_grGet3dfHeader(image_file, buffer, sizeof(buffer))) goto _loc1;

  /*
  ** grab statistics out of the header
  */
  if( sscanf(buffer,"3df v%s %s lod range: %i %i aspect ratio: %i %i\n",
         version,
         color_format,
         &small_lod, &large_lod,
         &aspect_width, &aspect_height) != 6)
    goto _loc1;

  /*
  ** determine aspect ratio, height, and width
  */
  i = 0;
  while ( ( i < 4 ) && ( !ratio_found ) )
  {
    if ( ( aspect_width << i ) == aspect_height )
    {
      Info->header.aspect_ratio = wh_aspect_table[i];
      ratio_found = FXTRUE;
    }
    i++;
  }
  i = 0;
  while ( ( i < 4 ) && ( !ratio_found ) )
  {
    if ( ( aspect_height << i ) == aspect_width )
    {
      Info->header.aspect_ratio = hw_aspect_table[i];
      ratio_found = FXTRUE;
    }
    i++;
  }
  if ( !ratio_found ) goto _loc1;

  /*
  ** determine height and width of the mip map
  */
  if ( aspect_width >= aspect_height )
  {
    Info->header.width  = large_lod;
    Info->header.height = large_lod / aspect_width;
  }
  else
  {
    Info->header.height = large_lod;
    Info->header.width  = large_lod / aspect_height;
  }


  /*
  ** calculate proper LOD values
  */
  switch ( small_lod )
  {
     case 1:
       Info->header.small_lod = GR_LOD_1;
       break;
     case 2:
       Info->header.small_lod = GR_LOD_2;
       break;
     case 4:
       Info->header.small_lod = GR_LOD_4;
       break;
     case 8:
       Info->header.small_lod = GR_LOD_8;
       break;
     case 16:
       Info->header.small_lod = GR_LOD_16;
       break;
     case 32:
       Info->header.small_lod = GR_LOD_32;
       break;
     case 64:
       Info->header.small_lod = GR_LOD_64;
       break;
     case 128:
       Info->header.small_lod = GR_LOD_128;
       break;
     case 256:
       Info->header.small_lod = GR_LOD_256;
       break;
  }

  switch ( large_lod )
  {
     case 1:
       Info->header.large_lod = GR_LOD_1;
       break;
     case 2:
       Info->header.large_lod = GR_LOD_2;
       break;
     case 4:
       Info->header.large_lod = GR_LOD_4;
       break;
     case 8:
       Info->header.large_lod = GR_LOD_8;
       break;
     case 16:
       Info->header.large_lod = GR_LOD_16;
       break;
     case 32:
       Info->header.large_lod = GR_LOD_32;
       break;
     case 64:
       Info->header.large_lod = GR_LOD_64;
       break;
     case 128:
       Info->header.large_lod = GR_LOD_128;
       break;
     case 256:
       Info->header.large_lod = GR_LOD_256;
       break;
  }

  /*
  ** determine the color format of the input image
  */
  {
    char *tempStr = (char*)color_format;
    while (*tempStr != '\0') {
          if (*tempStr >= 'a' && *tempStr <= 'z')
              *tempStr -= ('a'-'A');
          tempStr++;
    }
  }

  i = 0;
  while ( ( cftable[i].name != 0 ) && ( !format_found ) )
  {
    if ( strcmp( color_format, cftable[i].name ) == 0 )
    {
      Info->header.format = cftable[i].fmt;
      format_found = FXTRUE;
    }
    i++;
  }

  /*
  ** close the input file
  */
 _loc1:
  fclose(image_file);

  if ( format_found ) {
      FxI32 lod;
      Info->mem_required = 0;
      for( lod = Info->header.large_lod; lod <= Info->header.small_lod; lod++ ) {
          Info->mem_required +=
          _grMipMapHostSize[_gr_aspect_index_table[Info->header.aspect_ratio]]
                           [lod] << (Info->header.format>=GR_TEXFMT_16BIT);
      }
  }

  return format_found;
}

/*---------------------------------------------------------------------------
** gu3dfLoad
*/
GR_DIENTRY(gu3dfLoad, FxBool, ( const char *filename, Gu3dfInfo *info ))
{
  FILE *image_file  = 0;
  FxU32 index       = 0;
  char  buffer[100] = "";

  GDBG_INFO((81,"gu3dfLoad(%s,0x%x)\n",filename,info));

  if ((image_file = fopen(filename, openmode)) == NULL) return FXFALSE;
  if (!_grGet3dfHeader(image_file, buffer, sizeof(buffer))) goto _loc1;

  /*
  ** If necessary, read in the YIQ decompression table
  */
  if ( ( info->header.format == GR_TEXFMT_YIQ_422 ) ||
       ( info->header.format == GR_TEXFMT_AYIQ_8422 ) )
  {
    /*
    ** read in Y
    */
    for ( index = 0; index < 16; index++ )
    {
      FxU16 val;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.yRGB[index] = val & 0xFF;
    }

    /*
    ** read in I
    */
    for ( index = 0; index < 4; index++ )
    {
      FxU16 val;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.iRGB[index][0] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.iRGB[index][1] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.iRGB[index][2] = val & 0x1FF;
    }

    /*
    ** read in Q
    */
    for ( index = 0; index < 4; index++ )
    {
      FxU16 val;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.qRGB[index][0] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.qRGB[index][1] = val & 0x1FF;
      if (!ReadDataShort(image_file, &val)) goto _loc1;
      info->table.nccTable.qRGB[index][2] = val & 0x1FF;
    }

    /*
    ** pack the table Y entries
    */
    for ( index = 0; index < 4; index++ )
    {
       FxU32 packedvalue;

       packedvalue  = ( ( FxU32 ) info->table.nccTable.yRGB[index*4+0] );
       packedvalue |= ( ( FxU32 ) info->table.nccTable.yRGB[index*4+1] ) << 8;
       packedvalue |= ( ( FxU32 ) info->table.nccTable.yRGB[index*4+2] ) << 16;
       packedvalue |= ( ( FxU32 ) info->table.nccTable.yRGB[index*4+3] ) << 24;

       info->table.nccTable.packed_data[index] = packedvalue;
    }

    /*
    ** pack the table I entries
    */
    for ( index = 0; index < 4; index++ )
    {
       FxU32 packedvalue;

       packedvalue  = ( ( FxU32 ) info->table.nccTable.iRGB[index][0] ) << 18;
       packedvalue |= ( ( FxU32 ) info->table.nccTable.iRGB[index][1] ) << 9;
       packedvalue |= ( ( FxU32 ) info->table.nccTable.iRGB[index][2] ) << 0;

       info->table.nccTable.packed_data[index+4] = packedvalue;
    }

    /*
    ** pack the table Q entries
    */
    for ( index = 0; index < 4; index++ )
    {
       FxU32 packedvalue;

       packedvalue  = ( ( FxU32 ) info->table.nccTable.qRGB[index][0] ) << 18;
       packedvalue |= ( ( FxU32 ) info->table.nccTable.qRGB[index][1] ) << 9;;
       packedvalue |= ( ( FxU32 ) info->table.nccTable.qRGB[index][2] ) << 0;

       info->table.nccTable.packed_data[index+8] = packedvalue;
    }
  }

  /*
  ** If necessary, read in the Palette
  */
  if ( ( info->header.format == GR_TEXFMT_P_8 ) ||
       ( info->header.format == GR_TEXFMT_AP_88 ) ) {
      FxU32 i;
      for( i = 0; i < 256; i++ )
      {
        FxU32 val;
        if (!ReadDataLong(image_file, &val)) goto _loc1;
        info->table.palette.data[i] = val;
      }
  }

  /*
  ** Read in the image
  */
  switch ( info->header.format )
  {
  case GR_TEXFMT_INTENSITY_8:
  case GR_TEXFMT_ALPHA_8:
  case GR_TEXFMT_ALPHA_INTENSITY_44:
  case GR_TEXFMT_YIQ_422:
  case GR_TEXFMT_RGB_332:
  case GR_TEXFMT_P_8:
    if(!Read8Bit((FxU8*)info->data, image_file, info->header.small_lod, info->header.large_lod, info->header.aspect_ratio))
       goto _loc1;
    break;
  case GR_TEXFMT_RGB_565:
  case GR_TEXFMT_ARGB_8332:
  case GR_TEXFMT_ARGB_1555:
  case GR_TEXFMT_AYIQ_8422:
  case GR_TEXFMT_ARGB_4444:
  case GR_TEXFMT_ALPHA_INTENSITY_88:
  case GR_TEXFMT_AP_88:
    if (!Read16Bit((FxU16*)info->data, image_file, info->header.small_lod, info->header.large_lod, info->header.aspect_ratio))
        goto _loc1;
    break;

  default:
 _loc1:
    fclose(image_file);
    return FXFALSE;
  }

  fclose(image_file);
  return FXTRUE;
}

/*
** Read8Bit
**
** Read in an 8-bit texture map, unpacked.
*/
static FxBool Read8Bit( FxU8 *data, FILE *image_file, int small_lod, int large_lod, GrAspectRatio_t aspect_ratio )
{
  int lod;
  FxU32 cnt;

  for (lod = large_lod; lod <= small_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWH[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWH[aspect_ratio][lod][1];

    if (fread(data, 1, cnt, image_file) != cnt)
      return FXFALSE;
    data += cnt;
  }
  return FXTRUE;
}

/*
** Read16Bit
**
** Read in a 16-bit texture map, unpacked.
*/
static FxBool Read16Bit( FxU16 *data, FILE *image_file, int small_lod, int large_lod, GrAspectRatio_t aspect_ratio )
{
  FxU32 idx, cnt;
  int lod;

  for (lod = large_lod; lod <= small_lod; lod++) {
    cnt = (FxU32)_grMipMapHostWH[aspect_ratio][lod][0] *
          (FxU32)_grMipMapHostWH[aspect_ratio][lod][1];

    for (idx = 0; idx < cnt; idx++) {
      if (!ReadDataShort(image_file,data))
        return FXFALSE;
      data++;
    }
  }
  return FXTRUE;
}

/*
** FxU16 ReadDataShort
*/
static FxBool ReadDataShort(FILE *fp, FxU16 *data)
{
  FxU16 value;
  int b;

  /*
  ** read in the MSB
  */
  b = getc (fp);
  if (b == EOF) return FXFALSE;
  value = (FxU16) ((b&0xFF)<<8);

  /*
  ** read in the LSB
  */
  b = getc (fp);
  if (b == EOF) return FXFALSE;
  value |= (FxU16) (b & 0x00FF);

  *data = value;
  return FXTRUE;
}

/*
** ReadDataLong
*/
static FxBool ReadDataLong(FILE *fp, FxU32 *data)
{
    FxU8 byte[4];

    if (fread(byte, 1, 4, fp) != 4)
      return FXFALSE;

   *data = (((FxU32) byte[0]) << 24) |
           (((FxU32) byte[1]) << 16) |
           (((FxU32) byte[2]) <<  8) |
            ((FxU32) byte[3]);

    return FXTRUE;
}



// from gtex.c

extern struct _GlideRoot_s _GlideRoot;
#define SST_TMU(tmu) (1 << ((tmu)+9))

extern const int _grMipMapHostWH[GR_ASPECT_1x8+1][GR_LOD_1+1][2];
extern FxU32 _grMipMapHostSize[][16];
extern FxU32 _gr_aspect_index_table[];
extern FxU32 _gr_evenOdd_xlate_table[];
extern FxU32 _gr_aspect_xlate_table[];

/*---------------------------------------------------------------------------
** grTexClampMode
*/

GR_ENTRY(grTexClampMode, void, ( GrChipID_t tmu, GrTextureClampMode_t s_clamp_mode, GrTextureClampMode_t t_clamp_mode ))
{
  FxU32 texturemode;
  FxU32 clampMode = (
    (s_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPS : 0) |
    (t_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPT : 0)
  );

  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d, %d,%d)\n",tmu,s_clamp_mode,t_clamp_mode));
  GR_CHECK_TMU(myName, tmu);
  
  texturemode  = gc->state.tmu_config[tmu].textureMode;
  texturemode &= ~( SST_TCLAMPS | SST_TCLAMPT );
  texturemode |=  clampMode;

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texturemode );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].textureMode = texturemode;
} /* grTexClampMode */

/*---------------------------------------------------------------------------
** grTexCombine
*/

GR_ENTRY(grTexCombine, void, ( GrChipID_t tmu, GrCombineFunction_t rgb_function, GrCombineFactor_t rgb_factor,  GrCombineFunction_t alpha_function, GrCombineFactor_t alpha_factor, FxBool rgb_invert, FxBool alpha_invert ))
{
  FxU32 texturemode;
  FxU32 tLod;
  FxU32 tmuMask;

  GR_BEGIN("grTexCombine",88,8+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d, %d,%d, %d,%d, %d,%d)\n",
                 tmu, rgb_function, rgb_factor, 
                 alpha_function, alpha_factor,
                 rgb_invert, alpha_invert));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_W( myName,
               rgb_function < GR_COMBINE_FUNCTION_ZERO ||
               rgb_function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported texture color combine function" );
  GR_CHECK_W( myName,
               alpha_function < GR_COMBINE_FUNCTION_ZERO ||
               alpha_function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported texture alpha combine function" );
  GR_CHECK_W( myName,
               (rgb_factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
               (rgb_factor & 0x7) > GR_COMBINE_FACTOR_LOD_FRACTION ||
               rgb_factor > GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
               "unsupported texture color combine scale factor" );
  GR_CHECK_W( myName,
               (alpha_factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
               (alpha_factor & 0x7) > GR_COMBINE_FACTOR_LOD_FRACTION ||
               alpha_factor > GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
               "unsupported texture alpha combine scale factor" );

  /* tmuMask tells grColorCombineFunction what to turn off an on if 
     the ccFunc requires texture mapping */
  texturemode = gc->state.tmu_config[tmu].textureMode;
  texturemode &= ~(SST_TCOMBINE | SST_TACOMBINE);
  tLod = gc->state.tmu_config[tmu].tLOD;
  tLod &= ~(SST_LOD_ODD);

  tmuMask = GR_TMUMASK_TMU0 << tmu;
  gc->state.tmuMask &= ~tmuMask;

  /* setup scale factor bits */
  texturemode |= ( rgb_factor & 0x7 ) << SST_TC_MSELECT_SHIFT;

  if ( !( rgb_factor & 0x8 ) )
    texturemode |= SST_TC_REVERSE_BLEND;

  if ( ( ( rgb_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL ) ||
       ( ( rgb_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL_ALPHA ) )
    gc->state.tmuMask |= tmuMask;

  texturemode |= ( alpha_factor & 0x7 ) << SST_TCA_MSELECT_SHIFT;

  if ( !( alpha_factor & 0x8 ) )
    texturemode |= SST_TCA_REVERSE_BLEND;

  if ( ( ( alpha_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL ) ||
       ( ( alpha_factor & 0x7 ) == GR_COMBINE_FACTOR_LOCAL_ALPHA ) )
    gc->state.tmuMask |= tmuMask;

  /* setup invert output bits */

  if ( rgb_invert )
    texturemode |= SST_TC_INVERT_OUTPUT;

  if ( alpha_invert )
    texturemode |= SST_TCA_INVERT_OUTPUT;

  /* setup core color combine unit bits */
  
  switch ( rgb_function )
  {
  case GR_COMBINE_FUNCTION_ZERO:
    texturemode |= SST_TC_ZERO_OTHER;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    texturemode |= SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    texturemode |= SST_TC_SUB_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TC_ZERO_OTHER | SST_TC_SUB_CLOCAL | SST_TC_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;
  }
  
  switch ( alpha_function )
  {
  case GR_COMBINE_FUNCTION_ZERO:
    texturemode |= SST_TCA_ZERO_OTHER;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    texturemode |= SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    texturemode |= SST_TCA_SUB_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL;
    gc->state.tmuMask |= tmuMask;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    texturemode |= SST_TCA_ZERO_OTHER | SST_TCA_SUB_CLOCAL | SST_TCA_ADD_ALOCAL;
    gc->state.tmuMask |= tmuMask;
    break;
  }
  
  /* Hack to enable TWO-PASS Trilinear 
     
   */
  if ( texturemode & SST_TRILINEAR ) {
    if ( ( texturemode & SST_TC_ZERO_OTHER ) &&
         ( texturemode & SST_TC_BLEND_LODFRAC ) &&
         !( texturemode & SST_TC_REVERSE_BLEND ) ) {
        tLod |= SST_LOD_ODD;
    }
  }
  tLod |= _gr_evenOdd_xlate_table[gc->state.tmu_config[tmu].evenOdd];

  /* update register */
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texturemode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD, tLod );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].textureMode = texturemode;
  gc->state.tmu_config[tmu].tLOD = tLod;

  /* update paramIndex */
  _grUpdateParamIndex();

  GR_END();
} /* grTexCombine */

/*
** _grTexDetailControl, NOTE: its up to caller to account for bytes
*/
GR_DDFUNC(_grTexDetailControl, void, ( GrChipID_t tmu, FxU32 detail ))
{
  GR_BEGIN("_grTexDetailControl",88,4+2*PACKER_WORKAROUND_SIZE);

  GR_CHECK_TMU( "_grTexDetailControl", tmu );

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_tDetail , detail );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].tDetail = detail;
  GR_END();
} /* _grTexDetailControl */

/*---------------------------------------------------------------------------
** grTexFilterMode
*/

GR_ENTRY(grTexFilterMode, void, ( GrChipID_t tmu, GrTextureFilterMode_t minfilter, GrTextureFilterMode_t magfilter ))
{
  FxU32 texMode;

  GR_BEGIN("grTexFilterMode",99,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d)\n",tmu,minfilter,magfilter));
  GR_CHECK_TMU( myName, tmu );

  texMode  = gc->state.tmu_config[tmu].textureMode;
  texMode &= ~( SST_TMINFILTER | SST_TMAGFILTER );
  texMode |= (minfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
             (magfilter == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0);

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].textureMode = texMode;
  GR_END();
} /* grTexFilterMode */

/*---------------------------------------------------------------------------
** grTexLodBiasValue
*/

GR_ENTRY(grTexLodBiasValue, void, ( GrChipID_t tmu, float fvalue ))
{
  FxU32 tLod;
  
  GR_BEGIN("grTexLodBiasValue",88,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%g)\n",tmu,fvalue));
  GR_CHECK_TMU(myName,tmu);
  
  tLod = gc->state.tmu_config[tmu].tLOD;
  tLod &= ~( SST_LODBIAS );
  tLod |= _grTexFloatLODToFixedLOD( fvalue ) << SST_LODBIAS_SHIFT;

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].tLOD = tLod;
  GR_END();
} /* grTexLodBiasValue */

/*-------------------------------------------------------------------
  Function: grTexMipMapMode
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Sets the mip map mode for the specified TMU
    "Ex" because glide's grTexMipMapMode is inadequate for 
    low level texture memory management
  Arguments:
    tmu       - tmu to update
    mmMode   - mipmap mode 
      One of:
        GR_MIPMAP_DISABLE
        GR_MIPMAP_NEAREST
        GR_MIPMAP_NEAREST_DITHER
    lodBlend - enable lodBlending
      FXTRUE  - enabled
      FXFALSE - disabled
  Return:
  none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexMipMapMode, void, ( GrChipID_t tmu, GrMipMapMode_t mmMode, FxBool lodBlend ))
{
  FxU32
    tLod,
    texMode;

  GR_BEGIN("grTexMipMapMode",88,8+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d)\n",tmu,mmMode,lodBlend));
  GR_CHECK_TMU(myName,tmu);
  
  /*--------------------------------------------------------------
    Get Current tLod and texMode register values
    --------------------------------------------------------------*/
  tLod  = gc->state.tmu_config[tmu].tLOD;
  texMode = gc->state.tmu_config[tmu].textureMode;

  /*--------------------------------------------------------------
    Clear LODMIN, LODMAX and LODDITHER
    --------------------------------------------------------------*/
  tLod  &= ~(SST_LODMIN|SST_LODMAX|SST_LOD_ODD);
  texMode &= ~(SST_TLODDITHER|SST_TRILINEAR);

  /*--------------------------------------------------------------
    Encode Mipmap Mode Bits
    --------------------------------------------------------------*/
  switch ( mmMode ) {
  case GR_MIPMAP_DISABLE:
    /*----------------------------------------------------------
      To disable mipmapping set the min and max lods to the same
      value
      ----------------------------------------------------------*/
    tLod |= SST_TLOD_MINMAX_INT(gc->state.tmu_config[tmu].largeLod,
                                gc->state.tmu_config[tmu].largeLod);
    break;
  case GR_MIPMAP_NEAREST_DITHER:
    if (gc->state.allowLODdither)
      texMode |= SST_TLODDITHER;
    /* intentional fall-through to set lodmin and lodmax values */
  case GR_MIPMAP_NEAREST:
    /*----------------------------------------------------------
      Set LODMIN and LODMAX in the tLod register to the 
      actual min and max LODs of the current texture.
      ----------------------------------------------------------*/
    tLod |= SST_TLOD_MINMAX_INT(gc->state.tmu_config[tmu].largeLod,
                                gc->state.tmu_config[tmu].smallLod);
    break;
  default:
    GrErrorCallback( "grTexMipMapMode:  invalid mode passed", FXFALSE );
    break;
  }
  gc->state.tmu_config[tmu].mmMode = mmMode;
  
  /*--------------------------------------------------------------
    Fix trilinear and evenOdd bits -

    This is a bit of a hack to make two pass trilinear work with
    full textures.  The assumption here is that the only reason
    you would ever set up Multiply by LODFRAC w/o REVERSE BLEND
    is for the ODD pass of trilinear.  
    --------------------------------------------------------------*/
  if ( lodBlend ) {
    texMode |= SST_TRILINEAR;
    if ( ( texMode & SST_TC_ZERO_OTHER ) &&
         ( texMode & SST_TC_BLEND_LODFRAC ) &&
         !( texMode & SST_TC_REVERSE_BLEND ) ) {
        tLod |= SST_LOD_ODD;
    }
  }
  tLod |= _gr_evenOdd_xlate_table[gc->state.tmu_config[tmu].evenOdd];
  
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].tLOD        = tLod;
  gc->state.tmu_config[tmu].textureMode = texMode;
  GR_END();
} /* grTexMipMapMode */

/*-------------------------------------------------------------------
  Function: grTexMinAddress
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Returns address of start of texture ram for a TMU
  Arguments:
    tmu
  Return:
    integer texture base address, this pointer is not to be dereferenced
    by the application, it is on to be used by grTexDownload(),
    and grTExDownloadLevel()
  -------------------------------------------------------------------*/
/*-------------------------------------------------------------------
  Function: grTexNCCTable
  Date: 6/3
  Implementor(s): jdt
  Library: glide
  Description:
    select one of the two NCC tables
  Arguments:
    tmu - which tmu
    table - which table to select
        One of:
            GR_TEXTABLE_NCC0
            GR_TEXTABLE_NCC1
            GR_TEXTABLE_PALETTE
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexNCCTable, void, ( GrChipID_t tmu, GrNCCTable_t table ))
{
  FxU32 texMode;
  
  GR_BEGIN("grTexNCCTable",88,4+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",tmu));
  GR_CHECK_TMU(myName,tmu);
  GR_CHECK_F(myName, table>0x1, "invalid ncc table specified");

  /*------------------------------------------------------------------
    Update local state
    ------------------------------------------------------------------*/
  gc->state.tmu_config[tmu].nccTable = table;
  
  /*------------------------------------------------------------------
    Grab shadow texMode, update TexMode, update shadow/real register
    ------------------------------------------------------------------*/
  texMode  = gc->state.tmu_config[tmu].textureMode;
  texMode &= ~( SST_TNCCSELECT );
  if ( table )
    texMode |= SST_TNCCSELECT;
  else 
    texMode &= ~(SST_TNCCSELECT);

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].textureMode = texMode;
  GR_END();
} /* grTexNCCTable */


/*-------------------------------------------------------------------
  Function: grTexSource
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Sets up the current texture for texture mapping on the specified
    TMU.
  Arguments:
    tmu          - which tmu
    startAddress - texture start address
    evenOdd  - which set of mipmap levels have been downloaded for
                the selected texture
                One of:
                  GR_MIPMAPLEVELMASK_EVEN 
                  GR_MIPMAPLEVELMASK_ODD
                  GR_MIPMAPLEVELMASK_BOTH
    info         - pointer to GrTexInfo structure containing
                   texture dimensions
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexSource, void, ( GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info ))
{
  FxU32 baseAddress, texMode, tLod;

  GR_BEGIN("grTexSource",88,12+2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x,%d,0x%x)\n",tmu,startAddress,evenOdd,info));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_F( myName, startAddress >= gc->tmu_state[tmu].total_mem, "invalid startAddress" );
  GR_CHECK_F( myName,
               startAddress + grTexTextureMemRequired( evenOdd, info ) >= gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress" );
  GR_CHECK_F( myName, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F( myName, !info, "invalid info pointer" );
  
  /*-------------------------------------------------------------
    Update Texture Unit State
    -------------------------------------------------------------*/
  gc->state.tmu_config[tmu].smallLod = info->smallLod; 
  gc->state.tmu_config[tmu].largeLod = info->largeLod; 
  gc->state.tmu_config[tmu].evenOdd  =  evenOdd; 
  
  /*-------------------------------------------------------------
    Calculate Base Address
    -------------------------------------------------------------*/
  baseAddress = _grTexCalcBaseAddress( startAddress,
                                       info->largeLod, 
                                       info->aspectRatio,
                                       info->format,
                                       evenOdd ) >> 3;
  /*-------------------------------------------------------------
    Update Texture Mode
    -------------------------------------------------------------*/
  texMode = gc->state.tmu_config[tmu].textureMode;
  texMode &= ~SST_TFORMAT;
  texMode |= ( info->format << SST_TFORMAT_SHIFT ) | SST_TPERSP_ST | SST_TCLAMPW;
  
  /*-------------------------------------------------------------
    Compute TLOD (keep LODBIAS in tact)
    -------------------------------------------------------------*/
  tLod = gc->state.tmu_config[tmu].tLOD;
  tLod &= ~(SST_LODMIN | SST_LODMAX | SST_LOD_ASPECT |
            SST_LOD_TSPLIT | SST_LOD_ODD | SST_LOD_S_IS_WIDER);
  tLod |= SST_TLOD_MINMAX_INT(info->largeLod,
                     gc->state.tmu_config[tmu].mmMode==GR_MIPMAP_DISABLE ? 
                              info->largeLod : info->smallLod);
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[info->aspectRatio];

  /* Write relevant registers out to hardware */
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  PACKER_WORKAROUND;
  
  /* update shadows */
  gc->state.tmu_config[tmu].texBaseAddr = baseAddress; 
  gc->state.tmu_config[tmu].textureMode = texMode; 
  gc->state.tmu_config[tmu].tLOD        = tLod; 
  
  GR_END();
} /* grTexSource */


/*-------------------------------------------------------------------
  Function: grTexMultibase
  Date: 11/4/96
  Implementor(s): gmt
  Library: Glide
  Description:
    Enable multiple base addresses for texturing.
  Arguments:
    tmu    - which tmu
    enable - flag which enables/disables multibase
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexMultibase, void, ( GrChipID_t tmu, FxBool enable ))
{
  FxU32 tLod;
    
  GR_BEGIN("grTexMultibase",88,4+PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d)\n",tmu,enable));
  GR_CHECK_TMU(myName,tmu);
  
  tLod  = gc->state.tmu_config[tmu].tLOD;
  if ( enable )
    tLod |= SST_TMULTIBASEADDR;
  else
    tLod &= ~SST_TMULTIBASEADDR;
  /*--------------------------------------------------------------
    Write State To Hardware and Update Glide Shadow State
    --------------------------------------------------------------*/
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  PACKER_WORKAROUND;

  gc->state.tmu_config[tmu].tLOD = tLod;
  GR_END();
} /* grTexMultibase */

/*-------------------------------------------------------------------
  Function: grTexMultibaseAddress
  Date: 11/4/96
  Implementor(s): gmt
  Library: Glide
  Description:
    Set the base address for a particular set of mipmaps
  Arguments:
    tmu    - which tmu
    range  - range of lods that are based at this starting address
             One of:
             GR_TEXBASE_256
             GR_TEXBASE_128
             GR_TEXBASE_64
             GR_TEXBASE_32_TO_1
    startAddress - start address that data was downloaded to 
                    hardware with using grTexDownload/Level
    info         - pointer to GrTexInfo structure containing
                   texture dimensions
  Return:
    none
  -------------------------------------------------------------------*/

GR_ENTRY(grTexMultibaseAddress, void, ( GrChipID_t tmu, GrTexBaseRange_t range, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info ))
{
  FxU32 baseAddress;

  GR_BEGIN("grTexMultibaseAddress",88,4+PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,0x%x)\n",tmu,range,startAddress));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_F( myName, range > GR_TEXBASE_32_TO_1, "invalid range" );
  GR_CHECK_F( myName, startAddress >= gc->tmu_state[tmu].total_mem, "invalid startAddress" );
  GR_CHECK_F( myName, evenOdd > 0x3, "evenOdd mask invalid" );
  GR_CHECK_F( myName, info, "invalid info pointer" );
  

  /* Write relevant registers out to hardware and shadows */
  PACKER_WORKAROUND;
  switch (range) {
    case GR_TEXBASE_256:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_256,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr = baseAddress; 
      break;
    case GR_TEXBASE_128:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_128,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr_1 = baseAddress; 
      break;
    case GR_TEXBASE_64:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_64,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr_2 = baseAddress; 
      break;
    case GR_TEXBASE_32_TO_1:
      baseAddress = _grTexCalcBaseAddress( startAddress,
                                           GR_LOD_32,
                                           info->aspectRatio,
                                           info->format,
                                           evenOdd ) >> 3;
      GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
      gc->state.tmu_config[tmu].texBaseAddr_3_8 = baseAddress; 
      break;
  }
  PACKER_WORKAROUND;
  GR_END();
} /* grTexMultibaseAddress */

#if 0
/*
** _grTexForceLod
**
** Forces tLOD to a specific LOD level.  This is useful only for
** debugging purposes.  GMT: obsolete, please remove
*/
void 
_grTexForceLod( GrChipID_t tmu, int value )
{
  GR_DCL_GC;
  GR_DCL_HW;
  FxU32 tLod = gc->state.tmu_config[0].tLOD;

  GR_CHECK_TMU("_grTexForceLod",tmu);

  tLod &= ~(SST_LODMIN | SST_LODMAX);
  tLod |= SST_TLOD_MINMAX_INT(value,value);

  PACKER_WORKAROUND;
  GR_SET( SST_TMU(hw,tmu)->tLOD , tLod );
  PACKER_WORKAROUND;
  gc->state.tmu_config[tmu].tLOD = tLod;
} /* _grTexForceLod */
#endif


// from ditex.c

FxU32 _gr_aspect_index_table[] =
{
   3,
   2,
   1,
   0,
   1,
   2,
   3,
};

/* size in texels  */
FxU32 _grMipMapHostSize[4][16] = 
{
  {                             /* 1:1 aspect ratio */
    65536,                      /* 0 : 256x256  */
    16384,                      /* 1 : 128x128  */
    4096,                       /* 2 :  64x64   */
    1024,                       /* 3 :  32x32   */
    256,                        /* 4 :  16x16   */
    64,                         /* 5 :   8x8    */
    16,                         /* 6 :   4x4    */
    4,                          /* 7 :   2x2    */
    1,                          /* 8 :   1x1    */
  },
  {                             /* 2:1 aspect ratio */
    32768,                      /* 0 : 256x128  */
    8192,                       /* 1 : 128x64   */
    2048,                       /* 2 :  64x32   */
    512,                        /* 3 :  32x16   */
    128,                        /* 4 :  16x8    */
    32,                         /* 5 :   8x4    */
    8,                          /* 6 :   4x2    */
    2,                          /* 7 :   2x1    */
    1,                          /* 8 :   1x1    */
  },
  {                             /* 4:1 aspect ratio */
    16384,                      /* 0 : 256x64   */
    4096,                       /* 1 : 128x32   */
    1024,                       /* 2 :  64x16   */
    256,                        /* 3 :  32x8    */
    64,                         /* 4 :  16x4    */
    16,                         /* 5 :   8x2    */
    4,                          /* 6 :   4x1    */
    2,                          /* 7 :   2x1    */
    1,                          /* 8 :   1x1    */
  },
  {                             /* 8:1 aspect ratio */
    8192,                       /* 0 : 256x32   */
    2048,                       /* 1 : 128x16   */
    512,                        /* 2 :  64x8    */
    128,                        /* 3 :  32x4    */
    32,                         /* 4 :  16x2    */
    8,                          /* 5 :   8x1    */
    4,                          /* 6 :   4x1    */
    2,                          /* 7 :   2x1    */
    1,                          /* 8 :   1x1    */
  }
};

const int _grMipMapHostWH[GR_ASPECT_1x8+1][GR_LOD_1+1][2] =
{
   { 
     { 256 , 32 }, 
     { 128 , 16 }, 
     { 64  , 8 }, 
     { 32  , 4 }, 
     { 16  , 2 }, 
     { 8   , 1 }, 
     { 4   , 1 }, 
     { 2   , 1 }, 
     { 1   , 1 }
   },
   { 
     { 256 , 64 }, 
     { 128 , 32 }, 
     { 64  , 16 }, 
     { 32  , 8 }, 
     { 16  , 4 }, 
     { 8   , 2 },
     { 4   , 1 }, 
     { 2   , 1 }, 
     { 1   , 1 }
   },
   { 
     { 256 , 128 }, 
     { 128 , 64 }, 
     { 64  , 32 }, 
     { 32  , 16 },
     { 16  , 8 },
     { 8   , 4 }, 
     { 4   , 2 }, 
     { 2   , 1 }, 
     { 1   , 1 }
   },
   { 
     { 256 , 256 }, 
     { 128 , 128 }, 
     { 64  , 64 },
     { 32  , 32 },
     { 16  , 16 },
     { 8   , 8 }, 
     { 4   , 4 }, 
     { 2   , 2 }, 
     { 1   , 1 }
   },
   { 
     { 128, 256 },
     { 64,  128 },
     { 32,  64  },
     { 16,  32  },
     { 8,   16  }, 
     { 4,   8   }, 
     { 2,   4   }, 
     { 1,   2   }, 
     { 1,   1 }
   },
   {
     { 64,  256 },
     { 32,  128 },
     { 16,  64  },
     { 8,   32  },
     { 4,   16  },
     { 2,   8   }, 
     { 1,   4   }, 
     { 1,   2   }, 
     { 1,   1 }
   },
   { 
     { 32,  256 },
     { 16,  128 },
     { 8,   64  },
     { 4,   32  },
     { 2,   16  },
     { 1,   8   }, 
     { 1,   4   }, 
     { 1,   2   },
     { 1,   1 }
   }
};

/* translates GR_ASPECT_* to bits for the TLOD register */
FxU32 _gr_aspect_xlate_table[] =
{
   (3<< SST_LOD_ASPECT_SHIFT) | SST_LOD_S_IS_WIDER,
   (2<< SST_LOD_ASPECT_SHIFT) | SST_LOD_S_IS_WIDER,
   (1<< SST_LOD_ASPECT_SHIFT) | SST_LOD_S_IS_WIDER,
    0<< SST_LOD_ASPECT_SHIFT,
    1<< SST_LOD_ASPECT_SHIFT,
    2<< SST_LOD_ASPECT_SHIFT,
    3<< SST_LOD_ASPECT_SHIFT
};

FxU32 _gr_evenOdd_xlate_table[] = 
{
  0xFFFFFFFF,                      /* invalid */
  SST_LOD_TSPLIT,                  /* even */
  SST_LOD_TSPLIT | SST_LOD_ODD,    /* odd  */
  0,                               /* both */
};

/* the size of each mipmap level in texels, 4 is the minimum no matter what */
/* index is [aspect_ratio][lod]                                             */
unsigned long _grMipMapSize[4][16] = {
  {   /* 8:1 aspect ratio */
    0x02000,        /* 0 : 256x32   */
    0x00800,        /* 1 : 128x16   */
    0x00200,        /* 2 :  64x8    */
    0x00080,        /* 3 :  32x4    */
    0x00020,        /* 4 :  16x2    */
    0x00010,        /* 5 :   8x1    */
    0x00008,        /* 6 :   4x1    */
    0x00004,        /* 7 :   2x1    */
    0x00004,        /* 8 :   1x1    */
  },
  {   /* 4:1 aspect ratio */
    0x04000,        /* 0 : 256x64   */
    0x01000,        /* 1 : 128x32   */
    0x00400,        /* 2 :  64x16   */
    0x00100,        /* 3 :  32x8    */
    0x00040,        /* 4 :  16x4    */
    0x00010,        /* 5 :   8x2    */
    0x00008,        /* 6 :   4x1    */
    0x00004,        /* 7 :   2x1    */
    0x00004,        /* 8 :   1x1    */
  },
  {   /* 2:1 aspect ratio */
    0x08000,        /* 0 : 256x128  */
    0x02000,        /* 1 : 128x64   */
    0x00800,        /* 2 :  64x32   */
    0x00200,        /* 3 :  32x16   */
    0x00080,        /* 4 :  16x8    */
    0x00020,        /* 5 :   8x4    */
    0x00008,        /* 6 :   4x2    */
    0x00004,        /* 7 :   2x1    */
    0x00004,        /* 8 :   1x1    */
  },
  {   /* 1:1 aspect ratio */
    0x10000,        /* 0 : 256x256  */
    0x04000,        /* 1 : 128x128  */
    0x01000,        /* 2 :  64x64   */
    0x00400,        /* 3 :  32x32   */
    0x00100,        /* 4 :  16x16   */
    0x00040,        /* 5 :   8x8    */
    0x00010,        /* 6 :   4x4    */
    0x00004,        /* 7 :   2x2    */
    0x00004,        /* 8 :   1x1    */
  },
};


/* the offset from mipmap level 0 of each mipmap level in texels            */
/* index is [aspect_ratio][lod]                                             */
unsigned long _grMipMapOffset[4][16];
unsigned long _grMipMapOffset_Tsplit[4][16];

/* initialize the MipMap Offset arrays */
void
_grMipMapInit(void)
{
  int ar,lod;

  for (ar=0; ar<4; ar++) {              /* for each aspect ratio           */
    _grMipMapOffset[ar][0] = 0;         /* start off with offset=0         */
    for (lod=1; lod<=9; lod++) {        /* for each lod, add in prev size */
      _grMipMapOffset[ar][lod] = _grMipMapOffset[ar][lod-1] +
        _grMipMapSize[ar][lod-1];
    }
    _grMipMapOffset_Tsplit[ar][0] = 0;  /* start off with offset=0 */
    _grMipMapOffset_Tsplit[ar][1] = 0;  /* start off with offset=0 */
    for (lod=2; lod<=9; lod++) {        /* for each lod, add in prev size */
      _grMipMapOffset_Tsplit[ar][lod] = _grMipMapOffset_Tsplit[ar][lod-2] +
        _grMipMapSize[ar][lod-2];
    }
  }
} /* _grMipMapInit */

/*---------------------------------------------------------------------------
**
*/
FxU32
_grTexTextureMemRequired( GrLOD_t small_lod, GrLOD_t large_lod, 
                           GrAspectRatio_t aspect, GrTextureFormat_t format,
                           FxU32 evenOdd )
{
  FxU32 memrequired;

  GR_CHECK_W("_grTexTextureMemRequired", small_lod < large_lod,
                 "small_lod bigger than large_lod" );
  GR_CHECK_F( "_grTexTextureMemRequired", evenOdd >  GR_MIPMAPLEVELMASK_BOTH || evenOdd == 0, "invalid evenOdd mask" );

  if ( aspect > GR_ASPECT_1x1 )         /* mirror aspect ratios         */
    aspect =  GR_ASPECT_1x8 - aspect;

  if ( evenOdd == GR_MIPMAPLEVELMASK_BOTH ) {
    memrequired  = _grMipMapOffset[aspect][small_lod+1];
    memrequired -= _grMipMapOffset[aspect][large_lod];
  }
  else {
    memrequired = 0;
    /* construct XOR mask   */
    evenOdd = (evenOdd == GR_MIPMAPLEVELMASK_EVEN) ? 1 : 0;  
    while (large_lod <= small_lod) {    /* sum up all the mipmap levels */
      if ((large_lod ^ evenOdd) & 1)    /* that match the XOR mask      */
        memrequired += _grMipMapSize[aspect][large_lod];
      large_lod++;
    }
  }

  if ( format >= GR_TEXFMT_16BIT )      /* convert from texels to bytes */
    memrequired <<= 1;                  /* 2 bytes per texel            */

  memrequired += 7;                     /* round up to 8 byte boundary  */
  memrequired &= ~7;
  return memrequired;
} /* _grTexTextureMemRequired */

FxU16
_grTexFloatLODToFixedLOD( float value )
{
  float num_quarters;
  int   new_value;

  num_quarters = ( value + .125F ) / .25F;
  new_value    = ( int ) num_quarters;

  new_value   &= 0x003F;

  return new_value;
} /* _grTexFloatLODToFixedLOD */

/*---------------------------------------------------------------------------
** _grTexCalcBaseAddress
*/
FxU32
_grTexCalcBaseAddress( FxU32 start, GrLOD_t large_lod,
                       GrAspectRatio_t aspect, GrTextureFormat_t format,
                       FxU32 odd_even_mask )
{
  FxU32 sum_of_lod_sizes;

  if ( aspect > GR_ASPECT_1x1 )         /* mirror aspect ratios         */
    aspect =  GR_ASPECT_1x8 - aspect;

  if ( odd_even_mask == GR_MIPMAPLEVELMASK_BOTH )
    sum_of_lod_sizes = _grMipMapOffset[aspect][large_lod];
  else {
    if (
        ((odd_even_mask == GR_MIPMAPLEVELMASK_EVEN) && (large_lod & 1)) ||
        ((odd_even_mask == GR_MIPMAPLEVELMASK_ODD) && !(large_lod & 1))
        )
      large_lod += 1;
    sum_of_lod_sizes = _grMipMapOffset_Tsplit[aspect][large_lod];
  }
    
  if ( format >= GR_TEXFMT_16BIT )
    sum_of_lod_sizes <<= 1;
  return ( start - sum_of_lod_sizes );
} /* _grTexCalcBaseAddress */

/*---------------------------------------------------------------------------
** grTexCalcMemRequired
*/
GR_DIENTRY(grTexCalcMemRequired, FxU32,
           ( GrLOD_t small_lod, GrLOD_t large_lod, 
            GrAspectRatio_t aspect, GrTextureFormat_t format ))
{
  FxU32 memrequired;

  memrequired = _grTexTextureMemRequired(small_lod, large_lod, 
                                         aspect, format,
                                         GR_MIPMAPLEVELMASK_BOTH );
  GDBG_INFO((88,"grTexCalcMemRequired(%d,%d,%d,%d) => 0x%x(%d)\n",
                small_lod,large_lod,aspect,format,memrequired,memrequired));
  return memrequired;
} /* grTexCalcMemRequired */


/*---------------------------------------------------------------------------
** grTexDetailControl
*/
GR_DIENTRY(grTexDetailControl, void,
           ( GrChipID_t tmu, int lod_bias, FxU8 detail_scale,
            float detail_max ))
{
  FxU32 tDetail;
  FxU32 dmax    = ( FxU32 ) ( detail_max * _GlideRoot.pool.f255 );
  FxU32 dscale  = detail_scale;
  
  GR_BEGIN_NOFIFOCHECK("grTexDetailControl",88);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%g)\n",tmu,detail_scale,detail_max));
  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_F( myName, lod_bias < -32 || lod_bias > 31, "lod_bias out of range" );
  GR_CHECK_F( myName, detail_scale > 7, "detail_scale out of range" );
  GR_CHECK_F( myName, detail_max < 0.0 || detail_max > 1.0, "detail_max out of range" );

  tDetail  = ( ( lod_bias << SST_DETAIL_BIAS_SHIFT ) & SST_DETAIL_BIAS );
  tDetail |= ( ( dmax << SST_DETAIL_MAX_SHIFT ) & SST_DETAIL_MAX );
  tDetail |= ( ( dscale << SST_DETAIL_SCALE_SHIFT ) & SST_DETAIL_SCALE );
  
  /* MULTIPLAT */
  _grTexDetailControl( tmu, tDetail );
  GR_END();
} /* grTexDetailControl */

GR_DIENTRY(grTexMinAddress, FxU32, ( GrChipID_t tmu ))
{
  GR_BEGIN_NOFIFOCHECK("grTexMinAddress",88);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",tmu));
  GR_CHECK_TMU(myName,tmu);
  FXUNUSED( tmu );
  return 0;
} /* grTexMinAddress */


/*-------------------------------------------------------------------
  Function: grTexMaxAddress
  Date: 6/2
  Implementor(s): GaryT
  Library: glide
  Description:
    Returns address of maximum extent of texture ram for a given TMU
  Arguments:
    tmu
  Return:
    the largest valid texture start Address
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexMaxAddress, FxU32, ( GrChipID_t tmu ))
{
  GR_BEGIN_NOFIFOCHECK("grTexMaxAddress",88);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",tmu));
  GR_CHECK_TMU( myName, tmu );
  return (gc->tmu_state[tmu].total_mem-8);
} /* grTexMaxAddress */


/*-------------------------------------------------------------------
  Function: grTexTextureMemRequired
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Returns the tmu memory required to store the specified mipmap    
    ( Gary and I don't like the name of this function, but are 
      a little backed into a corner because of the existence 
      of grTexMemRequired() which does not imply any distinction
      between texture memory and system ram )
  Arguments:
    evenOdd  - which set of mipmap levels are to be stored
                One of:
                  GR_MIPMAPLEVELMASK_EVEN 
                  GR_MIPMAPLEVELMASK_ODD
                  GR_MIPMAPLEVELMASK_BOTH
    info      - pointer to GrTexInfo structure defining dimensions
                of texture
  Return:
    offset to be added to current texture base address to calculate next 
    valid texture memory download location
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexTextureMemRequired, FxU32,
           ( FxU32 evenOdd, GrTexInfo *info))
{
  FxU32 memrequired;

  GR_CHECK_F( "grTexTextureMemRequired", !info, "invalid info pointer" );
  memrequired = _grTexTextureMemRequired( info->smallLod, info->largeLod, 
                                          info->aspectRatio, info->format,
                                          evenOdd );
                        
  GDBG_INFO((88,"grTexTextureMemRequired(%d,0x%x) => 0x%x(%d)\n",
                evenOdd,info,memrequired,memrequired));
  return memrequired;
} /* grTexTextureMemRequired */


/*-------------------------------------------------------------------
  Function: grTexDownloadMipMap
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Downloads a texture mipmap to the specified tmu at the specified
    base address.
  Arguments:
    tmu          - which tmu
    startAddress - starting address for texture download,
    evenOdd  - which set of mipmap levels have been downloaded for
                the selected texture
                One of:
                  GR_MIPMAPLEVELMASK_EVEN
                  GR_MIPMAPLEVELMASK_ODD
                  GR_MIPMAPLEVELMASK_BOTH
    info       - pointer to GrTexInfo structure defining dimension of
                 texture to be downloaded and containing texture data
  Return:
    none
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexDownloadMipMap, void, 
           ( GrChipID_t tmu, FxU32 startAddress, FxU32
            evenOdd, GrTexInfo  *info ))
{
  GR_DCL_GC;
  GrLOD_t lod;
  char *src_base;
  FxU32 size = grTexTextureMemRequired( evenOdd, info );

  FXUNUSED(gc);

  GDBG_INFO((89,"grTexDownloadMipMap(%d,0x%x,%d,0x%x\n",tmu,startAddress,evenOdd,info));
  GR_CHECK_TMU( "grTexDownloadMipMap", tmu );
  GR_CHECK_F( "grTexDownloadMipMap", startAddress + size > gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress" );
  GR_CHECK_F( "grTexDownloadMipMap", evenOdd > 0x3, "evenOdd mask invalid" );
  GR_CHECK_F( "grTexDownloadMipMap", !info, "info invalid" );
  
  if ((startAddress < 0x200000) && (startAddress + size > 0x200000))
      GrErrorCallback("grTexDownloadMipMap: mipmap "
                      " cannot span 2 Mbyte boundary",FXTRUE);

  src_base = (char *)info->data;
  
  /*---------------------------------------------------------------
    Download one mipmap level at a time
    ---------------------------------------------------------------*/
  for( lod = info->largeLod; lod <= info->smallLod; lod++ ) {
    grTexDownloadMipMapLevel( tmu, 
                              startAddress, 
                              lod,
                              info->largeLod,
                              info->aspectRatio,
                              info->format,
                              evenOdd,
                              src_base );
               
    src_base += _grMipMapHostSize[_gr_aspect_index_table[info->aspectRatio]][lod]
                    << (info->format>=GR_TEXFMT_16BIT);
  }
} /* grTexDownloadMipMap */


/*-------------------------------------------------------------------
  Function: grTexDownloadTablePartial
  Date: 6/3
  Implementor(s): GaryT
  Library: glide
  Description:
    download part of a look up table data to a tmu
  Arguments:
    tmu - which tmu
    type - what type of table to download
        One of:
            GR_TEXTABLE_NCC0
            GR_TEXTABLE_NCC1
            GR_TEXTABLE_PALETTE
    void *data - pointer to table data
  Return:
    none
  -------------------------------------------------------------------*/
GR_DIENTRY(grTexDownloadTablePartial, void,
           ( GrChipID_t tmu, GrTexTable_t type, 
            void *data, int start, int end ))
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadTablePartial",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,0x%x, %d,%d)\n",tmu,type,data,start,end));
  GR_CHECK_TMU(myName,tmu);
  GR_CHECK_F(myName, type > 0x2, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");
#if (GLIDE_PLATFORM & GLIDE_HW_SST1)
  GR_CHECK_F(myName, _GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[0].tmuRev < 1,
               "Texelfx rev 0 does not support paletted textures");
#endif

  if ( type == GR_TEXTABLE_PALETTE )     /* Need Palette Download Code */
    _grTexDownloadPalette( tmu, (GuTexPalette *)data, start, end );
  else {                                 /* Type is an ncc table */
    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, start, end );
  }
  GR_END();
} /* grTexDownloadTable */

/*---------------------------------------------------------------------------
** grTexDownloadMipMapLevel
*/
GR_DIENTRY(grTexDownloadMipMapLevel, void, 
           ( GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod,
            GrLOD_t largeLod, GrAspectRatio_t aspectRatio,
            GrTextureFormat_t format, FxU32 evenOdd, void *data )) 
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadMipMapLevel",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x)\n",
                  tmu,startAddress,thisLod,largeLod,aspectRatio,
                  format,evenOdd,data));
  grTexDownloadMipMapLevelPartial( tmu, startAddress,
                                   thisLod, largeLod, 
                                   aspectRatio, format,
                                   evenOdd, data,
                                   0, _grMipMapHostWH[aspectRatio][thisLod][1]-1 );
  GR_END();
} /* grTexDownloadMipmapLevel */

FxU16 rle_line[256];
FxU16 *rle_line_end;

#ifndef  __WATCOMC__
void rle_decode_line_asm(FxU16 *tlut,FxU8 *src,FxU16 *dest)
{
   /* don't do anything just shut up the compiler */
}
#endif


// from gtexdl.c

/* externals from gtex.c */
extern FxU32 _gr_aspect_xlate_table[];
extern FxU32 _gr_evenOdd_xlate_table[];
extern const int _grMipMapHostWH[GR_ASPECT_1x8+1][GR_LOD_1+1][2];

/*---------------------------------------------------------------------------
** _grTexDownloadNccTable
**
** Downloads an ncctable to the specified _physical_ TMU(s).  This
** function is called internally by Glide and should not be executed
** by an application.
*/
GR_DDFUNC(_grTexDownloadNccTable, void, ( GrChipID_t tmu, FxU32 which, const GuNccTable *table, int start, int end ))
{
  int i;
  // FxU32 *hwNCC;
  
  GR_BEGIN_NOFIFOCHECK("_grTexDownloadNccTable",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d, 0x%x, %d,%d)\n",tmu,which,table,start,end));
  GR_ASSERT( start==0 );
  GR_ASSERT( end==11 );

  /* check for null pointer */
  if ( table == 0 )
    return;
  _GlideRoot.stats.palDownloads++;
  _GlideRoot.stats.palBytes += (end-start+1)<<2;

  if (gc->tmu_state[tmu].ncc_table[which] != table ) {
    GR_SET_EXPECTED_SIZE(48+2*PACKER_WORKAROUND_SIZE);
    PACKER_WORKAROUND;
    // hw = SST_TMU(hw,tmu);
    // hwNCC = which == 0 ? hw->nccTable0 : hw->nccTable1;
    FxU32 hwNCC = which == 0 ? SST_TMU(tmu) + OFFSET_nccTable0 : SST_TMU(tmu) + OFFSET_nccTable1;

    for ( i = 0; i < 12; i++ )
      GR_SET(hwNCC+i, table->packed_data[i] );

    gc->tmu_state[tmu].ncc_table[which] = table;
    PACKER_WORKAROUND;
    P6FENCE;
    GR_CHECK_SIZE();
  }
  GR_END();
} /* _grTexDownloadNccTable */

/*-------------------------------------------------------------------
  Function: grTexDownloadTable
  Date: 6/3
  Implementor(s): jdt, GaryMcT
  Library: glide
  Description:
    download look up table data to a tmu
  Arguments:
    tmu - which tmu
    type - what type of table to download
        One of:
            GR_TEXTABLE_NCC0
            GR_TEXTABLE_NCC1
            GR_TEXTABLE_PALETTE
    void *data - pointer to table data
  Return:
    none
  -------------------------------------------------------------------*/
GR_ENTRY(grTexDownloadTable, void,
         ( GrChipID_t tmu, GrTexTable_t type,  void *data ))
{
  GR_BEGIN_NOFIFOCHECK("grTexDownloadTable",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,0x%x)\n",tmu,type,data));
  GR_CHECK_TMU(myName,tmu);
  GR_CHECK_F(myName, type > 0x2, "invalid table specified");
  GR_CHECK_F(myName, !data, "invalid data pointer");
#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  GR_CHECK_F(myName, _GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[0].tmuRev < 1,
               "Texelfx rev 0 does not support paletted textures");
#endif

  if ( type == GR_TEXTABLE_PALETTE )     /* Need Palette Download Code */
    _grTexDownloadPalette( tmu, (GuTexPalette *)data, 0, 255 );
  else {                                 /* Type is an ncc table */
    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, 0, 11 );
    /*    _grTexDownloadNccTable( tmu, type, (GuNccTable*)data, 0, 11 ); */
  }
  GR_END();
} /* grTexDownloadTable */


/*-------------------------------------------------------------------
  Function: grTexDownloadMipMapLevelPartial
  Date: 6/2
  Implementor(s): GaryMcT, Jdt
  Library: glide
  Description:
    Downloads a mipmap level to the specified tmu at the given
    texture start address
  Arguments:
    tmu           - which tmu
    startAddress - starting address for texture download,
                     this should be some value between grTexMinAddress()
                     and grTexMaxAddress()
    thisLod      - lod constant that describes the mipmap level
                    to be downloaded
    largeLod     - largest level of detail in complete mipmap to 
                   be downloaded at startAddress of which level to
                   be downloaded is a part
    aspectRatio  - aspect ratio of this mipmap
    format        - format of mipmap image data
    evenOdd      - which set of mipmap levels have been downloaded for
                    the selected texture
                    One of:
                      GR_MIPMAPLEVELMASK_EVEN 
                      GR_MIPMAPLEVELMASK_ODD
                      GR_MIPMAPLEVELMASK_BOTH
    data          - pointer to mipmap data
  Return:
    none
  -------------------------------------------------------------------*/

void grTexDownloadMipMapLevelPartial( GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t   aspectRatio, 
                                    GrTextureFormat_t format, FxU32 evenOdd, void *data, int t, int max_t )
{
  const FxU8  *src8  = ( const FxU8  * ) data; 
  const FxU16 *src16 = ( const FxU16 * ) data;
  FxI32   sh, bytesPerTexel;
  FxU32 max_s, s, width, tex_address, tmu_baseaddress;
  FxU32 tLod, texMode, baseAddress,size;

  GR_BEGIN_NOFIFOCHECK("grTexDownloadMipMapLevelPartial",89);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d,%d)\n",
                  tmu,startAddress,thisLod,largeLod,aspectRatio,
                  format,evenOdd,data,t,max_t));
  size = _grTexTextureMemRequired(thisLod, thisLod, aspectRatio, format, evenOdd);
  GR_CHECK_TMU(myName, tmu);
  GR_CHECK_F(myName, startAddress + size > gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress" );
  GR_CHECK_F(myName, startAddress & 0x7, "unaligned startAddress");
  GR_CHECK_F(myName, thisLod > GR_LOD_1, "thisLod invalid");
  GR_CHECK_F(myName, largeLod > GR_LOD_1, "largeLod invalid");
  GR_CHECK_F(myName, thisLod < largeLod, "thisLod may not be larger than largeLod");
  GR_CHECK_F(myName, aspectRatio > GR_ASPECT_1x8 || aspectRatio < GR_ASPECT_8x1, "aspectRatio invalid");
  GR_CHECK_F(myName, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F(myName, !data, "invalid data pointer");
  GR_CHECK_F(myName, max_t >= _grMipMapHostWH[aspectRatio][thisLod][1], "invalid end row");

  if ((startAddress < 0x200000) && (startAddress + size > 0x200000))
    GrErrorCallback("grTexDownloadMipMapLevelPartial: mipmap level cannot span 2 Mbyte boundary",FXTRUE);

  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if ( !(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
      goto all_done;

  /*------------------------------------------------------------
    Determine max_s
    ------------------------------------------------------------*/
  width = _grMipMapHostWH[aspectRatio][thisLod][0];
  if ( format < GR_TEXFMT_16BIT ) { /* 8-bit texture */
    bytesPerTexel = 1;
    max_s = width >> 2;
    if (max_s < 1)
      max_s = 1;
  } else { /* 16-bit texture */
    bytesPerTexel = 2;
    max_s = width >> 1;
    if (max_s < 1)
      max_s = 1;
  }
  /* assume max_s is a power of two */
  GR_ASSERT(( (max_s) & (max_s -1) ) == 0);

  
  /*------------------------------------------------------------
    Compute Base Address Given Start Address Offset
    ------------------------------------------------------------*/
  baseAddress = _grTexCalcBaseAddress( startAddress,
                                       largeLod, 
                                       aspectRatio,
                                       format,
                                       evenOdd );
  baseAddress >>= 3;

  /*------------------------------------------------------------
    Compute Physical Write Pointer
    ------------------------------------------------------------*/
  tmu_baseaddress = (FxU32)gc->tex_ptr;
  tmu_baseaddress += (((FxU32)tmu)<<21) + (((FxU32)thisLod)<<17);
  
  /*------------------------------------------------------------
    Compute pertinant contents of tLOD and texMode registers 
    ------------------------------------------------------------*/
  tLod = SST_TLOD_MINMAX_INT(largeLod,GR_LOD_1);
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[aspectRatio];
  texMode = format << SST_TFORMAT_SHIFT;
  if (gc->state.tmu_config[tmu].textureMode & SST_SEQ_8_DOWNLD) {
    sh = 2;
    texMode |= SST_SEQ_8_DOWNLD;
  }
  else sh = 3;

  /* account for 3 register writes and for smallest 1xN and 2xN levels */
  /* and also 4xN level for 8-bit textures (or 4x32x8bpp) */
  /* Also note that each texture write requires 10 actual fifo entry bytes */
  /* but since we are counting bytes/2 we multiply by 5 */
  GR_SET_EXPECTED_SIZE(3*4 + 2*PACKER_WORKAROUND_SIZE + 32*5);

  /*------------------------------------------------------------
    Update TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  PACKER_WORKAROUND;

  /* Flush the write buffers before the texture downloads */
  P6FENCE;
  _GlideRoot.stats.texBytes += max_s * (max_t-t+1) * 4;

// # define SET_TRAM(a,b) GR_SET( (*((FxU32 *)(a))) , (b) )
// nand2mario: Write to TMU RAM, byte address a starts at 8MB
# define SET_TRAM(a,b) voodoo_w((a)>>2, (b), 0xffffffff)
  /*------------------------------------------------------------
    Handle 8-bit Textures
    ------------------------------------------------------------*/
  if ( format < GR_TEXFMT_16BIT ) { /* 8 bit textures */
    switch( width ) {
    /* Cases 1, 2 and 4 don't need inner loops for s */
    case 1:                         /* 1xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU8*) src8);
        src8 += 1;
        tex_address += (1 << 9); 
      }
      break;

    case 2:                         /* 2xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU16*) src8);
        src8 += 2;
        tex_address += (1 << 9); 
      }
      break;

    case 4:                         /* 4xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU32*) src8);
        src8 += 4;
        tex_address += (1 << 9); 
      }
      break;

    default:                        /* >4xN texture */
      if (sh == 3) {                /* Old TMUs */
        /* Inner loop unrolled to process 2 dwords per iteration */
        for ( ; t <= max_t; t++) {
          GR_CHECK_SIZE_SLOPPY();
          GR_SET_EXPECTED_SIZE(max_s*5);
          tex_address = tmu_baseaddress + ( t << 9 );
          for ( s = 0; s < max_s; s+=2) {
            FxU32  t0, t1;

            t0 = * (const FxU32 *) (src8    );
            t1 = * (const FxU32 *) (src8 + 4);
            SET_TRAM( tex_address    , t0);
            SET_TRAM( tex_address + 8, t1);
            tex_address += 16;
            src8 += 8;
          }
        } 
      } else {                      /* New TMUs    */
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)

#define DW_PER_GWP 32
#define W_PER_GWP (DW_PER_GWP << 1)
#define BYTES_PER_GWP (DW_PER_GWP << 2)
#define MASK(n) ((1 << (n)) - 1)

      for ( ; t <= max_t; t++ ) {
        FxU32 t0, t1;
        FxU32 j;

        GR_CHECK_SIZE_SLOPPY();
        GR_SET_EXPECTED_SIZE((max_s + (max_s >> 4) + 2) << 2);
        tex_address = tmu_baseaddress + ( t << 9 );
        if (max_s >= DW_PER_GWP) {              /* can use maximum GWP(s) */
          for (s=0; s<max_s; s+=DW_PER_GWP ) {

            GWH_BEGIN_TEXDL_PACKET(0xffffffff, tex_address);
            /* Loop unrolled to keep GWP happy */
            for (j=0; j<16; j++) {
              t0 = * (const FxU32 *) (src8 + (j << 3) );
              t1 = * (const FxU32 *) (src8 + (j << 3) + 4);
              GR_SET_GW(t0);
              GR_SET_GW(t1);
            }
            tex_address += BYTES_PER_GWP;
            src8 += BYTES_PER_GWP;
          } /* end for s */
        } else {                        /* partial GWP */
          FxU32 mask = MASK(max_s);     /* we can assume s is even */
          GWH_BEGIN_TEXDL_PACKET(mask, tex_address);
          /* Loop unrolled to keep GWP happy */
          for (j=0; j<(max_s>>1); j++) {
            t0 = * (const FxU32 *) (src8 + (j << 3) );
            t1 = * (const FxU32 *) (src8 + (j << 3) + 4);
            GR_SET_GW(t0);
            GR_SET_GW(t1);
          }
          tex_address += (max_s << 2);
          src8 += (max_s << 2);
        }
      } /* end for t */

#else   /* SST-1 */
        for ( ; t <= max_t; t++) {
          GR_CHECK_SIZE_SLOPPY();
          GR_SET_EXPECTED_SIZE(max_s*5);
          tex_address = tmu_baseaddress + ( t << 9 );
          for ( s = 0; s < max_s; s+=2) {
            FxU32  t0, t1;

            t0 = * (const FxU32 *) (src8    );
            t1 = * (const FxU32 *) (src8 + 4);
            SET_TRAM( tex_address    , t0);
            SET_TRAM( tex_address + 4, t1);
            tex_address += 8;
            src8 += 8;
          }
        } 
#endif
      }
      break;
    }
  } else { 

    /*------------------------------------------------------------
      Handle 16-bit Textures
      ------------------------------------------------------------*/

    switch( width ) {
    /* Cases 1, 2 don't need inner loops for s */
    case 1:                         /* 1xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *src16 );
        src16 += 1;
        tex_address += (1 << 9); 
      }
      break;

    case 2:                         /* 2xN texture */
      tex_address = tmu_baseaddress + ( t << 9 );
      for ( ; t <= max_t; t++) {
        SET_TRAM( tex_address, *(const FxU32 *)src16 );
        src16 += 2;
        tex_address += (1 << 9); 
      }
      break;

    default:                        /* All other textures */
#if (GLIDE_PLATFORM & GLIDE_HW_SST96)
      for ( ; t <= max_t; t++ ) {
        FxU32 t0, t1;
        FxU32 j;

        GR_CHECK_SIZE_SLOPPY();
        GR_SET_EXPECTED_SIZE((max_s + (max_s >> 4) + 2) << 2);
        tex_address = tmu_baseaddress + ( t << 9 );
        if (max_s >= DW_PER_GWP) {              /* can use maximum GWP(s) */
          for (s=0; s<max_s; s+=DW_PER_GWP ) {

            GWH_BEGIN_TEXDL_PACKET(0xffffffff, tex_address);
            /* Loop unrolled to keep GWP happy */
            for (j=0; j<16; j++) {
              t0 = * (const FxU32 *) (src16 + (j << 2) );
              t1 = * (const FxU32 *) (src16 + (j << 2) + 2);
              GR_SET_GW(t0);
              GR_SET_GW(t1);
            }
            tex_address += BYTES_PER_GWP;
            src16 += W_PER_GWP;
          } /* end for s */
        } else {                        /* partial GWP */
          FxU32 mask = MASK(max_s);     /* we can assume s is even */
          GWH_BEGIN_TEXDL_PACKET(mask, tex_address);
          /* Loop unrolled to keep GWP happy */
          for (j=0; j<(max_s>>1); j++) {
            t0 = * (const FxU32 *) (src16 + (j << 2) );
            t1 = * (const FxU32 *) (src16 + (j << 2) + 2);
            GR_SET_GW(t0);
            GR_SET_GW(t1);
          }
          tex_address += (max_s << 2);
          src16 += (max_s << 1);
        }
      } /* end for t */
#else   /* SST-1 */
      for ( ; t <= max_t; t++ ) {
        GR_CHECK_SIZE_SLOPPY();
        GR_SET_EXPECTED_SIZE(max_s*5);
        tex_address = tmu_baseaddress + ( t << 9 );
        /* Loop unrolled to process 2 dwords per iteration */
        for ( s = 0; s < max_s; s += 2 ) {
          FxU32  t0, t1;

          t0 = * (const FxU32 *) (src16    );
          t1 = * (const FxU32 *) (src16 + 2);
          SET_TRAM( tex_address    , t0);
          SET_TRAM( tex_address + 4, t1);
          tex_address += 8;
          src16 += 4;
        }
      }
#endif
      break;
    }
  } /* end switch( width ) */
  
  /* Flush the write buffers after the texture downloads */
  P6FENCE;

  /*------------------------------------------------------------
    Restore TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  GR_CHECK_SIZE_SLOPPY();
  GR_SET_EXPECTED_SIZE(3*4 + 2*PACKER_WORKAROUND_SIZE);
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , gc->state.tmu_config[tmu].texBaseAddr );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , gc->state.tmu_config[tmu].textureMode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD        , gc->state.tmu_config[tmu].tLOD );
  PACKER_WORKAROUND;

all_done:
  _GlideRoot.stats.texDownloads++;
  GR_END_SLOPPY();
} /* grTexDownloadMipmapLevelPartial */


/*-------------------------------------------------------------------
  Function: _grTexDownloadPalette
  Date: 6/9
  Implementor(s): jdt
  Library: Glide
  Description:
    Private function to download a palette to the specified tmu
  Arguments:
    tmu - which tmu to download the palette to
    pal - the pallete data
    start - beginning index to download
    end   - ending index to download
  Return:
    none
  -------------------------------------------------------------------*/
GR_DDFUNC(_grTexDownloadPalette, void, ( GrChipID_t tmu, GuTexPalette *pal, int start, int end ))
{
  GR_BEGIN("_grTexDownloadPalette",89, 4*(end-start+1) + 2*PACKER_WORKAROUND_SIZE);
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x, %d,%d)\n",tmu,pal,start,end));
  GR_CHECK_F( myName, !pal, "pal invalid" );
  GR_CHECK_F( myName, start<0, "invalid start index" );
  GR_CHECK_F( myName, end>255, "invalid end index" );

  PACKER_WORKAROUND;
  // hw = SST_TMU(hw,tmu);
  _GlideRoot.stats.palDownloads++;
  _GlideRoot.stats.palBytes += (end-start+1)<<2;

  while (start <= end) {
      GR_SET( SST_TMU(tmu) + OFFSET_nccTable0 + (4+(start&0x7)),
              0x80000000 |  ((start & 0xFE) << 23) | (pal->data[start] & 0xffffff) );
      start++;
      if ((start&0x7)==0) P6FENCE;
  }

  PACKER_WORKAROUND;
  P6FENCE;
  GR_END();
} /* _grTexDownloadPalette */ 


#ifndef __linux__
/* 
   Let me take this opportunity to register my formal opposition to
   this function.  Either we do this or we don't.  Let's not hack like
   this.

   CHD
*/

GR_ENTRY(ConvertAndDownloadRle, void, ( GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 evenOdd, FxU8 *bm_data, long bm_h, FxU32 u0, FxU32 v0, FxU32 width, FxU32 height, FxU32 dest_width, FxU32 dest_height, FxU16 *tlut))
{
  FxI32 sh;
  FxU32 max_s,s,t,max_t,tex_address, tmu_baseaddress;
  FxU32 tLod, texMode, baseAddress,size;
  FxU32 offset,expected_size;
  unsigned long  i;
  FxU16 *src;
  extern FxU16 rle_line[256];
  extern FxU16 *rle_line_end;


  GR_BEGIN_NOFIFOCHECK("grTexDownloadMipMapLevelPartial",89);

/* make sure even number */
  width&=0xFFFFFFFE;
  
  max_s=width>>1;
  max_t=height;
  
  GDBG_INFO_MORE((gc->myLevel,"(%d,0x%x, %d,%d,%d, %d,%d 0x%x, %d)\n",
                  tmu,startAddress,thisLod,largeLod,aspectRatio,
                  format,evenOdd,bm_data,max_t));

  size = _grTexTextureMemRequired(thisLod, thisLod, aspectRatio, format, evenOdd);
  GR_CHECK_TMU(myName, tmu);
  GR_CHECK_F(myName, startAddress + size > gc->tmu_state[tmu].total_mem,
               "insufficient texture ram at startAddress" );
  GR_CHECK_F(myName, startAddress & 0x7, "unaligned startAddress");
  GR_CHECK_F(myName, thisLod > GR_LOD_1, "thisLod invalid");
  GR_CHECK_F(myName, largeLod > GR_LOD_1, "largeLod invalid");
  GR_CHECK_F(myName, thisLod < largeLod, "thisLod may not be larger than largeLod");
  GR_CHECK_F(myName, aspectRatio > GR_ASPECT_1x8 || aspectRatio < GR_ASPECT_8x1, "aspectRatio invalid");
  GR_CHECK_F(myName, evenOdd > 0x3 || evenOdd == 0, "evenOdd mask invalid");
  GR_CHECK_F(myName, !bm_data, "invalid data pointer");

  GR_CHECK_F(myName, (dest_height-1) >= (FxU32)_grMipMapHostWH[aspectRatio][thisLod][1], "invalid end row");

  if ((startAddress < 0x200000) && (startAddress + size > 0x200000))
    GrErrorCallback("grTexDownloadMipMapLevelPartial: mipmap level cannot span 2 Mbyte boundary",FXTRUE);

  /*------------------------------------------------------------
    Skip this level entirely if not in odd/even mask
    ------------------------------------------------------------*/
  if ( !(evenOdd & (thisLod & 0x1 ? GR_MIPMAPLEVELMASK_ODD:GR_MIPMAPLEVELMASK_EVEN)))
      goto all_done;
  
  /*------------------------------------------------------------
    Compute Base Address Given Start Address Offset
    ------------------------------------------------------------*/
  baseAddress = _grTexCalcBaseAddress( startAddress,
                                       largeLod, 
                                       aspectRatio,
                                       format,
                                       evenOdd );
  baseAddress >>= 3;

  /*------------------------------------------------------------
    Compute Physical Write Pointer
    ------------------------------------------------------------*/
  tmu_baseaddress = (FxU32)gc->tex_ptr;
  tmu_baseaddress += (((FxU32)tmu)<<21) + (((FxU32)thisLod)<<17);
  
  /*------------------------------------------------------------
    Compute pertinant contents of tLOD and texMode registers 
    ------------------------------------------------------------*/
  tLod = SST_TLOD_MINMAX_INT(largeLod,GR_LOD_1);
  tLod |= _gr_evenOdd_xlate_table[evenOdd];
  tLod |= _gr_aspect_xlate_table[aspectRatio];
  texMode = format << SST_TFORMAT_SHIFT;
  if (gc->state.tmu_config[tmu].textureMode & SST_SEQ_8_DOWNLD) {
    sh = 2;
    texMode |= SST_SEQ_8_DOWNLD;
  }
  else sh = 3;

  /* account for 6 register writes and for smallest 1xN and 2xN levels*/
  GR_SET_EXPECTED_SIZE(3*4 + 2*PACKER_WORKAROUND_SIZE);

  /*------------------------------------------------------------
    Update TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  PACKER_WORKAROUND;

  /* Flush the write buffers before the texture downloads */
  P6FENCE;
  _GlideRoot.stats.texBytes += dest_width * (dest_height) * 2;

  /* here I can do my writes and conversion and I will be so happy */

        offset=4+bm_h;
   for (i=0; i<v0; i++ )
                offset += bm_data[4+i];

   max_s=dest_width>>1;
   expected_size=max_s*5;

   rle_line_end=rle_line+width+u0;

   for(t=0;t<max_t;t++)
   {
      GR_CHECK_SIZE_SLOPPY();
      GR_SET_EXPECTED_SIZE(expected_size);
      tex_address = tmu_baseaddress + ( t << 9 );
/*      decode_rle_line(&bm_data[offset]); */
      rle_decode_line_asm(tlut,&bm_data[offset],rle_line);
      src=rle_line+u0;
      for(s=0;s<max_s;s++)
      {
         SET_TRAM( tex_address + ( s << 2 ), *( FxU32 * ) src );
         src+=2;
      }
      offset+=bm_data[4+i++];
   }
   if (dest_height>height)
   {
      GR_CHECK_SIZE_SLOPPY();
      GR_SET_EXPECTED_SIZE(expected_size);
      tex_address = tmu_baseaddress + ( t << 9 );
      src=rle_line+u0;
      for(s=0;s<max_s;s++)
      {
         SET_TRAM( tex_address + ( s << 2 ), *( FxU32 * ) src );
         src+=2;
      }
   }

  /* Flush the write buffers after the texture downloads */
  P6FENCE;

  /*------------------------------------------------------------
    Restore TLOD, texMode, baseAddress
    ------------------------------------------------------------*/
  GR_CHECK_SIZE_SLOPPY();
  GR_SET_EXPECTED_SIZE(3*4 + 2*PACKER_WORKAROUND_SIZE);
  PACKER_WORKAROUND;
  GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , gc->state.tmu_config[tmu].texBaseAddr );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , gc->state.tmu_config[tmu].textureMode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD        , gc->state.tmu_config[tmu].tLOD );
  PACKER_WORKAROUND;

all_done:
  _GlideRoot.stats.texDownloads++;
  GR_END_SLOPPY();
}
#endif


// from digutex.c

/* externals from ditex.c */
extern FxU32 _grMipMapHostSize[][16];
extern FxU32 _gr_aspect_index_table[];
extern FxU32 _gr_aspect_xlate_table[];
extern FxU32 _gr_evenOdd_xlate_table[];


/*---------------------------------------------------------------------------
** guTexAllocateMemory
*/
GR_DIENTRY(guTexAllocateMemory, GrMipMapId_t, ( GrChipID_t tmu,
                    FxU8 odd_even_mask,
                    int width, int height,
                    GrTextureFormat_t format,
                    GrMipMapMode_t mipmap_mode,
                    GrLOD_t small_lod, GrLOD_t large_lod,
                    GrAspectRatio_t aspect_ratio,
                    GrTextureClampMode_t s_clamp_mode,
                    GrTextureClampMode_t t_clamp_mode,
                    GrTextureFilterMode_t minfilter_mode,
                    GrTextureFilterMode_t magfilter_mode,
                    float lod_bias,
                    FxBool trilinear
                    ))
{
  FxU32
    memrequired,
    memavail,
    baseAddress,
    tLod,
    texturemode,
    filterMode,                 /* filter mode bits */
    clampMode;                  /* clamp mode bits */

  GrMipMapId_t
    mmid = (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
  int
    int_lod_bias;
  GrTexInfo info;

  GR_BEGIN_NOFIFOCHECK("guTexAllocateMemory",99);
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d, %d,%d, %d,%d, %d,%d,%d, %d,%d, %d,%d)\n",
                tmu,odd_even_mask,width,height,format,mipmap_mode,
                small_lod,large_lod,aspect_ratio,
                s_clamp_mode,t_clamp_mode, minfilter_mode,magfilter_mode));
  /*
   ** The constants are actually reverse of each other so the following
   ** test IS valid!
   */
  GR_CHECK_F(myName, small_lod < large_lod, "smallest_lod is larger than large_lod");
  
  info.smallLod = small_lod;
  info.largeLod = large_lod;
  info.aspectRatio = aspect_ratio;
  info.format = format;
  memrequired = grTexTextureMemRequired(odd_even_mask, &info);

  /*
   ** Make sure to not cross 2 MByte texture boundry
   */
  if ((gc->tmu_state[tmu].freemem_base < 0x200000) &&
    (gc->tmu_state[tmu].freemem_base + memrequired > 0x200000))
    gc->tmu_state[tmu].freemem_base = 0x200000;
  
  /*
   ** If we have enough memory and a free mip map handle then go for it
   */
  memavail = guTexMemQueryAvail( tmu );
  
  if ( memavail < memrequired )
    return (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
  
  if (gc->mm_table.free_mmid >= MAX_MIPMAPS_PER_SST )
    return (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
  
  /*
   ** Allocate the mip map id
   */
  mmid = gc->mm_table.free_mmid++;

  /*
   ** calculate baseAddress (where LOD 0 would go)
   */
  baseAddress = _grTexCalcBaseAddress( gc->tmu_state[tmu].freemem_base, 
                                       large_lod, 
                                       aspect_ratio, 
                                       format, 
                                       odd_even_mask );

  GDBG_INFO((gc->myLevel,"  baseAddress = 0x%x (in bytes)\n",baseAddress));
  
  /*
   ** reduce available memory to reflect allocation
   */
  gc->tmu_state[tmu].freemem_base += memrequired;

  /*
   ** Create the tLOD register value for this mip map
   */
  int_lod_bias = _grTexFloatLODToFixedLOD( lod_bias );
  tLod = mipmap_mode==GR_MIPMAP_DISABLE ? large_lod : small_lod;
  tLod = SST_TLOD_MINMAX_INT(large_lod,tLod);
  tLod |= _gr_evenOdd_xlate_table[odd_even_mask];
  tLod |= _gr_aspect_xlate_table[aspect_ratio];
  tLod |= int_lod_bias << SST_LODBIAS_SHIFT;
  filterMode = (
                (minfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
                (magfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0)
                );
  
  clampMode = (
               (s_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPS : 0) |
               (t_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPT : 0)
               );  
  
  /*
   ** Create the tTextureMode register value for this mip map
   */
  texturemode  = ( format << SST_TFORMAT_SHIFT );
  texturemode |= SST_TCLAMPW;
  texturemode |= SST_TPERSP_ST;
  texturemode |= filterMode;
  texturemode |= clampMode;
  
  if ( mipmap_mode == GR_MIPMAP_NEAREST_DITHER )
    texturemode |= SST_TLODDITHER;
  
  if ( trilinear ) {
      texturemode |= SST_TRILINEAR;

      if ( odd_even_mask & GR_MIPMAPLEVELMASK_ODD )
        tLod |= SST_LOD_ODD;

      if ( odd_even_mask != GR_MIPMAPLEVELMASK_BOTH )
        tLod |= SST_LOD_TSPLIT;
  }
  
  /*
   ** Fill in the mm_table data for this mip map
   */
  gc->mm_table.data[mmid].format         = format;
  gc->mm_table.data[mmid].mipmap_mode    = mipmap_mode;
  gc->mm_table.data[mmid].magfilter_mode = magfilter_mode;
  gc->mm_table.data[mmid].minfilter_mode = minfilter_mode;
  gc->mm_table.data[mmid].s_clamp_mode   = s_clamp_mode;
  gc->mm_table.data[mmid].t_clamp_mode   = t_clamp_mode;
  gc->mm_table.data[mmid].tLOD           = tLod;
  gc->mm_table.data[mmid].tTextureMode   = texturemode;
  gc->mm_table.data[mmid].lod_bias       = int_lod_bias;
  gc->mm_table.data[mmid].lod_min        = small_lod;
  gc->mm_table.data[mmid].lod_max        = large_lod;
  gc->mm_table.data[mmid].tmu            = tmu;
  gc->mm_table.data[mmid].odd_even_mask  = odd_even_mask;
  gc->mm_table.data[mmid].tmu_base_address = baseAddress;
  gc->mm_table.data[mmid].trilinear      = trilinear;
  gc->mm_table.data[mmid].aspect_ratio   = aspect_ratio;
  gc->mm_table.data[mmid].data           = 0;
  /*   gc->mm_table.data[mmid].ncc_table      = 0; */
  gc->mm_table.data[mmid].sst            = _GlideRoot.current_sst;
  gc->mm_table.data[mmid].valid          = FXTRUE;
  gc->mm_table.data[mmid].width          = width;
  gc->mm_table.data[mmid].height         = height;
  
  return mmid;
} /* guTexAllocateMemory */

static void
_guTexRebuildRegisterShadows( GrMipMapId_t mmid )
{
  GR_DCL_GC;
  GrMipMapInfo *mminfo = &gc->mm_table.data[mmid];
  int texturemode      = 0;
  int tLod             = 0;
  FxU32 
    filterMode,                 /* filter mode bits of texturemode */
    clampMode;                  /* clamp mode bits of texturemode */

  /* build filterMode */
  filterMode = (
                (mminfo->minfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMINFILTER : 0) |
                (mminfo->magfilter_mode == GR_TEXTUREFILTER_BILINEAR ? SST_TMAGFILTER : 0)
                );
  clampMode = (
               (mminfo->s_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPS : 0) |
               (mminfo->t_clamp_mode == GR_TEXTURECLAMP_CLAMP ? SST_TCLAMPT : 0)
               );
   
  /*
   ** build up tTextureMode
   */
  texturemode |= ( mminfo->format << SST_TFORMAT_SHIFT );
  texturemode |= SST_TCLAMPW;
  texturemode |= SST_TPERSP_ST;
  texturemode |= filterMode;
  texturemode |= clampMode;

  if ( mminfo->mipmap_mode == GR_MIPMAP_NEAREST_DITHER )
    texturemode |= SST_TLODDITHER;

  if ( mminfo->trilinear )
    texturemode |= SST_TRILINEAR;

  /*
   ** build up tLOD
   */
  tLod = mminfo->mipmap_mode == GR_MIPMAP_DISABLE ? mminfo->lod_max : mminfo->lod_min;
  tLod = SST_TLOD_MINMAX_INT(mminfo->lod_max,tLod);
  tLod |= _gr_evenOdd_xlate_table[mminfo->odd_even_mask];
  tLod |= _gr_aspect_xlate_table[mminfo->aspect_ratio];
  tLod |= mminfo->lod_bias << SST_LODBIAS_SHIFT;

  /*
   ** assign them
   */
  mminfo->tTextureMode = texturemode;
  mminfo->tLOD         = tLod;
} /* guTexRebuildRegisterShadows */


/*---------------------------------------------------------------------------
**  guTexChangeAttributes 
*/
GR_DIENTRY(guTexChangeAttributes, FxBool, ( GrMipMapId_t mmid,
                      int width, int height,
                      GrTextureFormat_t fmt,
                      GrMipMapMode_t mm_mode,
                      GrLOD_t smallest_lod, GrLOD_t largest_lod,
                      GrAspectRatio_t aspect,
                      GrTextureClampMode_t s_clamp_mode,
                      GrTextureClampMode_t t_clamp_mode,
                      GrTextureFilterMode_t minFilterMode,
                      GrTextureFilterMode_t magFilterMode
                      ))
{
  GrMipMapInfo *mminfo;

  GR_BEGIN_NOFIFOCHECK("guTexChangeAttributes",88);
  GDBG_INFO_MORE((gc->myLevel,"(%d, %d,%d, %d,%d, %d,%d,%d, %d,%d, %d,%d)\n",
                mmid,width,height,fmt,mm_mode,
                smallest_lod,largest_lod,aspect,
                s_clamp_mode,t_clamp_mode, minFilterMode,magFilterMode));
  /*
  ** Make sure that mmid is not NULL
  */
  if ( mmid == GR_NULL_MIPMAP_HANDLE ) {
    return (FXFALSE);
  }

  mminfo = &gc->mm_table.data[mmid];

  /*
  ** Fill in the mm_table data for this mip map
  */
  if ( fmt != -1 )
     mminfo->format         = fmt;

  if ( mm_mode != -1 )
     mminfo->mipmap_mode    = mm_mode;

  if ( smallest_lod != -1 )
     mminfo->lod_min        = smallest_lod;
  if ( largest_lod != -1 )
     mminfo->lod_max        = largest_lod;
  if ( minFilterMode != -1 )
     mminfo->minfilter_mode = minFilterMode;
  if ( magFilterMode != -1 )
     mminfo->magfilter_mode = magFilterMode;
  if ( s_clamp_mode != -1 )
     mminfo->s_clamp_mode   = s_clamp_mode;
  if ( t_clamp_mode != -1 )
     mminfo->t_clamp_mode   = t_clamp_mode;
  if ( aspect != -1 )
     mminfo->aspect_ratio   = aspect;
  if ( width != -1 )
     mminfo->width          = width;
  if ( height != -1 )
     mminfo->height         = height;

  _guTexRebuildRegisterShadows( mmid );
  return FXTRUE;
} /* guTexChangeAttributes */

/*---------------------------------------------------------------------------
** grTexCombineFunction - obsolete
**                              
*/
GR_DIENTRY(grTexCombineFunction, void,
           (GrChipID_t tmu, GrTextureCombineFnc_t tc)) 
{
  guTexCombineFunction( tmu, tc );
}

/*---------------------------------------------------------------------------
** guTexCombineFunction
**                              
** Sets the texture combine function.  For a dual TMU system this function
** will configure the TEXTUREMODE registers as appropriate.  For a
** single TMU system this function will configure TEXTUREMODE if
** possible, or defer operations until grDrawTriangle() is called.
*/
GR_DIENTRY(guTexCombineFunction, void,
           (GrChipID_t tmu, GrTextureCombineFnc_t tc))
{
  GDBG_INFO((99,"guTexCombineFunction(%d,%d)\n",tmu,tc));
  switch ( tc )  {
  case GR_TEXTURECOMBINE_ZERO:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_DECAL:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_ONE:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
                  GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, FXTRUE, FXTRUE );
    break;

  case GR_TEXTURECOMBINE_ADD:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_ONE,
                  GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_MULTIPLY:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL,
                  GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_DETAIL:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_DETAIL_OTHER:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_DETAIL_FACTOR,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_DETAIL_FACTOR, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_TRILINEAR_ODD:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_TRILINEAR_EVEN:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_LOD_FRACTION,
                  GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_LOD_FRACTION, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_SUBTRACT:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL, GR_COMBINE_FACTOR_ONE,
                  GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL, GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE );
    break;

  case GR_TEXTURECOMBINE_OTHER:
    grTexCombine( tmu, GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE,
                  GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE );
    break;

  default:
    GrErrorCallback( "guTexCombineFunction:  Unsupported function", FXTRUE );
    break;
  }
} /* guTexCombineFunction */

/*---------------------------------------------------------------------------
** guTexDownloadMipMap
**
** Downloads a mip map (previously allocated with guTexAllocateMemory) to
** the hardware using the given data and ncctble.  The "data" is assumed
** to be in row major order from largest mip map to smallest mip map.
*/
GR_DIENTRY(guTexDownloadMipMap, void, 
           (GrMipMapId_t mmid, const void *src, const GuNccTable
            *ncc_table ) )
{
  GR_DCL_GC;
  GrLOD_t     lod;
  const void *ptr = src;

  GDBG_INFO((99,"guTexDownloadMipMap(%d,0x%x,0x%x)\n",mmid,src,ncc_table));
  GR_ASSERT(gc != NULL);
  GR_ASSERT(src != NULL);
  GR_CHECK_F("guTexDownloadMipMap",
              ( mmid == GR_NULL_MIPMAP_HANDLE ) || ( mmid >= gc->mm_table.free_mmid ),
              "invalid mip map handle passed");

#if 0 /* Fixme!!! XXX ??? */
  GR_CHECK_F("guTexDownloadMipMap",
              gc->mm_table.data[mmid].format == GR_TEXFMT_P_8,
              "guTex* does not support palletted textures - use grTex* instead");
#endif /* 0 */

  /*
  ** Bind data and ncc table to this mip map
  */
  gc->mm_table.data[mmid].data      = (void *) ptr;
  if (gc->mm_table.data[mmid].format == GR_TEXFMT_YIQ_422 ||
      gc->mm_table.data[mmid].format == GR_TEXFMT_AYIQ_8422) {
    gc->mm_table.data[mmid].ncc_table = *ncc_table;
  }

  /*
  ** Start downloading mip map levels, note that ptr is updated by the caller
  */
  for ( lod = gc->mm_table.data[mmid].lod_max; lod <= gc->mm_table.data[mmid].lod_min; lod++ ) {
     guTexDownloadMipMapLevel( mmid, lod, &ptr );
  }
} /* guTexDownloadMipMap */

/*---------------------------------------------------------------------------
** guTexDownloadMipMapLevel
**
** Downloads a single mip map level to a mip map.  "src" is considered to be
** row major data of the correct aspect ratio and format.
*/
GR_DIENTRY(guTexDownloadMipMapLevel, void,
           (GrMipMapId_t mmid, GrLOD_t lod,
            const void **src_base))
{
  FxU32 i;
  const  GrMipMapInfo *mminfo;
  GR_DCL_GC;
 
  GDBG_INFO((99,"guTexDownloadMipMapLevel(%d,%d,0x%x)\n",mmid,lod,src_base));
  GR_ASSERT(src_base != NULL);
  mminfo = &gc->mm_table.data[mmid];
  GR_CHECK_F( "guTexDownloadMipMapLevel",
              ( lod > mminfo->lod_min ) || ( lod < mminfo->lod_max ),
              "specified lod is out of range");

  /* GMT: replace with array access */
  /* download this level */
  i = _grTexCalcBaseAddress( 0,
                            mminfo->lod_max,
                            mminfo->aspect_ratio,
                            mminfo->format,
                            mminfo->odd_even_mask);
  grTexDownloadMipMapLevel( mminfo->tmu,
                            mminfo->tmu_base_address - i,
                            lod,
                            mminfo->lod_max,
                            mminfo->aspect_ratio,
                            mminfo->format,
                            mminfo->odd_even_mask,
                            (void *)*src_base );

  /* update src_base to point to next mipmap level */
  *src_base = (void *) (((FxU8 *)*src_base) +
               (_grMipMapHostSize[_gr_aspect_index_table[mminfo->aspect_ratio]][lod]
                    << (mminfo->format>=GR_TEXFMT_16BIT)));

} /* guTexDownloadMipmapLevel */

/*---------------------------------------------------------------------------
** guTexGetCurrentMipMap
*/
GR_DIENTRY(guTexGetCurrentMipMap, GrMipMapId_t, ( GrChipID_t tmu ))
{
  GR_BEGIN_NOFIFOCHECK("guTexGetCurrentMipMap",99);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",tmu));
  GR_CHECK_TMU( myName, tmu );

  return (gc->state.current_mm[tmu]);
} /* guTexGetCurrentMipMap */

/*---------------------------------------------------------------------------
** guTexGetMipMapInfo
*/
GR_DIENTRY(guTexGetMipMapInfo, GrMipMapInfo *, ( GrMipMapId_t mmid ))
{
  GR_BEGIN_NOFIFOCHECK("guTexGetMipMapInfo",99);
  GDBG_INFO_MORE((gc->myLevel,"(%d) => 0x%x\n",mmid,&gc->mm_table.data[mmid]));
  return &( gc->mm_table.data[mmid] );
} /* guTexGetMipMapInfo */

/*---------------------------------------------------------------------------
** guTexMemQueryAvail
**
** returns the amount of available texture memory on a specified TMU.
*/
GR_DIENTRY(guTexMemQueryAvail, FxU32, ( GrChipID_t tmu ))
{
  GR_BEGIN_NOFIFOCHECK("guTexMemQueryAvail",99);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",tmu));
  GR_CHECK_TMU( myName, tmu );
  return (gc->tmu_state[tmu].total_mem - gc->tmu_state[tmu].freemem_base);
} /* guTexQueryMemAvail */

/*---------------------------------------------------------------------------
** guTexMemReset
**
** Clears out texture buffer memory.
*/
GR_DIENTRY(guTexMemReset, void, ( void ))
{
  int i;
  
  GR_BEGIN_NOFIFOCHECK("guTexMemReset",99);
  GDBG_INFO_MORE((gc->myLevel,"()\n"));

  memset( gc->mm_table.data, 0, sizeof( gc->mm_table.data ) );
  gc->mm_table.free_mmid = 0;
  
  for ( i = 0; i < gc->num_tmu; i++ ) {
    gc->state.current_mm[i] = (GrMipMapId_t) GR_NULL_MIPMAP_HANDLE;
    gc->tmu_state[i].freemem_base = 0;
    gc->tmu_state[i].ncc_mmids[0] = 
      gc->tmu_state[i].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;    
    gc->tmu_state[i].ncc_table[0] = 
      gc->tmu_state[i].ncc_table[1] = 0;
  }
  GR_END();
} /* guTexMemReset */


// From gutex.c

/*---------------------------------------------------------------------------
** guTexSource
*/

GR_ENTRY(guTexSource, void, ( GrMipMapId_t mmid ))
{
  FxU32               texMode, tLod;
  FxU32               oldtexMode;
  FxU32               baseAddress;
  int                 tmu;
  const GrMipMapInfo *mminfo;

  GR_BEGIN_NOFIFOCHECK("guTexSource",99);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mmid));

  /*
  ** Make sure that mmid is not NULL
  */
  if ( mmid == GR_NULL_MIPMAP_HANDLE )
  {
    return;
  }

  /*
  ** get a pointer to the relevant GrMipMapInfo struct
  */
  mminfo = &gc->mm_table.data[mmid];
  tmu = mminfo->tmu;

  GR_CHECK_TMU( myName, tmu );
  GR_CHECK_W( myName, mmid == gc->state.current_mm[tmu], "setting same state twice" );

  gc->state.current_mm[tmu] = mmid;

  /*
  ** Set up new glide state for this mmid
  */
  gc->state.tmu_config[tmu].mmMode =  mminfo->mipmap_mode;
  gc->state.tmu_config[tmu].smallLod = mminfo->lod_min;
  gc->state.tmu_config[tmu].largeLod = mminfo->lod_max;
  gc->state.tmu_config[tmu].evenOdd =  mminfo->odd_even_mask;
  gc->state.tmu_config[tmu].nccTable = 0;

  /*
  ** Set up base address, texMode, and tLod registers
  */
  baseAddress = mminfo->tmu_base_address >> 3;
  texMode     = mminfo->tTextureMode;
  tLod        = mminfo->tLOD;

  oldtexMode = gc->state.tmu_config[tmu].textureMode;
  oldtexMode &= ~( SST_TFORMAT | SST_TCLAMPT | 
                   SST_TCLAMPS | SST_TNCCSELECT | 
                   SST_TLODDITHER | SST_TCLAMPW | 
                   SST_TMAGFILTER | SST_TMINFILTER | 
                   SST_TRILINEAR );
  texMode |= oldtexMode;
  if (!gc->state.allowLODdither)
    texMode &= ~SST_TLODDITHER;

 /* 
  **  Download the NCC table, if needed.  
  */
  if (
      (mminfo->format==GR_TEXFMT_YIQ_422) ||
      (mminfo->format==GR_TEXFMT_AYIQ_8422)
      )
  {
    int
      table;                    /* ncc table we'll use */
    /* See if it's already down there */
    if (gc->tmu_state[tmu].ncc_mmids[0] == mmid) {
      /* Table 0 has what we need, so make it current */
      table = 0;
    } else if (gc->tmu_state[tmu].ncc_mmids[1] == mmid) {
      /* Table 1 has what we need, so make it current */
      table = 1;
    } else {
      /*
      **  it's not down there, so we need to pick the table and
      **  download it
      */
      /* Which table should we use? */
      table = gc->tmu_state[tmu].next_ncc_table;
      /* Download NCC table */
      _grTexDownloadNccTable( tmu, table, &mminfo->ncc_table, 0, 11 );
      /* Set the mmid so we known it's down there */
      gc->tmu_state[tmu].ncc_mmids[table] = mmid;
      /* Set the state to know which table was the LRA */
      gc->tmu_state[tmu].next_ncc_table =
        (table == 0 ? 1 : 0);
    } /* we had to download it */
    /*
    **  Setting the TNCCSelect bit to 0 selects table 0, setting it to 1
    **  selects table 1
    */
    if (table == 0)
      texMode &= ~(SST_TNCCSELECT);
    else
      texMode |= SST_TNCCSELECT;
  } /* if it's an NCC texture */

  GR_SET_EXPECTED_SIZE(12+2*PACKER_WORKAROUND_SIZE);

  /* Write relevant registers out to hardware */

  PACKER_WORKAROUND;
  // hw = SST_TMU(hw,tmu);
  GR_SET( SST_TMU(tmu) + OFFSET_texBaseAddr , baseAddress );
  GR_SET( SST_TMU(tmu) + OFFSET_textureMode , texMode );
  GR_SET( SST_TMU(tmu) + OFFSET_tLOD , tLod );
  PACKER_WORKAROUND;

  /* update shadows */
  gc->state.tmu_config[tmu].texBaseAddr = baseAddress;
  gc->state.tmu_config[tmu].textureMode = texMode;
  gc->state.tmu_config[tmu].tLOD = tLod;
 
  GR_END();
} /* guTexSource */

