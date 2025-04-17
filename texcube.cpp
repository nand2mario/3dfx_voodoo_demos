/*

3dfx Voodoo 1 examples - texcube

Texture mapping and lighting

*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glide.h>

#include "voodoo_emu.h"

const char *TEXTURE_FILE = "assets/miro.3df";
// const char *TEXTURE_FILE = "assets/stone2.3df";

GrHwConfiguration hwconfig;
extern bool key_pressed;
extern void sdl_process_events();

/* A simple 3D vector struct */
typedef struct {
    float x, y, z;
} Vec3;

/* A simple struct for a Glide vertex, including texture coordinates */
typedef struct {
    float x, y, z;
    float s, t;  // texture coordinates
} GlideVertex;

/* Vertices of a cube in model space, centered at origin */
static const Vec3 cubeVertices[8] = {
    { -1.f, -1.f, -1.f }, // 0
    {  1.f, -1.f, -1.f }, // 1
    {  1.f,  1.f, -1.f }, // 2
    { -1.f,  1.f, -1.f }, // 3
    { -1.f, -1.f,  1.f }, // 4
    {  1.f, -1.f,  1.f }, // 5
    {  1.f,  1.f,  1.f }, // 6
    { -1.f,  1.f,  1.f }, // 7
};

/* Indices for the 12 triangles making up a cube (2 per face) */
// nand2mario: we are looking at the cube from the back side, so the front is invisible
static const unsigned short cubeIndices[36] = {
    // Each face is two triangles, vertex indices in counterclockwise order
    // Front face (z = 1)
    4, 5, 6,            
    4, 6, 7,
    // Back face (z = -1)
    0, 3, 2,
    0, 2, 1,
    // Left face (x = -1)
    0, 4, 7,
    0, 7, 3,
    // Right face (x = 1)
    1, 2, 6,
    1, 6, 5,
    // Top face (y = 1)
    3, 7, 6,
    3, 6, 2,
    // Bottom face (y = -1)
    0, 1, 5,
    0, 5, 4
};

/* 
 * A simple function to do a perspective projection and viewport transform. 
 * For a real application, you'd use your own matrix library.
 */
static void projectVertex(const Vec3 *in, float angle, float aspect, GlideVertex *out) {
    // Rotate around Y-axis
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);

    // Simple rotate about Y
    float rx = cos_angle * in->x + sin_angle * in->z;
    float rz = -sin_angle * in->x + cos_angle * in->z;
    float ry = in->y;

    // Set texture coordinates
    if (in->z > 0) { // front face
        out->s = (in->x + 1.0f) * 0.5f;  // 0..1
        out->t = (in->y + 1.0f) * 0.5f;
    } else if (in->z < 0) { // back face
        out->s = (1.0f - in->x) * 0.5f;
        out->t = (in->y + 1.0f) * 0.5f;
    } else if (in->x < 0) { // left face
        out->s = (in->z + 1.0f) * 0.5f;
        out->t = (in->y + 1.0f) * 0.5f;
    } else if (in->x > 0) { // right face
        out->s = (1.0f - in->z) * 0.5f;
        out->t = (in->y + 1.0f) * 0.5f;
    } else if (in->y > 0) { // top face
        out->s = (in->x + 1.0f) * 0.5f;
        out->t = (in->z + 1.0f) * 0.5f;
    } else { // bottom face
        out->s = (in->x + 1.0f) * 0.5f;
        out->t = (1.0f - in->z) * 0.5f;
    }

    // Perspective projection (fov ~ 90°, near plane ~ 1, etc.)
    float dist = 4.0f; // distance from camera
    float z_cam = rz + dist;    // range: 3..5

    float fov = 1.0f;  // scale factor
    out->x = (rx * fov / z_cam) * 300.0f + 320.0f; // 640/2=320
    out->y = (ry * fov * aspect / z_cam) * 300.0f + 240.0f; // 480/2=240
    out->z = z_cam; // keep a stable Z in range [0.1..1], rz is [-1,1]
}

