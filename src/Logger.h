#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

class Logger {
public:
    Logger();
    ~Logger();

    void Debug(const char* format, ...);   // Variadic function for Debug messages
    void Error(const char* format, ...);   // Variadic function for Error messages
    void Shader(const char* format, ...);  // Variadic function for Shader messages
    void Info(const char* format, ...);    // Variadic function for Info messages

private:
    FILE *debugFile;
    FILE *errorFile;
    FILE *shaderFile;
    FILE *infoFile;

    FILE* openLogFile(const char* filename);
    char* getCurrentDateTime();
};

#endif // LOGGER_H
