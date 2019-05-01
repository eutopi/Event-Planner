//
//  main.cpp
//  EventPlanner
//
//  Created by Tongyu Zhou on 4/12/19.
//  Copyright © 2019 AIT. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>        // must be downloaded
#include <GL/freeglut.h>    // must be downloaded unless you have an Apple
#endif

const unsigned int windowWidth = 512, windowHeight = 512;

// OpenGL major and minor versions
int majorVersion = 3, minorVersion = 0;

// row-major matrix 4x4
struct mat4
{
    float m[4][4];
public:
    mat4() {}
    mat4(float m00, float m01, float m02, float m03,
         float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23,
         float m30, float m31, float m32, float m33)
    {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }
    
    mat4 operator*(const mat4& right)
    {
        mat4 result;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
            }
        }
        return result;
    }
    operator float*() { return &m[0][0]; }
};


// 3D point in homogeneous coordinates
struct vec4
{
    float v[4];
    
    vec4(float x = 0, float y = 0, float z = 0, float w = 1)
    {
        v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    }
    
    vec4 operator*(const mat4& mat)
    {
        vec4 result;
        for (int j = 0; j < 4; j++)
        {
            result.v[j] = 0;
            for (int i = 0; i < 4; i++) result.v[j] += v[i] * mat.m[i][j];
        }
        return result;
    }
    
    vec4 operator+(const vec4& vec)
    {
        vec4 result(v[0] + vec.v[0], v[1] + vec.v[1], v[2] + vec.v[2], v[3] + vec.v[3]);
        return result;
    }
};

// 2D point in Cartesian coordinates
struct vec2
{
    float x, y;
    
    vec2(float x = 0.0, float y = 0.0) : x(x), y(y) {}
    
    vec2 operator+(const vec2& v)
    {
        return vec2(x + v.x, y + v.y);
    }
};


class Shader
{
protected:
    
    unsigned int shaderProgram;
    
    void getErrorInfo(unsigned int handle)
    {
        int logLen;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0)
        {
            char * log = new char[logLen];
            int written;
            glGetShaderInfoLog(handle, logLen, &written, log);
            printf("Shader log:\n%s", log);
            delete log;
        }
    }
    
    // check if shader could be compiled
    void checkShader(unsigned int shader, char * message)
    {
        int OK;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
        if (!OK)
        {
            printf("%s!\n", message);
            getErrorInfo(shader);
        }
    }
    
    // check if shader could be linked
    void checkLinking(unsigned int program)
    {
        int OK;
        glGetProgramiv(program, GL_LINK_STATUS, &OK);
        if (!OK)
        {
            printf("Failed to link shader program!\n");
            getErrorInfo(program);
        }
    }
    
public:
    Shader() {
        shaderProgram = 0;
    }
    
    void CompileShader(const char *vertexSource, const char *fragmentSource)
    {
        // create vertex shader from string
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
        
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkShader(vertexShader, "Vertex shader error");
        
        // create fragment shader from string
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
        
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkShader(fragmentShader, "Fragment shader error");
        
        // attach shaders to a single program
        shaderProgram = glCreateProgram();
        if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
        
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        
    }
    
    void LinkShader()
    {
        // program packaging
        glLinkProgram(shaderProgram);
        checkLinking(shaderProgram);
        
    }
    
    //deconstructor
    ~Shader() {
        glDeleteProgram(shaderProgram);
    }
    
    void Run()
    {
        // make this program run
        glUseProgram(shaderProgram);
    }
    
    virtual void UploadColor(vec4 color) {}
    virtual void UploadStripeColor(vec4 color) {}
    virtual void UploadStripeSize(int size) {}
    virtual void UploadM(mat4 M) {}
    virtual void UploadSelected(bool b) {}
    
};

class StandardShader : public Shader
{
    
public:
    StandardShader()
    {
        // vertex shader in GLSL
        const char *vertexSource = R"(
#version 410
        precision highp float;
        
        in vec2 vertexPosition;        // variable input from Attrib Array selected by glBindAttribLocation
        uniform vec3 vertexColor;
        uniform mat4 M;
        uniform bool selected;
        out vec3 color;            // output attribute
        out vec2 modelSpacePos;
        
        void main()
        {
            if (selected) {
                color = vec3(1,1,1);
            }
            else {
                color = vertexColor;
            }
            modelSpacePos = vertexPosition;
            gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * M;      // copy position from input to output
        }
        )";
        
