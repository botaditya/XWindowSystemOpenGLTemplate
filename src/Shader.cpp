#include "Shader.h"
#include "Logger.h"
#include <cstdio>
#include <iostream>

Shader::Shader() : programID(0) {
    // Initialize shaderIDs array
    for (int i = 0; i < static_cast<int>(ShaderType::NumShaderTypes); ++i) {
        shaderIDs[i] = 0;
    }
}

Shader::~Shader() {
    cleanup();
}

void Shader::addShaderFromSource(ShaderType type, const char* source) {
    unsigned int shaderID = compileShader(type, source);
    shaderIDs[static_cast<int>(type)] = shaderID;
}

void Shader::addShaderFromFile(ShaderType type, const char* filePath) {
    char* source = readFile(filePath);
    if (source) {
        addShaderFromSource(type, source);
        delete[] source; // Clean up allocated memory after use
    } else {
        logger.Shader("Failed to read file: %s", filePath);
    }
}

void Shader::linkProgram() {
    programID = glCreateProgram();

    for (int i = 0; i < static_cast<int>(ShaderType::NumShaderTypes); ++i) {
        if (shaderIDs[i] != 0) {
            glAttachShader(programID, shaderIDs[i]);
        }
    }

    glLinkProgram(programID);

    // Check linking status
    int success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        logger.Shader("Shader program linking failed: %s\n", infoLog);
    }

    // Detach and delete shaders after linking
    for (int i = 0; i < static_cast<int>(ShaderType::NumShaderTypes); ++i) {
        if (shaderIDs[i] != 0) {
            glDetachShader(programID, shaderIDs[i]);
            glDeleteShader(shaderIDs[i]);
        }
    }
}

void Shader::use() {
    glUseProgram(programID);
}

void Shader::cleanup() {
    if (programID != 0) {
        glDeleteProgram(programID);
        programID = 0;
    }
}

char* Shader::readFile(const char* filePath) {
    FILE* file = fopen(filePath, "rb");
    if (!file) {
        logger.Shader("Failed to open file: %s", filePath);

        return nullptr;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Allocate buffer for file contents
    char* buffer = new char[fileSize + 1];
    if (!buffer) {
        char logBuffer[128];
        logger.Shader("Failed to allocate memory for file: ", logBuffer);

        fclose(file);
        return nullptr;
    }

    // Read file contents into buffer
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    buffer[bytesRead] = '\0'; // Null-terminate the string

    // Close file and return buffer
    fclose(file);
    return buffer;
}

unsigned int Shader::compileShader(ShaderType type, const char* source) {
    unsigned int shaderID;
    switch (type) {
        case ShaderType::Vertex:
            shaderID = glCreateShader(GL_VERTEX_SHADER);
            break;
        case ShaderType::Fragment:
            shaderID = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        case ShaderType::Geometry:
            shaderID = glCreateShader(GL_GEOMETRY_SHADER);
            break;
        case ShaderType::TessControl:
            shaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
            break;
        case ShaderType::TessEvaluation:
            shaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
            break;
        case ShaderType::Compute:
            shaderID = glCreateShader(GL_COMPUTE_SHADER);
            break;
        default:
            logger.Shader("Unsupported shader type.");
            return 0;
    }

    glShaderSource(shaderID, 1, &source, nullptr);
    glCompileShader(shaderID);

    // Check compilation status
    int success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
        logger.Shader("Shader compilation failed:\n", infoLog);

        return 0;
    }

    return shaderID;
}
