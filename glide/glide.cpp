/*
 *
 * Mock 3dfx Glide API for voodoo simulation.
 * 
 * nand2mario, 2025.4
 */

#define SST1INIT_VIDEO_ALLOCATE

#include <stdio.h>
#include <string.h>
#include "glide.h"
#include "fxglide.h"

#include "init.h"

/*
** Globals
*/
struct _GlideRoot_s _GlideRoot;

void (*GrErrorCallback)( const char *string, FxBool fatal );

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

/*
** API functions
*/

void grGlideInit() {
    _GlideInitEnvironment();
}

void grGlideShutdown() {
    // nothing
}

FxBool grSstQueryHardware(GrHwConfiguration *hwc) {
    FxBool retVal;
    /* init and copy the data back to the user's structure */
    retVal = _GlideRoot.hwConfig.num_sst > 0;
    *hwc = _GlideRoot.hwConfig;

    return retVal;
}

void grErrorSetCallback(void (*function) ( const char *string, FxBool fatal) ) {
    GrErrorCallback = function;
}

void grSstSelect(int which) {
    if ( which >= _GlideRoot.hwConfig.num_sst )
        GrErrorCallback( "grSstSelect:  non-existent SST", FXTRUE );

    _GlideRoot.current_sst = which;
    _GlideRoot.curGC = &_GlideRoot.GCs[which];

    /* now begin a normal Glide routine's flow */
    {
        GR_BEGIN_NOFIFOCHECK("grSstSelect",80);
        GDBG_INFO_MORE((gc->myLevel,"(%d)\n",which));

        _GlideRoot.packerFixAddress  = gc->tex_ptr;
        _GlideRoot.packerFixAddress += ( ( ( FxU32 ) 3 ) << 21 );
        _GlideRoot.packerFixAddress += ( ( ( FxU32 ) 1 ) << 17 );

        /* Now that we have selected a board, we can build the offests and register
        lists for the  optimized triangle setup code */
        _grRebuildDataList();

        // initDeviceSelect( which );

        GR_END();
    }
}

/*
  Initialize the selected SST
  1. Video Init 
  2. Command Transport Init (not needed)
  3. GC Init
  4. 3D State Init
*/
FxBool grSstWinOpen( 
    FxU32                   hWnd,
    GrScreenResolution_t    resolution,
    GrScreenRefresh_t       refresh,
    GrColorFormat_t         format,
    GrOriginLocation_t      origin,
    int                     nColBuffers,
    int                     nAuxBuffers) 
{
    FxBool rv = FXTRUE;
    int tmu;
    InitFIFOData fifoInfo;
    int xres, yres, fbStride;

    printf("grSstWinOpen: initializing SST\n");
    GrGC *gc = _GlideRoot.curGC;

    rv = initSetVideo( hWnd, resolution, 
                       refresh, format, origin, 
                       nColBuffers, nAuxBuffers, 
                       &xres, &yres, &fbStride, _GlideRoot.GCs[_GlideRoot.current_sst].vidTimings );
    if (!rv) {
        printf("grSstWinOpen: initSetVideo failed\n");
        return rv;
    }
    GDBG_INFO((gc->myLevel, 
             "  Video init succeeded. xRes = %.04d, yRes = %.04d\n",
             xres, yres ));    

    GDBG_INFO((gc->myLevel, "  Command Transport Init\n" ));

    fifoInfo.cpuType = _GlideRoot.CPUType;
    rv = initEnableTransport( &fifoInfo );    
    if (!rv) {
        printf("grSstWinOpen: initEnableTransport failed\n");
        return rv;
    }

    gc->nopCMD = FXFALSE;

    GDBG_INFO((gc->myLevel, "  GC Init\n" ));
    gc->state.screen_width  = xres;
    gc->state.screen_height = yres;
    gc->state.num_buffers   = nColBuffers;
    gc->state.color_format  = format;
    gc->grSstRez            = resolution;
    gc->grSstRefresh        = refresh;

    gc->lockPtrs[GR_LFB_READ_ONLY]  = (FxU32)-1;
    gc->lockPtrs[GR_LFB_WRITE_ONLY] = (FxU32)-1;
    gc->lfbSliOk = 0;
    gc->fbStride = fbStride;

    /* Initialize the read/write registers, following info.c:fbiMemSize() */
    gc->state.fbi_config.fbzColorPath  = SST_CC_MONE;
    gc->state.fbi_config.fogMode       = 0;
    gc->state.fbi_config.alphaMode     = 0;
    gc->state.fbi_config.fbzMode       = SST_RGBWRMASK | SST_ZAWRMASK | SST_DRAWBUFFER_FRONT;
    gc->state.fbi_config.lfbMode       = SST_LFB_ZZ | SST_LFB_WRITEFRONTBUFFER | SST_LFB_READDEPTHABUFFER;
    gc->state.fbi_config.clipLeftRight = 0;
    gc->state.fbi_config.clipBottomTop = 0;
    gc->state.fbi_config.fogColor      = 0;
    gc->state.fbi_config.zaColor       = 0;
    gc->state.fbi_config.chromaKey     = 0;
    gc->state.fbi_config.stipple       = 0;
    gc->state.fbi_config.color0        = 0;
    gc->state.fbi_config.color1        = 0;
    for (tmu = 0; tmu < gc->num_tmu; tmu += 1) {
        FxU32 textureMode = (FxU32)SST_SEQ_8_DOWNLD;
        if ((_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].type == GR_SSTTYPE_VOODOO) && 
            (_GlideRoot.hwConfig.SSTs[_GlideRoot.current_sst].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRev == 0)) {
            textureMode = 0;
        }
        gc->state.tmu_config[tmu].textureMode     = textureMode;
        gc->state.tmu_config[tmu].tLOD            = 0x00000000;
        gc->state.tmu_config[tmu].tDetail         = 0x00000000;
        gc->state.tmu_config[tmu].texBaseAddr     = 0x00000000;
        gc->state.tmu_config[tmu].texBaseAddr_1   = 0x00000000;
        gc->state.tmu_config[tmu].texBaseAddr_2   = 0x00000000;
        gc->state.tmu_config[tmu].texBaseAddr_3_8 = 0x00000000;
        gc->state.tmu_config[tmu].mmMode          = GR_MIPMAP_NEAREST;
        gc->state.tmu_config[tmu].smallLod        = GR_LOD_1;
        gc->state.tmu_config[tmu].largeLod        = GR_LOD_1;
        gc->state.tmu_config[tmu].evenOdd         = GR_MIPMAPLEVELMASK_BOTH;
        gc->state.tmu_config[tmu].nccTable        = GR_NCCTABLE_NCC0;
    }    

    GDBG_INFO((gc->myLevel, "  3D State Init\n" ));

    // nand2mario: skip 3d state init for now
    // see gsst.c:grSstWinOpen()
    gc->open = FXTRUE;
    _GlideRoot.windowsInit = FXTRUE; /* to avoid race with grSstControl() */


    // grHints( GR_HINT_FIFOCHECKHINT, 
    //         fifoInfo.hwDep.vgFIFOData.memFifoStatusLwm + 0x100 );
    _grReCacheFifo( 0 );    
    
    return rv;
}

void grBufferClear(GrColor_t color, GrAlpha_t alpha, FxU16 depth ) {
    GrColor_t oldc1;
    FxU32 oldzacolor, zacolor;
    GrGC *gc = _GlideRoot.curGC;
    // Sstregs *hw = (Sstregs *)gc->reg_ptr;

    oldc1   = gc->state.fbi_config.color1;
    zacolor = oldzacolor = gc->state.fbi_config.zaColor;

    /*
    ** Setup source registers
    */
    if ( gc->state.fbi_config.fbzMode & SST_RGBWRMASK )
    {
        _grSwizzleColor( &color );
        GR_SET( OFFSET_c1, color );
    }
    if ( ( gc->state.fbi_config.fbzMode & ( SST_ENALPHABUFFER | SST_ZAWRMASK ) ) == ( SST_ENALPHABUFFER | SST_ZAWRMASK ) )
    {
        zacolor &= ~SST_ZACOLOR_ALPHA;
        zacolor |= ( ( ( FxU32 ) alpha ) << SST_ZACOLOR_ALPHA_SHIFT );
        GR_SET( OFFSET_zaColor, zacolor );
    }
    if ( ( gc->state.fbi_config.fbzMode & ( SST_ENDEPTHBUFFER | SST_ZAWRMASK ) ) == ( SST_ENDEPTHBUFFER | SST_ZAWRMASK ) )  {
        zacolor &= ~SST_ZACOLOR_DEPTH;
        zacolor |= ( ( ( FxU32 ) depth ) << SST_ZACOLOR_DEPTH_SHIFT );
        GR_SET( OFFSET_zaColor, zacolor );
    }    

    /*
    ** Execute the FASTFILL command
    */
    GR_SET(OFFSET_fastfillCMD,1);

    /*
    ** Restore C1 and ZACOLOR
    */
    GR_SET( OFFSET_c1, oldc1 );
    GR_SET( OFFSET_zaColor, oldzacolor );    

}