        // fragment shader in GLSL
        const char *fragmentSource = R"(
#version 410
        precision highp float;
        
        in vec3 color;            // variable input: interpolated from the vertex colors
        in vec2 modelSpacePos;
        out vec4 fragmentColor;        // output that goes to the raster memory as told by glBindFragDataLocation
        
        void main()
        {
            fragmentColor = vec4(color, 1); // extend RGB to RGBA
        }
        )";
        
        CompileShader(vertexSource, fragmentSource);
        
        // connect Attrib Array to input variables of the vertex shader
        glBindAttribLocation(shaderProgram, 0, "vertexPosition"); // vertexPosition gets values from Attrib Array 0
        
        // connect the fragmentColor to the frame buffer memory
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor"); // fragmentColor goes to the frame buffer memory
        
        LinkShader();
        
    }
    
    void UploadColor(vec4 color) {
        int location = glGetUniformLocation(shaderProgram, "vertexColor");
        if (location >= 0) glUniform3fv(location, 1, &color.v[0]); // set uniform variable vertexColor
        else printf("uniform vertex color cannot be set\n");
    }
    
    void UploadM(mat4 M) {
        int location = glGetUniformLocation(shaderProgram, "M");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
        else printf("uniform M cannot be set\n");
    }
    
    void UploadSelected(bool selected) {
        int location = glGetUniformLocation(shaderProgram, "selected");
        if (location >= 0) glUniform1i(location, selected);
        else printf("uniform selected boolean cannot be set\n");
    }
    
};

class StripesShader : public Shader
{
    
public:
    StripesShader()
    {
        // vertex shader in GLSL
        const char *vertexSource = R"(
#version 410
        precision highp float;
        
        in vec2 vertexPosition;        // variable input from Attrib Array selected by glBindAttribLocation
        uniform vec3 vertexColor;
        uniform vec3 stripeColor;
        uniform float stripeSize;
        uniform mat4 M;
        uniform bool selected;
        out vec3 color;            // output attribute
        out vec3 scolor;
        out float size;
        out vec2 modelSpacePos;
        
        void main()
        {
            if (selected) {
                color = vec3(1,1,1);
            }
            else {
                color = vertexColor;
            }
            scolor = stripeColor;
            size = stripeSize;
            modelSpacePos = vertexPosition;
            gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * M;      // copy position from input to output
        }
        )";
        
        // fragment shader in GLSL
        const char *fragmentSource = R"(
#version 410
        precision highp float;
        
        in vec3 color;            // variable input: interpolated from the vertex colors
        in vec3 scolor;
        in float size;
        
        in vec2 modelSpacePos;
        out vec4 fragmentColor;        // output that goes to the raster memory as told by glBindFragDataLocation
        
        void main()
        {
            float li = mix(modelSpacePos.x, modelSpacePos.y, 0.5);
            if (fract(li * size) < 0.5 )
                fragmentColor = vec4(scolor, 1);
            else
                fragmentColor = vec4(color, 1);
        }
        )";
        
        CompileShader(vertexSource, fragmentSource);
        
        // connect Attrib Array to input variables of the vertex shader
        glBindAttribLocation(shaderProgram, 0, "vertexPosition"); // vertexPosition gets values from Attrib Array 0
        
        // connect the fragmentColor to the frame buffer memory
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor"); // fragmentColor goes to the frame buffer memory
        
        LinkShader();
        
    }
    
    void UploadColor(vec4 color) {
        int location = glGetUniformLocation(shaderProgram, "vertexColor");
        if (location >= 0) glUniform3fv(location, 1, &color.v[0]);
        else printf("uniform vertex color cannot be set\n");
    }
    
    void UploadStripeColor(vec4 stripeColor) {
        int location = glGetUniformLocation(shaderProgram, "stripeColor");
        if (location >= 0) glUniform3fv(location, 1, &stripeColor.v[0]);
        else printf("uniform stripe color cannot be set\n");
    }
    
    //glUniform1f float
    //glUniform1i int
    void UploadStripeSize(float size) {
        int location = glGetUniformLocation(shaderProgram, "stripeSize");
        if (location >= 0) glUniform1f(location, size);
        else printf("uniform stripe size cannot be set\n");
    }
    
    
    void UploadM(mat4 M) {
        int location = glGetUniformLocation(shaderProgram, "M");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
        else printf("uniform M cannot be set\n");
    }
    
    void UploadSelected(bool selected) {
        int location = glGetUniformLocation(shaderProgram, "selected");
        if (location >= 0) glUniform1i(location, selected);
        else printf("uniform selected boolean cannot be set\n");
    }
    
};

