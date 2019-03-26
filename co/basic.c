// apt install libsdl2 libsdl2-gfx-dev

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>

#define NHOR 100
#define NVER 100
#define DT 20

#define WRAP(x, n) (x>=n ? x-n : (x<0 ? x+n : x))
#define GET(dx, dy) ((*state)[WRAP(idy+(dy), NVER)][WRAP(idx+(dx), NHOR)])
#define NEXT(dx, dy) ((*state_next)[WRAP(idy+(dy), NVER)][WRAP(idx+(dx), NHOR)])

static float state_buffer_a[NVER][NHOR];
static float state_buffer_b[NVER][NHOR];

static void
fill(
        float (* state)[NHOR][NVER],
        float value
        ) {
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            GET(0, 0) = value;
        }
    }
}

static float
randfloat(
        ) {
    return (float)rand()/(float)(RAND_MAX);
}

static void
fill_random(
        float (* state)[NHOR][NVER]
        ) {
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            GET(0, 0) = randfloat();
        }
    }
}

static void
update(
        float (* state)[NHOR][NVER],
        float (* state_next)[NHOR][NVER]
        ) {
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            NEXT(0, 0) = GET(0, +1); 
        }
    }
}

static void
draw(
        SDL_Renderer * renderer,
        int width,
        int height,
        float (* state)[NHOR][NVER]
        ) {
    SDL_Rect rect;
    int r, g, b, a = 0xff,
        blockw = width / NHOR,
        blockh = height / NVER;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);   float value;
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            value = (*state)[idy][idx];
            r = g = b = (int)(0xff * value);
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            rect = (SDL_Rect){ idx*blockw, idy*blockh, blockw, blockh };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

int
main(
        int argc,
        char ** argv
        ) {
    int width = 600;
    int height = 600;
    bool running = true;
    float (* state_current)[NVER][NHOR] = &state_buffer_a;
    float (* state_next)[NVER][NHOR] = &state_buffer_b;
    float (* state_swap_temp)[NVER][NHOR];
    SDL_Event event;
    SDL_Window * window = SDL_CreateWindow(
            "CO",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN
            );
    SDL_Renderer * renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
            );
    SDL_SetRenderDrawBlendMode(
            renderer,
            SDL_BLENDMODE_BLEND
            );
    fill_random(state_current);
    int dt, now, tick_begin = SDL_GetTicks();
    draw(renderer, width, height, state_current);
    SDL_RenderPresent(renderer);
    for (;;) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                default:
                    break;
            }
        }
        if (!running) {
            break;
        }
        now = SDL_GetTicks();
        if ((dt = now - tick_begin) > DT) {
            tick_begin = now;
            update(state_current, state_next);
            state_swap_temp = state_current;
            state_current = state_next;
            state_next = state_swap_temp;
            draw(renderer, width, height, state_current);
            SDL_RenderPresent(renderer);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}
