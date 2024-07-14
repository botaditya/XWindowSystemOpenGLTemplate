#include "WindowManager.h"
#include "Shader.h"

// X11 Header Files
#include <X11/Xlib.h>   // For all X11 XLib api
#include <X11/Xutil.h>  // For all XVisuaInfo and related API's
#include <X11/XKBlib.h> // For Keyboard relate functionality handling

#include <cstring>
#include <cstdlib>

enum
{
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR = 1
};

WindowManager::WindowManager(int width, int height, char *title)
    : width(width), height(height), fullscreen(true),
      running(true), focused(true), display(nullptr), window(0), colormap(0), visualInfo(nullptr),
      glXCreateContextAttribsARB(nullptr), glxFBConfig(0), glxContext(nullptr), logger()
{
    if (title == nullptr) {
        // Use default title
        this->title = new char[strlen("XWindow::OpenGL|Window") + 1];
        strcpy(this->title, "XWindow::OpenGL|Window");
    } else {
        // Allocate memory and copy provided title
        this->title = new char[strlen(title) + 1];
        strcpy(this->title, title);
    }

    createWindow();
}

WindowManager::~WindowManager()
{
}

void WindowManager::initialize()
{
    // Setup OpenGL context
    setupGL();

    // Setup GLEW
    setupGLEW();

    // warmup resize
    resize(this->width, this->height);
}

void WindowManager::run()
{
    while (running)
    {
        handleEvents();

        if (focused)
        {
            // Render
            render();

            // Update
            update();
        }
    }

    uninitialize();

    exit(0);
}

void WindowManager::resize(int width, int height)
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

