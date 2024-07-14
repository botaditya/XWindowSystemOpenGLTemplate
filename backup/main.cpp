// Standard header files
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// X11 Header Files
#include <X11/Xlib.h>   // For all X11 XLib api
#include <X11/Xutil.h>  // For all XVisuaInfo and related API's
#include <X11/XKBlib.h> // For Keyboard relate functionality handling

// OpenGL Header Files
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "Shader.h"

// Macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

Display *display;
Window window;
Colormap colormap;
XVisualInfo *visualInfo = NULL;

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
glXCreateContextAttribsARBProc glXCreateContextAttribsARB = NULL;
GLXFBConfig glxFBConfig;
GLXContext glxContext = NULL;

FILE *gpFile = NULL;

GLuint shaderProgramObject = 0;

enum
{
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR = 1
};

GLuint vao_triangle = 0;
GLuint vbo_position_triange = 0;
GLuint vbo_color_triangle = 0;

Bool bFullscreen = False;
Bool bActiveWindow = False;

int initialize(void)
{
    // function declarations
    void resize(int width, int height);
    void printGLInfo(void);
    void uninitialize(void);

    // local variables
    int attribs_new[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 6,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None};
    int attribs_old[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None};

    // code

    // get the address of function in funtion pointer
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((GLubyte *)"glXCreateContextAttribsARB");
    if (glXCreateContextAttribsARB == NULL)
    {
        fprintf(gpFile, "Cannot get required function address");

        uninitialize();

        exit(-1);
    }

    // Create PP Compatible OpenGL Context
    glxContext = glXCreateContextAttribsARB(display, glxFBConfig, 0, True, attribs_new);
    if (!glxContext)
    {
        fprintf(gpFile, "Core profile based context cannot be obtained.\n Falling back to OLD Context\n");

        // getting old context
        glxContext = glXCreateContextAttribsARB(display, glxFBConfig, 0, True, attribs_old);
        if (!glxContext)
        {
            fprintf(gpFile, "Old GLXContext cannot be found!\n");

            uninitialize();

            exit(-1);
        }
        else
        {
            fprintf(gpFile, "OLD GLXContext Found!");
        }

        return (-1);
    }
    else
    {
        fprintf(gpFile, "Core Profile GLXContext obtained successfully!");
    }

    // check if the context supports direct rendering
    if (!glXIsDirect(display, glxContext))
    {
        fprintf(gpFile, "Does not support direct rendering!\n");
    }
    else
    {
        fprintf(gpFile, "Supports direct rendering!\n");
    }

    // Make this context as current context
    if (glXMakeCurrent(display, window, glxContext) == False)
    {
        fprintf(gpFile, "glXMakeCurrent() failed\n");

        return (-2);
    }

    // initialize GLEW (GLSL Extension Wrangler)
    if (glewInit() != GLEW_OK)
    {
        fprintf(gpFile, "glewInit() failed!\n");
        return (-6);
    }

    // print OpenGL Information
    printGLInfo();

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // OpenGL related initializations
    // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // 1. Vertex Shader
    const GLchar *vertexShaderSourceCode =
        "#version 460 core"
        "\n"
        "in vec4 aPosition;"
        "in vec4 aColor;"
        "out vec4 oColor;"
        "void main(void)"
        "{"
        "gl_Position = aPosition;"
        "oColor = aColor;"
        "}";

    // 2. Create Vertex Shader Object
    GLuint vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

    // 3. Provide OpenGL the source code of Vertex Shader Object
    glShaderSource(vertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);

    // 4. Compile the Vertex Shader
    glCompileShader(vertexShaderObject);

    // 5. Error checking for compilation of Vertex Shader
    GLint status = 0;
    GLint infoLogLength = 0;
    GLchar *szInfoLog = NULL;

    /// a. get the status of compilation
    glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        /// b. get the length of error in log
        glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            /// c. get the error in log
            szInfoLog = (GLchar *)malloc(infoLogLength);
            if (szInfoLog != NULL)
            {
                /// d. get and write the error to log
                glGetShaderInfoLog(vertexShaderObject, infoLogLength, NULL, szInfoLog);

                // e. write error in log
                fprintf(gpFile, "Vertex Shader Compilation Log : %s\n", szInfoLog);

                /// f. free the memory
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        /// g. uninitialize and exit the application
        uninitialize();
    }

    // 2. Fragment Shader
    const GLchar *fragmentShaderSourceCode =
        "#version 460 core"
        "\n"
        "in vec4 oColor;"
        "out vec4 FragColor;"
        "void main(void)"
        "{"
        "FragColor = oColor;"
        "}";

    // 3. Create Fragment Shader Object
    GLuint fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

    // 4. Provide OpenGL the source code of Fragment Shader Object
    glShaderSource(fragmentShaderObject, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

    // 5. Compile the Fragment Shader
    glCompileShader(fragmentShaderObject);

    // 6. Error checking for compilation of Fragment Shader
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;

    /// a. get the status of compilation
    glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        /// b. get the length of error in log
        glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            /// c. get the error in log
            szInfoLog = (GLchar *)malloc(infoLogLength);
            if (szInfoLog != NULL)
            {
                /// d. get and write the error to log
                glGetShaderInfoLog(fragmentShaderObject, infoLogLength, NULL, szInfoLog);

                // e. write error in log
                fprintf(gpFile, "Fragment Shader Compilation Log : %s\n", szInfoLog);

                /// f. free the memory
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        /// g. uninitialize and exit the application
        uninitialize();
    }

    // 7. Create Shader Program Object
    shaderProgramObject = glCreateProgram();

    // 8. Attach Vertex Shader Object to Shader Program Object
    glAttachShader(shaderProgramObject, vertexShaderObject);
    glAttachShader(shaderProgramObject, fragmentShaderObject);

    // 9. Bind our attribute name with the shader program attribute variable
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_POSITION, "aPosition");

    // 10. Bind our attribute name with the shader program attribute variable
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_COLOR, "aColor");

    // 11. Link the Shader Program
    glLinkProgram(shaderProgramObject);

    // 12. Error checking for linking of Shader Program
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;

    /// a. get the status of linking
    glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        /// b. get the length of error in log
        glGetProgramiv(shaderProgramObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            /// c. get the error in log
            szInfoLog = (GLchar *)malloc(infoLogLength);
            if (szInfoLog != NULL)
            {
                /// d. get and write the error to log
                glGetProgramInfoLog(shaderProgramObject, infoLogLength, NULL, szInfoLog);

                // e. write error in log
                fprintf(gpFile, "Shader Program Link Log : %s\n", szInfoLog);

                /// f. free the memory
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        /// g. uninitialize and exit the application
        uninitialize();
    }

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    const GLfloat triangle_position[] =
        {
            0.0f, 1.0f, 0.0f,   // apex
            -1.0f, -1.0f, 0.0f, // left-bottom
            1.0f, -1.0f, 0.0f   // right-bottom
        };

    const GLfloat triangle_colors[] =
        {
            1.0f, 0.0f, 0.0f, // red
            0.0f, 1.0f, 0.0f, // green
            0.0f, 0.0f, 1.0f  // blue
        };

    // VAO
    glGenVertexArrays(1, &vao_triangle);

    // Bind VAO
    glBindVertexArray(vao_triangle);

    // VBO for triangle position
    glGenBuffers(1, &vbo_position_triange);

    // Bind VBO for triangle position
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_triange);

    // Push triangle vertices into the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_position), triangle_position, GL_STATIC_DRAW);

    // Specify the data of position attribute pointer
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    // Enable Vertex Attribute Array for position
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);

    // Unbind VBO for triangle position
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // VBO for triangle color
    glGenBuffers(1, &vbo_color_triangle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color_triangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_colors), triangle_colors, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_COLOR);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind with VAO
    glBindVertexArray(0);

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    // Enabling depth
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // set clear color to blue
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // warm-up call to resize
    resize(WIN_WIDTH, WIN_HEIGHT);

    return (0);
}

