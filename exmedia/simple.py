import sys
import numba
import time
import numpy as np
import sdl2.ext as sdl2ext
from sdl2.ext.ebs import World, Entity, System
from sdl2 import (SDL_QUIT, SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN, SDL_MOUSEMOTION, SDL_Color,
                  SDL_BlitSurface, SDL_GetPerformanceFrequency, SDL_GetError,
                  SDL_GetPerformanceCounter, SDL_KEYDOWN)

W, H = 50, 50

state = np.zeros((H, W)).astype(int)
state[H//2:,:] = 0


EX = 5
REF = 8
# rest = 0
# excited = 1, 2, 3, 4, 5, EX
# refractory = 6, 7, 8 (EX, ... REF)

def get_dt():
    now = time.monotonic()
    dt = now - get_dt.prev_time
    get_dt.prev_time = now
    return dt
get_dt.prev_time = time.monotonic()

@numba.jit(nopython=True)
def update(state, newstate, dt):
    for idy in range(H):
        for idx in range(W):
            def ex(dx, dy):
                return 1 if 1 <= state[(idy+dy)%H, (idx+dx)%W] <= EX else 0
            mystate = state[idy, idx]
            if mystate == 0:
                excount = (ex(-1, -1) + ex(-1,  0) + ex(-1, +1) +
                           ex( 0, -1) +            + ex( 0, +1) +
                           ex(+1, -1) + ex(+1,  0) + ex(+1, +1))
                if excount >= 3:
                    newstate[idy, idx] = 1
                else:
                    newstate[idy, idx] = 0
            elif mystate == REF:
                newstate[idy, idx] = 0
            else:
                newstate[idy, idx] = mystate + 1
            mystate = newstate[idy, idx]
    return newstate

def draw(surface, width, height):
    blockw = width / W
    blockh = width / H
    for idy in range(H):
        for idx in range(W):
            x, y = idx * blockw, idy * blockw
            value = state[idy, idx]
            if value == 0:
                r, g, b = (0, 0 ,0)
            elif 1 <= value <= EX:
                r, g, b = (0, 0xff//value ,0)
            else:
                r, g, b = (0, 0 ,0xff//(value-EX))
            color = sdl2ext.Color(r, g, b)
            sdl2ext.fill(surface, color, (x, y, blockw, blockh))

def main():
    global state
    newstate = np.copy(state)
    width = 600
    height = 600
    sdl2ext.init()
    window = sdl2ext.Window('GRIDLEARN', size=(width, height))
    window.show()
    surface = window.get_surface()
    running = True
    begin_tick = begin_time = time.monotonic()
    mousedown = False
    while running:
        now = time.monotonic()
        elapsed = now - begin_time
        events = sdl2ext.get_events()
        for event in events:
            if event.type == SDL_QUIT:
                running = False
                break
            elif event.type == SDL_KEYDOWN:
                if event.key.keysym.sym == 27:
                    running = False
                    break
                elif event.key.keysym.sym == 113:
                    state[:] = 0
            elif event.type == SDL_MOUSEBUTTONDOWN:
                mousedown = True
            elif event.type == SDL_MOUSEBUTTONUP:
                mousedown = False
            elif mousedown and event.type == SDL_MOUSEMOTION:
                print(dir(event.button))
                x, y = event.button.x, event.button.y
                idx = int(x / width * W)
                idy = int(y / height * H)
                state[idy, idx] = 1
                break
        if now - begin_tick > 0.1:
            begin_tick = now
            newstate = update(state, newstate, 0.1)
            newstate, state = state, newstate
            draw(surface, width, height)
            window.refresh()
    sdl2ext.quit()

if __name__ == '__main__':
    main()
