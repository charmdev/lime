#include "NevoShaders.h"

#include <iostream>

#ifdef ANDROID
#include <android/log.h>
#endif

using namespace nme;

namespace nevo
{

//GPUProgram
GPUProgram::GPUProgram()
{
    mProg = 0;
}

GPUProgram::~GPUProgram()
{
    glDeleteProgram(mProg);
}

void GPUProgram::bind()
{
    glUseProgram(mProg);
}

void GPUProgram::unbind()
{
        glUseProgram(0);
}

void GPUProgram::setUniform1f(GLuint loc, GLfloat v)
{
    glUniform1f(loc, v);
}

void GPUProgram::setUniform2f(GLuint loc, GLfloat v0, GLfloat v1)
{
    glUniform2f(loc, v0, v1);
}

void GPUProgram::setUniform3f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
    glUniform3f(loc, v0, v1, v2);
}

void GPUProgram::setUniform4f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    glUniform4f(loc, v0, v1, v2, v3);
}

void GPUProgram::setUniform1i(GLuint loc, GLfloat v)
{
    glUniform1i(loc, v);
}

void GPUProgram::setUniform4i(GLuint loc, GLint v0, GLint v1, GLint v2, GLint v3)
{
    glUniform4i(loc, v0, v1, v2, v3);
}

void GPUProgram::setUniform1iv(GLuint loc, GLsizei count, const GLint* v)
{
    glUniform1iv(loc, count, v);
}

void GPUProgram::setMatrix4x4fv(GLuint loc, const GLfloat *v)
{
    glUniformMatrix4fv(loc, 1, 0, v);
}

void GPUProgram::setAttribPointer(GLuint bufId, GLuint attrib, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *ptr)
{
    glBindBuffer(GL_ARRAY_BUFFER, bufId);
    glVertexAttribPointer(attrib, size, type, normalized, stride, ptr);
    glEnableVertexAttribArray(attrib);
}

void GPUProgram::setAttrib4f(GLuint attrib, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    glDisableVertexAttribArray(attrib);
    glVertexAttrib4f(attrib, v0, v1, v2, v3);
}

void GPUProgram::setAttrib1f(GLuint attrib, GLfloat v0)
{
    glDisableVertexAttribArray(attrib);
    glVertexAttrib1f(attrib, v0);
}

void GPUProgram::draw(GLuint bufId, GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufId);
    glDrawElements(mode, count, type, indices);
}

void GPUProgram::createProgram(std::vector<std::string> attribs)
{
    GLint success;
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    const char *vs = mVertSrc.c_str();
    glShaderSource(vert, 1, &vs, NULL);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vert, 512, NULL, log);
        std::cout << "Vertex Compilation Error: " << log << std::endl;
#ifdef ANDROID
        __android_log_print(ANDROID_LOG_INFO, "Vertex Compilation Error: ", log);
