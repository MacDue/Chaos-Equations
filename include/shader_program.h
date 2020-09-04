#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define _BASIC_UNIFORM_SETTER(ctype, type_suffix) \
    void uniform ## type_suffix (std::string const & key, std::initializer_list<ctype> &&values) {                       \
        GLuint location = glGetUniformLocation(m_program_id, key.c_str());                                               \
        const ctype * values_ptr = values.begin();                                                                       \
        switch (values.size())                                                                                           \
        {                                                                                                                \
        case 1:                                                                                                          \
            glUniform1 ## type_suffix(location, values_ptr[0]); break;                                                   \
        case 2:                                                                                                          \
            glUniform2 ## type_suffix(location, values_ptr[0], values_ptr[1]); break;                                    \
        case 3:                                                                                                          \
            glUniform3 ## type_suffix(location, values_ptr[0], values_ptr[1], values_ptr[2]); break;                     \
        case 4:                                                                                                          \
            glUniform4 ## type_suffix(location, values_ptr[0], values_ptr[1], values_ptr[2], values_ptr[3]); break;      \
        default:                                                                                                         \
            std::cerr << "SHADER_PROGRAM::UNSUPPORTED_UNIFORM\n";                                                        \
            break;                                                                                                       \
        }                                                                                                                \
    }
#define BASIC_UNIFORM_SETTER(ctype, type_suffix) _BASIC_UNIFORM_SETTER(ctype, type_suffix)

class ShaderProgram
{
    private:
        GLint m_program_id;

    public:
        ShaderProgram(
            std::string vertex_shader_path,
            std::string frag_shader_path,
            std::string geom_shader_path = nullptr);
        ~ShaderProgram() { glDeleteProgram(m_program_id); }

        GLint get_prog_id() const { return m_program_id; }
        void use() const { glUseProgram(m_program_id); };

        BASIC_UNIFORM_SETTER(GLfloat, f)
        BASIC_UNIFORM_SETTER(GLint, i)
        BASIC_UNIFORM_SETTER(GLuint, ui)

        void uniformMat4f(std::string const & key, glm::mat4 const & mat) {
          GLuint location = glGetUniformLocation(m_program_id, key.c_str());
          glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
        }
};

#endif
