#include "appglfw.hxx"

#define STB_IMAGE_IMPLEMENTATION
#include "texture.hxx"

// Use typed enums to associate binding attributes with types.
enum typename vattrib_t {
  vattrib_position = vec3,
  vattrib_texcoord = vec2,
};

// Can do it for samplers too.
enum typename sampler_t {
  sampler_color = sampler2D,
};

struct uniforms_t {
  mat4 view_proj;
  float seconds;
};

[[spirv::vert]]
void vert_main() {
  // Create a rotation matrix.
  mat4 rotate = make_rotateY(shader_ubo<0, uniforms_t>.seconds);

  // Rotate the position.
  vec4 position = rotate * vec4(shader_in<vattrib_position>, 1);

  // Write to a builtin variable.
  glvert_Output.Position = uniforms.view_proj * position;

  // Pass texcoord through.
  shader_out<vattrib_texcoord> = shader_in<vattrib_texcoord>;
}

[[spirv::frag]]
void frag_main() {
  // Load the inputs. The locations correspond to the outputs from the
  // vertex shader.
  vec2 texcoord = shader_in<vattrib_texcoord>;
  vec4 color = texture(shader_sampler<sampler_color>, texcoord);

  // Write to a variable template.
  shader_out<0, vec4> = color;
}

////////////////////////////////////////////////////////////////////////////////

struct myapp_t : app_t {
  myapp_t();
  void display() override;
  void create_shaders();
  void create_vao();

  GLuint program;
  GLuint texture;
  GLuint vao;
  GLuint ubo;
};

myapp_t::myapp_t() : app_t("C++ shaders") {
  // Set the camera.
  camera.distance = 4;
  camera.pitch = radians(30.f);

  create_shaders();
  create_vao();
}

void myapp_t::create_shaders() {
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  GLuint shaders[] { vs, fs };
  glShaderBinary(2, shaders, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
    __spirv_data, __spirv_size);

  glSpecializeShader(vs, @spirv(vert_main), 0, nullptr, nullptr);
  glSpecializeShader(fs, @spirv(frag_main), 0, nullptr, nullptr);

  program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
}

void myapp_t::create_vao() {
  // Create the vertex array.
  struct vertex_t {
    vec3 pos;
    uint16_t u, v;
  };

  const vertex_t vertices[36] {
    +1, +1, +1, 0x0000, 0xffff, 
    +1, +1, -1, 0x0000, 0x0000, 
    +1, -1, -1, 0xffff, 0x0000, 
    +1, +1, +1, 0x0000, 0xffff, 
    +1, -1, -1, 0xffff, 0x0000, 
    +1, -1, +1, 0xffff, 0xffff, 
    +1, +1, +1, 0x0000, 0xffff, 
    +1, -1, +1, 0x0000, 0x0000, 
    -1, -1, +1, 0xffff, 0x0000, 
    +1, +1, +1, 0x0000, 0xffff, 
    -1, -1, +1, 0xffff, 0x0000, 
    -1, +1, +1, 0xffff, 0xffff, 
    +1, +1, +1, 0x0000, 0xffff, 
    -1, +1, +1, 0x0000, 0x0000, 
    -1, +1, -1, 0xffff, 0x0000, 
    +1, +1, +1, 0x0000, 0xffff, 
    -1, +1, -1, 0xffff, 0x0000, 
    +1, +1, -1, 0xffff, 0xffff, 
    -1, -1, -1, 0x0000, 0xffff, 
    +1, -1, -1, 0x0000, 0x0000, 
    +1, +1, -1, 0xffff, 0x0000, 
    -1, -1, -1, 0x0000, 0xffff, 
    +1, +1, -1, 0xffff, 0x0000, 
    -1, +1, -1, 0xffff, 0xffff, 
    -1, -1, -1, 0x0000, 0xffff, 
    -1, +1, -1, 0x0000, 0x0000, 
    -1, +1, +1, 0xffff, 0x0000, 
    -1, -1, -1, 0x0000, 0xffff, 
    -1, +1, +1, 0xffff, 0x0000, 
    -1, -1, +1, 0xffff, 0xffff, 
    -1, -1, -1, 0x0000, 0xffff, 
    -1, -1, +1, 0x0000, 0x0000, 
    +1, -1, +1, 0xffff, 0x0000, 
    -1, -1, -1, 0x0000, 0xffff, 
    +1, -1, +1, 0xffff, 0x0000, 
    +1, -1, -1, 0xffff, 0xffff, 
  };
  GLuint vbo;
  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(vbo, sizeof(vertices), vertices, 0);  

  // Create the VAO.
  // Associate vbo into bindingindex 0.
  glCreateVertexArrays(1, &vao);
  glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex_t));

  // Enable vertex attribute 0. Associate with binding index 0.
  glEnableVertexArrayAttrib(vao, vattrib_position);
  glVertexArrayAttribBinding(vao, vattrib_position, 0);
  glVertexArrayAttribFormat(vao, vattrib_position, 3, GL_FLOAT, GL_FALSE, 
    offsetof(vertex_t, pos));

  glEnableVertexArrayAttrib(vao, vattrib_texcoord);
  glVertexArrayAttribBinding(vao, vattrib_texcoord, 0);
  glVertexArrayAttribFormat(vao, vattrib_texcoord, 2, GL_UNSIGNED_SHORT, 
    GL_TRUE, offsetof(vertex_t, u));

  // Make an OpenGL texture.
  texture = load_texture("../assets/thisisfine.jpg");

  // Make a UBO.
  glCreateBuffers(1, &ubo);
  glNamedBufferStorage(ubo, sizeof(uniforms_t), nullptr, 
    GL_DYNAMIC_STORAGE_BIT);
}

void myapp_t::display() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW);

  const float bg[4] { .2, 0, 0, 0 };
  glClearBufferfv(GL_COLOR, 0, bg);
  glClear(GL_DEPTH_BUFFER_BIT);

  glUseProgram(program);
  double timer = glfwGetTime();

  // Set the view matrix.
  int width, height;
  glfwGetWindowSize(window, &width, &height);

  mat4 projection = camera.get_perspective(width, height);
  mat4 view = camera.get_view();

  uniforms_t uniforms;
  uniforms.view_proj = projection * view;
  uniforms.seconds = glfwGetTime();
  glNamedBufferSubData(ubo, 0, sizeof(uniforms), &uniforms);

  // Bind the UBO.
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

  // Bind the texture.
  glBindTextureUnit(sampler_color, texture);

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
}

int main() {
  glfwInit();
  gl3wInit();
  myapp_t app;
  app.loop();
  return 0;
}
