#define GLEW_STTIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  GLenum type;
  const char * name;
  const char * template;
} ShaderSource;

static GLuint elements[] = {0, 1, 2, 1, 2, 3};
static const int W = 1000, H = 1000;
static GLuint drawProgram, initProgram, updateProgramEven, updateProgramOdd, vao, tex;
static GLuint seedAttrEven, seedAttrOdd, betaAttrEven, betaAttrOdd, fieldAttrEven, fieldAttrOdd;
static float beta = 0.0, field = 0.0;

static float vertices[] = {
  -1, -1,
  -1,  1,
   1, -1,
   1,  1,
};

static ShaderSource vertexShaderSource = {
  .type = GL_VERTEX_SHADER,
  .name = "vertex",
  .template =
    "#version 150 core\n"
    "in vec2 position;"
    "out vec2 Texcoord;"
    "void main() {"
      "gl_Position = vec4(position, 0.0, 1.0);"
      "Texcoord = vec2(position.x/2.0+0.5, position.y/2.0+0.5);"
    "}"
};

static ShaderSource fragmentShaderSource = {
  .type = GL_FRAGMENT_SHADER,
  .name = "fragment",
  .template = 
    "#version 150 core\n"
    "in vec2 Texcoord;"
    "out vec4 outColor;"
    "uniform sampler2D tex;"
    "void main() {"
      "outColor = texture(tex, Texcoord);"
    "}"
};

static ShaderSource initShaderSource = {
  .type = GL_COMPUTE_SHADER,
  .name = "init",
  .template = 
    "#version 430\n"
    "layout(local_size_x = 1, local_size_y = 1) in;"
    "layout(rgba32f, binding = 0) uniform image2D tex;"
    "float PHI = 1.61803398874989484820459 * 00000.1;"
    "float PI  = 3.14159265358979323846264 * 00000.1;"
    "float SRT = 1.41421356237309504880169 * 10000.0;"
    "float rand(vec2 co) {"
      "return fract(sin(dot(co*PI, vec2(PHI, PI)))*SRT);"
    "}"
    "void main() {"
      "ivec2 coord = ivec2(gl_GlobalInvocationID.xy);"
      "float value = sign(rand(coord)-0.5);"
      "value = value < 0 ? 0 : 1;"
      "vec4 pixel = vec4(value, value, value, 1.0);"
      "imageStore(tex, coord, pixel);"
    "}"
};

static ShaderSource updateShaderSource = {
  .type = GL_COMPUTE_SHADER,
  .name = "update",
  .template = 
    "#version 430\n"
    "layout(local_size_x = 1, local_size_y = 1) in;"
    "layout(rgba32f, binding = 0) uniform image2D tex;"
    "uniform float seed;"
    "uniform float beta;"
    "uniform float field;"
    "float PHI = 1.61803398874989484820459 * 00000.1;"
    "float PI  = 3.14159265358979323846264 * 00000.1;"
    "float SRT = 1.41421356237309504880169 * 10000.0;"
    "float rand(vec2 co) {"
      "return fract(sin(dot(co*seed, vec2(PHI, PI)))*SRT);"
    "}"
    "int getSpinDX(ivec2 base, ivec2 size, int dx) {"
      "ivec2 coord = base + ivec2(dx, 0);"
      "if (coord.x == -1) coord.x = size.x - 1;"
      "else if (coord.x == size.x) coord.x = 0;"
      "float color = imageLoad(tex, coord).x;"
      "return color > 0.5 ? 1 : -1;"
    "}"
    "int getSpinDY(ivec2 base, ivec2 size, int dy) {"
      "ivec2 coord = base + ivec2(0, dy);"
      "if (coord.y == -1) coord.y = size.y - 1;"
      "else if (coord.y == size.y) coord.y = 0;"
      "float color = imageLoad(tex, coord).x;"
      "return color > 0.5 ? 1 : -1;"
    "}"
    "void step(ivec2 base, ivec2 size) {"
      "int spin = getSpinDX(base, size, 0);"
      "float dE = 2 * spin * (field+"
        "getSpinDX(base, size,   1)+"
        "getSpinDX(base, size,  -1)+"
        "getSpinDY(base, size,   1)+"
        "getSpinDY(base, size,  -1));"
        "if (dE < 0.0 || rand(base) < exp(-dE/beta)) {"
          "float flip = spin < 0 ? 1 : 0;"
          "vec4 pixel = vec4(flip, flip, flip, 1.0);"
          "imageStore(tex, base, pixel);"
      "}"
    "}"
    "void main() {"
      "ivec2 size = 2 * ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);"
      "ivec2 base = 2 * ivec2(gl_GlobalInvocationID.xy);"
  "if (rand(base+ivec2(2, 0)) < 0.5)"
      "step(base+ivec2(%d, %d), size);"
  "if (rand(base+ivec2(0, 2)) < 0.5)"
      "step(base+ivec2(%d, %d), size);"
    "}"
};