class HeartbeatShader : public Shader
{
    
public:
    HeartbeatShader()
    {
        // vertex shader in GLSL
        const char *vertexSource = R"(
#version 410
        precision highp float;
        
        in vec2 vertexPosition;        // variable input from Attrib Array selected by glBindAttribLocation
        uniform vec3 vertexColor;
        uniform mat4 M;
        uniform float t;
        uniform bool selected;
        out vec3 color;            // output attribute
        out float time;
        
        void main()
        {
            if (selected) {
                color = vec3(1,1,1);
            }
            else {
                color = vertexColor;
            }
            time = t;
            gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * M;      // copy position from input to output
        }
        )";
        
        // fragment shader in GLSL
        const char *fragmentSource = R"(
#version 410
        precision highp float;
        
        in vec3 color;            // variable input: interpolated from the vertex colors
        in float time;
        out vec4 fragmentColor;        // output that goes to the raster memory as told by glBindFragDataLocation
        
        void main()
        {
            vec4 color1 = vec4(1.0, (sin(time) / 2.0f + 0.5f), 1.0, 1.0);
            float a = time - int(time);
            fragmentColor = mix(vec4(color, 1), color1, a);
        }
        )";
        
        CompileShader(vertexSource, fragmentSource);
        
        // connect Attrib Array to input variables of the vertex shader
        glBindAttribLocation(shaderProgram, 0, "vertexPosition"); // vertexPosition gets values from Attrib Array 0
        
        // connect the fragmentColor to the frame buffer memory
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor"); // fragmentColor goes to the frame buffer memory
        
        LinkShader();
        
    }
    
    void UploadColor(vec4 color) {
        int location = glGetUniformLocation(shaderProgram, "vertexColor");
        if (location >= 0) glUniform3fv(location, 1, &color.v[0]); // set uniform variable vertexColor
        else printf("uniform vertex color cannot be set\n");
    }
    
    void UploadM(mat4 M) {
        int location = glGetUniformLocation(shaderProgram, "M");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
        else printf("uniform M cannot be set\n");
    }
    
    void UploadTime(float time) {
        int location = glGetUniformLocation(shaderProgram, "t");
        if (location >= 0) glUniform1f(location, time);
        else printf("uniform stripe size cannot be set\n");
    }
    
    void UploadSelected(bool selected) {
        int location = glGetUniformLocation(shaderProgram, "selected");
        if (location >= 0) glUniform1i(location, selected);
        else printf("uniform selected boolean cannot be set\n");
    }
    
};

class Material {
    
    Shader* shader;
    
public:
    Material(Shader* shader) : shader(shader) {}
    
    virtual void UploadAttributes() {}
    virtual void SetSelected(bool b) {}
};

class StandardMaterial : public Material {
    
    StandardShader* shader;
    vec4 color;
    
public:
    StandardMaterial(StandardShader* shader, vec4 color) : Material(shader), shader(shader), color(color) {}
    
    void UploadAttributes() {
        shader->UploadColor(color);
    }
};

class WideRedStripes : public Material {
    
    StripesShader* shader;
    vec4 color;
    vec4 stripeColor;
    float stripeSize;
    
public:
    WideRedStripes(StripesShader* shader, vec4 color) :
    Material(shader), shader(shader), color(color) {
        stripeColor = vec4(1, 0, 0);
        stripeSize = 1.0;
    }
    
    void UploadAttributes() {
        shader->UploadColor(color);
        shader->UploadStripeColor(stripeColor);
        shader->UploadStripeSize(stripeSize);
    }
};

class NarrowCyanStripes : public Material {
    
    StripesShader* shader;
    vec4 color;
    vec4 stripeColor;
    float stripeSize;
    
public:
    NarrowCyanStripes(StripesShader* shader, vec4 color) :
    Material(shader), shader(shader), color(color) {
        stripeColor = vec4(0, 1, 1);
        stripeSize = 5.0;
    }
    
