#include <GLXW/glxw.h>
#include <GL/freeglut.h>
#include "array_view.hpp"
#include "shader_utils.hpp"
#include "image_utils.hpp"
#include "gl_error_check.hpp"
#include <iostream>
#include <algorithm>
#include <array>
#include <vector>
#include <tuple>
#include <sstream>

// Range of valid RGB values
struct RGB {
  float r;
  float g;
  float b;
};

// Threshold values on the filtered image
const RGB min_rgb_threshold = { 0.0f, 0.0f, 0.0f };
const RGB max_rgb_threshold = { 0.5f, 1.0f, 1.0f };

// Type unsafe way of converting an integer to a char pointer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// Template helper for 'auto arr = make_array<char>('H', 'e', 'l', 'l', 'o');'
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
      std::copy(list.begin() + 8, list.end()      , std::begin(uv));
    }
    //
    // XYZ coords
    //
    //  ^ Y
    //  |
    //  |
    //  --------------> X
    // / Z (positive is from the monitor towards you)
    //
    // Vertices are usually defined in counter-clockwise order
    float xyzw[4] =  { 0, 0, 0, 1 };
    // RGB and Alpha (0 - transparent, 1 - opaque)
    float rgba[4] =  { 1, 1, 1, 1 };
    //
    // UV coords (also known as ST coords). Always in range [0;1]
    // ------> U
    // |
    // | V
    // v
    //
    // Tiling is possible when the range is exceeded.
    // Remember that openGL considers the first row of a texture image to be
    // the bottom one.
    float uv[2]   =  { 0, 0 };
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

  void setUV(float u, float v) {
    auto temp = make_array<float>( u, v );
    std::copy(temp.begin(), temp.end(), std::begin(this->data_.uv));
  }

  void writeElements(arv::array_view<TexturedVertex::PackedData> view) const {
    view = {
      data_.xyzw[0], data_.xyzw[1], data_.xyzw[2], data_.xyzw[3],
      data_.rgba[0], data_.rgba[1], data_.rgba[2], data_.rgba[3],
      data_.uv[0], data_.uv[1]
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
  static constexpr const int uv_byte_offset = offsetof(PackedData, uv);
private:
  PackedData data_;
};


std::unique_ptr<ShaderProgram> shader_program;
GLuint texture_id;
GLuint filtered_texture_id;
GLuint vaoId;
GLuint vboId;
GLuint vboiId;
GLsizei indices_count;

// Set up a quad
void setupQuad() {
  // Define our quad using 4 vertices of the custom 'TexturedVertex' class
  TexturedVertex v0;
  v0.setXYZ(-0.5f, 0.5f, 0); v0.setRGB(1, 0, 0); v0.setUV(0, 1);
  TexturedVertex v1;
  v1.setXYZ(-0.5f, -0.5f, 0); v1.setRGB(0, 1, 0); v1.setUV(0, 0);
  TexturedVertex v2;
  v2.setXYZ(0.5f, -0.5f, 0); v2.setRGB(0, 0, 1); v2.setUV(1, 0);
  TexturedVertex v3;
  v3.setXYZ(0.5f, 0.5f, 0); v3.setRGB(1, 1, 1); v3.setUV(1, 1);
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
  
  GL_ERROR_CHECK(glGenVertexArrays(1, &vaoId)); // Generate an unused vertex array name
  GL_ERROR_CHECK(glBindVertexArray(vaoId)); // Bind and create a VAO

  GL_ERROR_CHECK(glGenBuffers(1, &vboId));
  GL_ERROR_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vboId)); // Bind and create a VBO (0-sized for now)
  GL_ERROR_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(TexturedVertex::PackedData) * vertices_buffer.size(),
    vertices_buffer.data(), GL_STATIC_DRAW));

  // Set vertex shader attribute 0 - position
  GL_ERROR_CHECK(glVertexAttribPointer(0, TexturedVertex::position_components_count, GL_FLOAT,
    false, TexturedVertex::stride, BUFFER_OFFSET(TexturedVertex::position_byte_offset)));

  // Set vertex shader attribute 1 - color
  GL_ERROR_CHECK(glVertexAttribPointer(1, TexturedVertex::color_components_count, GL_FLOAT,
    false, TexturedVertex::stride, BUFFER_OFFSET(TexturedVertex::color_byte_offset)));

  // Set vertex shader attribute 3 - uv coords
  GL_ERROR_CHECK(glVertexAttribPointer(2, TexturedVertex::uv_components_count, GL_FLOAT,
    false, TexturedVertex::stride, BUFFER_OFFSET(TexturedVertex::uv_byte_offset)));

  GL_ERROR_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0)); // Unbind VBO
  GL_ERROR_CHECK(glBindVertexArray(0)); // Unbind VAO

  // Create a new VBO for the indices and select it (bind it)  
  GL_ERROR_CHECK(glGenBuffers(1, &vboiId));
  GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboiId));
  GL_ERROR_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(char), indices.data(), GL_STATIC_DRAW));
  GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

