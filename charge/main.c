#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// #define BOUNDED

typedef struct {
  double x;
  double y;
} vec;

typedef struct {
  vec p;
  vec v;
  vec a;
  double c;
  double m;
} part;

typedef struct {
  size_t count;
  part * parts;
  double k;
  double dt;
} sym;

static vec c(double x, double y) {
  vec r;
  r.x = x;
  r.y = y;
  return r;
}

static const double bound = 5;

static const double LJ_S = 0.1;
static const double LJ_E = 20;
static const double MAXSPEED = 5;

static vec scal(double a, vec x) {
  return c(a*x.x, a*x.y);
}

static vec axpy(double a, vec x, vec y) {
  return c(a*x.x+y.x, a*x.y+y.y);
}

static vec xpab(vec x, double a, double b) {
  return c(x.x+a, x.y+b);
}

static double nrm2(vec x) {
  return x.x*x.x + x.y*x.y;
}

static vec diff(vec a, vec b) {
  return c(b.x-a.x, b.y-a.y);
}

static vec sdiff(vec a, vec b) {
  vec d1, d2, d3, d4, d5, d6, d7, d8, d9;
  d1 = diff(a, b);
#ifdef BOUNDED
  d2 = diff(a, xpab(b, bound, 0));
  d3 = diff(a, xpab(b, 0, bound));
  d4 = diff(a, xpab(b, bound, bound));
  d5 = diff(a, xpab(b, -bound, 0));
  d6 = diff(a, xpab(b, 0, -bound));
  d7 = diff(a, xpab(b, -bound, -bound));
  d8 = diff(a, xpab(b, bound, -bound));
  d9 = diff(a, xpab(b, -bound, bound));
  d1 = nrm2(d1) < nrm2(d2) ? d1 : d2;
  d1 = nrm2(d1) < nrm2(d3) ? d1 : d3;
  d1 = nrm2(d1) < nrm2(d4) ? d1 : d4;
  d1 = nrm2(d1) < nrm2(d5) ? d1 : d5;
  d1 = nrm2(d1) < nrm2(d6) ? d1 : d6;
  d1 = nrm2(d1) < nrm2(d7) ? d1 : d7;
  d1 = nrm2(d1) < nrm2(d8) ? d1 : d8;
  d1 = nrm2(d1) < nrm2(d9) ? d1 : d9;
#endif
  return d1;
}

static double force_lj(double r2) {
  double sr6 = pow((LJ_S*LJ_S)/r2, 3);
  double f =  24. * LJ_E / pow(r2, 0.5) * (2*sr6*sr6 - sr6);
  if (f < -10000) f = -10000;
  if (f > 10000) f = 10000;
  return f;
}

static vec force(double k, part a, part b) {
  vec r = sdiff(a.p, b.p);
  double r2 = nrm2(r);
  if (r2 < .01) return c(0, 0);
  return scal(force_lj(r2)+k*a.c*b.c/pow(r2, 3.0/2.0), r);
}

static double rem(double x, double y) {
  x = fmod(x, y);
  if (x < 0) x += y;
  return x;
}

void update_a(sym * s) {
  size_t i, j;
  vec f;
  for (i = 0; i < s->count; i++) {
    s->parts[i].a = c(0.0, 0.0);
  }
  for (i = 0; i < s->count; i++) {
    for (j = 0; j < i; j++) {
      f = force(s->k, s->parts[i], s->parts[j]);
      s->parts[i].a = axpy(1.0/s->parts[i].m, f, s->parts[i].a);
      s->parts[j].a = axpy(-1.0/s->parts[j].m, f, s->parts[j].a);
    }
  }
}

static vec bounded(vec p) {
#ifdef BOUNDED
  return c(rem(p.x, bound), rem(p.y, bound));
#else
  return p;
#endif
}

static void update(sym * s) {
  size_t i;
  double v;
  update_a(s);
  for (i = 0; i < s->count; i++) {
    s->parts[i].v = axpy(s->dt, s->parts[i].a, s->parts[i].v);
    v = pow(nrm2(s->parts[i].v), 0.5);
    if (v > MAXSPEED) {
      s->parts[i].v = scal(MAXSPEED/v, s->parts[i].v);
    } else {
      s->parts[i].v = scal(0.9998, s->parts[i].v);
    }
    s->parts[i].p = bounded(axpy(s->dt, s->parts[i].v, s->parts[i].p));
  }
}

static void psym(sym * s) {
  size_t i;
  for (i = 0; i < s->count; i++) {
    printf("%.2f %.4f %.4f%c", s->parts[i].c, s->parts[i].p.x, s->parts[i].p.y,
           i == s->count-1 ? '\n' : ' ');
  }
}

static void setup_random(sym * s) {
  srand48(time(0));
  size_t i, j;
  int placed, fails = 0;
  for (i = 0; i < s->count; i++) {
    placed = 0;
    while (placed == 0) {
        s->parts[i].p = scal(bound, c(drand48(), drand48()));
        placed = 1;
        for (j = 0; j < i; j++) { 
          if (nrm2(sdiff(s->parts[i].p, s->parts[j].p)) < 4*LJ_S*LJ_S) {
            placed = 0;
          }
        }
        if (placed == 0) {
          fails += 1;
        }
        if (fails > 10000) {
          fprintf(stderr, "too crowded\n");
          exit(1);
        }
    }
    s->parts[i].v = c(0, 0);
    s->parts[i].a = c(0, 0);
    s->parts[i].c = i % 2 == 0 ? -0.4 : 0.4;
    // s->parts[i].c = drand48()*2.0 - 1.0;
    s->parts[i].m = 1;
  }
  fprintf(stderr, "%d\n", fails);
}

static void setup_lattice(sym * s){
  size_t i, j, side = (size_t)sqrt(s->count);
  double sp = bound / (double)side;
  for (i = 0; i < side; i++) {
    for (j = 0; j < side; j++) {
      s->parts[i*side+j].p = c(((double)i+drand48()*0.1)*sp+sp/2, ((double)j+drand48()*0.1)*sp+sp/2);
      s->parts[i*side+j].v = c(0, 0);
      s->parts[i*side+j].a = c(0, 0);
      s->parts[i*side+j].c = ((i & 1) == 0 ? 1 : -1) * ((j & 1) == 0 ? 1 : -1);
      s->parts[i*side+j].m = 1;
    }
  }
}

int main(int argc, char ** argv) {
#ifndef BOUNDED
  printf("nonbounded\n");
#endif
  sym s;
  int i = 1;
  s.count = argc > 1 ? atoi(argv[1]) : 99;
  s.dt = argc > 2 ? atof(argv[2]) : 0.01;
  s.k = -0.3;
  s.parts = malloc(sizeof(part) * s.count);
  if (!s.parts) {
    return -2;
  }
  if (s.count == ((int)sqrt(s.count))*((int)sqrt(s.count))) {
    setup_lattice(&s);
  } else {
    setup_random(&s);
  }
  psym(&s);
  for (;;) {
    update(&s);
    if (i < 100 ? 1 == 1 : i < 1000 ? i % 10 == 0 : i % 200 == 0) {
      psym(&s);
    }
    i += 1;
  }
}
