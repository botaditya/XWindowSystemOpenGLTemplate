cmake_minimum_required(VERSION 3.10)
project(OpenGLApp)

# Find necessary packages
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(GLEW REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use modern OpenGL (core profile)
add_definitions(-DGL_GLEXT_PROTOTYPES)
set(OpenGL_GL_PREFERENCE "GLVND")

set(SOIL_INCLUDE_DIRS "/usr/include/SOIL")

# Add include directories
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${SOIL_INCLUDE_DIRS})
include_directories(include)

# Add executable and its sources
add_executable(OpenGLApp 
    src/main.cpp 
    src/WindowManager.cpp 
    src/Logger.cpp 
    src/Shader.cpp 
    include/Shader.h
)

# Link libraries
target_link_libraries(OpenGLApp 
    ${OPENGL_LIBRARIES} 
    ${X11_LIBRARIES}
    ${GLEW_LIBRARIES}
    SOIL
)

# Set output directory for executables
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME}.o)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/release)


# Optional: Set compiler flags
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif (UNIX)
