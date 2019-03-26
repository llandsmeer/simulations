// apt install libsdl2 libsdl2-gfx-dev

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>

#define NHOR 100
#define NVER 100
#define DT 5

#define GET(dx, dy) ((*state)[wrap(idy+(dy), NVER)][wrap(idx+(dx), NHOR)])
#define NEXT(dx, dy) ((*state_next)[wrap(idy+(dy), NVER)][wrap(idx+(dx), NHOR)])
#define DGET(dx, dy) ((*dstate)[wrap(idy+(dy), NVER)][wrap(idx+(dx), NHOR)])
#define DNEXT(dx, dy) ((*dstate_next)[wrap(idy+(dy), NVER)][wrap(idx+(dx), NHOR)])

static int wrap(int x, int n) {
    return x>=n ? x-n : (x<0 ? x+n : x);
}

static int bound(int x, int a, int b) {
    return x > b ? b : (x < a ? a : x);
}

static float state_buffer_a[NVER][NHOR];
static float state_buffer_b[NVER][NHOR];
static float dstate_buffer_a[NVER][NHOR];
static float dstate_buffer_b[NVER][NHOR];

static void
swap(
        float (** a)[NHOR][NVER],
        float (** b)[NHOR][NVER]
    ) {
    float (* temp)[NHOR][NVER] = *a;
    *a = *b;
    *b = temp;
}

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
            GET(0, 0) = 2 * randfloat() - 1;
        }
    }
}

static float
getmax(
        float (* state)[NHOR][NVER]
        ) {
    float ret = 100;
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            float value = fabs(GET(0, 0));
            if (value > ret) {
                ret = value;
            }
        }
    }
    return ret;
}

static void
input(
        float (* state)[NHOR][NVER],
        float (* dstate)[NHOR][NVER],
        int idx,
        int idy
     ) {
    if (GET(0, 0) < 30) {
        GET(0, 0) = GET(0, 0) + 10;
    }
}

static void
init(
        float (* state)[NHOR][NVER],
        float (* dstate)[NHOR][NVER]
        ) {
    fill(state, -70);
    fill(dstate, 0);
}

static void
update(
        float (* state)[NHOR][NVER],
        float (* state_next)[NHOR][NVER],
        float (* dstate)[NHOR][NVER],
        float (* dstate_next)[NHOR][NVER]
        ) {
    float dt = 0.01;
    float v, u, d2vdx2, d2vdy2, dvdt, dudt ,I;
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            v = GET(0, 0);
            u = DGET(0, 0);
            d2vdx2 = (GET(0, 1) - 2*v + GET(0, -1));
            d2vdy2 = (GET(1, 0) - 2*v + GET(-1, 0));
            I = 2*(d2vdx2 + d2vdy2);
            if (I < 0) {
                I = 0;
            }
            dvdt = 0.04 * v*v + 5*v + 140 - u + I;
            dudt = 0.02 * (0.2*v - u);
            v = v + dvdt * dt;
            u = u + dudt * dt;
            if (v >= 30) {
                v = -65;
                if (u < 10) {
                    u = u + 8;
                }
            }
            NEXT(0, 0) = v;
            DNEXT(0, 0) = u;
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
    float scale = getmax(state);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);
    float value;
    for (int idy = 0; idy < NVER; idy++) {
        for (int idx = 0; idx < NVER; idx++) {
            value = GET(0, 0);
            r = g = b = 0;
            if (value > 0) {
                r = (int)(0xff * value);
            } else {
                b = (int)(0xff * -value);
            }
            r = g = b = bound((int)(0xff * (0.5+value/2)), 0, 0xff);
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
    float (* dstate_current)[NVER][NHOR] = &dstate_buffer_a;
    float (* dstate_next)[NVER][NHOR] = &dstate_buffer_b;
    float (* state_swap_temp)[NVER][NHOR];
    int mouse_x, mouse_y;
    bool ismousedown = false;
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
    int dt, now, tick_begin = SDL_GetTicks();
    init(state_current, dstate_current);
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
                case SDL_MOUSEBUTTONDOWN:
                    ismousedown = true;
                    break;
                case SDL_MOUSEBUTTONUP:
                    ismousedown = false;
                    break;
                default:
                    break;
            }
        }
        if (ismousedown) {
            SDL_GetMouseState(&mouse_x, &mouse_y);
            input(state_current, dstate_current,
                    mouse_x * NHOR / width, mouse_y * NVER / height);
        }
        if (!running) {
            break;
        }
        now = SDL_GetTicks();
        if ((dt = now - tick_begin) > DT) {
            tick_begin = now;
            update(state_current, state_next, dstate_current, dstate_next);
            swap(&state_current, &state_next);
            swap(&dstate_current, &dstate_next);
            draw(renderer, width, height, state_current);
            SDL_RenderPresent(renderer);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}
