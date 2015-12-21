#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "array_view.hpp"
#include <iostream>
#include <algorithm>
#include <array>
#include <vector>
#include <tuple>
#include <sstream>

template<typename ...T>
auto make_array(T ...args)->std::array<typename std::tuple_element<0, std::tuple<T...>>::type, sizeof...(T)> {
  return std::array<typename std::tuple_element<0, std::tuple<T...>>::type, sizeof...(T)>{ {
      static_cast<typename std::tuple_element<0, std::tuple<T...>>::type>(args)...}};
}

template<typename T>
void reverse_aggregates(std::vector<T>& vec, size_t fields_per_element) {
  if (vec.size() % fields_per_element != 0)
    throw std::runtime_error("Invalid parameters");

  if (vec.size() < 2)
    return;

  for (size_t i = 0; i < (vec.size() / fields_per_element) / 2; ++i) {
    for (size_t j = 0; j < fields_per_element; ++j) {
      std::swap(vec[i * fields_per_element + j], vec[(vec.size() - i * fields_per_element) - fields_per_element + j]);
    }
  }
}

class TexturedVertex {

  // Vertex data
  std::array<float, 4> xyzw = { { 0, 0, 0, 1 } };
  std::array<float, 4> rgba = { { 1, 1, 1, 1 } };
  std::array<float, 2> st = { { 0, 0 } };

public:

  // Getters and setters

  void setXYZW(float x, float y, float z, float w) {
    this->xyzw = { {x, y, z, w} };
  }

  void setXYZ(float x, float y, float z) {
    this->setXYZW(x, y, z, 1);
  }

  void setRGBA(float r, float g, float b, float a) {
    this->rgba = {{r, g, b, 1} };
  }

  void setRGB(float r, float g, float b) {
    this->setRGBA(r, g, b, 1);
  }

  void setST(float s, float t) {
    this->st = { {s, t} };
  }

  void writeElements(arv::array_view<float> view) const {
    view = {
      xyzw[0], xyzw[1], xyzw[2], xyzw[3],
      rgba[0], rgba[1], rgba[2], rgba[3],
      st[0], st[1]
    };
  }

  // Number of components per each data field
  static constexpr const int position_components_count() {
    return static_cast<int>(std::tuple_size<decltype(xyzw)>::value);
  }

  static constexpr const int color_components_count() {
    return static_cast<int>(std::tuple_size<decltype(rgba)>::value);
  }

  static constexpr const int uv_components_count() {
    return static_cast<int>(std::tuple_size<decltype(st)>::value);
  }

  // Returns the size of TexturedVertex's data
  static constexpr const size_t data_size() {
    static_assert(
      position_components_count() >= 1 && position_components_count() <= 4 &&
      position_components_count() >= 1 && position_components_count() <= 4 &&
      position_components_count() >= 1 && position_components_count() <= 4,
      "Only 1,2,3 or 4 components can be bound to a vertex attribute"
    );
    return (position_components_count()
      + color_components_count()
      + uv_components_count()) * sizeof(float);
  }

  // Stride between same-component same-field elements in a buffer of adjacent
  // TexturedVertices
  static constexpr const size_t stride = (std::tuple_size<decltype(xyzw)>::value
    + std::tuple_size<decltype(rgba)>::value + std::tuple_size<decltype(st)>::value)
    * sizeof(float);
  // Byte offsets
  static constexpr const int position_byte_offset = 0;
  static constexpr const int color_byte_offset = position_byte_offset +
    (std::tuple_size<decltype(xyzw)>::value) * sizeof(float);
  static constexpr const int uv_byte_offset = color_byte_offset +
    (std::tuple_size<decltype(rgba)>::value) * sizeof(float);
};

