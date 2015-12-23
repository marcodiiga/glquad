#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "array_view.hpp"
#include "shader_utils.hpp"
#include "image_utils.hpp"
#include <algorithm>
#include <array>
#include <vector>
#include <tuple>
#include <sstream>

// Type unsafe way of converting an integer to a char pointer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

template<typename ...T>
auto make_array(T ...args)->std::array<typename std::tuple_element<0, std::tuple<T...>>::type, sizeof...(T)> {
  return std::array<typename std::tuple_element<0, std::tuple<T...>>::type, sizeof...(T)>{ {
      static_cast<typename std::tuple_element<0, std::tuple<T...>>::type>(args)...}};
}

class TexturedVertex {
public:

  // Vertex data
  struct PackedData {
    PackedData() = default;
    PackedData(std::initializer_list<float> list) {
      if (list.size() != 10)
        throw std::runtime_error("Wrong number of arguments");
      std::copy(list.begin()    , list.begin() + 4, std::begin(xyzw));
      std::copy(list.begin() + 4, list.begin() + 8, std::begin(rgba));
      std::copy(list.begin() + 8, list.end()      , std::begin(st));
    }
    float xyzw[4] =  { 0, 0, 0, 1 };
    float rgba[4] =  { 1, 1, 1, 1 };
    float st[2]   =  { 0, 0 };
  };

  static_assert(std::is_standard_layout<PackedData>::value,
                "Need a standard layout packed structure");

  // Getters and setters

  void setXYZW(float x, float y, float z, float w) {
    auto temp = make_array<float>( x, y, z, w );
    std::copy(temp.begin(), temp.end(), std::begin(this->data_.xyzw));
  }

  void setXYZ(float x, float y, float z) {
    this->setXYZW(x, y, z, 1);
  }

  void setRGBA(float r, float g, float b, float a) {
    auto temp = make_array<float>( r, g, b, 1 );
    std::copy(temp.begin(), temp.end(), std::begin(this->data_.rgba));
  }

  void setRGB(float r, float g, float b) {
    this->setRGBA(r, g, b, 1);
  }

  void setST(float s, float t) {
    auto temp = make_array<float>( s, t );
    std::copy(temp.begin(), temp.end(), std::begin(this->data_.st));
  }

  void writeElements(arv::array_view<TexturedVertex::PackedData> view) const {
    view = {
      data_.xyzw[0], data_.xyzw[1], data_.xyzw[2], data_.xyzw[3],
      data_.rgba[0], data_.rgba[1], data_.rgba[2], data_.rgba[3],
      data_.st[0], data_.st[1]
    };
  }

  // Number of components per each data field
  static constexpr const int position_components_count = 4;
  static constexpr const int color_components_count = 4;
  static constexpr const int uv_components_count = 2;

  // Returns the size of TexturedVertex's packed data
  static constexpr const size_t packed_data_size = sizeof(PackedData);

  // Stride between same-component same-field elements in a buffer of adjacent
  // PackedData structures
  static constexpr const size_t stride = sizeof(PackedData);  

  // Byte offsets
  static constexpr const int position_byte_offset = 0;
  static constexpr const int color_byte_offset = offsetof(PackedData, rgba);
  static constexpr const int uv_byte_offset = offsetof(PackedData, st);
private:
  PackedData data_;
};


std::unique_ptr<ShaderProgram> shader_program;
GLuint texture_id;
GLuint vaoId;
GLuint vboId;
GLuint vboiId;
GLsizei indices_count;

// Set up a quad
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
  const int N = 4; // Number of vertices


  // Store the textured vertices data and indices in two host buffers
  constexpr size_t vertex_size = TexturedVertex::packed_data_size;
  std::vector<TexturedVertex::PackedData> vertices_buffer(N);
  v0.writeElements(arv::array_view<TexturedVertex::PackedData>{vertices_buffer, 0});
  v1.writeElements(arv::array_view<TexturedVertex::PackedData>{vertices_buffer, 1});
  v2.writeElements(arv::array_view<TexturedVertex::PackedData>{vertices_buffer, 2});
  v3.writeElements(arv::array_view<TexturedVertex::PackedData>{vertices_buffer, 3});
  // OpenGL expects to draw vertices in counter clockwise order by default
  auto indices = make_array<char>(
    0, 1, 2,
    2, 3, 0
  );
  indices_count = static_cast<GLsizei>(indices.size());

  // Set up VAO and VBO
  
  glGenVertexArrays(1, &vaoId); // Generate an unused vertex array name
  glBindVertexArray(vaoId); // Bind and create a VAO

  glGenBuffers(1, &vboId);
  glBindBuffer(GL_ARRAY_BUFFER, vboId); // Bind and create a VBO (0-sized for now)
  glBufferData(GL_ARRAY_BUFFER, sizeof(TexturedVertex::PackedData) * vertices_buffer.size(),
    vertices_buffer.data(), GL_STATIC_DRAW);

  // Set vertex shader attribute 0 - position
  glVertexAttribPointer(0, TexturedVertex::position_components_count, GL_FLOAT,
    false, TexturedVertex::stride, BUFFER_OFFSET(TexturedVertex::position_byte_offset));

  // Set vertex shader attribute 1 - color
  glVertexAttribPointer(1, TexturedVertex::color_components_count, GL_FLOAT,
    false, TexturedVertex::stride, BUFFER_OFFSET(TexturedVertex::color_byte_offset));

  // Set vertex shader attribute 3 - uv coords
  glVertexAttribPointer(2, TexturedVertex::uv_components_count, GL_FLOAT,
    false, TexturedVertex::stride, BUFFER_OFFSET(TexturedVertex::uv_byte_offset));

  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
  glBindVertexArray(0); // Unbind VAO

  // Create a new VBO for the indices and select it (bind it)  
  glGenBuffers(1, &vboiId);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboiId);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(char), indices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