    void UploadAttributes() {
        shader->UploadColor(color);
        shader->UploadStripeColor(stripeColor);
        shader->UploadStripeSize(stripeSize);
    }
};

class HeartbeatMaterial : public Material {
    
    HeartbeatShader* shader;
    vec4 color;
    
public:
    HeartbeatMaterial(HeartbeatShader* shader, vec4 color) : Material(shader), shader(shader), color(color) {}
    
    void UploadAttributes() {
        shader->UploadColor(color);
        float time = glutGet(GLUT_ELAPSED_TIME) * 0.001;
        shader->UploadTime(time);
    }
    
};

class Geometry{
    
protected: unsigned int vao;    // vertex array object id
    
public:
    Geometry(){
        glGenVertexArrays(1, &vao);    // create a vertex array object
    }
    
    virtual void Draw() = 0;
};

class Triangle : public Geometry
{
    unsigned int vbo;        // vertex buffer object
    
public:
    Triangle()
    {
        glBindVertexArray(vao);        // make it active
        
        glGenBuffers(1, &vbo);        // generate a vertex buffer object
        
        // vertex coordinates: vbo -> Attrib Array 0 -> vertexPosition of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
        static float vertexCoords[] = { 0, 0, 1, 0, 0, 1 };    // vertex data on the CPU
        
        glBufferData(GL_ARRAY_BUFFER,    // copy to the GPU
                     sizeof(vertexCoords),    // size of the vbo in bytes
                     vertexCoords,        // address of the data array on the CPU
                     GL_STATIC_DRAW);    // copy to that part of the memory which is not modified
        
        // map Attribute Array 0 to the currently bound vertex buffer (vbo)
        glEnableVertexAttribArray(0);
        
        // data organization of Attribute Array 0
        glVertexAttribPointer(0,    // Attribute Array 0
                              2, GL_FLOAT,        // components/attribute, component type
                              GL_FALSE,        // not in fixed point format, do not normalized
                              0, NULL);        // stride and offset: it is tightly packed
    }
    
    void Draw()
    {
        glBindVertexArray(vao);    // make the vao and its vbos active playing the role of the data source
        glDrawArrays(GL_TRIANGLES, 0, 3); // draw a single triangle with vertices defined in vao
    }
};

class Quad : public Geometry
{
    unsigned int vbo;
    
public:
    Quad()
    {
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        static float vertexCoords[] = { 0, 0, 1, 0, 0, 1, 1, 1};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

class RoundTable : public Geometry
{
    unsigned int vbo;
    int res = 30;
    float radius = 1;
    
public:
    
    RoundTable(float radius, int res) : radius(radius), res(res)
    {
        float vertexCoords[(res+2)*2];
        vertexCoords[0] = 0;
        vertexCoords[1] = 0;
        float theta = 0;
        
        for (int i = 1; i < res+2; i++) {
            float x = radius * cosf(theta * M_PI / 180.0f);
            float y = radius * sinf(theta * M_PI / 180.0f);
            vertexCoords[2*i] = x;
            vertexCoords[2*i+1] = y;
            theta += 360/res;
        }
        
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, res+2);
    }
};

class Plant : public Geometry
{
    unsigned int vbo;
    int res = 10;
    float radius = 1;
    
public:
    Plant()
    {
        float vertexCoords[(res+2)*2];
        vertexCoords[0] = 0;
        vertexCoords[1] = 0;
        float theta = 0;
        
        for (int i = 1; i < res+2; i++) {
            float r = radius;
            if (i % 2 == 0) {
                r = radius/2;
            }
            
            float x = r * cosf(theta * M_PI / 180.0f);
            float y = r * sinf(theta * M_PI / 180.0f);
            
            vertexCoords[2*i] = x;
            vertexCoords[2*i+1] = y;
            
            theta += 360/res;
        }
        
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, res+2);
    }
};

class CoatRack : public Geometry
{
    unsigned int vbo;
    float k;
    int res;
    
public:
    