void setupQuad() {
  // Define our quad using 4 vertices of the custom 'TexturedVertex' class
  TexturedVertex v0;
  v0.setXYZ(-0.5f, 0.5f, 0); v0.setRGB(1, 0, 0); v0.setST(0, 0);
  TexturedVertex v1;
  v1.setXYZ(-0.5f, -0.5f, 0); v1.setRGB(0, 1, 0); v1.setST(0, 1);
  TexturedVertex v2;
  v2.setXYZ(0.5f, -0.5f, 0); v2.setRGB(0, 0, 1); v2.setST(1, 1);
  TexturedVertex v3;
  v3.setXYZ(0.5f, 0.5f, 0); v3.setRGB(1, 1, 1); v3.setST(1, 0);


  // Store the textured vertices data and indices in two host buffers
  constexpr size_t vertex_size = TexturedVertex::data_size();
  constexpr size_t vertex_floats_count = vertex_size / sizeof(float);
  std::vector<float> vertices_buffer(vertex_floats_count * 4);
  v0.writeElements(arv::array_view<float>{vertices_buffer, vertex_floats_count * 0, vertex_floats_count});
  v1.writeElements(arv::array_view<float>{vertices_buffer, vertex_floats_count * 1, vertex_floats_count});
  v2.writeElements(arv::array_view<float>{vertices_buffer, vertex_floats_count * 2, vertex_floats_count});
  v3.writeElements(arv::array_view<float>{vertices_buffer, vertex_floats_count * 3, vertex_floats_count});
  auto indices = make_array<char>(
    0, 1, 2,
    2, 3, 0
  );
  // OpenGL expects to draw vertices in counter clockwise order by default
  std::reverse(indices.begin(), indices.end());
  reverse_aggregates(vertices_buffer, vertex_floats_count);

  // Set up VAO and VBO

  GLuint vaoId;
  glGenVertexArrays(1, &vaoId); // Generate an unused vertex array name
  glBindVertexArray(vaoId); // Bind and create a VAO

  GLuint vboId;
  glGenBuffers(1, &vboId);
  glBindBuffer(GL_ARRAY_BUFFER, vboId); // Bind and create a VBO (0-sized for now)
  glBufferData(GL_ARRAY_BUFFER, vertices_buffer.size() * sizeof(float), vertices_buffer.data(), GL_STATIC_DRAW);

  // Set vertex shader attribute 0 - position
  glVertexAttribPointer(0, TexturedVertex::position_components_count(), GL_FLOAT,
    false, TexturedVertex::stride, &TexturedVertex::position_byte_offset);

  // Set vertex shader attribute 1 - color
  glVertexAttribPointer(1, TexturedVertex::color_components_count(), GL_FLOAT,
    false, TexturedVertex::stride, &TexturedVertex::color_byte_offset);

  // Set vertex shader attribute 3 - uv coords
  glVertexAttribPointer(2, TexturedVertex::uv_components_count(), GL_FLOAT,
    false, TexturedVertex::stride, &TexturedVertex::uv_byte_offset);

  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
  glBindVertexArray(0); // Unbind VAO

  // Create a new VBO for the indices and select it (bind it)
  GLuint vboiId;
  glGenBuffers(1, &vboiId);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboiId);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(char), indices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}




































// Flag telling us to keep executing the main loop
static int continue_in_main_loop = 1;

static void displayProc(void) {
  glClear(GL_COLOR_BUFFER_BIT);

  glPushMatrix();

  glColor4f(0.5f, 0.0, 0.0, 1.0f);
  glBegin(GL_TRIANGLES);
    glVertex3f(0.0, 1.0, 0.0);
    glVertex3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
  glEnd();

  glPopMatrix();
  glutSwapBuffers();
}

static void keyProc(unsigned char key, int x, int y) {
  int need_redisplay = 1;

  switch (key) {
    case 27:  // Escape key
      continue_in_main_loop = 0;
      break;

    default:
      need_redisplay = 0;
      break;
  }
  if (need_redisplay)
    glutPostRedisplay();
}

static void reshapeProc(int width, int height) {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, width, height);
  glutPostRedisplay();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  int argc = 0;
  char **argv = nullptr;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(200, 200);
  glutInitWindowPosition(140, 140);
  glutCreateWindow("filter");

  glutKeyboardFunc(keyProc);
  glutDisplayFunc(displayProc);
  glutReshapeFunc(reshapeProc);

  glewInit();
  std::stringstream ss;
  ss << "OpenGL version supported by this platform: [" << glGetString(GL_VERSION) << "]\n";
  OutputDebugString(ss.str().c_str());

  setupQuad();

  while (continue_in_main_loop)
    glutMainLoopEvent();

  return 0;
}
