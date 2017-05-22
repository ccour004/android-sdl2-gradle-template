#include "SDL.h"
#include "BulletSim.h"
#include "ShapeCreator.h"

#include <GLES/gl.h>

/* Following gluLookAt implementation is adapted from the
 * Mesa 3D Graphics library. http://www.mesa3d.org
 */
static void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
                      GLfloat centerx, GLfloat centery, GLfloat centerz,
                      GLfloat upx, GLfloat upy, GLfloat upz)
{
    GLfloat m[16];
    GLfloat x[3], y[3], z[3];
    GLfloat mag;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = (float)sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {			/* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }

    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;

    /* X vector = Y cross Z */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];

    /* Recompute Y = Z cross X */
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];

    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */

    mag = (float)sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = (float)sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#define M(row,col)  m[col*4+row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M
    {
        int a;
        GLfixed fixedM[16];
        for (a = 0; a < 16; ++a)
            fixedM[a] = (GLfixed)(m[a] * 65536);
        glMultMatrixx(fixedM);
    }

    /* Translate Eye to Origin */
    glTranslatex((GLfixed)(-eyex * 65536),
                 (GLfixed)(-eyey * 65536),
                 (GLfixed)(-eyez * 65536));
}

MeshBuilder modelBuilder;
Shader shader;
Mesh mesh;
MeshInstance* instance;

class SDL_Manager{
public:
    SDL_Window* window;
    SDL_GLContext glContext;

    SDL_Manager(){
        //Initialization flag
        bool success = true;
        SDL_Log("++START SDL++");
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow(
                "Test Window",
                SDL_WINDOWPOS_UNDEFINED,           // initial x position
                SDL_WINDOWPOS_UNDEFINED,           // initial y position
                640,                               // width, in pixels
                480,                               // height, in pixels
                SDL_WINDOW_OPENGL                  // flags - see below
        );

        glContext = SDL_GL_CreateContext(window);

        //Use OpenGL 3.1 core
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

        // Turn on double buffering with a 24bit Z buffer.
        // You may need to change this to 16 or 32 for your system
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);


        //Use Vsync
        if( SDL_GL_SetSwapInterval( 1 ) < 0 )
        {
            SDL_Log( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
        }

        //Initialize OpenGL
        SDL_Log("++INIT GL++");

        //Setup gl settings.
        glClearDepthf(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        //glViewport(0, 0, 640, 513);
        glClearColor( 0.f, 0.f, 0.f, 1.f );

        //Init meshes.
        shader.init();
        mesh = modelBuilder.build(2,2,2,20,20);
        mesh.init();

        instance = new MeshInstance(&mesh,&shader,btVector3(0,0,0));
    }

    ~SDL_Manager(){
        SDL_Log("~~STOP SDL~~");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
} *sdl;

void dispose(){
    delete sdl;
}

int err(const char* fmt){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,fmt, SDL_GetError());
    dispose();
    return 1;
}

void render()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    instance->render();
}

int main(int argc, char* argv[])
{
    //Setup.
    sdl = new SDL_Manager;
    if (sdl->window == NULL){
        err("Could not create window: %s");
    }

    //Main loop.
    SDL_Event e;
    bool quit = false;
    //SDL_StartTextInput();

    //!!! THIS WORKS !!!
    //BulletSim* sim = new BulletSim;
    //sim->runSim();

    while(!quit){
        while(SDL_PollEvent(&e) != 0){
            switch(e.type){
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_TEXTINPUT:
                    SDL_Log("text input!");
                    if(e.text.text[0] == 'q')
                        quit = true;
                    break;
            }
        }

        render();
        SDL_GL_SwapWindow(sdl->window);
    }

    //Clean up.
    delete sdl;
    return 0;
}