    CoatRack(int k, int res) : k(k), res(res)
    {
        float vertexCoords[(res+2)*2];
        vertexCoords[0] = 0;
        vertexCoords[1] = 0;
        float theta = 0;
        
        for (int i = 1; i < res+2; i++) {
            float radius = cosf(k * theta * M_PI / 180.0f);
            float x = radius * cosf(theta * M_PI / 180.0f);
            float y = radius * sinf(theta * M_PI / 180.0f);
            
            vertexCoords[2*i] = x;
            vertexCoords[2*i+1] = y;
            
            //printf( "%f\n", theta );
            theta += 360.0/res;
        }
        
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, res+2);
    }
};

class Mesh{
    
    Geometry *geometry;
    Material *material;
    
public:
    Mesh(Geometry *geometry,
         Material *material) : geometry(geometry), material(material) {}
    
    void Draw() {
        material->UploadAttributes();
        geometry->Draw();
    }
};

bool keyboardState[256] = {false};

class Camera {
    
    vec2 center;
    float horizontal_size;
    float vertical_size;
public:
    Camera(vec2 center, float horizontal_size, float vertical_size) {
        this->center = center;
        this->horizontal_size = horizontal_size;
        this->vertical_size = vertical_size;
    }
    
    mat4 GetViewTransformationMatrix() {
        mat4 M = {1/horizontal_size,0,0,0,
            0,1/vertical_size,0,0,
            0,0,1,0,
            -center.x,-center.y,0,1};
        return M;
    }
    
    void Move(double dt) {
        if (keyboardState['z']) {
            this->horizontal_size = horizontal_size - dt;
            this->vertical_size = horizontal_size - dt;
        }
        if (keyboardState['x']) {
            this->horizontal_size = horizontal_size + dt;
            this->vertical_size = horizontal_size + dt;
        }
        if (keyboardState['i']) {
            this->center.y = center.y + dt;
        }
        if (keyboardState['k']) {
            this->center.y = center.y - dt;
        }
        if (keyboardState['l']) {
            this->center.x = center.x + dt;
        }
        if (keyboardState['j']) {
            this->center.x = center.x - dt;
        }
        glutPostRedisplay();
    }
};

Camera camera(vec2(0,0),1.5,1.5);

class Object{
    Shader *shader;
    Mesh *mesh;
    vec2 position, scaling;
    vec2 offset_position = vec2(0, 0);
    float orientation;
    float offset_orientation;
    bool selected = false;
    
public:
    Object(Shader *shader, Mesh *mesh, vec2 position, vec2 scaling, float orientation) :
    shader(shader), mesh(mesh), position(position), scaling(scaling), orientation(orientation) {}
    
    void UploadAttributes() {
        mat4 S = {scaling.x,0,0,0,
            0,scaling.y,0,0,
            0,0,1,0,
            0,0,0,1};
        
        float radians = (orientation+offset_orientation)/180*M_PI;
        mat4 R = {cos(radians),sin(radians),0,0,
            -sin(radians),cos(radians),0,0,
            0,0,1,0,
            0,0,0,1};
        
        mat4 T = {1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            position.x+offset_position.x, position.y+offset_position.y,0,1};
        
        mat4 V = camera.GetViewTransformationMatrix();
        mat4 M = S * R * T * V; // scaling, rotation, and translation
        shader->UploadM(M);
        shader->UploadSelected(selected);
        
    }
    
    Shader* GetShader() {
        return shader;
    }
    
    void SetSelected(bool b) {
        selected = b;
    }
    
    bool GetSelected() {
        return selected;
    }
    
    void SetOffsetPosition(vec2 p) {
        offset_position = p;
    }
    
    void SetPosition(vec2 p) {
        position = vec2(position.x + p.x, position.y + p.y);
        offset_position = vec2(0, 0);
    }
    
    vec2 GetPosition() {
        return position;
    }
    
    void SetOrientation(double t) {
        offset_orientation = offset_orientation + (float)t*200;
    }
    
    void Draw() {
        UploadAttributes();
        mesh->Draw();
    }
};