static GLuint compileShader(ShaderSource shaderSource, ...) {
  GLuint shader = glCreateShader(shaderSource.type);
  GLint status;
  char sourceBuffer[10*1024];
  const char * sourceBufferPtr = sourceBuffer;
  va_list ap;
  va_start(ap, shaderSource);
  status = vsnprintf(sourceBuffer, sizeof(sourceBuffer), shaderSource.template, ap);
  va_end(ap);
  if (status < 0) {
    fprintf(stderr, "=== ERROR: SHADER <%s> - BUFFER TOO SMALL ===\n", shaderSource.name);
    exit(EXIT_FAILURE);
  }
  glShaderSource(shader, 1, &sourceBufferPtr, 0);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    fprintf(stderr, "=== ERROR: SHADER <%s> ===\n", shaderSource.name);
    exit(EXIT_FAILURE);
  }
  return shader;
}

static GLuint buildDrawProgram(ShaderSource vertexShaderSource, ShaderSource fragmentShaderSource) {
  GLuint vertexShader, fragmentShader, program = glCreateProgram();
  vertexShader = compileShader(vertexShaderSource);
  fragmentShader = compileShader(fragmentShaderSource);
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glBindFragDataLocation(program, 0, "outColor"); // unnecessary
  glLinkProgram(program);
  {
    GLuint posAttr = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(posAttr);
    glVertexAttribPointer(posAttr, 2 /* components */, GL_FLOAT, GL_FALSE, 0, 0);
  }
  return program;
}

static GLuint buildComputeProgram(ShaderSource shaderSource) {
  GLuint shader, program = glCreateProgram();
  shader = compileShader(shaderSource);
  glAttachShader(program, shader);
  glLinkProgram(program);
  return program;
}

static GLuint buildUpdateProgram(int dx1, int dy1, int dx2, int dy2) {
  GLuint shader, program = glCreateProgram();
  shader = compileShader(updateShaderSource, dx1, dy1, dx2, dy2);
  glAttachShader(program, shader);
  glLinkProgram(program);
  return program;
}

static void setup() {
  GLuint ebo, vbo;
  float texBorderColor[4] = {1, 0, 0, 1};
  // generate
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &ebo);
  glGenBuffers(1, &vbo);
  glGenTextures(1, &tex);
  // bind
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBindTexture(GL_TEXTURE_2D, tex);
  // config
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
  // texture
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, texBorderColor);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, 0);
  glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
  // compile
  drawProgram = buildDrawProgram(vertexShaderSource, fragmentShaderSource);
  initProgram = buildComputeProgram(initShaderSource);
  updateProgramEven = buildUpdateProgram(0, 1, 1, 0);
  updateProgramOdd = buildUpdateProgram(0, 0, 1, 1);
  // init
  glUseProgram(initProgram);
  glDispatchCompute((GLuint)W, (GLuint)H, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  // random
  srand48(time(0));
  seedAttrEven = glGetUniformLocation(updateProgramEven, "seed");
  seedAttrOdd = glGetUniformLocation(updateProgramOdd, "seed");
  betaAttrEven = glGetUniformLocation(updateProgramEven, "beta");
  betaAttrOdd = glGetUniformLocation(updateProgramOdd, "beta");
  fieldAttrEven = glGetUniformLocation(updateProgramEven, "field");
  fieldAttrOdd = glGetUniformLocation(updateProgramOdd, "field");
}

static void loop(int i) {
  (void)i;
  // update even
  glUseProgram(updateProgramEven);
  glUniform1f(seedAttrEven, drand48());
  glUniform1f(betaAttrEven, beta);
  glUniform1f(fieldAttrEven, field);
  glDispatchCompute((GLuint)W/2, (GLuint)H/2, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  // update odd
  glUseProgram(updateProgramOdd);
  glUniform1f(seedAttrOdd, drand48());
  glUniform1f(betaAttrOdd, beta);
  glUniform1f(fieldAttrOdd, field);
  glDispatchCompute((GLuint)W/2, (GLuint)H/2, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  // draw
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(drawProgram);
  glBindVertexArray(vao);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                     const GLchar * message, const void * userParam) {
  (void)source; (void)type; (void)id; (void)severity; (void)length; (void)userParam;
  fprintf(stderr, "%s\n", message);
}

int main() {
  GLFWwindow * window;
  GLuint unusedIds = 0;
  double speedFactor, currentTime, prevTime = glfwGetTime();
  int windowWidth, windowHeight, i = 0;
  if (!glfwInit()) {
    return EXIT_FAILURE;
  }
  glewExperimental = GL_TRUE;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  window = glfwCreateWindow(W, H, "main", 0, 0);
  glfwMakeContextCurrent(window);
  glewInit();
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(errorCallback, 0);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, GL_TRUE);
  assert((W & 1) == 0 && (W & 1) == 0);
  setup();
  while (!glfwWindowShouldClose(window)) {
    currentTime = glfwGetTime();
    speedFactor = (currentTime-prevTime) * 50.;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    loop(i++);
    glfwSwapBuffers(window);
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
      beta += 0.1 * speedFactor;
      if (beta > 10) {
        beta = 10;
      }
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
      beta -= 0.1 * speedFactor;
      if (beta < 0) {
        beta = 0;
      }
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      field += 0.1 * speedFactor;
      if (field > 2) {
        field = 2;
      }
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
      field -= 0.1 * speedFactor;
      if (field < -2) {
        field = -2;
      }
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
      beta = 0;
      field = 0;
    }
    prevTime = currentTime;
  }
  glfwTerminate();
  return EXIT_SUCCESS;
}
