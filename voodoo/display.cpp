#include <SDL.h>
#include "fxglide.h"
#include "voodoo_emu.h"
#include <string>

void Voodoo_UpdateScreenStart() {
    printf("Voodoo_UpdateScreenStart\n");
}

void Voodoo_Output_Enable(bool enabled) {
    printf("Voodoo_Output_Enable: %d\n", enabled);
}

// for register status read
bool Voodoo_GetRetrace() {
    // printf("Voodoo_GetRetrace\n");
    return false;
}

// for register hvRetrace read
double Voodoo_GetVRetracePosition() {
    printf("Voodoo_GetVRetracePosition\n");
    return 0;
}

double Voodoo_GetHRetracePosition() {
    printf("Voodoo_GetHRetracePosition\n");
    return 0;
}

const int H_RES = 640;
const int V_RES = 480;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
const char *_title;

void Voodoo_Initialize(const char *title) {
    // Init emulated hardware
    v = new voodoo_state;
    v->ogl = false;
	voodoo_init(VOODOO_1);

    // Init glide driver and "detect" hardware
    _grSstDetectResources();

    // initialize SDL window
    SDL_Init(SDL_INIT_VIDEO);
    _title = title;
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, H_RES, V_RES, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1,
							      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_TARGET, H_RES, V_RES);
}

int frame_count;
bool key_pressed;

void Voodoo_frame_done() {
    uint32_t woff = v->fbi.rgboffs[v->fbi.frontbuf];
    printf("Voodoo_frame_done. frontbuf=%d, woff=%d\n", v->fbi.frontbuf, woff);
    SDL_UpdateTexture(texture, NULL, v->fbi.ram + woff, H_RES * 2);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

    frame_count++;
    SDL_SetWindowTitle(window, (std::string(_title) + " - " + std::to_string(frame_count)).c_str());
}

void sdl_process_events() {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            printf("SDL_QUIT\n");
            exit(0);
        } else if (e.type == SDL_KEYDOWN) {
            printf("SDL_KEYDOWN\n");
            key_pressed = true;
        }
    }
}