int main( void ) { 
    printf("Starting cube...\n");
    Voodoo_Initialize("texcube");
    grGlideInit();
    grSstQueryHardware( &hwconfig );
    grSstSelect( 0 );
    // upper left origin, double buffer, z-buffer enabled
    grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1);

    // Set render state - flat shading or smooth shading, cull mode, etc.
    // grCullMode(GR_CULL_POSITIVE);
    grCullMode(GR_CULL_DISABLE);
    grFogMode(GR_FOG_DISABLE);

    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthMask(FXTRUE);
    grDepthBufferFunction(GR_CMP_GREATER);      // larger ooz is closer

    // Load texture
    Gu3dfInfo info;
    GrMipMapId_t mmid;
    
    if (!gu3dfGetInfo(TEXTURE_FILE, &info)) {
        fprintf(stderr, "Failed to get texture info\n");
        return -1;
    }
    
    info.data = malloc(info.mem_required);
    if (!gu3dfLoad(TEXTURE_FILE, &info)) {
        fprintf(stderr, "Failed to load texture\n");
        free(info.data);
        return -1;
    }

    // Allocate texture memory
    mmid = guTexAllocateMemory(0, 0x3, 
                              info.header.width, info.header.height,
                              info.header.format,
                              GR_MIPMAP_NEAREST_DITHER,
                              info.header.small_lod, info.header.large_lod,
                              info.header.aspect_ratio,
                              GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                              GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR,
                              0.0F,
                              FXFALSE);

    // Download texture data
    guTexDownloadMipMap(mmid, info.data, &info.table.nccTable);
    free(info.data);

    // A basic loop rotating the cube
    float angle = 0.0f;
    float aspect = 1; // aspect ratio
    int done = 0;

    while (1) {
        if (1 /*key_pressed*/) {
            key_pressed = false;
            
            // press any key to advance frame
            // Simple event/polling or time-based break – in a real system, 
            // you'd check for user input or a close event
            angle += 0.01f;
            if (angle > 6.283185f) {  // 2π
                angle = 0.0f;
            }

            // Clear to black
            grBufferClear(0, 0, 0 /*0xFFFF*/);

            // Set up texture combining
            guTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_DECAL);
            guColorCombineFunction(GR_COLORCOMBINE_DECAL_TEXTURE);
            guTexSource(mmid);

            // Transform & project all 8 cube corners
            GlideVertex vtx[8];
            for (int i = 0; i < 8; i++) {
                projectVertex(&cubeVertices[i], angle, aspect, &vtx[i]);
            }

            // Draw all triangles
            // In Glide 2, we can use grDrawTriangle() with three specific "GrVertex" items,
            // but we have to pass them as pointers to structures. 
            // We'll do them one at a time for clarity:
            // for (int i = 6; i < 7; i += 3) {
            for (int i = 0; i < 36; i += 3) {
                GrVertex gv[3];
                // Convert from our custom GlideVertex to GrVertex
                // (They have a slightly different layout in older Glide versions.)
                for (int j = 0; j < 3; j++) {
                    int idx = cubeIndices[i + j];
                    gv[j].x = vtx[idx].x;
                    gv[j].y = vtx[idx].y;
                    gv[j].oow = 1.0f / vtx[idx].z;                          // 1/w is 1/z here
                    gv[j].ooz = 65535.0f * gv[j].oow;                       // 1/z for Glide
                    gv[j].tmuvtx[0].sow = vtx[idx].s * 255.0f * gv[j].oow;  // texture coordinates
                    gv[j].tmuvtx[0].tow = vtx[idx].t * 255.0f * gv[j].oow;
                }
                // printf("Drawing triangle %i: (%.3f, %.3f, %.3f) - (%.3f, %.3f, %.3f) - (%.3f, %.3f, %.3f)\n", i, 
                //         gv[0].x, gv[0].y, gv[0].oow, gv[1].x, gv[1].y, gv[1].oow, gv[2].x, gv[2].y, gv[2].oow);
                // printf("Color              : (%.0f, %.0f, %.0f) - (%.0f, %.0f, %.0f) - (%.0f, %.0f, %.0f)\n", 
                //         gv[0].r, gv[0].g, gv[0].b, gv[1].r, gv[1].g, gv[1].b, gv[2].r, gv[2].g, gv[2].b);
                // Draw the triangle
                grDrawTriangle(&gv[0], &gv[1], &gv[2]);
            }

            grBufferSwap(0);            // immediately flip the buffer
            Voodoo_frame_done();        // update SDL display
        }
        sdl_process_events();           // process SDL events
    }
    grGlideShutdown();
}