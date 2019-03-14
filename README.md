# Charge

Simple simulation of charged or neutral particles.
When specifying integer square as particle count a grid layout is used, otherwise particles are distributed randomly.

Entrypoints:

 - `c [particles=70] [size=5]`
 - `c-large [particles=100] [size=10]`
 - `c-neutral [particles] [size]`
 - `c-non [particles=99]`
 - `c-profile`: run callgrind and produce graph

<img src=charge.png width=500>

# Ising

Pure OpenGL ising model.
Contains error.

<img src=ising.png width=500>

# Mandel

Mandelbrot renderer, with code from the Ising model.

<img src=mandel.png width=500>

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