void toggleFullscreen(void)
{
    // local variable declarations
    Atom windowManagerStateNormal;
    Atom windowManagerStateFullscreen;
    XEvent event;

    // code

    /// 1. Get WindowmManager normal window state atom
    windowManagerStateNormal = XInternAtom(
        display,
        "_NET_WM_STATE", // _NET => make Atom usable across the network
        False            // True => Always create, False => Create only when not present
    );

    /// 2. Create atom of WindowmManager fullscreen state
    windowManagerStateFullscreen = XInternAtom(
        display,
        "_NET_WM_STATE_FULLSCREEN", // _NET => make Atom usable across the network
        False                       // True => Always create, False => Create only when not present
    );

    /// 3. Declare and memset the event struct and fill it with our two atoms from above
    memset((void *)&event, 0, sizeof(XEvent));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = windowManagerStateNormal;
    event.xclient.format = 32;
    event.xclient.data.l[0] = bFullscreen ? 0 : 1;
    event.xclient.data.l[1] = windowManagerStateFullscreen;

    /// 4. Send the event
    XSendEvent(
        display,
        XRootWindow(display, visualInfo->screen),
        False, // Weather to propogate the fullscreen event to all children windows
        SubstructureNotifyMask,
        &event);
}

void resize(int width, int height)
{
    // code
    // precaution to avoid divide-by-zero
    // height can become 0 in case of Minimize or whatever other reason may be
    // we will reset the height to 1 in that case to avoid divide by 0 error
    if (height <= 0)
        height = 1;

    // set the viewport as per the window's aspect ratio
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void update(void)
{
    // code
    return;
}

void printGLInfo(void)
{
    // variable declarations
    GLint numExtensions;
    GLint i;

    // code
    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    fprintf(gpFile, "----------------------\n");
    fprintf(gpFile, "Supported Extensions are %d :\n", numExtensions);
    fprintf(gpFile, "----------------------\n\n");

    // lsiting of supported extensions
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    for (i = 0; i < numExtensions; i++)
    {
        fprintf(gpFile, "%s\n", glGetStringi(GL_EXTENSIONS, i));
    }

    fprintf(gpFile, "----------------------\n\n");
}

void uninitialize(void)
{
    glXMakeCurrent(display, None, nullptr);
    glXDestroyContext(display, glxContext);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void render()
{
    // code
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader program object
    glUseProgram(shaderProgramObject);

    // Bind wth VAO
    glBindVertexArray(vao_triangle);

    // Draw geometry
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Unbind with vao
    glBindVertexArray(0);

    // unuse shader program object
    glUseProgram(0);

    glXSwapBuffers(display, window);
}

int main(void)
{
    // Local Function declarations
    void toggleFullscreen(void);
    int initialize(void);
    void resize(int, int);
    void render(void);
    void update(void);
    void uninitialize(void);

    // Local Variable Declarations
    int defaultScreen;
    XSetWindowAttributes windowAttributes;
    int styleMask;
    Atom windowManagerDelete;
    XEvent event;
    KeySym keySym;
    char keys[26];
    int screenWidth, screenHeight;
    int frameBufferAttributes[] = {
        // PP related
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        // //////////////////////////////
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24, // 24 for depth buffer
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None, // XWindows enforces to end every config array with 0 OR None
    };
    Bool bDone = False;

    // PP Related Variables
    GLXFBConfig *glxFBConfigs;
    GLXFBConfig bestGLXFBConfig;
    XVisualInfo *tempXVisualInfo = NULL;
    int numFBConfigs;

    int bestFrameBufferConfig = -1,
        bestNumberOfSamples = -1;
    int worstFrameBufferConfig = -1,
        worstNumberOfSamples = 999;
    int sampleBuffers, samples;
    int i;

    // Code

    // Open log file
    gpFile = fopen("log.txt", "w");
    if (gpFile == NULL)
    {
        printf("fopen() failed to create a log file!");
    }
    else
    {
        fprintf(gpFile, "Log file successfuly created!\n");
    }

    /// 1. Open the connection with XServer and get open display
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(gpFile, "XOpenDisplay() failed!\n");
        uninitialize();

        // exit with error
        exit(1);
    }

    /// 2. Get default screen from above display
    defaultScreen = XDefaultScreen(display);

    /// 3. Visual Info Code
    // a. get available FBConfigs from System
    glxFBConfigs = glXChooseFBConfig(display, defaultScreen, frameBufferAttributes, &numFBConfigs);
    if (glxFBConfigs == NULL)
    {
        fprintf(gpFile, "Matching FBConfigs cannot be found!");

        uninitialize();

        // exit with error
        exit(-1);
    }

    fprintf(gpFile, "Matching %d FBConfigs found!", numFBConfigs);

    // b. fnd best matching fbconfig
    for (i = 0; i < numFBConfigs; i++)
    {
        tempXVisualInfo = glXGetVisualFromFBConfig(display, glxFBConfigs[i]);
        if (tempXVisualInfo != NULL)
        {
            // i. Get Sample Buffers
            glXGetFBConfigAttrib(display, glxFBConfigs[i], GLX_SAMPLE_BUFFERS, &sampleBuffers);
            // ii. Get Samples
            glXGetFBConfigAttrib(display, glxFBConfigs[i], GLX_SAMPLES, &samples);

            if (bestFrameBufferConfig < 0 || (sampleBuffers && samples > bestNumberOfSamples))
            {
                bestFrameBufferConfig = i;
                bestNumberOfSamples = samples;
            }

            if (worstFrameBufferConfig < 0 || !sampleBuffers || samples < worstNumberOfSamples)
            {
                worstFrameBufferConfig = i;
                worstNumberOfSamples = samples;
            }

            XFree(tempXVisualInfo);
            tempXVisualInfo = NULL;
        }
    }

    // Accordingly, get bext GLXFBConfig
    bestGLXFBConfig = glxFBConfigs[bestFrameBufferConfig];

    // assign this best found best glxfbconfig to global glxfbcnfig
    glxFBConfig = bestGLXFBConfig;

    // Free memory for array
    XFree(glxFBConfigs);

    visualInfo = glXGetVisualFromFBConfig(display, bestGLXFBConfig);
    if (visualInfo == NULL)
    {
        fprintf(gpFile, "glXChooseVisual() Failed!");

        uninitialize();

        // exit with error
        exit(-1);
    }

    fprintf(gpFile, "The chosen visual's id is 0x%lu\n", visualInfo->visualid);

    /// 5. Set Window attributes/properties
    memset((void *)&windowAttributes, 0, sizeof(XSetWindowAttributes));
    windowAttributes.border_pixel = 0;                                            // border color of window (0: Use default)
    windowAttributes.background_pixel = XBlackPixel(display, visualInfo->screen); // window background color
    windowAttributes.background_pixmap = 0;                                       // Background image
    windowAttributes.colormap = XCreateColormap(
        display,
        XRootWindow(display, visualInfo->screen),
        visualInfo->visual,
        AllocNone // Do you want to store the colormap and allocate memory for it? AllocNone => No, AllocAll => Yes
    );

    /// 6. Assign this colormap to global colormap
    colormap = windowAttributes.colormap;

    /// 7. Set the style of the window
    styleMask = CWBorderPixel | CWBackPixel | CWColormap | CWEventMask;

    /// 8. Create the window
    window = XCreateWindow(
        display,
        XRootWindow(display, visualInfo->screen),
        0,                  // x pos
        0,                  // y pos
        WIN_WIDTH,          // width
        WIN_HEIGHT,         // height
        0,                  // border width (0: use default)
        visualInfo->depth,  // depth detected in XMatchVisualInfo
        InputOutput,        // type of window
        visualInfo->visual, // visual of window
        styleMask,          // Style of window
        &windowAttributes   // window attributes
    );
    if (!window)
    {
        fprintf(gpFile, "XCreateWindow() Failed!");
        uninitialize();

        // exit with error
        exit(-1);
    }

    /// 9. Specify to which events this window should respond
    XSelectInput(
        display,
        window,
        ExposureMask | VisibilityChangeMask | StructureNotifyMask | KeyPressMask | ButtonPressMask | PointerMotionMask | FocusChangeMask);

    /// 10. Specify window manager delete atom
    windowManagerDelete = XInternAtom(
        display,
        "WM_DELETE_WINDOW",
        True);

    /// 11. Add above atom as protocol for window manager
    XSetWMProtocols(
        display,
        window,
        &windowManagerDelete, // array of atoms (Base address as single)
        1                     // size of array of 3rd param array
    );

    /// 12. Give caption to the window
    XStoreName(
        display,
        window,
        "Mr. Aditya Sanjeev Kulkarni");

    /// 13. Show/Map the window
    XMapWindow(display, window);

    // Center the window
    screenWidth = XWidthOfScreen(XScreenOfDisplay(display, visualInfo->screen));
    screenHeight = XHeightOfScreen(XScreenOfDisplay(display, visualInfo->screen));

    XMoveWindow(
        display,
        window,
        ((screenWidth - WIN_WIDTH) / 2),
        ((screenHeight - WIN_HEIGHT) / 2));

    // OpenGL initialization
    int iResult = initialize();
    if (iResult != 0)
    {
        fprintf(gpFile, "Failed in initialize(): OpenGL Initialize");

        uninitialize();

        exit(-1);
    }

    // 14. Game Loop
    while (bDone == False)
    {
        while (XPending(display))
        {
            memset((void *)&event, 0, sizeof(XEvent));
            XNextEvent(display, &event);

            switch (event.type)
            {
            case MapNotify: // ShowWindow(): WIN32 (WM_CREATE)
                break;
            case FocusIn:
                bActiveWindow = True;
                break;
            case FocusOut:
                bActiveWindow = False;
                break;
            case ConfigureNotify:
                resize(event.xconfigure.width, event.xconfigure.height);
                break;
            case ButtonPress:
                switch (event.xbutton.button)
                {
                case 1:
                    break;
                default:
                    break;
                }
                break;
            case KeyPress:
                keySym = XkbKeycodeToKeysym(
                    display,
                    event.xkey.keycode, // XWindows KeyCode
                    0,                  // Keycode Group Representation
                    0                   // Shift Status
                );
                switch (keySym)
                {
                case XK_Escape:
                    bDone = True;
                    break;
                default:
                    break;
                }

                XLookupString(
                    &event.xkey,
                    keys,
                    sizeof(keys),
                    NULL, // Array to save all keySym for every key pressed
                    NULL  // State persistence of the keys pressed
                );

                switch (keys[0])
                {
                case 'F':
                case 'f':
                    if (bFullscreen == False)
                    {
                        toggleFullscreen();
                        bFullscreen = True;
                    }
                    else
                    {
                        toggleFullscreen();
                        bFullscreen = False;
                    }

                    break;
                default:
                    break;
                }

                break;
            case 33: // atom protocol exit
                bDone = True;
                break;
            default:
                break;
            }
        }

        // Rendering
        if (bActiveWindow == True)
        {
            // Render
            render();

            // Update
            update();
        }
    }

    uninitialize();

    return (0);
}
