/*

3dfx Voodoo 1 Hello world

- draw a triangle
- press any key to advance frame

*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glide.h>

#include "voodoo_emu.h"

GrHwConfiguration hwconfig;
extern bool key_pressed;
extern void sdl_process_events();

int main( void ) { 
    printf("Starting triangle\n");
    Voodoo_Initialize("Glibe experiment - Triangle");

    float color = 255.0;
    puts( "renders a Gouraud-shaded triangle" );
    grGlideInit();
    if ( !grSstQueryHardware( &hwconfig ) ) {
        printf( "main: grSstQueryHardware failed!\n" );
        return 1;
    }
    /* Select SST 0 and open up the hardware */
    grSstSelect( 0 );

    if ( !grSstWinOpen( 0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz, 
            GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT, 2, 0 ) ) {
        printf( "main: grSstWinOpen failed!\n" );
        return 1;
    }
    
    int x = 0;
    while ( 1 ) {
        if (1 /*key_pressed*/) {
            key_pressed = false;
            GrVertex vtx1, vtx2, vtx3;
            grBufferClear( 0, 0, GR_WDEPTHVALUE_FARTHEST );
            guColorCombineFunction( GR_COLORCOMBINE_ITRGB );
            vtx1.x = x+160; vtx1.y = 120; vtx1.r = color; vtx1.g = 0;     vtx1.b = 0;     vtx1.a = 0;
            vtx2.x = x+480; vtx2.y = 180; vtx2.r = 0;     vtx2.g = color; vtx2.b = 0;     vtx2.a = 128;
            vtx3.x = x+320; vtx3.y = 360; vtx3.r = 0;     vtx3.g = 0;     vtx3.b = color; vtx3.a = 255;
            grDrawTriangle( &vtx1, &vtx2, &vtx3 );

            x = x < 200 ? x+1 : 0; 

            grBufferSwap( 0 );          // immediately flip the buffer
            Voodoo_frame_done();        // update display
        }
        sdl_process_events();
    }
    grGlideShutdown();
}