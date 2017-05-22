#include <LinearMath/btVector3.h>
#include <LinearMath/btHashMap.h>
#include <math.h>
#include <GLES3/gl3.h>

struct Shader{
    GLuint gProgramID = 0;
    GLint gVertexPos2DLocation = -1,gVertexTransformLocation = -1;
public:
    GLuint getProgramID(){return gProgramID;}
    GLint getVertLocation(){return gVertexPos2DLocation;}
    GLint getVertTransform(){return gVertexTransformLocation;}
    bool init() {
        //Generate program
        gProgramID = glCreateProgram();
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vertexShaderSource[] =
                {
                        "#version 300 es\nin vec3 LVertexPos2D;void main() { gl_Position = vec4( LVertexPos2D.x, LVertexPos2D.y, LVertexPos2D.z, 1 ); }"
                        //"#version 300 es\nin vec3 LVertexPos2D; in uniform vec3 LVertexTransform; void main() { gl_Position = vec4(LVertexTransform,1) * vec4( LVertexPos2D.x, LVertexPos2D.y, LVertexPos2D.z, 1 ); }"
                };

        glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        //Check vertex shader for errors
        GLint vShaderCompiled = GL_FALSE;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
        if (vShaderCompiled != GL_TRUE) {
            SDL_Log("Unable to compile vertex shader %d!\n", vertexShader);
            printShaderLog(vertexShader);
            return false;
        } else {
            //Attach vertex shader to program
            glAttachShader(gProgramID, vertexShader);


            //Create fragment shader
            GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

            //Get fragment source
            const GLchar *fragmentShaderSource[] =
                    {
                            "#version 300 es\nout vec4 LFragment; void main() { LFragment = vec4( 1.0, 1.0, 1.0, 1.0 ); }"
                    };

            //Set fragment source
            glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

            //Compile fragment source
            glCompileShader(fragmentShader);

            //Check fragment shader for errors
            GLint fShaderCompiled = GL_FALSE;
            glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
            if (fShaderCompiled != GL_TRUE) {
                SDL_Log("Unable to compile fragment shader %d!\n", fragmentShader);
                printShaderLog(fragmentShader);
                return false;
            } else {
                //Attach fragment shader to program
                glAttachShader(gProgramID, fragmentShader);

                //Link program
                glLinkProgram(gProgramID);

                //Check for errors
                GLint programSuccess = GL_TRUE;
                glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
                if (programSuccess != GL_TRUE) {
                    SDL_Log("Error linking program %d!\n", gProgramID);
                    printProgramLog(gProgramID);
                    return false;
                }else{
                    gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D" );
                    if( gVertexPos2DLocation == -1 )
                    {
                        SDL_Log( "LVertexPos2D is not a valid glsl program variable!\n" );
                        return false;
                    }
                    /*gVertexTransformLocation = glGetUniformLocation(gProgramID, "LVertexTransform" );
                    if( gVertexTransformLocation == -1 )
                    {
                        SDL_Log( "LVertexTransform is not a valid glsl program variable!\n" );
                        return false;
                    }*/
                }
            }
        }
        return true;
    }

    void printShaderLog( GLuint shader )
    {
        //Make sure name is shader
        if( glIsShader( shader ) )
        {
            //Shader log length
            int infoLogLength = 0;
            int maxLength = infoLogLength;

            //Get info string length
            glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );

            //Allocate string
            char* infoLog = new char[ maxLength ];

            //Get info log
            glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
            if( infoLogLength > 0 )
            {
                //Print Log
                SDL_Log( "%s\n", infoLog );
            }

            //Deallocate string
            delete[] infoLog;
        }
        else
        {
            SDL_Log( "Name %d is not a shader\n", shader );
        }
    }

    void printProgramLog( GLuint program )
    {
        //Make sure name is shader
        if( glIsProgram( program ) )
        {
            //Program log length
            int infoLogLength = 0;
            int maxLength = infoLogLength;

            //Get info string length
            glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );

            //Allocate string
            char* infoLog = new char[ maxLength ];

            //Get info log
            glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
            if( infoLogLength > 0 )
            {
                //Print Log
                SDL_Log( "%s\n", infoLog );
            }

            //Deallocate string
            delete[] infoLog;
        }
        else
        {
            SDL_Log( "Name %d is not a program\n", program );
        }
    }
};