class Scene {
    StandardShader* shader;
    StripesShader* shader2;
    HeartbeatShader* shader3;
    std::vector<Material*> materials;
    std::vector<Geometry*> geometries;
    std::vector<Mesh*> meshes;
    std::vector<Object*> objects;
public:
    Scene() {
        shader = 0;
        shader2 = 0;
        shader3 = 0;
    }
    void Initialize() {
        
        shader = new StandardShader();
        shader2 = new StripesShader();
        shader3 = new HeartbeatShader();
        
        materials.push_back(new StandardMaterial(shader, vec4(1, 0, 0)));
        materials.push_back(new StandardMaterial(shader, vec4(0, 1, 0)));
        materials.push_back(new StandardMaterial(shader, vec4(0, 0, 1)));
        
        geometries.push_back(new RoundTable(1,30));
        geometries.push_back(new Plant());
        geometries.push_back(new CoatRack(4,80));
        
        meshes.push_back(new Mesh(geometries[0], materials[0]));
        meshes.push_back(new Mesh(geometries[1], materials[1]));
        meshes.push_back(new Mesh(geometries[2], materials[2]));
        
        objects.push_back(new Object(shader, meshes[0], vec2(-0.5, -0.5), vec2(0.5, 0.5), 10.0));
        objects.push_back(new Object(shader, meshes[1], vec2(0.25, 0.5), vec2(0.5, 0.5), -30.0));
        objects.push_back(new Object(shader, meshes[2], vec2(0, 0), vec2(0.5, 0.5), 0));
        
        materials.push_back(new WideRedStripes(shader2, vec4(1.0, 1.0, 0.5)));
        geometries.push_back(new RoundTable(1,30));
        meshes.push_back(new Mesh(geometries[3], materials[3]));
        objects.push_back(new Object(shader2, meshes[3], vec2(0.5, -0.5), vec2(0.3, 0.3), 0));
        
        materials.push_back(new NarrowCyanStripes(shader2, vec4(1.0, 0.5, 0)));
        geometries.push_back(new Plant());
        meshes.push_back(new Mesh(geometries[4], materials[4]));
        objects.push_back(new Object(shader2, meshes[4], vec2(0.9, 0), vec2(0.3, 0.3), 0));
        
        materials.push_back(new HeartbeatMaterial(shader3, vec4(0.5, 0, 0)));
        geometries.push_back(new CoatRack(3,60));
        meshes.push_back(new Mesh(geometries[5], materials[5]));
        objects.push_back(new Object(shader3, meshes[5], vec2(-0.7, 0.7), vec2(0.8, 0.8), 0));
        
    }
    
    std::vector<Material*> GetMaterials() {
        return materials;
    }
    
    void SetMaterials(std::vector<Material*> m) {
        materials = m;
    }
    
    std::vector<Geometry*> GetGeometries() {
        return geometries;
    }
    
    void SetGeometries(std::vector<Geometry*> g) {
        geometries = g;
    }
    
    std::vector<Mesh*> GetMeshes() {
        return meshes;
    }
    
    void SetMeshes(std::vector<Mesh*> m) {
        meshes = m;
    }
    
    std::vector<Object*> GetObjects() {
        return objects;
    }
    
    void SetObjects(std::vector<Object*> o) {
        objects = o;
    }
    
    ~Scene() {
        for(int i = 0; i < materials.size(); i++) delete materials[i];
        for(int i = 0; i < geometries.size(); i++) delete geometries[i];
        for(int i = 0; i < meshes.size(); i++) delete meshes[i];
        for(int i = 0; i < objects.size(); i++) delete objects[i];
        if(shader) delete shader;
        if(shader2) delete shader2;
        if(shader3) delete shader3;
    }
    
    void Draw()
    {
        for(int i = 0; i < objects.size(); i++) {
            objects[i]->GetShader()->Run();
            objects[i]->Draw();
        }
    }
};

Scene *gScene = 0;

// initialization, create an OpenGL context
void onInitialization()
{
    glViewport(0, 0, windowWidth, windowHeight);
    
    gScene = new Scene();
    gScene->Initialize();
}

void onExit()
{
    delete gScene;
    printf("exit");
}

// window has become invalid: redraw
void onDisplay()
{
    glClearColor(0, 0, 0, 0); // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
    
    gScene->Draw();
    
    glutSwapBuffers(); // exchange the two buffers
}

vec2 mouseStartLocation;
vec2 offset;