void grDrawTriangle(const GrVertex *va, const GrVertex *vb, const GrVertex *vc) {
    GrGC *gc = _GlideRoot.curGC;
    // Sstregs *hw = (Sstregs *)gc->reg_ptr;

    // TRISETUP( va, vb, vc );
    const float *fa = &va->x;
    const float *fb = &vb->x;
    const float *fc = &vc->x;
    float ooa, dxAB, dxBC, dyAB, dyBC;
    int i,j,culltest;
    int ay, by, cy;
    uint32_t foff;
    struct datalist_s *dlp;
    
    culltest = gc->state.cull_mode; /* 1 if negative, 0 if positive */
    _GlideRoot.stats.trisProcessed++;
    
    /*
    **  Sort the vertices.
    **  Whenever the radial order is reversed (from counter-clockwise to
    **  clockwise), we need to change the area of the triangle.  Note
    **  that we know the first two elements are X & Y by looking at the
    **  grVertex structure.  
    */
    ay = *(int *)&va->y;
    by = *(int *)&vb->y;
    if (ay < 0) ay ^= 0x7FFFFFFF;
    cy = *(int *)&vc->y;
    if (by < 0) by ^= 0x7FFFFFFF;
    if (cy < 0) cy ^= 0x7FFFFFFF;
    if (ay < by) {
        if (by > cy) {              /* acb */
        if (ay < cy) {
            fa = &va->x;
            fb = &vc->x;
            fc = &vb->x;
            culltest ^= 1;
        } else {                  /* cab */
            fa = &vc->x;
            fb = &va->x;
            fc = &vb->x;
        }
        /* else it's already sorted */
        }
    } else {
        if (by < cy) {              /* bac */
        if (ay < cy) {
            fa = &vb->x;
            fb = &va->x;
            fc = &vc->x;
            culltest ^= 1;
        } else {                  /* bca */
            fa = &vb->x;
            fb = &vc->x;
            fc = &va->x;
        }
        } else {                    /* cba */
        fa = &vc->x;
        fb = &vb->x;
        fc = &va->x;
        culltest ^= 1;
        }
    }
    /* Compute Area */
    dxAB = fa[GR_VERTEX_X_OFFSET] - fb[GR_VERTEX_X_OFFSET];
    dxBC = fb[GR_VERTEX_X_OFFSET] - fc[GR_VERTEX_X_OFFSET];
    
    dyAB = fa[GR_VERTEX_Y_OFFSET] - fb[GR_VERTEX_Y_OFFSET];
    dyBC = fb[GR_VERTEX_Y_OFFSET] - fc[GR_VERTEX_Y_OFFSET];
    
    /* this is where we store the area */
    _GlideRoot.pool.ftemp1 = dxAB * dyBC - dxBC * dyAB;
    
    /* Zero-area triangles are BAD!! */
    j = *(long *)&_GlideRoot.pool.ftemp1;
    if ((j & 0x7FFFFFFF) == 0)
        return;
    
    /* Backface culling, use sign bit as test */
    if (gc->state.cull_mode != GR_CULL_DISABLE) {
        if ((j ^ (culltest<<31)) >= 0)
            return;
    }
    
    GR_SET_EXPECTED_SIZE(_GlideRoot.curTriSize);
    
    ooa = _GlideRoot.pool.f1 / _GlideRoot.pool.ftemp1;
    /* GMT: note that we spread out our PCI writes */
    /* write out X & Y for vertex A */
    GR_SETF( OFFSET_FvA, fa[GR_VERTEX_X_OFFSET] );
    GR_SETF( OFFSET_FvA+1, fa[GR_VERTEX_Y_OFFSET] );    

    dlp = gc->dataList;
    i = dlp->i;
    
    /* write out X & Y for vertex B */
    GR_SETF( OFFSET_FvB, fb[GR_VERTEX_X_OFFSET] );
    GR_SETF( OFFSET_FvB+1, fb[GR_VERTEX_Y_OFFSET] );
    
    /* write out X & Y for vertex C */
    GR_SETF( OFFSET_FvC, fc[GR_VERTEX_X_OFFSET] );
    GR_SETF( OFFSET_FvC+1, fc[GR_VERTEX_Y_OFFSET] );

    /*
    ** Divide the deltas by the area for gradient calculation.
    */
    dxBC *= ooa;
    dyAB *= ooa;
    dxAB *= ooa;
    dyBC *= ooa;

/* access a floating point array with a byte index */
#define FARRAY(p,i) (*(float *)((i)+(uint8_t *)(p)))

    /* 
    ** The src vector contains offsets from fa, fb, and fc to for which
    **  gradients need to be calculated, and is null-terminated.
    */
    while (i) {
        foff = dlp->addr >> 2;
        if (i & 1) {                   /* packer bug check */
            if (i & 2) P6FENCE;
            GR_SETF( foff, 0.0F );
            if (i & 2) P6FENCE;
            dlp++;
            i = dlp->i;
        } else {
            float dpAB, dpBC,dpdx, dpdy;

            dpBC = FARRAY(fb,i);
            dpdx = FARRAY(fa,i);
            GR_SETF( foff, dpdx );

            dpAB = dpdx - dpBC;
            dpBC = dpBC - FARRAY(fc,i);
            dpdx = dpAB * dyBC - dpBC * dyAB;

            GDBG_INFO((285,"p0,1x: %g %g dpdx: %g\n",dpAB * dyBC,dpBC * dyAB,dpdx));
            GR_SETF( foff + (DPDX_OFFSET>>2) , dpdx );
            dpdy = dpBC * dxAB - dpAB * dxBC;

            GDBG_INFO((285,"p0,1y: %g %g dpdy: %g\n",dpBC * dxAB,dpAB * dxBC,dpdy));
            dlp++;
            i = dlp->i;
            GR_SETF( foff + (DPDY_OFFSET>>2) , dpdy );
        }
    }

    /* Draw the triangle by writing the area to the triangleCMD register */
    GR_SETF( OFFSET_FtriangleCMD, _GlideRoot.pool.ftemp1 );         // nand2mario: this immediately draws the triangle
    _GlideRoot.stats.trisDrawn++;

    GR_CHECK_SIZE();
    _GlideRoot.stats.trisDrawn++;
    return;
}

void grBufferSwap(int swapInterval) {
    GrGC *gc = _GlideRoot.curGC;
    // Sstregs *hw = (Sstregs *)gc->reg_ptr;
    int vSync;

    /* check for environmental override */
    if (_GlideRoot.environment.swapInterval >= 0) {
        swapInterval = _GlideRoot.environment.swapInterval;
    }

    vSync = swapInterval > 0 ? 1 : 0;   
    GR_SET(OFFSET_swapbufferCMD, (swapInterval<<1) | vSync);

    _GlideRoot.stats.bufferSwaps++;
}

void guColorCombineFunction(GrColorCombineFnc_t fnc) {
    switch ( fnc )
    {
    case GR_COLORCOMBINE_ZERO:
        grColorCombine( GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_NONE, FXFALSE );
        break;

    case GR_COLORCOMBINE_CCRGB:
        grColorCombine( GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_NONE, FXFALSE );
        break;

    case GR_COLORCOMBINE_ITRGB_DELTA0:
        _grColorCombineDelta0Mode( FXTRUE );
        /* FALL THRU */
    case GR_COLORCOMBINE_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_NONE, FXFALSE );
        break;

    case GR_COLORCOMBINE_DECAL_TEXTURE:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_TIMES_CCRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_DELTA0:
        _grColorCombineDelta0Mode( FXTRUE );
        /* FALL THRU */
    case GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB_ADD_ALPHA:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_TIMES_ALPHA:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL_ALPHA, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_TIMES_ALPHA_ADD_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_LOCAL_ALPHA, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_ADD_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_TEXTURE_SUB_ITRGB:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_CCRGB_BLEND_ITRGB_ON_TEXALPHA:
        grColorCombine( GR_COMBINE_FUNCTION_BLEND, GR_COMBINE_FACTOR_TEXTURE_ALPHA, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_ITERATED, FXFALSE );
        break;

    case GR_COLORCOMBINE_DIFF_SPEC_A:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_LOCAL_ALPHA, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_DIFF_SPEC_B:
        grColorCombine( GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE );
        break;

    case GR_COLORCOMBINE_ONE:
        grColorCombine( GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_NONE, FXTRUE );
        break;
        
    default:
        GR_CHECK_F("grColorCombineFunction", 1, "unsupported color combine function");
        break;
    }
}


/*---------------------------------------------------------------------------
** grCullMode
**
** GMT: warning - gaa.c has the guts of this in-line
*/

void grCullMode( GrCullMode_t mode )
{
  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));
  gc->state.cull_mode = mode;
} /* grCullMode */

/*---------------------------------------------------------------------------
** grFogMode
*/

void grFogMode( GrFogMode_t mode )
{
  FxU32 fogmode;

  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));

  fogmode = gc->state.fbi_config.fogMode;
  fogmode &= ~( SST_ENFOGGING | SST_FOGADD | SST_FOGMULT | SST_FOG_ALPHA | SST_FOG_Z | SST_FOG_CONSTANT );

  switch ( mode & 0xFF ) {    /* switch based on lower 8 bits */
    case GR_FOG_DISABLE:
      break;
    case GR_FOG_WITH_ITERATED_ALPHA:
      fogmode |= ( SST_ENFOGGING | SST_FOG_ALPHA );
      break;
    case GR_FOG_WITH_ITERATED_Z:        /* Bug 735 */
      fogmode |= ( SST_ENFOGGING | SST_FOG_Z );
      break;
    case GR_FOG_WITH_TABLE:
      fogmode |= ( SST_ENFOGGING );
  }
  if ( mode &  GR_FOG_MULT2 ) fogmode |= SST_FOGMULT;
  if ( mode &  GR_FOG_ADD2 ) fogmode |= SST_FOGADD;

  /*
  ** Update the hardware and Glide state
  */
  GR_SET(OFFSET_fogMode, fogmode );
  gc->state.fbi_config.fogMode = fogmode;

  _grUpdateParamIndex();
} /* grFogMode */

/*---------------------------------------------------------------------------
** grFogColorValue
*/
void grFogColorValue( GrColor_t color )
{
  GR_BEGIN("grFogColorValue",85,4);
  GDBG_INFO_MORE((gc->myLevel,"(0x%x)\n",color));

  _grSwizzleColor( &color );
  GR_SET(OFFSET_fogColor, color );
  gc->state.fbi_config.fogColor = color;
} /* grFogColorValue */

/*---------------------------------------------------------------------------
** grDepthBufferFunction
*/
void grDepthBufferFunction( GrCmpFnc_t fnc )
{
  FxU32 fbzMode;

  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",fnc));

  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~SST_ZFUNC;
  fbzMode |= ( fnc << SST_ZFUNC_SHIFT );
  GR_SET(OFFSET_fbzMode, fbzMode );
  gc->state.fbi_config.fbzMode = fbzMode;
} /* grDepthBufferFunction */

/*---------------------------------------------------------------------------
** grDepthBufferMode
*/

void grDepthBufferMode( GrDepthBufferMode_t mode )
{
  FxU32 fbzMode;

  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",mode));

   /*
   ** depth buffering cannot be enabled on systems running at:
   **      800x600 w/ 2MB
   **      640x480 w/ 1MB
   */
   GR_CHECK_F( myName,
                mode != GR_DEPTHBUFFER_DISABLE && !_grCanSupportDepthBuffer(),
                "cannot enable depthbuffer with configuration" );

  /* turn off all the bits and then turn them back on selectively */
  fbzMode = gc->state.fbi_config.fbzMode;
  fbzMode &= ~(SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ENZBIAS | SST_ZCOMPARE_TO_ZACOLOR);

  switch (mode) {
  case GR_DEPTHBUFFER_DISABLE:
    break;
    
  case GR_DEPTHBUFFER_ZBUFFER:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ENZBIAS;
    break;

  case GR_DEPTHBUFFER_WBUFFER:
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ENZBIAS;
    break;

  case GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    break;

  case GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS:
    fbzMode |= SST_ENDEPTHBUFFER | SST_WBUFFER | SST_ZCOMPARE_TO_ZACOLOR;
    break;
  }

  /*
  ** Update hardware and Glide state
  */
  GR_SET(OFFSET_fbzMode, fbzMode);
  gc->state.fbi_config.fbzMode = fbzMode;

  _grUpdateParamIndex();
} /* grDepthBufferMode */

/*---------------------------------------------------------------------------
** grDepthMask
*/
void grDepthMask( FxBool enable )
{
  FxU32 fbzMode;

  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",enable));

  fbzMode = gc->state.fbi_config.fbzMode;
  GR_CHECK_F( myName,
                enable && !_grCanSupportDepthBuffer(),
                "called in a non-depthbufferable configuration" );

  if ( enable )
    fbzMode |= SST_ZAWRMASK;
  else
    fbzMode &= ~SST_ZAWRMASK;

  GR_SET(OFFSET_fbzMode, fbzMode );
  gc->state.fbi_config.fbzMode = fbzMode;
} /* grDepthMask */

/*---------------------------------------------------------------------------
** grRenderBuffer
**
**  Although SST1 supports triple buffering, it's a hack in the hardware,
**  and the only drawbuffer modes supported by the fbzMode register are 0
**  (back) and 1 (front)
*/

GR_ENTRY(grRenderBuffer, void, ( GrBuffer_t buffer ))
{

  GR_BEGIN_NOFIFOCHECK("grRenderBuffer",85);
  GDBG_INFO_MORE((gc->myLevel,"(%d)\n",buffer));
  GR_CHECK_F( myName, buffer >= GR_BUFFER_AUXBUFFER, "invalid buffer" );

#if ( GLIDE_PLATFORM & GLIDE_HW_SST1 )
  {
      FxU32 fbzMode;
      GR_SET_EXPECTED_SIZE( 4 );
      
      fbzMode = gc->state.fbi_config.fbzMode;
      fbzMode &= ~( SST_DRAWBUFFER );
      fbzMode |= buffer == GR_BUFFER_FRONTBUFFER ? SST_DRAWBUFFER_FRONT : SST_DRAWBUFFER_BACK;
      
      GR_SET( OFFSET_fbzMode, fbzMode );
      gc->state.fbi_config.fbzMode = fbzMode;
  }
#elif ( GLIDE_PLATFORM & GLIDE_HW_SST96 )
  initRenderBuffer( buffer );
#else
#  error "No renderbuffer implementation"
#endif

  GR_END();
} /* grRenderBuffer */

/*
** Internal functions
*/
extern void _grMipMapInit();

