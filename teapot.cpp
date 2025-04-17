/*

3dfx Voodoo 1 examples - teapot

nand2mario, April 2025

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glide.h>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

#include "voodoo_emu.h"

GrHwConfiguration hwconfig;
extern bool key_pressed;
extern void sdl_process_events();

/* A simple 3D vector struct */
typedef struct {
    float x, y, z;
} Vec3;

/* A simple struct for a Glide vertex, including color */
typedef struct {
    float x, y, z;
    float r, g, b, a;
} GlideVertex;

/* Global variables for teapot mesh data */
std::vector<Vec3> teapotVertices;
std::vector<std::vector<int>> teapotFaces;
std::vector<Vec3> vertexNormals;  // Store computed vertex normals

/* Load teapot mesh data from file */
void loadTeapotMesh(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("Error opening teapot mesh file\n");
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            Vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            teapotVertices.push_back(vertex);
        }
        else if (type == "f") {
            std::vector<int> face;
            int v;
            while (iss >> v) {
                face.push_back(v - 1); // Convert to 0-based indexing
            }
            teapotFaces.push_back(face);
        }
    }
    file.close();
    printf("Loaded %zu vertices and %zu faces\n", teapotVertices.size(), teapotFaces.size());
}

/* 
 * A simple function to do a perspective projection and viewport transform
 */
static void projectVertex(const Vec3 *in, float angle, float aspect, GlideVertex *out, float r, float g, float b) {
    // Rotate around Y-axis
    float c = cosf(angle);
    float s = sinf(angle);

    // Simple rotate about Y
    float rx = c * in->x + s * in->z;
    float rz = -s * in->x + c * in->z;
    float ry = in->y;

    // Basic perspective projection (fov ~ 90°, near plane ~ 1, etc.)
    float dist = 7.0f; // distance from camera
    float z = rz + dist; 
    if (z < 0.001f) z = 0.001f;  // Avoid division by zero

    float fov = 1.5f;  // scale factor
    out->x = (rx * fov / z) * 300.0f + 320.0f; // 640/2=320
    out->y = (ry * fov * aspect / z) * 300.0f + 240.0f; // 480/2=240
    out->z = 0.5f + (rz / 10.0f); // keep a stable Z in range [0.1..1], rz is [-2,2]
    
    // Store color per vertex
    out->r = r;
    out->g = g;
    out->b = b;
    out->a = 1.0f;
}

/* Compute the normal of a triangle defined by three vertices */
Vec3 computeFaceNormal(const Vec3& v0, const Vec3& v1, const Vec3& v2) {
    Vec3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    
    // Cross product of edges to get normal
    Vec3 normal = {
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x
    };
    
    // Normalize the normal
    float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length > 0.0f) {
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
    }
    
    return normal;
}

/* Compute vertex normals by averaging adjacent face normals */
void computeVertexNormals() {
    // Initialize vertex normals to zero
    vertexNormals.resize(teapotVertices.size(), {0.0f, 0.0f, 0.0f});
    
    // For each face, add its normal to each of its vertices
    for (const auto& face : teapotFaces) {
        if (face.size() >= 3) {
            Vec3 normal = computeFaceNormal(
                teapotVertices[face[0]],
                teapotVertices[face[1]],
                teapotVertices[face[2]]
            );
            
            // Add this face's normal to each of its vertices
            for (int vertexIndex : face) {
                vertexNormals[vertexIndex].x += normal.x;
                vertexNormals[vertexIndex].y += normal.y;
                vertexNormals[vertexIndex].z += normal.z;
            }
        }
    }
    
    // Normalize all vertex normals
    for (auto& normal : vertexNormals) {
        float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 0.0f) {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }
    }
}

/* Compute lighting intensity based on vertex normal and light direction */
float computeLightingIntensity(const Vec3& normal, const Vec3& lightDir) {
    // Dot product between normal and light direction
    float dot = normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z;
    
    // Clamp to [0,1] range
    if (dot < 0.0f) dot = 0.0f;
    
    // Add some ambient light
    return 0.2f + 0.8f * dot;
}

