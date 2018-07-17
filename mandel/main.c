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

static const int W = 300, H = 300;
static int niter = 256;
static const float MOVESPEED = 0.1;
static GLuint drawProgram, vao, zoomAttr, offxAttr, offyAttr, niterAttr, ratioAttr;
static float zoom = 1.0, offx = 0.0, offy = 0.0, ratio = 1.0;

static GLuint elements[] = {0, 1, 2, 1, 2, 3};
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
    "out vec2 coord;"
    "uniform float zoom;"
    "uniform float offx;"
    "uniform float offy;"
    "uniform float ratio;"
    "void main() {"
      "gl_Position = vec4(position, 0.0, 1.0);"
      "coord = vec2(position.x/ratio*2.0/zoom+offx, position.y*2.0/zoom+offy);"
    "}"
};

static ShaderSource fragmentShaderSource = {
  .type = GL_FRAGMENT_SHADER,
  .name = "fragment",
  .template = 
    "#version 150 core\n"
    "in vec2 coord;"
    "out vec4 outColor;"
    "uniform int niter;"
    "void main() {"
      "float a = 0;"
      "float b = 0;"
      "float u = coord.x;"
      "float v = coord.y;"
      "float an;"
      "float l2;"
      "int i = 0;"
      "while (i < niter && a*a + b*b < 2*2) {"
        "an = a*a - b*b + u;"
        "b = 2*a*b + v;"
        "a = an;"
        "i += 1;"
      "}"
      "float norm = float(i) / float(niter);"
      "outColor = vec4(1-sin(norm*2*3.14)/2-0.5, norm, 1-cos(norm*2*3.14)/2-0.5, 1.0);"
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

static void setup() {
  GLuint ebo, vbo;
  // generate
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &ebo);
  glGenBuffers(1, &vbo);
  // bind
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  // config
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
  // compile
  drawProgram = buildDrawProgram(vertexShaderSource, fragmentShaderSource);
  glUseProgram(drawProgram);
  // zoom
  zoomAttr = glGetUniformLocation(drawProgram, "zoom");
  offxAttr = glGetUniformLocation(drawProgram, "offx");
  offyAttr = glGetUniformLocation(drawProgram, "offy");
  niterAttr = glGetUniformLocation(drawProgram, "niter");
  ratioAttr = glGetUniformLocation(drawProgram, "ratio");
}

static void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                     const GLchar * message, const void * userParam) {
  (void)source; (void)type; (void)id; (void)severity; (void)length; (void)userParam;
  fprintf(stderr, "%s\n", message);
}

int main() {
  GLFWwindow * window;
  GLuint unusedIds = 0;
  int windowWidth, windowHeight;
  if (!glfwInit()) {
    return EXIT_FAILURE;
  }
  glewExperimental = GL_TRUE;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
  window = glfwCreateWindow(W, H, "main", 0, 0);
  glfwMakeContextCurrent(window);
  glewInit();
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(errorCallback, 0);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, GL_TRUE);
  assert((W & 1) == 0 && (H & 1) == 0);
  setup();
  while (!glfwWindowShouldClose(window)) {
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    ratio = (float)windowHeight / (float)windowWidth;
    // begin draw
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1f(zoomAttr, zoom);
    glUniform1f(offxAttr, offx);
    glUniform1f(offyAttr, offy);
    glUniform1i(niterAttr, niter);
    glUniform1f(ratioAttr, ratio);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // end draw
    glfwSwapBuffers(window);
    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) offy += MOVESPEED/zoom;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) offy -= MOVESPEED/zoom;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) offx += MOVESPEED/zoom;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) offx -= MOVESPEED/zoom;
    if (glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS) niter += 5;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
      niter -= 5;
      if (niter < 5) {
        niter = 5;
      }
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) zoom *= 1.1;
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) zoom /= 1.1;
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
      offx = offy = 0;
      zoom = 1.0;
      niter = 256;
    }
  }
  glfwTerminate();
  return EXIT_SUCCESS;
}