const std::string vertex_shader_source = { R"(

#version 430

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

)" };

const std::string fragment_shader_source = { R"(

#version 430

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
  GL_ERROR_CHECK(glBindAttribLocation(shader_program->getId(), 0, "in_Position"));
  GL_ERROR_CHECK(glBindAttribLocation(shader_program->getId(), 1, "in_Color"));
  GL_ERROR_CHECK(glBindAttribLocation(shader_program->getId(), 2, "in_TextureCoord"));

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
    
    GL_ERROR_CHECK(glGenTextures(1, &texture_id)); // Create texture object
    GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0)); // Activate texunit 0
    GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id)); // Bind as 2D texture

    // Upload data and generate mipmaps (normalize unsigned values)
    GL_ERROR_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data.data()));
    GL_ERROR_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

    // Set up UV coords 
    GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

    // Set up scaling filters
    GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    
    GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}


// local_size_x/y/z layout variables define the work group size.
// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
// gl_LocalInvocationID is the local index within the work group, and
// gl_WorkGroupID is the work group's index
const std::string gaussian_filter_computeshader_source = { R"(

#version 430

// Invocations in the work group
// Operate on the image in a block of 16x16 "threads"
layout (local_size_x = 16, local_size_y = 16) in;

uniform layout(rgba8, binding = 0) readonly image2D input_texture;
uniform layout(rgba8, binding = 1) writeonly image2D output_texture;
uniform vec3 rgb_min_threshold;
uniform vec3 rgb_max_threshold;

bool in_range(float v, int rgb_index) {
  if (v < rgb_min_threshold[rgb_index] || v > rgb_max_threshold[rgb_index])
    return false;
  else
    return true;
}

void main() {
  // Coordinates of the texel we're about to process
  ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);

  // Read the pixel from the first texture.
  // vec4 pixel = imageLoad(input_texture, texelCoords);
  // imageStore(output_texture, texelCoords, pixel);

  const float gaussian_kernel[25] = float[](1.0, 4.0, 7.0, 4.0, 1.0,
    4.0, 16.0, 26.0, 16.0, 4.0,
    7.0, 26.0, 41.0, 26.0, 7.0,
    4.0, 16.0, 26.0, 16.0, 4.0,
    1.0, 4.0, 7.0, 4.0, 1.0);
  const float kernel_sum = 273.0;

  // Compute a simple gaussian blur
  float result_r = 0.0;
  float result_g = 0.0;
  float result_b = 0.0;
  float result_a = 0.0;
  for(int i=-2; i<=2; ++i) {
    for(int j=-2; j<=2; ++j) {
      int x = texelCoords.x + i;
      int y = texelCoords.y + j;
      vec4 pixel = vec4(0.0, 0.0, 0.0, 255.0);
      if(!(x < 0 || x > 255 || y < 0 || y > 255)) {
        vec4 pixel = imageLoad(input_texture, ivec2(x,y));
        float gauss_val = gaussian_kernel[(j + 2) * 5 + (i + 2)];
        result_r += pixel.r * gauss_val;
        result_g += pixel.g * gauss_val;
        result_b += pixel.b * gauss_val;
        result_a += pixel.a * gauss_val;
      }
    }
  }

  result_r /= kernel_sum;
  result_g /= kernel_sum;
  result_b /= kernel_sum;
  result_a /= kernel_sum;

  // [OT] Example of swapping the red and green channels
  // pixel.rg = pixel.gr;

  // Check for ranges to be in threshold
  if (in_range(result_r, 0) == false || in_range(result_g, 1) == false ||
      in_range(result_b, 2) == false) {
    // White -> out of range
    result_r = 1.0;
    result_g = 1.0;
    result_b = 1.0;
  }
 
  // Now write the modified pixel to the second texture.
  imageStore(output_texture, texelCoords, vec4(result_r, result_g, result_b, result_a));
}

)" };


