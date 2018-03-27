#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

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
  return d1;
}


static vec force(double k, part a, part b) {
  vec r = sdiff(a.p, b.p);
  double r2 = nrm2(r);
  if (r2 < .01) return c(0, 0);
  return scal(k*a.c*b.c/pow(r2, 3.0/2.0), r);
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
  return c(rem(p.x, bound), rem(p.y, bound));
}

static void update(sym * s) {
  size_t i;
  update_a(s);
  for (i = 0; i < s->count; i++) {
    s->parts[i].v = axpy(s->dt, s->parts[i].a, s->parts[i].v);
    s->parts[i].p = bounded(axpy(s->dt, s->parts[i].v, s->parts[i].p));
  }
}

static void psym(sym * s) {
  size_t i;
  for (i = 0; i < s->count; i++) {
    printf("%.2f %.4f %5.2f%c", s->parts[i].c, s->parts[i].p.x, s->parts[i].p.y,
           i == s->count-1 ? '\n' : ' ');
  }
}

static void setup_random(sym * s) {
  srand48(time(0));
  size_t i;
  for (i = 0; i < s->count; i++) {
    s->parts[i].p = scal(bound, c(drand48(), drand48()));
    s->parts[i].v = c(0, 0);
    s->parts[i].a = c(0, 0);
    s->parts[i].c = drand48()*2.0 - 1.0;
    s->parts[i].m = 1;
  }
}

static void setup_lattice(sym * s){
  size_t i, j, side = (size_t)sqrt(s->count);
  double sp = bound / (double)side;
  for (i = 0; i < side; i++) {
    for (j = 0; j < side; j++) {
      s->parts[i*side+j].p = c(((double)i+drand48()*0.1)*sp+sp/2, ((double)j+drand48()*0.1)*sp+sp/2);
      s->parts[i*side+j].v = c(0, 0);
      s->parts[i*side+j].a = c(0, 0);
      s->parts[i*side+j].c = 1; ((i & 1) == 0 ? 1 : -1) * ((j & 1) == 0 ? 1 : -1);
      s->parts[i*side+j].m = 1;
    }
  }
}

int main(int argc, char ** argv) {
  sym s;
  s.count = argc > 1 ? atoi(argv[1]) : 10;
  s.dt = argc > 2 ? atof(argv[2]) : 0.1;
  s.k = -0.5;
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
    psym(&s);
  }
}
