#!/usr/bin/env python3

import matplotlib.pyplot as plt

plt.ion()

def read():
    try:
        i = input()
        if i == 'nonbounded':
            b = 100
            ax.set_xlim([-b, b])
            ax.set_ylim([-b, b])
        elif i.startswith('bound'):
            b = float(i.split()[1])
            ax.set_xlim([0, b])
            ax.set_ylim([0, b])
        i = input()
        i = iter(map(float, i.split()))
    except EOFError:
        exit()
    x1 = []
    y1 = []
    x2 = []
    y2 = []
    try:
        while True:
            c = next(i)
            if c > 0:
                x1.append(next(i))
                y1.append(next(i))
            else:
                x2.append(next(i))
                y2.append(next(i))
    except StopIteration:
        return x1, y1, x2, y2


fig = plt.figure()
ax = fig.add_subplot(111)
ax.set_xlim([-1, 2])
ax.set_ylim([-1, 2])
b = 5

ax.set_xlim([0, b])
ax.set_ylim([0, b])

x1, y1, x2, y2 = read()
draw1, = ax.plot(x1, y1, 'o', c='red');
draw2, = ax.plot(x2, y2, 'o', c='blue');

try:
    while plt.fignum_exists(fig.number):
        x1, y1, x2, y2 = read()
        draw1.set_xdata(x1)
        draw1.set_ydata(y1)
        draw2.set_xdata(x2)
        draw2.set_ydata(y2)
        fig.canvas.flush_events()
        fig.canvas.draw()
except KeyboardInterrupt:
    pass
