#include <iostream>
#include <fstream>
#include <cerrno>
#include "shader_program.h"

static std::string read_file_contents(char const * path) {
  std::ifstream in(path, std::ios::in /*| std::ios::binary*/);
  if (!in) { throw errno; }
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();
  return contents;
}

static bool compile_shader(GLint shader) {
  glCompileShader(shader);
  int shader_compile_success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compile_success);
  return shader_compile_success;
}

static void print_object_errors(GLint obj, decltype(glGetShaderInfoLog) glInfoLog) {
  static char info_log[512];
  glInfoLog(obj, sizeof(info_log)/sizeof(info_log[0]), NULL, info_log);
  std::cerr << "\n!!! OpenGL ERROR!!! ---~v\n" << info_log << "\n\n";
  exit(1);
}

ShaderProgram::ShaderProgram (
  std::string vertex_shader_path,
  std::string frag_shader_path,
  std::string geom_shader_path
) : m_program_id(-1) {

  /* Read shader files */
  std::string vertex_shader, frag_shader, geom_shader;
  try {
    vertex_shader = read_file_contents(vertex_shader_path.c_str());
    frag_shader = read_file_contents(frag_shader_path.c_str());
    if (!geom_shader_path.empty()) {
      geom_shader = read_file_contents(geom_shader_path.c_str());
    }
  } catch (error_t error) {
    std::cerr << "ERROR::SHADER_PROGRAM::FILES_NOT_READABLE ERRNO(" << error << ")\n";
    exit(1);
  }
  char const * vertex_code = vertex_shader.c_str();
  char const * frag_code = frag_shader.c_str();
  char const * geom_code = geom_shader.c_str();

  /* Compile vertex shader */
  GLuint vertex_shader_id;
  vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader_id, 1, &vertex_code, NULL);
  if (!compile_shader(vertex_shader_id)) {
    std::cerr << "/* VERTEX SHADER */" << '\n';
    print_object_errors(vertex_shader_id, glGetShaderInfoLog);
  }

  /* Compile frag shader */
  GLuint frag_shader_id;
  frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader_id, 1, &frag_code, NULL);
  if (!compile_shader(frag_shader_id)) {
    std::cerr << "/* FRAG SHADER */" << '\n';
    print_object_errors(frag_shader_id, glGetShaderInfoLog);
  }


  GLuint geom_shader_id;
  if (!geom_shader.empty()) {
    geom_shader_id = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geom_shader_id, 1, &geom_code, NULL);
    if (!compile_shader(geom_shader_id)) {
      std::cerr << "/* GEOM SHADER */" << '\n';
      print_object_errors(geom_shader_id, glGetShaderInfoLog);
    }
  }


  /* Link program */
  m_program_id = glCreateProgram();
  glAttachShader(m_program_id, vertex_shader_id);
  if (!geom_shader.empty()) {
    glAttachShader(m_program_id, geom_shader_id);
  }
  glAttachShader(m_program_id, frag_shader_id);
  glLinkProgram(m_program_id);
  int link_sucess;
  glGetProgramiv(m_program_id, GL_LINK_STATUS, &link_sucess);
  if (!link_sucess) {
    std::cerr << "/* LINKING */" << '\n';
    print_object_errors(m_program_id, glGetProgramInfoLog);
  }

  /* Delete shaders */
  glDeleteShader(vertex_shader_id);
  glDeleteShader(frag_shader_id);
}