const std::string vertex_shader_source = {R"(

in vec4 in_Position;
in vec4 in_Color;
in vec2 in_TextureCoord;

out vec4 pass_Color;
out vec2 pass_TextureCoord;

void main(void) {
	gl_Position = in_Position;
	
	pass_Color = in_Color;
	pass_TextureCoord = in_TextureCoord;
}

)"};

const std::string fragment_shader_source = { R"(

uniform sampler2D texture_diffuse;

in vec4 pass_Color;
in vec2 pass_TextureCoord;

out vec4 out_Color;

void main(void) {
	out_Color = pass_Color;
	// Override out_Color with our texture pixel
	out_Color = texture(texture_diffuse, pass_TextureCoord);
}

)" };


void setupShaders() {
  // Load the vertex shader from file
  Shader vertex_textured(GL_VERTEX_SHADER);
  vertex_textured.loadFromString(vertex_shader_source);
  vertex_textured.compile();

  // Load the fragment shader
  Shader fragment_shader(GL_FRAGMENT_SHADER);
  fragment_shader.loadFromString(fragment_shader_source);
  fragment_shader.compile();

  // Create a program which binds the shaders
  shader_program = std::make_unique<ShaderProgram>();
  shader_program->addShader(std::move(vertex_textured));
  shader_program->addShader(std::move(fragment_shader));

  // Bind named attributes in the shader program to 1, 2 and 3 VAO indices
  glBindAttribLocation(shader_program->getId(), 0, "in_Position");
  glBindAttribLocation(shader_program->getId(), 1, "in_Color");
  glBindAttribLocation(shader_program->getId(), 2, "in_TextureCoord");

  shader_program->linkProgram();
  shader_program->validateProgram();
}


void loadPNGTexture() {

    int width, height;
    GLint format;
    std::vector<unsigned char> image_data;

    bool res = loadPNGFromFile("assets/textures/tex1.png", width, height, format, image_data);
    if (res == false)
      throw std::runtime_error("Could not load asset");

    auto isPowerOf2 = [](int val) {
      if (((val - 1) & val) == 0)
        return true;
      else
        return false;
    };
    if (isPowerOf2(width) == false || isPowerOf2(height) == false)
      // This is not true for all implementations, keep it as a safety measure
      throw std::runtime_error("Texture dimensions should be a power of two");
    
    glGenTextures(1, &texture_id); // Create texture object
    glActiveTexture(GL_TEXTURE0); // Activate texunit 0
    glBindTexture(GL_TEXTURE_2D, texture_id); // Bind as 2D texture

    // Upload data and generate mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set up UV coords 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set up scaling filters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

void unloadOpenGL() {
    glDeleteTextures(1, &texture_id);

    // Delete the shaders
    glUseProgram(0);
    shader_program.release(); // Also detaches shaders before deleting them
    
    // Select the VAO
    glBindVertexArray(vaoId);

    // Disable the VBO index from the VAO attributes list
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    // Delete the vertex VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vboId);

    // Delete the index VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vboiId);

    // Delete the VAO
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vaoId);
}


// Flag telling us to keep executing the main loop
static int continue_in_main_loop = 1;

static void displayProc(void) {
  
  // Render the scene  
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(shader_program->getId());

  // Bind the texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);

  // Bind to the VAO that has all the information about the vertices
  glBindVertexArray(vaoId);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  // Bind to the index VBO that has all the information about the order of the vertices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboiId);

  // Draw the vertices
  glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_BYTE, 0);

  // Put everything back to default (deselect)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glBindVertexArray(0);

  glUseProgram(0);

  glutSwapBuffers();
}

static void keyProc(unsigned char key, int x, int y) {
  int need_redisplay = 1;

  switch (key) {
    case 27:  // Escape key
      glutLeaveMainLoop();
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

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main(int argc, char **argv) {
#endif
  int argc = 0;
  char **argv = nullptr;
  // glutInitContextVersion(4, 1);
  // glutInitContextFlags(GLUT_DEBUG);
  glutInitContextProfile(GLUT_CORE_PROFILE); // 4.1 Core Profile context
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(300, 300);
  glutInitWindowPosition(140, 140);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  glutCreateWindow("filter");

  glClearColor(0.0, 0.0, 0.0, 1);

  glutKeyboardFunc(keyProc);
  glutDisplayFunc(displayProc);
  glutReshapeFunc(reshapeProc);

  glewExperimental = GL_TRUE; // Ensure all supported extension are active
  glewInit();

#ifdef _MSC_VER
#ifdef _DEBUG
  std::stringstream ss;
  ss << "OpenGL version supported by this platform: [" << glGetString(GL_VERSION) << "]\n";
  OutputDebugString(ss.str().c_str());
#endif
#endif

  setupQuad();
  setupShaders();
  loadPNGTexture();

  glutMainLoop(); // Start main window loop - return on close

  unloadOpenGL();

  return 0;
}