const float PI = 3.14159265358979323846f;

int main(void) {
    printf("Starting teapot...\n");
    
    // Load teapot mesh data
    loadTeapotMesh("assets/teapotmesh.txt");

    float zmin = 1000000.0f, zmax = -1000000.0f;

    // flip vertically as our zero is top left
    for (auto& vertex : teapotVertices) {
        vertex.y = 2-vertex.y;
        if (vertex.z < zmin) zmin = vertex.z;
        if (vertex.z > zmax) zmax = vertex.z;
    }
    printf("zmin: %f, zmax: %f\n", zmin, zmax);     // [-2,2]
    
    // Compute vertex normals for Gouraud shading
    computeVertexNormals();
    
    Voodoo_Initialize("Glibe experiment - Teapot");
    grGlideInit();
    grSstQueryHardware(&hwconfig);
    grSstSelect(0);
    // upper left origin, double buffer, z-buffer enabled
    grSstWinOpen(0, GR_RESOLUTION_640x480, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1);

    // Set render state
    grCullMode(GR_CULL_NEGATIVE);    
    grFogMode(GR_FOG_DISABLE);

    // Setup z-buffering
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthMask(FXTRUE);
    grDepthBufferFunction(GR_CMP_GREATER);

    // A basic loop rotating the teapot
    float angle = 0.0f;
    float aspect = 1; // aspect ratio
    int done = 0;

    // Light direction (will rotate with the model)
    Vec3 lightDir = {0.5f, 0.7f, 0.5f}; // Light from upper right front
    // Normalize the light direction
    float length = sqrtf(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
    lightDir.x /= length;
    lightDir.y /= length;
    lightDir.z /= length;

    while (1) {
        if (1 /*key_pressed*/) {
            key_pressed = false;
            
            // press any key to advance frame
            angle += 0.01f;
            if (angle > 2*PI) {  // 2π
                angle = 0.0f;
            }

            // Rotate light direction with the model
            float c = cosf(-angle-0.7*PI);
            float s = sinf(-angle-0.7*PI);
            Vec3 rotatedLightDir = {
                c * lightDir.x + s * lightDir.z,
                lightDir.y,
                -s * lightDir.x + c * lightDir.z
            };

            // Clear to black
            grBufferClear(0, 0, 0);
            guColorCombineFunction(GR_COLORCOMBINE_ITRGB);

            // Transform & project all vertices and compute their lighting
            std::vector<GlideVertex> projectedVertices(teapotVertices.size());
            for (size_t i = 0; i < teapotVertices.size(); i++) {
                // Compute lighting intensity for this vertex
                float intensity = computeLightingIntensity(vertexNormals[i], rotatedLightDir);
                projectVertex(&teapotVertices[i], angle, aspect, &projectedVertices[i], intensity, intensity, intensity);
            }

            // Draw all triangles
            for (const auto& face : teapotFaces) {
                if (face.size() >= 3) { // Ensure we have at least 3 vertices
                    GrVertex gv[3];
                    // Convert from our custom GlideVertex to GrVertex
                    for (int j = 0; j < 3; j++) {
                        int idx = face[j];
                        gv[j].x = projectedVertices[idx].x;
                        gv[j].y = projectedVertices[idx].y;
                        gv[j].ooz = 6553.0f * (1.0f / projectedVertices[idx].z); // 1/z for Glide
                        gv[j].oow = 1.0f / projectedVertices[idx].z;              // 1/w is 1/z here
                        gv[j].r = projectedVertices[idx].r * 128.0f;
                        gv[j].g = projectedVertices[idx].g * 255.0f;
                        gv[j].b = projectedVertices[idx].b * 255.0f;
                        gv[j].a = 255.0f;
                    }
                    grDrawTriangle(&gv[0], &gv[1], &gv[2]);
                }
            }

            grBufferSwap(0);            // immediately flip the buffer
            Voodoo_frame_done();        // update SDL display
        }
        sdl_process_events();           // process SDL events
    }
    grGlideShutdown();
} 