struct Mesh {
    btVector3 transform;
    btAlignedObjectArray<GLuint> indices;
    btAlignedObjectArray<btVector3> vertices;
    GLuint* indicesGL;
    GLfloat* verticesGL;
    GLuint gVBO = 0,gIBO = 0;
public:
    ~Mesh(){delete indicesGL; delete verticesGL;}
    void init(){
        //VBO/IBO data
        getVertices();
        getIndices();

        //Create VBO
        glGenBuffers( 1, &gVBO );
        glBindBuffer( GL_ARRAY_BUFFER, gVBO );
        glBufferData( GL_ARRAY_BUFFER, 3 * vertices.size() * sizeof(GLfloat), verticesGL, GL_STATIC_DRAW );

        //Create IBO
        glGenBuffers( 1, &gIBO );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gIBO );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indicesGL, GL_STATIC_DRAW );
    }

    void render(){
        //Set index data and render
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gIBO );
        glDrawElements( GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL );
    }

    void getIndices(){
        if(indicesGL == NULL){
            indicesGL = new GLuint[indices.size()];
            for(int i = 0;i < indices.size();i++) indicesGL[i] = indices.at(i);
        }
    }

    void getVertices(){
        if(verticesGL == NULL){
            verticesGL = new GLfloat[vertices.size() * 3];
            int index = 0;
            for(int i = 0;i < vertices.size();i++){
                verticesGL[index++] = vertices[i].x();
                verticesGL[index++] = vertices[i].y();
                verticesGL[index++] = vertices[i].z();
            }
        }
    }

    void printIndices(){
        for(int i = 0;i < indices.size();i += 3){
            SDL_Log("FACE %i: {%i,%i,%i}",i/3,indices[i],indices[i+1],indices[i+2]);
        }
    }

    void printVertices(){
        SDL_Log("PRINT VERT START: %i",vertices.size());
        for(int i = 0;i < vertices.size();i++){
            SDL_Log("VERT %i: {%f,%f,%f}",i,vertices[i].x(),vertices[i].y(),vertices[i].z());
        }
    }
};

struct MeshInstance{
    Mesh* mesh;
    Shader* shader;
    btVector3 transform;
public:
    MeshInstance(Mesh* mesh,Shader* shader,btVector3 transform){
        this->mesh = mesh;
        this->shader = shader;
        this->transform = transform;
    }

    void render(){
            //Bind program
            glUseProgram(shader->getProgramID());

            //Enable vertex position
            glEnableVertexAttribArray(shader->getVertLocation());

            //Set vertex data
            glBindBuffer( GL_ARRAY_BUFFER, mesh->gVBO );
            glVertexAttribPointer(shader->getVertLocation(), 3, GL_FLOAT, GL_FALSE,3 * sizeof(GLfloat), NULL );

            //Set transform
            //glVertexAttrib3f(shader->getVertTransform(),0,0,0);

            //Render mesh one time.
            mesh->render();

            //Disable vertex position
            glDisableVertexAttribArray(shader->getVertLocation());

            //Unbind program
            glUseProgram(NULL);
    }
};

class MeshBuilder
{
public:
    btAlignedObjectArray<Mesh> meshes;

    float degreesToRadians = 3.1415927f / 180.0f;

    Mesh build(float width, float height,float depth, int divisionsU,int divisionsV){
        return build(width,height,depth,divisionsU,divisionsV,0,360,0,180);
    }

    Mesh build(float width, float height, float depth,
                          int divisionsU, int divisionsV, float angleUFrom, float angleUTo, float angleVFrom, float angleVTo) {
        Mesh mesh;

        float hw = width * 0.5f;
        float hh = height * 0.5f;
        float hd = depth * 0.5f;
        float auo = degreesToRadians * angleUFrom;
        float stepU = (degreesToRadians * (angleUTo - angleUFrom)) / divisionsU;
        float avo = degreesToRadians * angleVFrom;
        float stepV = (degreesToRadians * (angleVTo - angleVFrom)) / divisionsV;
        float us = 1.0f / divisionsU;
        float vs = 1.0f / divisionsV;
        float u = 0.0f;
        float v = 0.0f;
        float angleU = 0.0f;
        float angleV = 0.0f;

        int s = divisionsU + 3;
        GLuint tmpIndices[s];
        int tempOffset = 0;

        for (int iv = 0; iv <= divisionsV; iv++) {
            angleV = avo + stepV * iv;
            float t = sin(angleV);
            float h = cos(angleV) * hh;
            for (int iu = 0; iu <= divisionsU; iu++) {
                angleU = auo + stepU * iu;
                // Fixme : wrong normal calculation if transform
                mesh.vertices.push_back(btVector3(cos(angleU) * hw * t, h, sin(angleU) * hd * t)/* * mesh.transform*/);
                tmpIndices[tempOffset] = mesh.vertices.size()-1;
                int o = tempOffset + s;
                if ((iv > 0) && (iu > 0)) // FIXME don't duplicate lines and points
                {
                    mesh.indices.push_back(tmpIndices[tempOffset]);                 //corner00 = tmpIndices.get(tempOffset)
                    mesh.indices.push_back(tmpIndices[(o - 1) % s]);                //corner10 = tmpIndices.get((o - 1) % s)
                    mesh.indices.push_back(tmpIndices[(o - (divisionsU + 2)) % s]); //corner11 = tmpIndices.get((o - (divisionsU + 2)) % s)
                    mesh.indices.push_back(tmpIndices[(o - (divisionsU + 2)) % s]); //corner11 = tmpIndices.get((o - (divisionsU + 2)) % s)
                    mesh.indices.push_back(tmpIndices[(o - (divisionsU + 1)) % s]); //corner01 = tmpIndices.get((o - (divisionsU + 1)) % s))
                    mesh.indices.push_back(tmpIndices[tempOffset]);                 //corner00 = tmpIndices.get(tempOffset)
                }
                tempOffset = (tempOffset + 1) % s;
            }
        }

        meshes.push_back(mesh);
        return meshes.at(meshes.size()-1);
    }
};
