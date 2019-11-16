#include "pch.h"
#include "shader.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>

lift::Shader::Shader(const std::string& file_path)
    : renderer_id_{0}, file_path_{file_path} {

    const ShaderProgramSource shader_program = parseShader(file_path);
    renderer_id_ = createShader(shader_program.vertex_source, shader_program.fragment_source);
}

lift::Shader::~Shader() {
    glDeleteProgram(renderer_id_);
}

void lift::Shader::bind() const {
    glUseProgram(renderer_id_);
}

void lift::Shader::unbind() {
    glUseProgram(0);
}

void lift::Shader::setUniform1I(const std::string& name, int value) {
    GL_CHECK(glUniform1i(getUniformLocation(name), value));
}

void lift::Shader::SetUniform1f(const std::string& name, float value) {
    GL_CHECK(glUniform1f(getUniformLocation(name), value));
}

void lift::Shader::setTexImage2D(const uint32_t width, const uint32_t height) {
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr));
}

auto lift::Shader::parseShader(const std::string& file_path) -> ShaderProgramSource {
    std::string line;
    std::stringstream string_stream[2];

    std::ifstream vertex_stream(file_path + ".vert");
    while (std::getline(vertex_stream, line))
        string_stream[0] << line << '\n';

    std::ifstream fragment_stream(file_path + ".frag");
    while (std::getline(fragment_stream, line))
        string_stream[1] << line << '\n';

    return {string_stream[0].str(), string_stream[1].str()};

}

auto lift::Shader::createShader(const std::string& vertex_source, const std::string& fragment_source) -> unsigned {
    const unsigned int program = glCreateProgram();
    const unsigned int vertex_shader = compileShader(GL_VERTEX_SHADER, vertex_source);
    const unsigned int fragment_shader = compileShader(GL_FRAGMENT_SHADER, fragment_source);

    GL_CHECK(glAttachShader(program, vertex_shader));
    GL_CHECK(glAttachShader(program, fragment_shader));
    GL_CHECK(glLinkProgram(program));
    GL_CHECK(glValidateProgram(program));

    GL_CHECK(glDeleteShader(vertex_shader));
    GL_CHECK(glDeleteShader(fragment_shader));

    return program;
}

auto lift::Shader::compileShader(const unsigned int type, const std::string& source) -> unsigned {
    const auto shader_id = glCreateShader(type);
    auto src = source.c_str();

    GL_CHECK(glShaderSource(shader_id, 1, &src, nullptr));
    GL_CHECK(glCompileShader(shader_id));

    int is_compiled = 0;
    GL_CHECK(glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled));
    if (is_compiled == GL_FALSE) {
        int buf_size = 0;
        GL_CHECK(glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &buf_size));

        std::vector<GLchar> info_log(buf_size);
        GL_CHECK(glGetShaderInfoLog(shader_id, buf_size, &buf_size, &info_log[0]));

        LF_ERROR("Failed to Compile {0}", (type == GL_VERTEX_SHADER ? "Vertex Shader" : "Fragment Shader"));
        LF_ERROR("{0}", info_log.data());
        GL_CHECK(glDeleteShader(shader_id));
        return 0;
    }

    return shader_id;
}

auto lift::Shader::getUniformLocation(const std::string& name) -> int {
    if (uniform_location_cache_.find(name) != uniform_location_cache_.end())
        return uniform_location_cache_[name];

    const int location = glGetUniformLocation(renderer_id_, name.c_str());
    if (location == -1)
        LF_WARN("Uniform {0} is not defined on shader {1}", name, file_path_);
    return location;
}
