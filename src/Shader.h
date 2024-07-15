#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <string>

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    TessControl,
    TessEvaluation,
    Compute,
    NumShaderTypes // Add a special type to count the number of shader types
};

class Shader {
public:
    Shader();
    ~Shader();

    void addShaderFromSource(ShaderType type, const char* source);
    void addShaderFromFile(ShaderType type, const char* filePath);
    void linkProgram();
    void use();
    void cleanup();

private:
    unsigned int programID;
    unsigned int shaderIDs[static_cast<int>(ShaderType::NumShaderTypes)];

    char* readFile(const char* filePath);
    unsigned int compileShader(ShaderType type, const char* source);
};

#endif // SHADER_H