void WindowManager::render()
{
    GLuint vao_triangle = 0;
    GLuint vbo_position_triange = 0;
    GLuint vbo_color_triangle = 0;
    Shader shaderProgram;

    // Add shaders (from source or file)
    shaderProgram.addShaderFromFile(ShaderType::Vertex, "shaders/triangle/vertexShader.glsl");
    shaderProgram.addShaderFromFile(ShaderType::Fragment, "shaders/triangle/fragmentShader.glsl");
    // Optionally add more shaders like Geometry, Tessellation, or Compute

    // Link the shader program
    shaderProgram.linkProgram();

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

    // Use the shader program
    shaderProgram.use();

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

void WindowManager::update()
{
    // code
}

void WindowManager::uninitialize()
{
    // Cleanup OpenGL context and display
    if (glxContext)
    {
        glXMakeCurrent(display, None, NULL);
        glXDestroyContext(display, glxContext);
    }

    if (display && window)
    {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
    }
}

void WindowManager::createWindow()
{
    // local variables
    // PP Related Variables
    GLXFBConfig *glxFBConfigs;
    GLXFBConfig bestGLXFBConfig;
    XVisualInfo *tempXVisualInfo = NULL;
    int numFBConfigs;
    Atom windowManagerDelete;

    int bestFrameBufferConfig = -1,
        bestNumberOfSamples = -1;
    int worstFrameBufferConfig = -1,
        worstNumberOfSamples = 999;
    int sampleBuffers, samples;

    // Open X display
    display = XOpenDisplay(nullptr);
    if (!display)
    {
        logger.Error("Failed to open X display.");
        exit(1);
    }

    // Get default screen
    int screen = XDefaultScreen(display);

    // Set fullscreen mode if required
    if (fullscreen)
    {
        width = DisplayWidth(display, screen);
        height = DisplayHeight(display, screen);
    }

    // Create visual info
    int attribs[] = {
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

    glxFBConfigs = glXChooseFBConfig(display, screen, attribs, &numFBConfigs);
    if (glxFBConfigs == nullptr)
    {
        logger.Error("Failed to Matching FBConfigs!");
        exit(1);
    }
    else
    {
        logger.Info("Matching %d FBConfigs found!", numFBConfigs);
    }

    // b. fnd best matching fbconfig
    for (int i = 0; i < numFBConfigs; i++)
    {
        tempXVisualInfo = glXGetVisualFromFBConfig(display, glxFBConfigs[i]);
        if (tempXVisualInfo != NULL)
        {
            // i. Get Sample Buffers
            glXGetFBConfigAttrib(display, glxFBConfigs[i], GLX_SAMPLE_BUFFERS, &sampleBuffers);
            // ii. Get Samples
            glXGetFBConfigAttrib(display, glxFBConfigs[i], GLX_SAMPLES, &samples);

            if (bestFrameBufferConfig < 0 || sampleBuffers && samples > bestNumberOfSamples)
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

    bestGLXFBConfig = glxFBConfigs[bestFrameBufferConfig];
    glxFBConfig = bestGLXFBConfig;
    XFree(glxFBConfigs);

    visualInfo = glXGetVisualFromFBConfig(display, bestGLXFBConfig);
    if (!visualInfo)
    {
        logger.Error("Failed to create OpenGL visual.");
        exit(1);
    }

    // Create colormap
    colormap = XCreateColormap(display, RootWindow(display, visualInfo->screen),
                               visualInfo->visual, AllocNone);

    // Set window attributes
    XSetWindowAttributes windowAttributes;
    memset((void *)&windowAttributes, 0, sizeof(XSetWindowAttributes));
    windowAttributes.border_pixel = 0;                                            // border color of window (0: Use default)
    windowAttributes.background_pixel = XBlackPixel(display, visualInfo->screen); // window background color
    windowAttributes.background_pixmap = 0;
    windowAttributes.colormap = colormap;

    int styleMask = CWBorderPixel | CWBackPixel | CWColormap | CWEventMask;

    // Create window
    window = XCreateWindow(
        display,
        XRootWindow(display, visualInfo->screen),
        0,                  // x pos
        0,                  // y pos
        this->width,        // width
        this->height,       // height
        0,                  // border width (0: use default)
        visualInfo->depth,  // depth detected in XMatchVisualInfo
        InputOutput,        // type of window
        visualInfo->visual, // visual of window
        styleMask,          // Style of window
        &windowAttributes   // window attributes
    );
    if (!window)
    {
        logger.Error("XCreateWindow() Failed!");
        exit(1);
    }

    // Specify to which events this window should respond
    XSelectInput(
        display,
        window,
        ExposureMask | VisibilityChangeMask | StructureNotifyMask | KeyPressMask | ButtonPressMask | PointerMotionMask | FocusChangeMask);

    // Specify window manager delete atom
    windowManagerDelete = XInternAtom(
        display,
        "WM_DELETE_WINDOW",
        True);

    // Add above atom as protocol for window manager
    XSetWMProtocols(
        display,
        window,
        &windowManagerDelete, // array of atoms (Base address as single)
        1                     // size of array of 3rd param array
    );

    // Set window title
    XStoreName(display, window, this->title);

    // Map window to display
    XMapWindow(display, window);

    // Center the window
    int screenWidth = XWidthOfScreen(XScreenOfDisplay(display, visualInfo->screen));
    int screenHeight = XHeightOfScreen(XScreenOfDisplay(display, visualInfo->screen));

    XMoveWindow(
        display,
        window,
        ((screenWidth - this->width) / 2),
        ((screenHeight - this->height) / 2));
}

void WindowManager::setupGL()
{
    // local variables
    int context_attribs_new[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 6,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None};
    int context_attribs_old[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None};

    // Load OpenGL functions
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress(
        (const GLubyte *)"glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB)
    {
        logger.Error("Failed to load glXCreateContextAttribsARB function. Reason: 'Cannot get required function address'");
        exit(1);
    }

    // Get framebuffer configuration
    int attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        None};

    glxContext = glXCreateContextAttribsARB(display, this->glxFBConfig, 0, True, context_attribs_new);
    if (!glxContext)
    {
        logger.Error("Core profile based context cannot be obtained.\n Falling back to OLD Context");

        // getting old context
        glxContext = glXCreateContextAttribsARB(display, glxFBConfig, 0, True, context_attribs_old);
        if (!glxContext)
        {
            logger.Error("Old GLXContext for compatibility profile cannot be found!");
        }
        else
        {
            logger.Info("Old GLXContext for compatibility profile context found!");
        }

        exit(1);
    }
    else
    {
        logger.Info("Core profile GLXContext found and obtained successfully!");
    }

    // check if the context supports direct rendering
    if (!glXIsDirect(display, glxContext))
    {
        logger.Info("Does not support direct rendering!");
    }
    else
    {
        logger.Info("Supports direct rendering!");
    }

    // Make the context current
    if (!glXMakeCurrent(display, window, glxContext))
    {
        logger.Error("Failed to make OpenGL context current.");
        exit(1);
    }

    // Enable depth testing, if needed
    // Enabling depth
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // set clear color to blue
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Additional OpenGL initialization can go here
}

void WindowManager::setupGLEW()
{
    // initialize GLEW (GLSL Extension Wrangler)
    if (glewInit() != GLEW_OK)
    {
        logger.Error("glewInit(): Failed to initialize GLEW");
        exit(1);
    }

    // printGLInfo();
}

void WindowManager::printGLInfo()
{
    // variable declarations
    GLint numExtensions;
    GLint i;

    // Get OpenGL information
    const char *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    const char *glslVersion = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Log OpenGL info
    logger.Info("OpenGL Vendor : %s\n", vendor);
    logger.Info("OpenGL Renderer : %s\n", renderer);
    logger.Info("OpenGL Version : %s\n", version);
    logger.Info("GLSL Version : %s\n", glslVersion);

    // Get number of supported extensions
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    // Log supported extensions
    logger.Info("Supported Extensions (%d) are:\n", numExtensions);
    for (int i = 0; i < numExtensions; i++)
    {
        const char *extension = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
        logger.Info("%s\n", extension);
    }

    logger.Info("----------------------\n\n");
}

void WindowManager::toggleFullscreen(void)
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
    event.xclient.data.l[0] = fullscreen ? 0 : 1;
    event.xclient.data.l[1] = windowManagerStateFullscreen;

    /// 4. Send the event
    XSendEvent(
        display,
        XRootWindow(display, visualInfo->screen),
        False, // Weather to propogate the fullscreen event to all children windows
        SubstructureNotifyMask,
        &event);
}

void WindowManager::handleEvents()
{
    // Handle events (e.g., user input, window events)
    XEvent event;
    KeySym keySym;
    char keys[26];

    while (XPending(display))
    {
        memset((void *)&event, 0, sizeof(XEvent));
        XNextEvent(display, &event);

        switch (event.type)
        {
        case MapNotify: // ShowWindow(): WIN32 (WM_CREATE)
            break;
        case FocusIn:
            focused = true;
            break;
        case FocusOut:
            focused = false;
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
                running = true;
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
                fullscreen = !fullscreen;
                toggleFullscreen();
                break;
            default:
                break;
            }
            break;
        case 33: // atom protocol exit
            running = true;
            break;
        default:
            break;
        }
    }
}