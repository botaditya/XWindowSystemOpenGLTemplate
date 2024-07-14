// OpenGL Header Files
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h> // Include Xlib for XInitThreads

#include "WindowManager.h"

int main(int argc, char *argv[]) {
    // Initialize Xlib threading support (required for OpenGL with X11)
    XInitThreads();

    // Initialize window manager
    WindowManager *windowManager = new WindowManager(800, 600, "OpenGL Window");
    
    windowManager->initialize();

    windowManager->run();

    windowManager->uninitialize();

    return 0;
}
