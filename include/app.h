#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "png.h"

struct App {
    SDL_Window * window;
    SDL_Renderer * renderer;
    SDL_Event event;
    int running;

    struct Image * img;
};

int AppInit(struct App * app);
void AppDeinit(struct App * app);
void AppRun(struct App * app);
void AppHandleEvents(struct App * app);
void AppDraw(struct App * app);