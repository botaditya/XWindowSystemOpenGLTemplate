#include "Logger.h"
#include <time.h>
#include <ctime>

// Definition of the global logger instance
Logger logger;

Logger::Logger()
{
    if (debugFile == nullptr)
    {
        debugFile = openLogFile("logs/debug.log");
    }
    if (errorFile == nullptr)
    {
        errorFile = openLogFile("logs/error.log");
    }
    if (shaderFile == nullptr)
    {
        shaderFile = openLogFile("logs/shader.log");
    }
    if (infoFile == nullptr)
    {
        infoFile = openLogFile("logs/info.log");
    }
}

Logger::~Logger()
{
    if (debugFile)
    {
        fclose(debugFile);
        debugFile = NULL;
    }
    if (errorFile)
    {
        fclose(errorFile);
        errorFile = NULL;
    }
    if (shaderFile)
    {
        fclose(shaderFile);
        shaderFile = NULL;
    }
    if (infoFile)
    {
        fclose(infoFile);
        infoFile = NULL;
    }
}

FILE *Logger::openLogFile(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == nullptr)
    {
        fprintf(stderr, "Error: Failed to open log file %s\n", filename);
    }
    return file;
}

char *Logger::getCurrentDateTime()
{
    char toReturn[64];
    time_t now;
    struct tm *timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    strftime(toReturn, sizeof(toReturn), "%Y-%m-%d %H:%M:%S", timeinfo);
    return toReturn;
}

void Logger::Debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(debugFile, "[%s] [DEBUG] ", getCurrentDateTime());
    vfprintf(debugFile, format, args);
    fprintf(debugFile, "\n");
    
    va_end(args);
}

void Logger::Error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(errorFile, "[%s] [ERROR] ", getCurrentDateTime());
    vfprintf(errorFile, format, args);
    fprintf(errorFile, "\n");
    
    va_end(args);
}

void Logger::Shader(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(shaderFile, "[%s] [SHADER] ", getCurrentDateTime());
    vfprintf(shaderFile, format, args);
    fprintf(shaderFile, "\n");
    
    va_end(args);
}

void Logger::Info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(infoFile, "[%s] [INFO] ", getCurrentDateTime());
    vfprintf(infoFile, format, args);
    fprintf(infoFile, "\n");
    
    va_end(args);
}