#endif
        return;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fs = mFragSrc.c_str();
    glShaderSource(frag, 1, &fs, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(frag, 512, NULL, log);
        std::cout << "Fragment Compilation Error: " << log << std::endl;
#ifdef ANDROID
        __android_log_print(ANDROID_LOG_INFO, "Fragment Compilation Error: ", log);
#endif
        return;
    }

    mProg = glCreateProgram();
    glAttachShader(mProg, vert);
    glAttachShader(mProg, frag);

    for (int i = 0; i < attribs.size(); ++i)
    {
        glBindAttribLocation(mProg, i, attribs[i].c_str());
    }

    glLinkProgram(mProg);
    glGetProgramiv(mProg, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(mProg, 512, NULL, log);
        std::cout << "Linking Error: " << log << std::endl;
#ifdef ANDROID
        __android_log_print(ANDROID_LOG_INFO, "Linking Error: ", log);
#endif
        return;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

GLuint GPUProgram::getUniformLoc(const char *uniformName)
{
    return glGetUniformLocation(mProg, uniformName);
}

GLuint GPUProgram::getAttribLoc(const char *attribName)
{
    return glGetAttribLocation(mProg, attribName);
}

//DefaultShader
DefaultShader::DefaultShader()
{
/*    
#ifdef NME_GLES
	mVertSrc += "precision highp float;";
#endif
    mVertSrc += "                                                   \
        uniform mat4 u_m;                                           \
        uniform vec4 u_c;                                           \
        uniform vec2 u_uv_s;                                        \
                                                                    \
        attribute vec2 a_xy;                                        \
        attribute vec2 a_uv;                                        \
        attribute vec4 a_c;                                         \
        attribute vec4 a_mtl;                                       \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
        varying vec4 v_mtl;                                         \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_uv = a_uv * u_uv_s;                                   \
            v_c = u_c.bgra * a_c.bgra;                              \
            v_mtl = a_mtl;                                          \
            gl_Position = vec4(a_xy, 0.0f, 1.0f) * u_m;             \
        }                                                           \
    ";

#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
        uniform sampler2D u_t[8];                                   \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
        varying vec4 v_mtl;                                         \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 texel = vec4(1.0f, 1.0f, 1.0f, 1.0f);              \
            if (v_mtl[0] == 0.0f) texel = texture2D(u_t[0], v_uv);  \
            if (v_mtl[0] == 1.0f) texel = texture2D(u_t[1], v_uv);  \
            if (v_mtl[0] == 2.0f) texel = texture2D(u_t[2], v_uv);  \
            if (v_mtl[0] == 3.0f) texel = texture2D(u_t[3], v_uv);  \
            if (v_mtl[0] == 4.0f) texel = texture2D(u_t[4], v_uv);  \
            if (v_mtl[0] == 5.0f) texel = texture2D(u_t[5], v_uv);  \
            if (v_mtl[0] == 6.0f) texel = texture2D(u_t[6], v_uv);  \
            if (v_mtl[0] == 7.0f) texel = texture2D(u_t[7], v_uv);  \
                                                                    \
            if (v_mtl[1] == 0.0f) texel.a = texture2D(u_t[0], v_uv).r;\
            if (v_mtl[1] == 1.0f) texel.a = texture2D(u_t[1], v_uv).r;\
            if (v_mtl[1] == 2.0f) texel.a = texture2D(u_t[2], v_uv).r;\
            if (v_mtl[1] == 3.0f) texel.a = texture2D(u_t[3], v_uv).r;\
            if (v_mtl[1] == 4.0f) texel.a = texture2D(u_t[4], v_uv).r;\
            if (v_mtl[1] == 5.0f) texel.a = texture2D(u_t[5], v_uv).r;\
            if (v_mtl[1] == 6.0f) texel.a = texture2D(u_t[6], v_uv).r;\
            if (v_mtl[1] == 7.0f) texel.a = texture2D(u_t[7], v_uv).r;\
                                                                    \
            texel.rgb *= mix(1.0f, 1.0f / texel.a, v_mtl[2]);       \
            texel *= v_c;                                           \
            texel.rgb *= texel.a;                                   \
            texel.a *= v_mtl[3];                                    \
            gl_FragColor = texel;                                   \
        }                                                           \
    ";
*/

#ifdef NME_GLES
	mVertSrc += "precision highp float;";
#endif
    mVertSrc += "                                                   \
        attribute vec2 a_xy;                                        \
        attribute vec2 a_uv;                                        \
        attribute vec4 a_c;                                         \
        attribute vec4 a_mtl;                                       \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
        varying vec4 v_mtl;                                         \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_uv = a_uv;                                            \
            v_c = a_c.bgra;                                         \
            v_mtl = a_mtl;                                          \
            gl_Position = vec4(a_xy, 0.0, 1.0);                     \
        }                                                           \
    ";

#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
        uniform sampler2D u_t[8];                                   \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
        varying vec4 v_mtl;                                         \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 texel = vec4(1.0, 1.0, 1.0, 1.0);                  \
            if (v_mtl[0] == 0.0) texel = texture2D(u_t[0], v_uv);  \
            if (v_mtl[0] == 1.0) texel = texture2D(u_t[1], v_uv);  \
            if (v_mtl[0] == 2.0) texel = texture2D(u_t[2], v_uv);  \
            if (v_mtl[0] == 3.0) texel = texture2D(u_t[3], v_uv);  \
            if (v_mtl[0] == 4.0) texel = texture2D(u_t[4], v_uv);  \
            if (v_mtl[0] == 5.0) texel = texture2D(u_t[5], v_uv);  \
            if (v_mtl[0] == 6.0) texel = texture2D(u_t[6], v_uv);  \
            if (v_mtl[0] == 7.0) texel = texture2D(u_t[7], v_uv);  \
                                                                    \
            if (v_mtl[1] == 0.0) texel.a = texture2D(u_t[0], v_uv).r;\
            if (v_mtl[1] == 1.0) texel.a = texture2D(u_t[1], v_uv).r;\
            if (v_mtl[1] == 2.0) texel.a = texture2D(u_t[2], v_uv).r;\
            if (v_mtl[1] == 3.0) texel.a = texture2D(u_t[3], v_uv).r;\
            if (v_mtl[1] == 4.0) texel.a = texture2D(u_t[4], v_uv).r;\
            if (v_mtl[1] == 5.0) texel.a = texture2D(u_t[5], v_uv).r;\
            if (v_mtl[1] == 6.0) texel.a = texture2D(u_t[6], v_uv).r;\
            if (v_mtl[1] == 7.0) texel.a = texture2D(u_t[7], v_uv).r;\
                                                                    \
            texel.rgb *= mix(1.0, 1.0 / texel.a, v_mtl[2]);       \
            texel *= v_c;                                           \
            texel.rgb *= texel.a;                                   \
            texel.a *= v_mtl[3];                                    \
            gl_FragColor = texel;                                   \
        }                                                           \
    ";

    std::vector<std::string> attribs;
    attribs.push_back("a_xy");
    attribs.push_back("a_uv");
    attribs.push_back("a_c");
    attribs.push_back("a_mtl");
    createProgram(attribs);

    // mA_XY = getAttribLoc("a_xy");
    // mA_UV = getAttribLoc("a_uv");
    // mA_C = getAttribLoc("a_c");
    // mA_MTL = getAttribLoc("a_mtl");
    // mU_M = getUniformLoc("u_m");
    // mU_C = getUniformLoc("u_c");
    // mU_UV_S = getUniformLoc("u_uv_s");
    // mU_T = getUniformLoc("u_t");

    mA_XY = getAttribLoc("a_xy");
    mA_UV = getAttribLoc("a_uv");
    mA_C = getAttribLoc("a_c");
    mA_MTL = getAttribLoc("a_mtl");
    mU_T = getUniformLoc("u_t");
}

}