void _GlideInitEnvironment( void )
{
  int i;

  if ( _GlideRoot.initialized ) /* only execute once */
    return;

  grErrorSetCallback(_grErrorDefaultCallback);

  _GlideRoot.CPUType = 5; // 5 is i586, _cpu_detect_asm();
  _GlideRoot.environment.triBoundsCheck = 0;
  _GlideRoot.environment.swapInterval = -1;
  _GlideRoot.environment.swFifoLWM = -1;
  _GlideRoot.environment.noSplash = 1;
  _GlideRoot.environment.shamelessPlug = 0;

  GDBG_INFO((80,"    triBoundsCheck: %d\n",_GlideRoot.environment.triBoundsCheck));
  GDBG_INFO((80,"      swapInterval: %d\n",_GlideRoot.environment.swapInterval));
  GDBG_INFO((80,"          noSplash: %d\n",_GlideRoot.environment.noSplash));
  GDBG_INFO((80,"     shamelessPlug: %d\n",_GlideRoot.environment.shamelessPlug));
  GDBG_INFO((80,"        rsrchFlags: %d\n",_GlideRoot.environment.rsrchFlags));
  GDBG_INFO((80,"               cpu: %d\n",_GlideRoot.CPUType));
  GDBG_INFO((80,"          snapshot: %d\n",_GlideRoot.environment.snapshot));
  GDBG_INFO((80,"  disableDitherSub: %d\n",_GlideRoot.environment.disableDitherSub));  
  /* GMT: BUG these are hardware dependent and really should come from the init code */
  _GlideRoot.stats.minMemFIFOFree = 0xffff;
  _GlideRoot.stats.minPciFIFOFree = 0x3f;

  /* constant pool */
  _GlideRoot.pool.f0   =   0.0F;
  _GlideRoot.pool.fHalf=   0.5F;
  _GlideRoot.pool.f1   =   1.0F;
  _GlideRoot.pool.f255 = 255.0F;
  _GlideRoot.pool.f256 = 256.0F;

  /* nand2mario: hardcode one board */
  /* gpci.c: _grSstDetectResources() */
  _GlideRoot.hwConfig.num_sst = 1;
  _GlideRoot.GCs[0].num_tmu = 1;

  _GlideRoot.hwConfig.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam = 2;
  _GlideRoot.GCs[0].tmu_state[0].total_mem    = 2<<20;      // 2MB TMU memory
  _GlideRoot.GCs[0].tmu_state[0].ncc_mmids[0] = GR_NULL_MIPMAP_HANDLE;
  _GlideRoot.GCs[0].tmu_state[0].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;

  _GlideRoot.current_sst = 0;   /* make sure there's a valid GC */
  _GlideRoot.curGC = &_GlideRoot.GCs[0]; /* just for 'booting' the library */

  _grMipMapInit();
  _GlideRoot.initialized = FXTRUE; /* save this for the end */
}

void 
_grSwizzleColor( GrColor_t *color )
{
  GR_DCL_GC;
  unsigned long red, green, blue, alpha;
  
  switch( gc->state.color_format ) {
  case GR_COLORFORMAT_ARGB:
    break;
  case GR_COLORFORMAT_ABGR:
    blue = *color & 0x00ff;
    red = ( *color >> 16 ) & 0xff;
    *color &= 0xff00ff00;
    *color |= ( blue << 16 );
    *color |= red;
    break;
  case GR_COLORFORMAT_RGBA:
    red    = ( *color & 0x0000ff00 ) >> 8;
    green  = ( *color & 0x00ff0000 ) >> 16;
    blue   = ( *color & 0xff000000 ) >> 24;
    alpha  = ( *color & 0x000000ff );
    *color = ( alpha << 24 ) | ( blue << 16 ) | ( green << 8 ) | red;
    break;
  case GR_COLORFORMAT_BGRA:
    red    = ( *color & 0xff000000 ) >> 24;
    green  = ( *color & 0x00ff0000 ) >> 16;
    blue   = ( *color & 0x0000ff00 ) >> 8;
    alpha  = ( *color & 0x000000ff );
    *color = ( alpha << 24 ) | ( blue << 16 ) | ( green << 8 ) | red;
    break;
  }
} /* _grSwizzleColor */

void grColorCombine( GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, FxBool invert )
{
  FxU32 fbzColorPath;
  FxU32 oldTextureEnabled;

  GrGC *gc = _GlideRoot.curGC;
  GDBG_INFO_MORE((gc->myLevel,"(%d,%d,%d,%d,%d)\n",function,factor,local,other,invert));

  GR_CHECK_W( myName,
               function < GR_COMBINE_FUNCTION_ZERO ||
               function > GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA,
               "unsupported color combine function" );

  GR_CHECK_W( myName,
              (factor & 0x7) < GR_COMBINE_FACTOR_ZERO ||
              (factor & 0x7) > GR_COMBINE_FACTOR_TEXTURE_ALPHA ||
              factor > GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA,
              "unsupported color combine scale factor");
  GR_CHECK_W( myName,
              local < GR_COMBINE_LOCAL_ITERATED || local > GR_COMBINE_LOCAL_DEPTH,
              "unsupported color combine local color");
  GR_CHECK_W( myName,
              other < GR_COMBINE_OTHER_ITERATED || other > GR_COMBINE_OTHER_CONSTANT,
              "unsupported color combine other color");

  fbzColorPath = gc->state.fbi_config.fbzColorPath;
  oldTextureEnabled = fbzColorPath & SST_ENTEXTUREMAP;
  fbzColorPath &= ~( SST_ENTEXTUREMAP |
                     SST_RGBSELECT |
                     SST_LOCALSELECT |
                     SST_CC_ZERO_OTHER |
                     SST_CC_SUB_CLOCAL |
                     SST_CC_MSELECT |
                     SST_CC_REVERSE_BLEND |
                     SST_CC_ADD_CLOCAL |
                     SST_CC_ADD_ALOCAL |
                     SST_CC_INVERT_OUTPUT );

  /* this is bogus, it should be done once, somewhere. */
  fbzColorPath |= SST_PARMADJUST;

  /* setup reverse blending first, then strip off the the high bit */
  if ( (factor & 0x8) == 0 )
    fbzColorPath |= SST_CC_REVERSE_BLEND;
  factor &= 0x7;

  /* NOTE: we use boolean OR instead of logical to avoid branches */
  gc->state.cc_requires_texture = ( factor == GR_COMBINE_FACTOR_TEXTURE_ALPHA ) |
                                  ( other == GR_COMBINE_OTHER_TEXTURE );
  gc->state.cc_requires_it_rgb = ( local == GR_COMBINE_LOCAL_ITERATED ) |
                                 ( other == GR_COMBINE_OTHER_ITERATED );

  /* setup scale factor bits */
  fbzColorPath |= factor << SST_CC_MSELECT_SHIFT;

  /* setup local color bits */
  fbzColorPath |= local << SST_LOCALSELECT_SHIFT;

  /* setup other color bits */
  fbzColorPath |= other << SST_RGBSELECT_SHIFT;

  /* setup invert output bits */
  if ( invert )
    fbzColorPath |= SST_CC_INVERT_OUTPUT;

  /* setup core color combine unit bits */
  switch ( function )
  {
  case GR_COMBINE_FUNCTION_ZERO:
    fbzColorPath |= SST_CC_ZERO_OTHER;
    break;

  case GR_COMBINE_FUNCTION_LOCAL:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_ADD_ALOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER:
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
    fbzColorPath |= SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_ADD_ALOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
    fbzColorPath |= SST_CC_SUB_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
    fbzColorPath |= SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_SUB_CLOCAL | SST_CC_ADD_ALOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL;
    break;

  case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
    fbzColorPath |= SST_CC_ZERO_OTHER | SST_CC_SUB_CLOCAL | SST_CC_ADD_ALOCAL;
    break;
  }

  /* if either color or alpha combine requires texture then enable it */
  if ( gc->state.cc_requires_texture || gc->state.ac_requires_texture )
    fbzColorPath |= SST_ENTEXTUREMAP;

  /* if we transition into/out of texturing ... add nopCMD */
  if(oldTextureEnabled != (fbzColorPath & SST_ENTEXTUREMAP)) {
    P6FENCE_CMD( GR_SET(OFFSET_nopCMD,0) );
  }

  /* update register */
  GR_SET( OFFSET_fbzColorPath, fbzColorPath );
  gc->state.fbi_config.fbzColorPath = fbzColorPath;

  /* setup paramIndex bits */
  _grUpdateParamIndex();

//   GR_END_SLOPPY();
} /* grColorCombine */

/*---------------------------------------------------------------------------
** _grColorCombineDelta0Mode
**
** GMT: when we are in delta0 mode, color comes from the RGB iterators
**      but the slopes are 0.0.  So when we enter delta0 mode we set
**      the iterators up and then we leave them alone during primitive
**      rendering
*/

void _grColorCombineDelta0Mode( FxBool delta0mode )
{
  GrGC *gc = _GlideRoot.curGC;

  if ( delta0mode ) {
    GR_SETF( OFFSET_Fr, gc->state.r );
    GR_SETF( OFFSET_Fg, gc->state.g );
    GR_SETF( OFFSET_Fb, gc->state.b );
    GR_SET( OFFSET_drdx, 0);
    GR_SET( OFFSET_drdy, 0);
    GR_SET( OFFSET_dgdx, 0);
    GR_SET( OFFSET_dgdy, 0);
    GR_SET( OFFSET_dbdx, 0);
    GR_SET( OFFSET_dbdy, 0);
  }

  gc->state.cc_delta0mode = delta0mode;
  printf("delta0mode: %d\n", delta0mode);

} /* _grColorCombineDeltaMode */