void gaussianFilterTexture() {

  // Creating the compute shader, and the program object containing the shader
  GLuint progHandle = glCreateProgram();
  GLuint cs = glCreateShader(GL_COMPUTE_SHADER);

  const char *source = gaussian_filter_computeshader_source.c_str();
  glShaderSource(cs, 1, &source, NULL);
  GL_ERROR_CHECK(glCompileShader(cs));
  int rvalue;
  GL_ERROR_CHECK(glGetShaderiv(cs, GL_COMPILE_STATUS, &rvalue));
  if (!rvalue) {
    fprintf(stderr, "Error in compiling the compute shader\n");
    GLchar log[10240];
    GLsizei length;
    glGetShaderInfoLog(cs, 10239, &length, log);
    fprintf(stderr, "Compiler log:\n%s\n", log);
    exit(40);
  }
  GL_ERROR_CHECK(glAttachShader(progHandle, cs));

  GL_ERROR_CHECK(glLinkProgram(progHandle));
  GL_ERROR_CHECK(glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue));
  if (!rvalue) {
    fprintf(stderr, "Error in linking compute shader program\n");
    GLchar log[10240];
    GLsizei length;
    glGetProgramInfoLog(progHandle, 10239, &length, log);
    fprintf(stderr, "Linker log:\n%s\n", log);
    exit(41);
  }
  GL_ERROR_CHECK(glUseProgram(progHandle));

  
  // Create other texture for output

  GL_ERROR_CHECK(glGenTextures(1, &filtered_texture_id)); // Create texture object
  GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1)); // Activate texunit 1
  GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, filtered_texture_id)); // Bind as 2D texture

  // Upload data and generate mipmaps
  GL_ERROR_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));
  GL_ERROR_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

  // Set up UV coords 
  GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  // Set up scaling filters
  GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_ERROR_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

  GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

  // Bind the textures to the compute shader

  GL_ERROR_CHECK(glBindImageTexture(
    0,
    texture_id,
    0,
    GL_FALSE,
    0,
    GL_READ_ONLY,
    GL_RGBA8 // Treat stores as normalized 8-bit unsigned integers
  ));

  GL_ERROR_CHECK(glBindImageTexture(
    1,
    filtered_texture_id,
    0,
    GL_FALSE,
    0,
    GL_WRITE_ONLY,
    GL_RGBA8 // Treat stores as normalized 8-bit unsigned integers
  ));

  // Bind the thresholds
  GLint min_threshold;
  GL_ERROR_CHECK(min_threshold = glGetUniformLocation(progHandle, "rgb_min_threshold"));
  GL_ERROR_CHECK(glUniform3f(min_threshold, min_rgb_threshold.r, min_rgb_threshold.g, min_rgb_threshold.b));
  
  GLint max_threshold;
  GL_ERROR_CHECK(max_threshold = glGetUniformLocation(progHandle, "rgb_max_threshold"));
  GL_ERROR_CHECK(glUniform3f(max_threshold, max_rgb_threshold.r, max_rgb_threshold.g, max_rgb_threshold.b));

  GL_ERROR_CHECK(glDispatchCompute(256 / 16, 256 / 16, 1)); // 256^2 threads in blocks of 16^2

  GL_ERROR_CHECK(glUseProgram(0));
}


