#include "app.h"

/* Initialize the app with a window (with the size of the image) and a renderer.
   Return 1 on success and 0 on failure. */
int AppInit(struct App * app) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error initializing SDL3: %s.\n", SDL_GetError());
        SDL_Quit();

        return 0;
    }

    app->window = SDL_CreateWindow("Lubaview", app->img->width, app->img->height, 0);

    if(app->window == NULL) {
        fprintf(stderr, "Error creating window: %s.\n", SDL_GetError());
        SDL_Quit();

        return 0;
    }

    app->renderer = SDL_CreateRenderer(app->window, NULL);

    if(app->renderer == NULL) {
        fprintf(stderr, "Error creating renderer: %s.\n", SDL_GetError());
        SDL_Quit();

        return 0;
    }

    return 1;
}

void AppDeinit(struct App * app) {
    if(app->renderer != NULL) {
        SDL_DestroyRenderer(app->renderer);
        app->renderer = NULL;
    }

    if(app->window != NULL) {
        SDL_DestroyWindow(app->window);
        app->window = NULL;
    }

    SDL_Quit();
}

void AppRun(struct App * app) {
    AppDraw(app);

    while(app->running) {
        AppHandleEvents(app);
    }
}

void AppHandleEvents(struct App * app) {
    while(SDL_PollEvent(&app->event)) {
        switch(app->event.type) {
            case SDL_EVENT_KEY_DOWN:
                if(app->event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    SDL_Log("Exiting...\n");
                    app->running = 0;
                }

                break;

            case SDL_EVENT_QUIT:
                SDL_Log("Exiting...\n");
                app->running = 0;

                break;

            default:
                break;
        }
    }
}

void AppDraw(struct App * app) {
    for(int y = 0; y < app->img->height; y++) {
        for(int x = 0; x < app->img->width; x++) {
            int offset = (y * app->img->bpr) + (x * app->img->bpp) + 1;

            uint8_t r = app->img->dataDecomp[offset];
            uint8_t g = app->img->dataDecomp[offset + 1];
            uint8_t b = app->img->dataDecomp[offset + 2];
            uint8_t a = app->img->dataDecomp[offset + 3];

            SDL_SetRenderDrawColor(app->renderer, r, g, b, a);
            SDL_RenderPoint(app->renderer, x, y);
        }
    }

    SDL_RenderPresent(app->renderer);
}