/*---------------------------------------------------------------------------
** _grUpdateParamIndex
**
** Updates the paramIndex bits based on Glide state and the hints.
**
*/
void _grUpdateParamIndex( void )
{
    GrGC *gc = _GlideRoot.curGC;
    FxU32 paramIndex = 0;
    FxU32 hints = gc->state.paramHints;
    FxU32 fbzColorPath = gc->state.fbi_config.fbzColorPath;
    FxU32 fogMode = gc->state.fbi_config.fogMode;
    FxU32 fbzMode = gc->state.fbi_config.fbzMode;

    /*
    ** First, turn on every bit that we think we need. We can prune them
    ** back later.
    */

    /* Turn on the texture bits based on what grTexCombine set */
    if (fbzColorPath & SST_ENTEXTUREMAP) {
        /* No matter what, we need oow from the main grvertex structure */
        static FxU32 paramI_array[8] = {
        STATE_REQUIRES_OOW_FBI,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 |
        STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2 |
        STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2 |
        STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1,
        STATE_REQUIRES_OOW_FBI | STATE_REQUIRES_W_TMU2 | STATE_REQUIRES_ST_TMU2 |
        STATE_REQUIRES_W_TMU0 | STATE_REQUIRES_ST_TMU0 |
        STATE_REQUIRES_W_TMU1 | STATE_REQUIRES_ST_TMU1
        };
        GR_ASSERT(gc->state.tmuMask < sizeof(paramI_array)/sizeof(paramI_array[0]));
        paramIndex |= paramI_array[gc->state.tmuMask];
    }  

    /* See if we need iterated RGB */
    if ( gc->state.cc_requires_it_rgb && !gc->state.cc_delta0mode ) {
        paramIndex |= STATE_REQUIRES_IT_DRGB;
    }

    /* See if we need to iterate alpha based on the value of
        ac_requires_it_alpha */ 
    if (gc->state.ac_requires_it_alpha) {
        paramIndex |= STATE_REQUIRES_IT_ALPHA;
    }

    /* See what fbzColorPath contributes - BUG 1084*/
    if ( ( fbzColorPath & SST_ALOCALSELECT ) == 
        ( SST_ALOCAL_Z ) ) {
        paramIndex |= STATE_REQUIRES_OOZ;
    }

    /* See what fbzMode contributes */
    if (fbzMode & SST_ENDEPTHBUFFER) {
        if (fbzMode & SST_WBUFFER) {
        paramIndex |= STATE_REQUIRES_OOW_FBI;
        } else {
        paramIndex |= STATE_REQUIRES_OOZ;
        }
    }

    /* See what fogMode contributes */
    if (fogMode & SST_ENFOGGING) {
        if (fogMode & SST_FOG_Z) {
        paramIndex |= STATE_REQUIRES_OOZ;
        } else {
        if (fogMode & SST_FOG_ALPHA) {
            paramIndex |= STATE_REQUIRES_IT_ALPHA;
        } else {
            paramIndex |= STATE_REQUIRES_OOW_FBI;
        }
        }
    }

    /*
    **  Now we know everything that needs to be iterated.  Prune back
    **  the stuff that isn't explicitly different
    **
    **  NOTE: by GMT, STATE_REQUIRES_OOW_FBI is set whenever texture mapping
    **        is enabled
    */
    /* Turn off W for TMU0 if we don't have a hint */
    if (paramIndex & STATE_REQUIRES_W_TMU0) {
        GR_ASSERT(paramIndex & STATE_REQUIRES_OOW_FBI);
        if (!(hints & GR_STWHINT_W_DIFF_TMU0))
        paramIndex &= ~STATE_REQUIRES_W_TMU0;
    }
    
    /* Turn off ST for TMU1 if TMU0 is active and TMU1 is not different */
    if (((paramIndex & (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) ==
                        (STATE_REQUIRES_ST_TMU0 | STATE_REQUIRES_ST_TMU1)) &&
        !(hints & GR_STWHINT_ST_DIFF_TMU1))
        paramIndex &= ~STATE_REQUIRES_ST_TMU1;
    
    /* Turn off W for TMU1 if we have a previous W, and don't have a hint */ 
    if ((paramIndex & STATE_REQUIRES_W_TMU1) && !(hints & GR_STWHINT_W_DIFF_TMU1))
        paramIndex &= ~STATE_REQUIRES_W_TMU1;

    gc->state.paramIndex = paramIndex;

    _grRebuildDataList();

} /* _grUpdateParamIndex */

  #define RED   Fr
  #define DRDX  Fdrdx
  #define DRDY  Fdrdy
  #define GRN   Fg
  #define DGDX  Fdgdx
  #define DGDY  Fdgdy
  #define BLU   Fb
  #define DBDX  Fdbdx
  #define DBDY  Fdbdy
  #define ALF   Fa
  #define DADX  Fdadx
  #define DADY  Fdady
  #define Z     Fz
  #define DZDX  Fdzdx
  #define DZDY  Fdzdy
  #define S     Fs
  #define DSDX  Fdsdx
  #define DSDY  Fdsdy
  #define T     Ft
  #define DTDX  Fdtdx
  #define DTDY  Fdtdy
  #define W     Fw
  #define DWDX  Fdwdx
  #define DWDY  Fdwdy

/*---------------------------------------------------------------------------
** _grRebuildDataList
*/
void _grRebuildDataList( void )
{
    GrGC *gc = _GlideRoot.curGC;
    // Sstregs *hw = (Sstregs *)gc->reg_ptr;

    int curTriSize, params, packerFlag;
    FxU32 i, packMask=1;
    // Sstregs *tmu0;
    // Sstregs *tmu1;
    
#ifdef GLIDE_DEBUG
    static char *p_str[] = {"x","y","z","r","g","b","ooz","a","oow",
                            "s0","t0","w0","s1","t1","w1","s2","t2","w2"};
#endif
  
    GDBG_INFO((80,"_grRebuildDataList() paramHints=0x%x paramIndex=0x%x\n",
                gc->state.paramHints,gc->state.paramIndex));
    
    curTriSize = params = 0;
    i = gc->state.paramIndex;
    if (_GlideRoot.CPUType == 6) packMask |= 2;
    
    // tmu0 = SST_CHIP(hw,0xE); /* tmu 0,1,2 */
    // tmu1 = SST_CHIP(hw,0xC); /* tmu 1,2 */
    
    if (i & STATE_REQUIRES_IT_DRGB) {
        /* add 9 to size array for r,drdx,drdy,g,dgdx,dgdy,b,dbdx,dbdy */
        gc->dataList[curTriSize + 0].i    = GR_VERTEX_R_OFFSET<<2;
        gc->dataList[curTriSize + 0].addr = OFFSET_Fr << 2;  // RED
        gc->dataList[curTriSize + 1].i    = GR_VERTEX_G_OFFSET<<2;
        gc->dataList[curTriSize + 1].addr = OFFSET_Fg << 2;  // GRN
        gc->dataList[curTriSize + 2].i    = GR_VERTEX_B_OFFSET<<2;
        gc->dataList[curTriSize + 2].addr = OFFSET_Fb << 2;  // BLU
        curTriSize += 3;
        params += 3;
    }
    
    if (i & STATE_REQUIRES_OOZ) {
        gc->dataList[curTriSize + 0].i    = GR_VERTEX_OOZ_OFFSET<<2;
        gc->dataList[curTriSize + 0].addr = OFFSET_Fz << 2;
        curTriSize += 1;
        params += 1;
    }
    
    if (i & STATE_REQUIRES_IT_ALPHA) {
        gc->dataList[curTriSize + 0].i    = GR_VERTEX_A_OFFSET<<2;
        gc->dataList[curTriSize + 0].addr = OFFSET_Fa << 2;
        curTriSize += 1;
        params += 1;
    }
    
    /* TMU1 --------------------------------- */
    /* always output to ALL chips, saves from having to change CHIP field */
    if (i & STATE_REQUIRES_ST_TMU0) {
        gc->dataList[curTriSize + 0].i    = GR_VERTEX_SOW_TMU0_OFFSET<<2;
        gc->dataList[curTriSize + 0].addr = OFFSET_Fs << 2;
        gc->dataList[curTriSize + 1].i    = GR_VERTEX_TOW_TMU0_OFFSET<<2;
        gc->dataList[curTriSize + 1].addr = OFFSET_Ft << 2;
        curTriSize += 2;
        params += 2;
    }
    
    /* we squeeze FBI.OOW in here for sequential writes in the simple case */
    if (i & STATE_REQUIRES_OOW_FBI) {
        gc->dataList[curTriSize + 0].i    = GR_VERTEX_OOW_OFFSET<<2;
        gc->dataList[curTriSize + 0].addr = OFFSET_Fw << 2;
        curTriSize += 1;
        params += 1;
    }

    const uint32_t TMU0 = 1 << 11;  // 10 + 1
    const uint32_t TMU1 = 1 << 12;  // 10 + 2
    const uint32_t TMU2 = 1 << 13;  // 10 + 3

    /* NOTE: this is the first */
    if (i & STATE_REQUIRES_W_TMU0) {
        gc->dataList[curTriSize + 0].i    =  packMask;
        gc->dataList[curTriSize + 0].addr = _GlideRoot.packerFixAddress;
        gc->dataList[curTriSize + 1].i    = GR_VERTEX_OOW_TMU0_OFFSET << 2;
        gc->dataList[curTriSize + 1].addr = TMU0 + (OFFSET_Fw << 2);
        curTriSize += 2;
        params += 1;
    }
    
    /* TMU1 --------------------------------- */
    if (i & STATE_REQUIRES_ST_TMU1) {
        gc->dataList[curTriSize + 0].i    = packMask;
        gc->dataList[curTriSize + 0].addr = _GlideRoot.packerFixAddress;
        packerFlag = 0;
        gc->dataList[curTriSize + 1].i    = GR_VERTEX_SOW_TMU1_OFFSET<<2;
        gc->dataList[curTriSize + 1].addr = TMU1 + (OFFSET_Fs << 2);
        gc->dataList[curTriSize + 2].i    = GR_VERTEX_TOW_TMU1_OFFSET<<2;
        gc->dataList[curTriSize + 2].addr = TMU1 + (OFFSET_Ft << 2);
        curTriSize += 3;
        params += 2;
    }
    else packerFlag = 1;
    
    if (i & STATE_REQUIRES_W_TMU1) {
        if (packerFlag) {
        gc->dataList[curTriSize + 0].i    = packMask;
        gc->dataList[curTriSize + 0].addr = _GlideRoot.packerFixAddress;
        curTriSize += 1;
        }
        gc->dataList[curTriSize + 0].i    = GR_VERTEX_OOW_TMU1_OFFSET<<2;
        gc->dataList[curTriSize + 0].addr = TMU1 + (OFFSET_Fw << 2);
        curTriSize += 1;
        params += 1;
    }
    
    /* if last write was not to chip 0 */
    if ( SST_CHIP_MASK & (FxU32)gc->dataList[curTriSize-1].addr ) {
        /* flag write as a packer bug fix  */
        gc->dataList[curTriSize].i = packMask; 
        gc->dataList[curTriSize].addr = _GlideRoot.packerFixAddress;
        curTriSize++;
    }

    
    gc->dataList[curTriSize++].i = 0; /* terminate the list with 0,*      */
                                    /* followed by the FtriangleCMD reg */
    gc->dataList[curTriSize].i = packMask;/* encode P6 flag here for asm code */
    gc->dataList[curTriSize].addr = OFFSET_FtriangleCMD << 2;
    
    /* 6 X,Y values plus AREA = 7, plus parameters */
    _GlideRoot.curTriSize = (6 + curTriSize + (params<<1)) <<2;

    /* Need to know tri size without gradients for planar polygons */
    _GlideRoot.curTriSizeNoGradient = _GlideRoot.curTriSize - (params<<3);
    

} /* _grRebuildDataList */

#include <stdio.h>
void _grErrorDefaultCallback( const char *s, FxBool fatal )
{
  if ( fatal ) {
    // grSstWinClose();
    grGlideShutdown();

    puts( s );
    exit( 1 );
  } else {
    puts( s );
  }
}

// nand2mario: combine init.c: initSetVideo() and vgdrv.c: setVideo()
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
                     sst1VideoTimingStruct *vidTimings) {

    FxBool rv = FXTRUE;

    static int _w[] = {320,320,400,512,640,640,640,640,800,960,856,512};
    static int _h[] = {200,240,256,384,200,350,400,480,600,720,480,256};

    rv = sst1InitVideo( 0,
                        sRes,
                        vRefresh, 
                        vidTimings );

    if ( !rv ) goto BAIL;

    if(vidTimings) {
        *xres = vidTimings->xDimension;
        *yres = vidTimings->yDimension;
    } else {
        *xres = _w[sRes];
        *yres = _h[sRes];
    }
    *fbStride = 1024; /* 1024 always */

BAIL:
    return rv;
}

#define INIT_PRINTF(a) printf a
sst1DeviceInfoStruct _sst1devinfo;
sst1DeviceInfoStruct *sst1CurrentBoard = &_sst1devinfo;


/*
** sst1InitSetResolution():
**  Setup FBI video resolution registers
**  This routine is used by sst1InitVideo()
**
**
*/
void sst1InitSetResolution(FxU32 *sstbase, 
    sst1VideoTimingStruct *sstVideoRez, FxU32 Banked)
{
    if(Banked)
        GR_SET(OFFSET_fbiInit2, (GR_GET(OFFSET_fbiInit2) & ~SST_VIDEO_BUFFER_OFFSET) |
            (sstVideoRez->memOffset << SST_VIDEO_BUFFER_OFFSET_SHIFT) |
            SST_DRAM_BANKING_CONFIG | SST_EN_DRAM_BANKED);
    else
        GR_SET(OFFSET_fbiInit2, (GR_GET(OFFSET_fbiInit2) & ~SST_VIDEO_BUFFER_OFFSET) |
            (sstVideoRez->memOffset << SST_VIDEO_BUFFER_OFFSET_SHIFT));

    printf("sst1InitSetResolution: sstVideoRez->tilesInX_Over2=%d\n", sstVideoRez->tilesInX_Over2);
    GR_SET(OFFSET_fbiInit1, (GR_GET(OFFSET_fbiInit1) & ~SST_VIDEO_TILES_IN_X) |
        (sstVideoRez->tilesInX_Over2 << SST_VIDEO_TILES_IN_X_SHIFT));

}