void unloadOpenGL() {
  GL_ERROR_CHECK(glDeleteTextures(1, &texture_id));

  // Delete the shaders
  GL_ERROR_CHECK(glUseProgram(0));
  shader_program.release(); // Also detaches shaders before deleting them

  // Select the VAO
  GL_ERROR_CHECK(glBindVertexArray(vaoId));

  // Disable the VBO index from the VAO attributes list
  GL_ERROR_CHECK(glDisableVertexAttribArray(0));
  GL_ERROR_CHECK(glDisableVertexAttribArray(1));

  // Delete the vertex VBO
  GL_ERROR_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
  GL_ERROR_CHECK(glDeleteBuffers(1, &vboId));

  // Delete the index VBO
  GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  GL_ERROR_CHECK(glDeleteBuffers(1, &vboiId));

  // Delete the VAO
  GL_ERROR_CHECK(glBindVertexArray(0));
  GL_ERROR_CHECK(glDeleteVertexArrays(1, &vaoId));
}


// Flag telling us to keep executing the main loop
static int continue_in_main_loop = 1;

static void displayProc(void) {
  
  // Render the scene  
  GL_ERROR_CHECK(glClear(GL_COLOR_BUFFER_BIT));

  GL_ERROR_CHECK(glUseProgram(shader_program->getId()));

  // Bind the texture to texture unit 1
  GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
  GL_ERROR_CHECK(glBindTexture(GL_TEXTURE_2D, filtered_texture_id));

  GLint sampler2D_loc;
  GL_ERROR_CHECK(sampler2D_loc = glGetUniformLocation(shader_program->getId(), "texture_diffuse"));
  GL_ERROR_CHECK(glUniform1i(sampler2D_loc, 1)); // Set texture unit 1 for the sampler2D

  // Bind to the VAO that has all the information about the vertices
  GL_ERROR_CHECK(glBindVertexArray(vaoId));
  GL_ERROR_CHECK(glEnableVertexAttribArray(0));
  GL_ERROR_CHECK(glEnableVertexAttribArray(1));
  GL_ERROR_CHECK(glEnableVertexAttribArray(2));

  // Bind to the index VBO that has all the information about the order of the vertices
  GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboiId));

  // Draw the vertices
  GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_BYTE, 0));

  // Put everything back to default (deselect)
  GL_ERROR_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  GL_ERROR_CHECK(glDisableVertexAttribArray(0));
  GL_ERROR_CHECK(glDisableVertexAttribArray(1));
  GL_ERROR_CHECK(glDisableVertexAttribArray(2));
  GL_ERROR_CHECK(glBindVertexArray(0));

  GL_ERROR_CHECK(glUseProgram(0));

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
  // glMatrixMode(GL_MODELVIEW);
  // glLoadIdentity();
  // glMatrixMode(GL_PROJECTION);
  // glLoadIdentity();
  GL_ERROR_CHECK(glViewport(0, 0, width, height));
  glutPostRedisplay();
}

int main(int argc, char **argv) {
  
  glutInitContextVersion(4, 3);
  //glutInitContextFlags(GLUT_DEBUG);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(300, 300);
  glutInitWindowPosition(140, 140);
  glutCreateWindow("filter");  

  glutKeyboardFunc(keyProc);
  glutDisplayFunc(displayProc);
  glutReshapeFunc(reshapeProc);

  if(glxwInit()) {
    std::cerr << "Failed to initialize GL3W" << std::endl;    
    return 1;
  }
  
  GL_ERROR_CHECK(glClearColor(0.0, 0.0, 0.0, 1));

  std::stringstream ss;
  ss << "OpenGL version supported by this platform: [" << glGetString(GL_VERSION) << "]\n";
  std::cout << ss.str();

  setupQuad();
  loadPNGTexture();
  setupShaders();
  gaussianFilterTexture();

  glutMainLoop(); // Start main window loop - return on close

  unloadOpenGL();

  return 0;
}
