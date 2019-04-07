# Charge

Simple simulation of charged or neutral particles.
When specifying integer square as particle count a grid layout is used, otherwise particles are distributed randomly.
Depending on input particles and densities different crystal lattices or
even coexistance regions are generated.

### Entrypoints:

 - `c [particles=70] [size=5]`
 - `c-large [particles=100] [size=10]`
 - `c-neutral [particles] [size]`
 - `c-non [particles=99]`
 - `c-profile`: run callgrind and produce graph

Depends on `python3-matplotlib`, working C compiler.

<img src=charge.png width=500>

# Ising

Pure OpenGL ising model.
Contains small error. The shader updates the spin field *in place*,
which under 0 field is not noticable, but creates artifacts under high fields.

### Control

<kbd>Up</kbd>/<kbd>Down</kbd> / <kbd>K</kbd>/<kbd>J</kbd> Control temperature
<kbd>Left</kbd>/<kbd>Right</kbd> / <kbd>H</kbd>/<kbd>L</kbd> Control external field
<kbd>q</kbd> Quit
<kbd>0</kbd> Reset external field and beta to 0

Change temperature with up/down

### Run

Depends on `libglfw3-dev`, `libgl1-mesa-dev`, `libglu1-mesa-dev`.

`sh c`

<img src=ising.png width=500>

# Mandel

GPU Mandelbrot renderer, based on code from the Ising model.

<img src=mandel.png width=500>

### Control

<kbd>Up</kbd>/<kbd>Down</kbd> / <kbd>K</kbd>/<kbd>J</kbd> Control temperature
<kbd>Left</kbd>/<kbd>Right</kbd> / <kbd>H</kbd>/<kbd>L</kbd> Control external field
<kbd>q</kbd> Quit
<kbd>0</kbd> Reset external field and beta to 0

### Run

Depends on `libglfw3-dev`, `libgl1-mesa-dev`, `libglu1-mesa-dev`.

`sh c`

<kbd>H</kbd>/<kbd>J</kbd>/<kbd>K</kbd>/<kbd>L</kbd> Move
<kbd>+</kbd>/<kbd>-</kbd> Zoom
<kbd>Q</kbd> Quit

# ExMedia

Really simple excitable media simulations.

### Run

```sh
python3 ./simple.py
python3 ./noisy.py
```

Quit using Esc. Reset using Q. Click to excite cells.

<img src=exmedia.png width=500>

### Dependencies

 - PySDL2 (`pip3 install pysdl2`)
 - Numba (optional with a one line code change)
 - Numpy

# CO

Originally a coupled oscilator simulation, now coupled models of neurons on a grid with diffusion term for interactions.
Model parameterization from [EM Izhikevich](https://www.izhikevich.org/publications/spnet.htm).
Should be easily configurable to solve other PDEs.

<img src=co.png width=500>

### Run

```
sh c
```