/*
** sst1InitCalcGrxClk():
**  Determine graphics clock frequency
**  NOTE: sst1InitCalcGrxClk() must be called prior to sst1InitGrxClk()
**
*/
FxBool sst1InitCalcGrxClk(FxU32 *sstbase)
{
    FxU32 clkFreq;

    /* Setup memory clock frequency based on board strapping bits... */
    if(sst1CurrentBoard->fbiBoardID == 0x0) {
        /* Obsidian GE Fab */
        clkFreq = 40 + (((sst1CurrentBoard->tmuConfig >> 3) & 0x7) << 2) +
            (sst1CurrentBoard->fbiConfig & 0x3);
    } else {
        /* Obsidian Pro Fab */
        clkFreq = 40 + (((sst1CurrentBoard->tmuConfig >> 3) & 0x7) << 2) +
            (sst1CurrentBoard->fbiConfig & 0x3);
        /* Fix for legacy/existing boards */
        if(clkFreq == 54)
            clkFreq = 50;
    }
    sst1CurrentBoard->grxClkFreq = clkFreq;
    return(FXTRUE);
}

/*
** sst1InitIdleFBINoNOP():
**  Return idle condition of FBI (ignoring idle status of TMU)
**  sst1InitIdleFBINoNOP() differs from sst1InitIdleFBI() in that no NOP command
**  is issued to flush the graphics pipeline.
**
**    Returns:
**      FXTRUE if FBI is idle (fifos are empty, graphics engines are idle)
**      FXFALSE if FBI has not been mapped
**
*/
FxBool sst1InitIdleFBINoNOP(FxU32 *sstbase)
{
    FxU32 cntr;

    /* ISET(sst->nopCMD, 0x0); */
    cntr = 0;
    while(1) {
        if(!(GR_GET(OFFSET_status) & SST_FBI_BUSY)) {
            if(++cntr > 5)
                break;
        } else
            cntr = 0;
    }
    return(FXTRUE);
}


/*
** sst1InitComputeClkParams():
**  Compute PLL parameters for given clock frequency
**
*/
FxBool sst1InitComputeClkParams(float freq, sst1ClkTimingStruct
    *clkTiming)
{
    float vcoFreqDivide = 0.0f, freqMultRatio, clkError;
    float clkErrorMin;
    FxU32 p, n, m, nPlusTwo;
    int mPlusTwo;

    /* Calculate P parameter */
    p = 4;
    if(((freq * (float) 1.) >= (float) 120.) &&
       ((freq * (float) 1.) <= (float) 240.)) {
           vcoFreqDivide = (float) 1.;
           p = 0;
    }
    if(((freq * (float) 2.) >= (float) 120.) &&
       ((freq * (float) 2.) <= (float) 240.)) {
           vcoFreqDivide = (float) 2.;
           p = 1;
    }
    if(((freq * (float) 4.) >= (float) 120.) &&
       ((freq * (float) 4.) <= (float) 240.)) {
           vcoFreqDivide = (float) 4.;
           p = 2;
    }
    if(((freq * (float) 8.) >= (float) 120.) &&
       ((freq * (float) 8.) <= (float) 240.)) {
           vcoFreqDivide = (float) 8.;
           p = 3;
    }
    if(p > 3)
        return(FXFALSE);

    /* Divide by 14.318 */
    freqMultRatio = (freq * vcoFreqDivide) * (float) 0.06984216;

    /* Calculate proper N and M parameters which yield the lowest error */
    clkErrorMin = (float) 9999.; n = 0; m = 0;
    for(nPlusTwo = 3; nPlusTwo < 32; nPlusTwo++) {
        mPlusTwo = (int) (((float) nPlusTwo * freqMultRatio) + (float) 0.5);
        clkError = ((float) mPlusTwo / (float) nPlusTwo) - freqMultRatio;
        if(clkError < (float) 0.0)
            clkError = -clkError;
        if((clkError < clkErrorMin) && ((mPlusTwo - 2) < 127)) {
            clkErrorMin = clkError;
            n = nPlusTwo - 2;
            m = mPlusTwo - 2;
        }
    }
    if(n == 0)
        return(FXFALSE);

    clkTiming->freq = freq;
    clkTiming->clkTiming_M = m;
    clkTiming->clkTiming_P = p;
    clkTiming->clkTiming_N = n;
    if(freq < (float) 37.) {
        clkTiming->clkTiming_L = 0xa;
        clkTiming->clkTiming_IB = 0x6;
    } else if(freq < (float) 45.) {
        clkTiming->clkTiming_L = 0xc;
        clkTiming->clkTiming_IB = 0x4;
    } else if(freq < (float) 58.) {
        clkTiming->clkTiming_L = 0x8;
        clkTiming->clkTiming_IB = 0x4;
    } else if(freq < (float) 66.) {
        clkTiming->clkTiming_L = 0xa;
        clkTiming->clkTiming_IB = 0x6;
    } else {
        clkTiming->clkTiming_L = 0xa;
        clkTiming->clkTiming_IB = 0x8;
    }
    return(FXTRUE);
}

/*
** sst1InitSetGrxClk():
**  Set graphics clock
**  NOTE: sst1InitSetGrxClk() resets the PCI fifo and the graphics subsystem
**  of FBI
**
*/
FxBool sst1InitSetGrxClk(FxU32 *sstbase,
  sst1ClkTimingStruct *sstGrxClk)
{
    printf("sst1InitSetGrxClk(): doing nothing\n");

    // FxBool retVal = FXFALSE;

    // if((sst1CurrentBoard->fbiDacType == SST_FBI_DACTYPE_ATT) ||
    //     (sst1CurrentBoard->fbiDacType == SST_FBI_DACTYPE_TI))
    //     retVal = sst1InitSetGrxClkATT(sstbase, sstGrxClk);
    // else if(sst1CurrentBoard->fbiDacType == SST_FBI_DACTYPE_ICS)
    //     retVal = sst1InitSetGrxClkICS(sstbase, sstGrxClk);

    // if(retVal == FXFALSE)
    //     return(FXFALSE);

    // /* Fix problem where first Texture downloads to TMU weren't being
    //    received properly */
    // ISET(*(long *) (0xf00000 + (long) sstbase), 0xdeadbeef);

    // if(sst1InitReturnStatus(sstbase) & SST_TREX_BUSY) {
    //     INIT_PRINTF(("sst1InitSetGrxClk(): Resetting TMUs after clock change...\n"));
    //     return(sst1InitResetTmus(sstbase));
    // }
    return(FXTRUE);
}

/*
** sst1InitGrxClk():
**  Initialize FBI and TREX Memory clocks
**  NOTE: sst1InitCalcGrxClk() must be called prior to sst1InitGrxClk()
**  NOTE: sst1InitGrxClk() resets the PCI fifo and the graphics subsystem of FBI
**
*/
FxBool sst1InitGrxClk(FxU32 *sstbase)
{
    sst1ClkTimingStruct sstGrxClk;

    if(sst1CurrentBoard->initGrxClkDone)
        return(FXTRUE);
    sst1CurrentBoard->initGrxClkDone = 1;

    INIT_PRINTF(("sst1InitGrxClk(): Setting up %d MHz Graphics Clock...\n",
        sst1CurrentBoard->grxClkFreq));
    if(sst1InitComputeClkParams((float) sst1CurrentBoard->grxClkFreq,
      &sstGrxClk) == FXFALSE)
        return(FXFALSE);

    return(sst1InitSetGrxClk(sstbase, &sstGrxClk));
}

/*
** sst1InitSetVidMode():
**  Set video Mode
**
*/
FxBool sst1InitSetVidMode(FxU32 *sstbase, FxU32 video16BPP)
{
    printf("sst1InitSetVidMode(): doing nothing\n");
    return FXTRUE;
    // if((sst1CurrentBoard->fbiDacType == SST_FBI_DACTYPE_ATT) ||
    //     (sst1CurrentBoard->fbiDacType == SST_FBI_DACTYPE_TI))
    //     return(sst1InitSetVidModeATT(sstbase, video16BPP));
    // else if(sst1CurrentBoard->fbiDacType == SST_FBI_DACTYPE_ICS)
    //     return(sst1InitSetVidModeICS(sstbase, video16BPP));
    // else
    //     return(FXFALSE);
}

