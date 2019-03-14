import sys
import numba
import time
import numpy as np
import sdl2.ext as sdl2ext
from sdl2.ext.ebs import World, Entity, System
from sdl2 import (SDL_QUIT, SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN, SDL_MOUSEMOTION, SDL_Color,
                  SDL_BlitSurface, SDL_GetPerformanceFrequency, SDL_GetError,
                  SDL_GetPerformanceCounter, SDL_KEYDOWN)

W, H = 100, 100

state = np.zeros((H, W)).astype(int)
state[H//2:,:] = 0
weights = np.random.random((H, W)) * 0.2 + 0.9

X, Y = np.meshgrid(np.linspace(0, 8*np.pi, H), np.linspace(0, 8*np.pi, W))
weights *= 1.0 + (np.sin(Y))/10 + (np.sin(X)) / 10

EX = 5
REF = 10
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
def update(state, newstate, weights, dt):
    for idy in range(H):
        for idx in range(W):
            def ex(dx, dy):
                weight = weights[(idy+dy)%H, (idx+dx)%W]
                return weight if 1 <= state[(idy+dy)%H, (idx+dx)%W] <= EX else 0
            mystate = state[idy, idx]
            if mystate == 0:
                excount = (ex(-1, -1) + ex(-1,  0) + ex(-1, +1) +
                           ex( 0, -1) +            + ex( 0, +1) +
                           ex(+1, -1) + ex(+1,  0) + ex(+1, +1))
                if excount >= 2.9:
                    newstate[idy, idx] = 1
                elif excount >= 1 and np.random.random() > 0.9:
                    newstate[idy, idx] = 1
                else:
                    newstate[idy, idx] = 0
            elif mystate == REF:
                newstate[idy, idx] = 0
            else:
                if np.random.random() > 0.05:
                    newstate[idy, idx] = mystate + 1
                else:
                    newstate[idy, idx] = mystate
            if np.random.random() < 0.25:
                myex = 1 if mystate > 0 else 0
                for dy in -1, 0, 1:
                    for dx in -1, 0, 1:
                        if dy == 0 == dx:
                            continue
                        weight = weights[(idy+dy)%H, (idx+dx)%W]
                        otherex = 1 if state[(idy+dy)%H, (idx+dx)%W] > 0 else 0
                        if myex == 0 == otherex:
                            break
                        if myex != otherex:
                            weights[(idy+dy)%H, (idx+dx)%W] = weight * 0.999
                        elif myex == otherex:
                            weights[(idy+dy)%H, (idx+dx)%W] = (2-weight) * 0.999 + 2 * 0.001

            mystate = newstate[idy, idx]
    return newstate

def draw(surface, width, height):
    blockw = width / W
    blockh = width / H
    for idy in range(H):
        for idx in range(W):
            x, y = idx * blockw, idy * blockw
            value = state[idy, idx]
            w = weights[idy, idx]
            if value == 0:
                r, g, b = (0, 0 ,0)
            elif 1 <= value <= EX:
                r, g, b = (0, 0xff//value ,0)
            else:
                r, g, b = (0, 0 ,0xff//(value-EX))
            r = int(0xff*w/2)
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
            newstate = update(state, newstate, weights, 0.1)
            newstate, state = state, newstate
            draw(surface, width, height)
            window.refresh()
    sdl2ext.quit()

if __name__ == '__main__':
    main()