void onMouse(int button, int state, int x, int y) {
    
    std::vector<Object*> objects = gScene->GetObjects();
    
    if (state == GLUT_DOWN) {
        // normalize
        float cx = (float)x / (float)windowWidth;
        float cy = (float)y / (float)windowHeight;
        
        // convert to coordinates
        cx = (cx-0.5)/0.5;
        cy = -(cy-0.5)/0.5;
        
        mouseStartLocation = vec2(cx, cy);
        
        float threshold = 0.3;
        int index = -1;
        
        
        for (int i = 0; i < objects.size(); i++) {
            vec2 pos = objects[i]->GetPosition();
            if (sqrt(pow((pos.x-cx), 2.0) + pow((pos.y-cy), 2.0)) <= threshold) index = i;
        }
        
        for (int i = 0; i < objects.size(); i++) objects[i]->SetSelected(false);
        if (index != -1) objects[index]->SetSelected(true);
        
        printf("On coordinate %f, %f\n", cx, cy);
    }
    else if (state == GLUT_UP) {
        for (int i = 0; i < objects.size(); i++) {
            if (objects[i]->GetSelected()) {
                objects[i]->SetPosition(offset);
            }
        }
        mouseStartLocation = vec2(0,0);
        offset = vec2(0,0);
    }
    glutPostRedisplay();
}

void onMouseDrag(int x, int y) {
    
    float cx = (float)x / (float)windowWidth;
    float cy = (float)y / (float)windowHeight;
    
    // convert to coordinates
    cx = (cx-0.5)/0.5;
    cy = -(cy-0.5)/0.5;
    
    offset = vec2(cx-mouseStartLocation.x, cy-mouseStartLocation.y);
    
    std::vector<Object*> objects = gScene->GetObjects();
    for (int i = 0; i < objects.size(); i++) {
        if (objects[i]->GetSelected()) {
            objects[i]->SetOffsetPosition(offset);
        }
    }
    
    // neeD some way to record last position in object
    glutPostRedisplay();
}

void onKeyboardUp(unsigned char key, int i, int j) {
    
    std::vector<Material*> materials = gScene->GetMaterials();
    std::vector<Geometry*> geometries = gScene->GetGeometries();
    std::vector<Mesh*> meshes = gScene->GetMeshes();
    std::vector<Object*> objects = gScene->GetObjects();
    for (int i = 0; i < objects.size(); i++) {
        if (keyboardState[127] && objects[i]->GetSelected()) {
            materials.erase(materials.begin()+i);
            geometries.erase(geometries.begin()+i);
            meshes.erase(meshes.begin()+i);
            objects.erase(objects.begin()+i);
        }
    }
    gScene->SetMaterials(materials);
    gScene->SetGeometries(geometries);
    gScene->SetMeshes(meshes);
    gScene->SetObjects(objects);
    
    keyboardState[key] = false;
    glutPostRedisplay();
}

void onKeyboard(unsigned char key, int i, int j) {
    keyboardState[key] = true;
}

void onIdle( ) {
    // time elapsed since program started, in seconds
    double t = glutGet(GLUT_ELAPSED_TIME) * 0.001;
    // variable to remember last time idle was called
    static double lastTime = 0.0;
    // time difference between calls: time step
    double dt = abs(t - lastTime);
    // store time
    lastTime = t;
    camera.Move(dt);
    
    std::vector<Object*> objects = gScene->GetObjects();
    for (int i = 0; i < objects.size(); i++) {
        if (objects[i]->GetSelected() && keyboardState['a']) {
            objects[i]->SetOrientation(dt);
        }
        if (objects[i]->GetSelected() && keyboardState['d']) {
            objects[i]->SetOrientation(-dt);
        }
    }
    
    glutPostRedisplay();
}

int main(int argc, char * argv[])
{
    glutInit(&argc, argv);
#if !defined(__APPLE__)
    glutInitContextVersion(majorVersion, minorVersion);
#endif
    glutInitWindowSize(windowWidth, windowHeight);     // application window is initially of resolution 512x512
    glutInitWindowPosition(50, 50);            // relative location of the application window
#if defined(__APPLE__)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutCreateWindow("Triangle Rendering");
    
#if !defined(__APPLE__)
    glewExperimental = true;
    glewInit();
#endif
    printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
    printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
    printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    onInitialization();
    
    glutDisplayFunc(onDisplay); // register event handlers
    glutMouseFunc(onMouse);
    glutMotionFunc(onMouseDrag);
    glutKeyboardFunc(onKeyboard);
    glutKeyboardUpFunc(onKeyboardUp);
    glutIdleFunc(onIdle);
    
    glutMainLoop();
    onExit();
    return 1;
}