/*
** sst1InitVgaPassCtrl():
**  Control VGA passthrough setting
**
**
*/
FxBool sst1InitVgaPassCtrl(FxU32 *sstbase, FxU32 enable)
{
    if(enable) {
        /* VGA controls monitor */
        GR_SET(OFFSET_fbiInit0, (GR_GET(OFFSET_fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 
            sst1CurrentBoard->vgaPassthruEnable);
        GR_SET(OFFSET_fbiInit1, GR_GET(OFFSET_fbiInit1) | SST_VIDEO_BLANK_EN);
    } else {
        /* SST-1 controls monitor */
        GR_SET(OFFSET_fbiInit0, (GR_GET(OFFSET_fbiInit0) & ~SST_EN_VGA_PASSTHRU) | 
            sst1CurrentBoard->vgaPassthruDisable);
        GR_SET(OFFSET_fbiInit1, GR_GET(OFFSET_fbiInit1) & ~SST_VIDEO_BLANK_EN);
    }

    return(FXTRUE);
}

void sst1InitIdleLoop(FxU32 *sstbase)
{
    FxU32 cntr;

    GR_SET(OFFSET_nopCMD, 0x0);
    cntr = 0;
    while(1) {
        if(!(GR_GET(OFFSET_status) & SST_BUSY)) {
            if(++cntr >= 3)
                break;
        } else
            cntr = 0;
    }
}

/*
** nand2mario: from sst/init/initvg/video.c
** sst1InitVideo():
**  Initialize video (including DAC setup) for the specified resolution
**
**    Returns:
**      FXTRUE if successfully initializes specified SST-1 video resolution
**      FXFALSE if cannot initialize SST-1 to specified video resolution
**
*/
FxBool sst1InitVideo(FxU32 *sstbase,
  GrScreenResolution_t screenResolution, GrScreenRefresh_t screenRefresh,
  sst1VideoTimingStruct *altVideoTiming)
{
    FxU32 n, vtmp;
    // volatile Sstregs *sst = (Sstregs *) sstbase;
    sst1VideoTimingStruct *sstVideoRez;
    FxU32 sst1MonitorRefresh;
    FxU32 sst1MonitorRez;
    FxU32 video16BPP;
    FxU32 memFifoEntries;
    FxU32 memFifoLwm, memFifoHwm, pciFifoLwm;
    FxU32 vInClkDel, vOutClkDel;
    FxU32 tf0_clk_del, tf1_clk_del, tf2_clk_del;
    FxU32 ft_clk_del;

    switch(screenResolution) {
        case(GR_RESOLUTION_512x256):
            sst1MonitorRez = 512256;
            sst1CurrentBoard->fbiVideoWidth = 512;
            sst1CurrentBoard->fbiVideoHeight = 256;
            break;
        case(GR_RESOLUTION_512x384):
            sst1MonitorRez = 512;
            sst1CurrentBoard->fbiVideoWidth = 512;
            sst1CurrentBoard->fbiVideoHeight = 384;
            break;
        case(GR_RESOLUTION_640x400):
            sst1MonitorRez = 640400;
            sst1CurrentBoard->fbiVideoWidth = 640;
            sst1CurrentBoard->fbiVideoHeight = 400;
            break;
        case(GR_RESOLUTION_640x480):
            sst1MonitorRez = 640;
            sst1CurrentBoard->fbiVideoWidth = 640;
            sst1CurrentBoard->fbiVideoHeight = 480;
            break;
        case(GR_RESOLUTION_800x600):
            sst1MonitorRez = 800;
            sst1CurrentBoard->fbiVideoWidth = 800;
            sst1CurrentBoard->fbiVideoHeight = 600;
            break;
        case(GR_RESOLUTION_856x480):
            sst1MonitorRez = 856;
            sst1CurrentBoard->fbiVideoWidth = 856;
            sst1CurrentBoard->fbiVideoHeight = 480;
            break;
        case(GR_RESOLUTION_960x720):
            sst1MonitorRez = 960;
            sst1CurrentBoard->fbiVideoWidth = 960;
            sst1CurrentBoard->fbiVideoHeight = 720;
            break;
        default:
            INIT_PRINTF(("sst1InitVideo(): Unsupported Resolution...\n"));
            return(FXFALSE);
            break;
    }

    switch(screenRefresh) {
        case(GR_REFRESH_60Hz):
            sst1MonitorRefresh = 60;
            break;
        case(GR_REFRESH_70Hz):
            sst1MonitorRefresh = 70;
            break;
        case(GR_REFRESH_72Hz):
            sst1MonitorRefresh = 72;
            break;
        case(GR_REFRESH_75Hz):
            sst1MonitorRefresh = 75;
            break;
        case(GR_REFRESH_80Hz):
            sst1MonitorRefresh = 80;
            break;
        case(GR_REFRESH_85Hz):
            sst1MonitorRefresh = 85;
            break;
        case(GR_REFRESH_90Hz):
            sst1MonitorRefresh = 90;
            break;
        case(GR_REFRESH_100Hz):
            sst1MonitorRefresh = 100;
            break;
        case(GR_REFRESH_120Hz):
            sst1MonitorRefresh = 120;
            break;
        default:
            INIT_PRINTF(("sst1InitVideo(): Unsupported Refresh Rate...\n"));
            return(FXFALSE);
            break;
    }

        switch(sst1MonitorRez) {
            case(512256):
                sstVideoRez = &SST_VREZ_512X256_60;
                break;
            case(512):
                if(sst1MonitorRefresh == 120)
                    sstVideoRez = &SST_VREZ_512X384_120;
                else if(sst1MonitorRefresh == 85)
                    sstVideoRez = &SST_VREZ_512X384_85;
                else if(sst1MonitorRefresh == 75)
                    sstVideoRez = &SST_VREZ_512X384_75;
                // else if(sst1MonitorRefresh == 60 && GETENV(("SST_ARCADE")))
                //     sstVideoRez = &SST_VREZ_512X384_60;
                else {
                    sst1MonitorRefresh = 72;
                    sstVideoRez = &SST_VREZ_512X384_72;
                }
                break;
            case(640400):
                if(sst1MonitorRefresh == 120)
                    sstVideoRez = &SST_VREZ_640X400_120;
                else if(sst1MonitorRefresh == 85)
                    sstVideoRez = &SST_VREZ_640X400_85;
                else if(sst1MonitorRefresh == 75)
                    sstVideoRez = &SST_VREZ_640X400_75;
                else {
                    sst1MonitorRefresh = 70;
                    sstVideoRez = &SST_VREZ_640X400_70;
                }
                break;
            case(640):
                if(sst1MonitorRefresh == 120)
                    sstVideoRez = &SST_VREZ_640X480_120;
                else if(sst1MonitorRefresh == 85)
                    sstVideoRez = &SST_VREZ_640X480_85;
                else if(sst1MonitorRefresh == 75)
                    sstVideoRez = &SST_VREZ_640X480_75;
                else {
                    sst1MonitorRefresh = 60;
                    sstVideoRez = &SST_VREZ_640X480_60;
                }
                break;

            case(800):
                if(sst1MonitorRefresh == 85)
                    sstVideoRez = &SST_VREZ_800X600_85;
                else if(sst1MonitorRefresh == 75)
                    sstVideoRez = &SST_VREZ_800X600_75;
                else {
                    sst1MonitorRefresh = 60;
                    sstVideoRez = &SST_VREZ_800X600_60;
                }
                break;
            case(856):
                sst1MonitorRefresh = 60;
                sstVideoRez = &SST_VREZ_856X480_60;
                break;
            case(960):
                sst1MonitorRefresh = 60;
                sstVideoRez = &SST_VREZ_960X720_60;
                break;
            default:
                INIT_PRINTF(("sst1InitVideo(): Unsupported Resolution %d...\n",
                    screenResolution));
                return(FXFALSE);
                break;
        }
        sst1CurrentBoard->fbiVideoRefresh = sst1MonitorRefresh;

        sst1CurrentBoard->fbiVideo16BPP = 0;

        if(sst1CurrentBoard->fbiVideo16BPP && (sstVideoRez->video16BPPIsOK ==
           FXFALSE))
               sst1CurrentBoard->fbiVideo16BPP = 0;
        if(!sst1CurrentBoard->fbiVideo16BPP && (sstVideoRez->video24BPPIsOK ==
           FXFALSE))
               sst1CurrentBoard->fbiVideo16BPP = 1;

        if(altVideoTiming == (sst1VideoTimingStruct *) NULL) {
            if(sst1CurrentBoard->sstSliDetect &&
              (sst1CurrentBoard->fbiBoardID == 0x1) &&
              (sstVideoRez->clkFreq24bpp > 52.0F)) {
                 sst1CurrentBoard->fbiVideo16BPP = 1;
            } else if(sst1CurrentBoard->fbiBoardID == 0x1) {
                 // Obsidian Pro Board...
                 if(sstVideoRez->clkFreq24bpp > 81.0F)
                    sst1CurrentBoard->fbiVideo16BPP = 1;
            } else {
                if((sst1MonitorRefresh >= 85) || (sst1MonitorRez == 800) ||
                   (sst1MonitorRez == 960))
                    sst1CurrentBoard->fbiVideo16BPP = 1;
            }
        }
    video16BPP = sst1CurrentBoard->fbiVideo16BPP;

    /* Reset Video Refresh Unit */
    GR_SET(OFFSET_fbiInit1, GR_GET(OFFSET_fbiInit1) | SST_VIDEO_RESET);

    GR_SET(OFFSET_hSync, ((sstVideoRez->hSyncOff << SST_VIDEO_HSYNC_OFF_SHIFT) |
                    (sstVideoRez->hSyncOn << SST_VIDEO_HSYNC_ON_SHIFT)));

    GR_SET(OFFSET_vSync, ((sstVideoRez->vSyncOff << SST_VIDEO_VSYNC_OFF_SHIFT) |
                    (sstVideoRez->vSyncOn << SST_VIDEO_VSYNC_ON_SHIFT)));

    GR_SET(OFFSET_backPorch,
                    ((sstVideoRez->vBackPorch << SST_VIDEO_VBACKPORCH_SHIFT) |
                    (sstVideoRez->hBackPorch << SST_VIDEO_HBACKPORCH_SHIFT)));

    GR_SET(OFFSET_videoDimensions,
                     ((sstVideoRez->yDimension << SST_VIDEO_YDIM_SHIFT) |
                      ((sstVideoRez->xDimension-1) << SST_VIDEO_XDIM_SHIFT)));

    /* Setup SST memory mapper for desired resolution */
    if(sst1CurrentBoard->fbiMemSize == 4)
        sst1InitSetResolution(sstbase, sstVideoRez, 1); 
    else
        sst1InitSetResolution(sstbase, sstVideoRez, 0); 

    // nand2mario: don't set clock frequency for now
    /* Calculate graphics clock frequency */
    if(sst1InitCalcGrxClk(sstbase) == FXFALSE)
        return(FXFALSE);

    /* Setup video fifo */
    /* NOTE: Lower values for the video fifo threshold improve video fifo */
    /* underflow problems */
    FxU32 vFifoThresholdVal = sstVideoRez->vFifoThreshold;

    if(sst1CurrentBoard->grxClkFreq < 45)
        /* Lower threshold value for slower graphics clocks */
        vFifoThresholdVal = sstVideoRez->vFifoThreshold - 4;
    GR_SET(OFFSET_fbiInit3, (GR_GET(OFFSET_fbiInit3) & ~SST_VIDEO_FIFO_THRESH) |
        (vFifoThresholdVal << SST_VIDEO_FIFO_THRESH_SHIFT));

    INIT_PRINTF(("sst1InitVideo() Setting up video for resolution (%d, %d), Refresh:%d Hz...\n",
        sstVideoRez->xDimension, sstVideoRez->yDimension, sst1MonitorRefresh));
    INIT_PRINTF(("sst1InitVideo(): Video Fifo Threshold = %d\n",
        (GR_GET(OFFSET_fbiInit3) & SST_VIDEO_FIFO_THRESH) >>
         SST_VIDEO_FIFO_THRESH_SHIFT));

    /* Initialize Y-Origin */
    sst1InitIdleFBINoNOP(sstbase);
    GR_SET(OFFSET_fbiInit3, (GR_GET(OFFSET_fbiInit3) & ~SST_YORIGIN_TOP) |
        ((sstVideoRez->yDimension - 1) << SST_YORIGIN_TOP_SHIFT));

    sst1InitIdleFBINoNOP(sstbase);

    memFifoLwm = 23;
    memFifoHwm = 54;
    pciFifoLwm = 13;
    INIT_PRINTF(("sst1InitVideo(): pciFifoLwm:%d  memFifoLwm:%d  memFifoHwm:%d\n",
        pciFifoLwm, memFifoLwm, memFifoHwm));

    /* Setup Memory FIFO */
    sst1InitIdleFBINoNOP(sstbase);
    GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) &
      ~(SST_MEM_FIFO_ROW_BASE | SST_MEM_FIFO_ROW_ROLL | SST_MEM_FIFO_LWM));
    sst1InitIdleFBINoNOP(sstbase);
    if(sst1CurrentBoard->fbiMemSize == 1)
        GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) |
          (0xff << SST_MEM_FIFO_ROW_ROLL_SHIFT));
    else if (sst1CurrentBoard->fbiMemSize == 2)
        GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) |
          (0x1ff << SST_MEM_FIFO_ROW_ROLL_SHIFT));
    else
        /* 4 MBytes frame buffer memory... */
        GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) |
          (0x3ff << SST_MEM_FIFO_ROW_ROLL_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    if((sst1CurrentBoard->fbiMemSize == 1 && sst1MonitorRez == 512) ||
       (sst1CurrentBoard->fbiMemSize == 1 && sst1MonitorRez == 512256) ||
       (sst1CurrentBoard->fbiMemSize == 2 && sst1MonitorRez == 800) ||
       (sst1CurrentBoard->fbiMemSize == 2 && sst1MonitorRez == 856))
        GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) |
            ((2*sstVideoRez->memOffset) << SST_MEM_FIFO_ROW_BASE_SHIFT));
    else if(sst1CurrentBoard->fbiMemSize < 4 && sst1MonitorRez == 960 &&
        sst1CurrentBoard->sstSliDetect == 0)
        return(FXFALSE);
    else
        GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) |
            ((3*sstVideoRez->memOffset) << SST_MEM_FIFO_ROW_BASE_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);
    GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) |
      (memFifoLwm << SST_MEM_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    /* Set PCI FIFO LWM */
    GR_SET(OFFSET_fbiInit0, (GR_GET(OFFSET_fbiInit0) & ~SST_PCI_FIFO_LWM) |
            (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    /* Enable Memory Fifo... */
    n = 1;

    /* Prohibit illegal memory fifo settings... */
    if(sst1CurrentBoard->fbiMemSize == 1 && sstVideoRez->xDimension > 512)
        n = 0;
    
    if(n) {
        sst1CurrentBoard->fbiMemoryFifoEn = 1;
        if(sst1CurrentBoard->fbiMemSize == 1)
            memFifoEntries = sstVideoRez->memFifoEntries_1MB;
        else if(sst1CurrentBoard->fbiMemSize == 2)
            memFifoEntries = sstVideoRez->memFifoEntries_2MB;
        else
            memFifoEntries = sstVideoRez->memFifoEntries_4MB;
        INIT_PRINTF(("sst1InitVideo(): Enabling Memory FIFO (Entries=%d)...\n",
            0xffff - (memFifoEntries << 5)));

        sst1CurrentBoard->memFifoStatusLwm = (memFifoEntries << 5) | 0x1f;
        GR_SET(OFFSET_fbiInit0, (GR_GET(OFFSET_fbiInit0) & ~(SST_MEM_FIFO_EN |
            SST_MEM_FIFO_HWM | SST_PCI_FIFO_LWM | SST_MEM_FIFO_BURST_HWM)) |
            (memFifoEntries << SST_MEM_FIFO_HWM_SHIFT) |
            (pciFifoLwm << SST_PCI_FIFO_LWM_SHIFT) |
            (memFifoHwm << SST_MEM_FIFO_BURST_HWM_SHIFT) |
            SST_MEM_FIFO_EN);
    }
    INIT_PRINTF(("sst1InitVideo(): Setting memory FIFO LWM to 0x%x (%d)\n",
                 sst1CurrentBoard->memFifoStatusLwm,
                 sst1CurrentBoard->memFifoStatusLwm));  

    vInClkDel = 0;
    if((sst1MonitorRez == 960 && !video16BPP) ||
       (sst1MonitorRez == 640 && !video16BPP && sst1MonitorRefresh == 120) ||
       (sst1MonitorRez == 800 && !video16BPP && sst1MonitorRefresh == 75) ||
       (sst1MonitorRez == 800 && !video16BPP && sst1MonitorRefresh == 85))
        vInClkDel = 2;
    if(sst1CurrentBoard->fbiRevision == 2)
        vInClkDel = 0;

    if(sst1CurrentBoard->fbiRevision == 2)
        vOutClkDel = 2;
    else
        vOutClkDel = 0;

    INIT_PRINTF(("sst1InitVideo(): vInClkDel=0x%x  vOutClkDel=0x%x\n",
        vInClkDel, vOutClkDel));

    /* Drive dac output signals and select input video clock */
    if(video16BPP) {
        INIT_PRINTF(("sst1InitVideo(): Setting 16BPP video mode...\n"));
        GR_SET(OFFSET_fbiInit1, (GR_GET(OFFSET_fbiInit1) & ~SST_VIDEO_MASK) |
            (SST_VIDEO_DATA_OE_EN |
             SST_VIDEO_BLANK_OE_EN |
             SST_VIDEO_HVSYNC_OE_EN |
             SST_VIDEO_DCLK_OE_EN |
             SST_VIDEO_VID_CLK_2X |
             (vInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
             (vOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
             SST_VIDEO_VCLK_SEL));
        sst1InitIdleFBINoNOP(sstbase);
    } else {
        INIT_PRINTF(("sst1InitVideo(): Setting 24BPP video mode...\n"));
        GR_SET(OFFSET_fbiInit1, (GR_GET(OFFSET_fbiInit1) & ~SST_VIDEO_MASK) |
            (SST_VIDEO_DATA_OE_EN |
             SST_VIDEO_BLANK_OE_EN |
             SST_VIDEO_HVSYNC_OE_EN |
             SST_VIDEO_DCLK_OE_EN |
             SST_VIDEO_VID_CLK_2X |
             SST_VIDEO_VCLK_DIV2 |
             (vInClkDel << SST_VIDEO_VCLK_DEL_SHIFT) |
             (vOutClkDel << SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT) |
             SST_VIDEO_24BPP_EN));
        sst1InitIdleFBINoNOP(sstbase);
        INIT_PRINTF(("sst1InitVideo(): Disabling Video Filtering...\n"));
    }

    /* Adjust FT-clock delay */
    if(sst1CurrentBoard->fbiRevision == 2) {
        /* .5 micron */
        if(sst1CurrentBoard->tmuRevision == 1)
            ft_clk_del = SST_FT_CLK_DEL_ADJ_DEFAULT_R21;
        else
            ft_clk_del = SST_FT_CLK_DEL_ADJ_DEFAULT_R20;
    } else {
        /* .6 micron */
        if(sst1CurrentBoard->tmuRevision == 1)
            ft_clk_del = SST_FT_CLK_DEL_ADJ_DEFAULT_R11;
        else
            ft_clk_del = SST_FT_CLK_DEL_ADJ_DEFAULT_R10;
    }

    /* Adjust TF-clock delay */
    if(sst1CurrentBoard->tmuRevision == 1) {
        if(sst1CurrentBoard->fbiRevision == 2) {
            tf0_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R21;
            tf1_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R21;
            tf2_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R21;
        } else {
            tf0_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R11;
            tf1_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R11;
            tf2_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R11;
        }
    } else {
        if(sst1CurrentBoard->fbiRevision == 2) {
            tf0_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R20;
            tf1_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R20;
            tf2_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R20;
        } else {
            tf0_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R10;
            tf1_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R10;
            tf2_clk_del = SST_TEX_TF_CLK_DEL_ADJ_DEFAULT_R10;
        }
    }
    /* Adjust clock delays for .5 micron to account for frequency
       dependencies */
    if(sst1CurrentBoard->fbiRevision == 2 && sst1CurrentBoard->tmuRevision == 1)
    {
        if(sst1CurrentBoard->grxClkFreq >= 55) {
            ft_clk_del = tf0_clk_del = tf1_clk_del = tf2_clk_del = 0x4;
        } else if(sst1CurrentBoard->grxClkFreq >= 50) {
            ft_clk_del = tf0_clk_del = tf1_clk_del = tf2_clk_del = 0x5;
        } else if(sst1CurrentBoard->grxClkFreq >= 48) {
            ft_clk_del = tf0_clk_del = tf1_clk_del = tf2_clk_del = 0x6;
        } else if(sst1CurrentBoard->grxClkFreq >= 45) {
            ft_clk_del = tf0_clk_del = tf1_clk_del = tf2_clk_del = 0x7;
        } else if(sst1CurrentBoard->grxClkFreq >= 43) {
            ft_clk_del = tf0_clk_del = tf1_clk_del = tf2_clk_del = 0x8;
        } else {
            ft_clk_del = tf0_clk_del = tf1_clk_del = tf2_clk_del = 0x9;
        }
    }

    INIT_PRINTF(("sst1InitVideo(): Setting FBI-to-TREX clock delay to 0x%x...\n", ft_clk_del));
    INIT_PRINTF(("sst1InitVideo(): Setting TREX#0 TREX-to-FBI clock delay to 0x%x\n",
        tf0_clk_del));
    INIT_PRINTF(("sst1InitVideo(): Setting TREX#1 TREX-to-FBI clock delay to 0x%x\n",
        tf1_clk_del));
    INIT_PRINTF(("sst1InitVideo(): Setting TREX#2 TREX-to-FBI clock delay to 0x%x\n",
        tf2_clk_del));

    GR_SET(OFFSET_fbiInit3, (GR_GET(OFFSET_fbiInit3) & ~SST_FT_CLK_DEL_ADJ) |
        (ft_clk_del << SST_FT_CLK_DEL_ADJ_SHIFT));
    sst1InitIdleFBINoNOP(sstbase);

    sst1CurrentBoard->tmuInit1[0] = (sst1CurrentBoard->tmuInit1[0] &
        ~SST_TEX_TF_CLK_DEL_ADJ) | (tf0_clk_del<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    sst1CurrentBoard->tmuInit1[1] = (sst1CurrentBoard->tmuInit1[1] &
        ~SST_TEX_TF_CLK_DEL_ADJ) | (tf1_clk_del<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);
    sst1CurrentBoard->tmuInit1[2] = (sst1CurrentBoard->tmuInit1[2] &
        ~SST_TEX_TF_CLK_DEL_ADJ) | (tf2_clk_del<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT);

    const uint32_t SST_TREX0 = (1 << 11);
    const uint32_t SST_TREX1 = (1 << 12);
    const uint32_t SST_TREX2 = (1 << 13);

    GR_SET(SST_TREX0 + OFFSET_trexInit1, sst1CurrentBoard->tmuInit1[0]);
    /* sst1InitIdleFBINoNOP(sstbase); */
    /* Can't use sst1InitIdleFbi because changing the tf clock delay may */
    /* incorrectly put data into the TREX-to-FBI fifo */
    for(n=0; n<100; n++) GR_GET(OFFSET_status);
    GR_SET(SST_TREX1 + OFFSET_trexInit1, sst1CurrentBoard->tmuInit1[1]);
    for(n=0; n<100; n++) GR_GET(OFFSET_status);
    GR_SET(SST_TREX2 + OFFSET_trexInit1, sst1CurrentBoard->tmuInit1[2]);
    for(n=0; n<100; n++) GR_GET(OFFSET_status);

    /* Setup graphics clock */
    if(sst1InitGrxClk(sstbase) == FXFALSE)
        return(FXFALSE);

    /* Setup video mode */
    if(sst1InitSetVidMode(sstbase, video16BPP) == FXFALSE)
        return(FXFALSE);

    /* Adjust Video Clock */
    // if(video16BPP) {
    //     if(sst1InitSetVidClk(sstbase, sstVideoRez->clkFreq16bpp) ==
    //         FXFALSE)
    //         return(FXFALSE);
    // } else {
    //     if(sst1InitSetVidClk(sstbase, sstVideoRez->clkFreq24bpp) ==
    //         FXFALSE)
    //         return(FXFALSE);
    // }

    /* Wait for video clock to stabilize */
    for(n=0; n<200000; n++)
        GR_GET(OFFSET_status);

    /* Run Video Reset Module */
    GR_SET(OFFSET_fbiInit1, GR_GET(OFFSET_fbiInit1) & ~SST_VIDEO_RESET);
    sst1InitIdleFBINoNOP(sstbase);

    GR_SET(OFFSET_fbiInit2, GR_GET(OFFSET_fbiInit2) | SST_EN_DRAM_REFRESH);

    sst1InitIdleFBINoNOP(sstbase);

    INIT_PRINTF(("sst1InitVideo(): Not Clearing Frame Buffer(s)...\n"));

    sst1InitVgaPassCtrl(sstbase, 0);

//  nand2mario: replace with sst1InitIdleLopp()
//    sst1InitIdle(sstbase);
    sst1InitIdleLoop(sstbase);

    // if(((GR_GET(OFFSET_status) & SST_SWAPBUFPENDING) >>
    //   SST_SWAPBUFPENDING_SHIFT) > 0)
    //     sst1InitClearSwapPending(sstbase);

    INIT_PRINTF(("sst1InitVideo() exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

FxBool initEnableTransport( InitFIFOData *info ) {
    FxBool rv = FXTRUE;
    if ( info ) {
        // rv = sst1InitGetDeviceInfo( 0, 
        //                             &sstInfo );
        info->hwDep.vgFIFOData.memFifoStatusLwm = sst1CurrentBoard->memFifoStatusLwm;
    }

// nand2mario: skip pci setup
//    sst1InitCachingOn();

    return rv;
}

/*
** fifoFree is kept in bytes, each fifo entry is 8 bytes, but since there
** are headers involved, we assume an average of 2 registers per 8 bytes
** or 4 bytes of registers stored in every fifo entry
*/
void _grReCacheFifo( FxI32 n )
{
  GrGC *gc = _GlideRoot.curGC;
  uint32_t status = GR_GET(OFFSET_status);
  gc->state.fifoFree = ((status >> SST_MEMFIFOLEVEL_SHIFT) & 0xffff)<<2;
  gc->state.fifoFree -= gc->hwDep.sst1Dep.swFifoLWM + n;
}


/*
** sst1InitMapBoard():
**  Find and map SST-1 board into virtual memory
**
**    Returns:
**      FxU32 pointer to base of SST-1 board if successful mapping occurs
**      FXFALSE if cannot map or find SST-1 board
**
*/
FxU32 * sst1InitMapBoard(FxU32 BoardNumber)
{
    /* Default settings */
    // sst1BoardInfo[BoardNumber].vgaPassthruDisable = 0x0;
    // sst1BoardInfo[BoardNumber].vgaPassthruEnable = SST_EN_VGA_PASSTHRU;
    // sst1BoardInfo[BoardNumber].fbiRegulatorPresent = 0x1;
    // sst1BoardInfo[BoardNumber].fbiVideo16BPP = 0;

    return NULL;
}

/*
** sst1InitRegisters():
**  Initialize registers and memory and return to power-on state
**
**    Returns:
**      FXTRUE if successfully initializes SST-1
**      FXFALSE if cannot initialize SST-1
**
*/
FxBool sst1InitRegisters(FxU32 *sstbase)
{
    FxU32 n, tf_fifo_thresh;
    FxU32 ft_clk_del, tf0_clk_del, tf1_clk_del, tf2_clk_del;
    sst1ClkTimingStruct sstGrxClk;
    int i;

    /* Reset all FBI and TREX Init registers */
    GR_SET(OFFSET_fbiInit0, SST_FBIINIT0_DEFAULT);
    GR_SET(OFFSET_fbiInit1, SST_FBIINIT1_DEFAULT);
    GR_SET(OFFSET_fbiInit2, SST_FBIINIT2_DEFAULT);
    GR_SET(OFFSET_fbiInit3,
        (SST_FBIINIT3_DEFAULT & ~(SST_FT_CLK_DEL_ADJ | SST_TF_FIFO_THRESH)) |
        (ft_clk_del << SST_FT_CLK_DEL_ADJ_SHIFT) |
        (tf_fifo_thresh << SST_TF_FIFO_THRESH_SHIFT));
    GR_SET(OFFSET_fbiInit4, SST_FBIINIT4_DEFAULT);
    sst1InitIdleFBINoNOP(sstbase);  /* Wait until init regs are reset */

    /* Enable Linear frame buffer reads */
    GR_SET(OFFSET_fbiInit1, GR_GET(OFFSET_fbiInit1) | SST_LFB_READ_EN);

    /* Swapbuffer algorithm is based on VSync initially */
    GR_SET(OFFSET_fbiInit2, (GR_GET(OFFSET_fbiInit2) & ~SST_SWAP_ALGORITHM) |
      SST_SWAP_VSYNC);

    /* Enable LFB read-aheads */
    GR_SET(OFFSET_fbiInit4, GR_GET(OFFSET_fbiInit4) | SST_EN_LFB_RDAHEAD);

    /* Enable triangle alternate register mapping */
    GR_SET(OFFSET_fbiInit3, GR_GET(OFFSET_fbiInit3) | SST_ALT_REGMAPPING);

    /* Setup DRAM Refresh */
    GR_SET(OFFSET_fbiInit2, (GR_GET(OFFSET_fbiInit2) & ~SST_DRAM_REFRESH_CNTR) |
        SST_DRAM_REFRESH_16MS);
    sst1InitIdleFBINoNOP(sstbase);
    GR_SET(OFFSET_fbiInit2, GR_GET(OFFSET_fbiInit2) | SST_EN_DRAM_REFRESH);
    sst1InitIdleFBINoNOP(sstbase);

    /* Return all other registers to their power-on state */
    sst1InitIdleFBINoNOP(sstbase);
    sst1InitSetResolution(sstbase, &SST_VREZ_640X480_60, 0);
    sst1InitIdleFBINoNOP(sstbase);

    GR_SET(OFFSET_fbzMode, SST_RGBWRMASK | SST_ZAWRMASK);   /* nand2mario: needs this for writing to RGB buffer */

    GR_SET(OFFSET_c1, 0x0);
    GR_SET(OFFSET_c0, 0x0);
    GR_SET(OFFSET_zaColor, 0x0);
    GR_SET(OFFSET_clipLeftRight, 640);
    GR_SET(OFFSET_clipBottomTop, 480);
    GR_SET(OFFSET_fastfillCMD, 0x0);   /* Frontbuffer & Z/A */
    GR_SET(OFFSET_nopCMD, 0x1); /* Clear fbistat registers after clearing screen */
    sst1InitIdleFBINoNOP(sstbase);

    // if(sst1InitFillDeviceInfo(sstbase, sst1CurrentBoard) == FXFALSE) {
    //     INIT_PRINTF(("sst1InitRegisters(): ERROR filling DeviceInfo...\n"));
    //     return(FXFALSE);
    // }

    /* LFB writes stored in memory FIFO? */
    n = 1;
    if(n) {
        INIT_PRINTF(("sst1InitRegisters(): LFB Writes go through memory FIFO...\n"));
        GR_SET(OFFSET_fbiInit0, GR_GET(OFFSET_fbiInit0) | SST_EN_LFB_MEMFIFO);
        sst1InitIdleFBINoNOP(sstbase);
    }

    /* Texture memory writes stored in memory FIFO? */
    n = 1;
    if(n) {
        INIT_PRINTF(("sst1InitRegisters(): TEXTURE Writes go through memory FIFO...\n"));
        GR_SET(OFFSET_fbiInit0, GR_GET(OFFSET_fbiInit0) | SST_EN_TEX_MEMFIFO);
        sst1InitIdleFBINoNOP(sstbase);
    }

    GR_SET(OFFSET_vRetrace, 0x0);
    GR_SET(OFFSET_backPorch, 0x0);
    GR_SET(OFFSET_videoDimensions, 0x0);
    GR_SET(OFFSET_hSync, 0x0);
    GR_SET(OFFSET_vSync, 0x0);
    GR_SET(OFFSET_videoFilterRgbThreshold, 0x0);

    sst1InitIdleFBINoNOP(sstbase);  /* Wait until init regs are reset */

    INIT_PRINTF(("sst1InitRegisters(): exiting with status %d...\n", FXTRUE));
    return(FXTRUE);
}

static InitDeviceInfo hwInfo[INIT_MAX_DEVICES];     /* Info for all supported devices */
static FxU32  numDevicesInSystem,           /* Number of supported devices. */
              numSst1s;                     /* # SST1s in system */
static FxBool libInitialized;               /* Well, has it been? */
  
/*-------------------------------------------------------------------
  Function: initEnumHardware
  Date: 10/9
  Implementor(s): jdt
  Library: init
  Description:
  Calls a user supplied function on an initialized InitDeviceInfo
  structure for each device in the system.  Calls a user supplied
  callback repeatedly for each device in the system.  The callback
  can stop the enumeration cycle by return a value of FXTRUE.
  Arguments:
  cb - callback function of type INitHWEnumCallback which is called on
       an initialzied InitDeviceInfo structure
  Return:
  none
  -------------------------------------------------------------------*/
void initEnumHardware( InitHWEnumCallback *cb )
{
    FxU32  busLocation;
    FxU32  device;
    if ( !libInitialized ) {

        /* When initializing the Library snoop out all 3Dfx devices 
        and fill a static data structure with pertinant data.  */

        numDevicesInSystem = 0;
        numSst1s = 0;

        FxU32 *base;
        sst1DeviceInfoStruct info;

        hwInfo[0].vendorID  = TDFXVID;
        hwInfo[0].deviceID  = SST1DID;
        hwInfo[0].devNumber = 0;
        hwInfo[0].hwClass   = INIT_VOODOO;

        /* On SST-1 We Have to Initialize the Registers
            to Discover the configuration of the board */
        base = sst1InitMapBoard(numSst1s);
        sst1InitRegisters(base);
        // sst1InitGetDeviceInfo( base, &info );

        hwInfo[0].hwDep.vgInfo.vgBaseAddr = 0;
        hwInfo[0].hwDep.vgInfo.pfxRev    = info.fbiRevision;
        hwInfo[0].hwDep.vgInfo.pfxRam    = info.fbiMemSize;
        hwInfo[0].hwDep.vgInfo.nTFX      = info.numberTmus;
        hwInfo[0].hwDep.vgInfo.tfxRev    = info.tmuRevision;
        hwInfo[0].hwDep.vgInfo.tfxRam    = info.tmuMemSize[0];
        hwInfo[0].hwDep.vgInfo.sliDetect = info.sstSliDetect;
        hwInfo[0].hwDep.vgInfo.slaveBaseAddr = 0;
        hwInfo[0].regs.hwDep.VGRegDesc.baseAddress = base;
        hwInfo[0].regs.hwDep.VGRegDesc.slavePtr = 0;

        numSst1s++;
        numDevicesInSystem++;

        /* Mark the library as initialized */
        libInitialized = FXTRUE;
    }

    if ( cb ) {
        for( device = 0; device < numDevicesInSystem; device++ ) {
            cb( &hwInfo[device] );  
        }
    }
    return;
} /* initEnumHardware */

volatile FxU32*
initMapBoard(const FxU32 boardNum)
{
    volatile FxU32* retVal = NULL;
    FxBool okP = (boardNum < INIT_MAX_DEVICES);
    
    if (okP) {
        InitDeviceInfo* infoP = (hwInfo + boardNum);
        const FxU32 vId = infoP->vendorID;
        const FxU32 dId = infoP->deviceID;
        
        okP = ((vId == TDFXVID) &&
                (dId == SST1DID));

        if (okP) {
            retVal = sst1InitMapBoard(boardNum);
            sst1InitRegisters((FxU32*)retVal);
        }
    }

    return retVal;
}


/*
  Recognized devices depend upon compile time flags
*/
FxBool 
_grSstDetectResources(void)
{
    InitDeviceInfo info;
    int ctx, device;
    FxBool rv = FXFALSE;

    GDBG_INFO((280,"_grSstDetectResources()\n"));

    initEnumHardware( 0 );

    _GlideRoot.hwConfig.num_sst = 0;

    // nand2mario: just 1 device
    info = hwInfo[0];

    if ( info.hwClass == INIT_VOODOO ) {
        int tmu;
        
        _GlideRoot.hwConfig.SSTs[ctx].type = GR_SSTTYPE_VOODOO;
        
        _GlideRoot.GCs[ctx].base_ptr  = info.hwDep.vgInfo.vgBaseAddr;
        _GlideRoot.GCs[ctx].reg_ptr   = info.hwDep.vgInfo.vgBaseAddr;
        _GlideRoot.GCs[ctx].lfb_ptr   = SST_LFB_ADDRESS(info.hwDep.vgInfo.vgBaseAddr);
        _GlideRoot.GCs[ctx].tex_ptr   = SST_TEX_ADDRESS(info.hwDep.vgInfo.vgBaseAddr);
        _GlideRoot.GCs[ctx].slave_ptr = info.hwDep.vgInfo.slaveBaseAddr;
        _GlideRoot.GCs[ctx].grSstRez  = GR_RESOLUTION_NONE;

        _GlideRoot.GCs[ctx].scanline_interleaved = info.hwDep.vgInfo.sliDetect;
        _GlideRoot.GCs[ctx].grSstRefresh         = GR_REFRESH_NONE;
        _GlideRoot.GCs[ctx].num_tmu              = info.hwDep.vgInfo.nTFX;
        _GlideRoot.GCs[ctx].fbuf_size            = info.hwDep.vgInfo.pfxRam;
        _GlideRoot.GCs[ctx].vidTimings           = NULL;
        
        _GlideRoot.hwConfig.num_sst++;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.fbiRev = 
          info.hwDep.vgInfo.pfxRev;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.fbRam =
          info.hwDep.vgInfo.pfxRam;
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.sliDetect =
          info.hwDep.vgInfo.sliDetect;
        
        _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.nTexelfx = 
          info.hwDep.vgInfo.nTFX;

        for (tmu = 0; tmu < _GlideRoot.GCs[ctx].num_tmu; tmu++) {
            _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRam =
            info.hwDep.vgInfo.tfxRam;
            _GlideRoot.hwConfig.SSTs[ctx].sstBoard.VoodooConfig.tmuConfig[tmu].tmuRev =
            info.hwDep.vgInfo.tfxRev;
            
            memset(&_GlideRoot.GCs[ctx].tmu_state[tmu], 0, sizeof(_GlideRoot.GCs[ctx].tmu_state[tmu]));
            _GlideRoot.GCs[ctx].tmu_state[tmu].ncc_mmids[0] = GR_NULL_MIPMAP_HANDLE;
            _GlideRoot.GCs[ctx].tmu_state[tmu].ncc_mmids[1] = GR_NULL_MIPMAP_HANDLE;
            _GlideRoot.GCs[ctx].tmu_state[tmu].total_mem    = info.hwDep.vgInfo.tfxRam<<20;
        }
        rv = FXTRUE;
        ctx++;
    }
  
    return rv;
} /* _grSstDetectResources */

void CPU_Core_Dyn_X86_SaveDHFPUState() {}
void CPU_Core_Dyn_X86_RestoreDHFPUState() {}
