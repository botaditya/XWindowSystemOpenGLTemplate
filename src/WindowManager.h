// OpenGL Header Files
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

class WindowManager
{
public:
    WindowManager(int width, int height, char *title);
    ~WindowManager();

    void initialize();
    void run();
    void render();
    void resize(int width, int height);
    void update();
    void uninitialize();

private:
    int width, height;
    char *title;
    bool fullscreen;
    bool running;
    bool focused;
    // Display related
    Display *display;
    Window window;
    Colormap colormap;
    XVisualInfo *visualInfo = NULL;
    // Context related
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = NULL;
    GLXFBConfig glxFBConfig;
    GLXContext glxContext = NULL;

    void createWindow();
    void setupGL();
    void setupGLEW();
    void printGLInfo();
    void toggleFullscreen();
    void handleEvents();
};
