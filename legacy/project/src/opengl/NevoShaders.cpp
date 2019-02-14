#include "NevoShaders.h"

#include <iostream>
#include <sstream>

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

void GPUProgram::setUniform1iv(GLuint loc, GLsizei count, const GLint* v)
{
    glUniform1iv(loc, count, v);
}

void GPUProgram::setMatrix4x4fv(GLuint loc, const GLfloat *v)
{
    glUniformMatrix4fv(loc, 1, 0, v);
}

void GPUProgram::createProgram()
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
        return;
    }

    mProg = glCreateProgram();
    glAttachShader(mProg, vert);
    glAttachShader(mProg, frag);
    glLinkProgram(mProg);
    glGetProgramiv(mProg, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(mProg, 512, NULL, log);
        std::cout << "Linking Error: " << log << std::endl;
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
DefaultShader::DefaultShader(int numTexChannels)
{
    mU_T_Val.resize(numTexChannels);
    for (int i = 0; i < numTexChannels; ++i)
    {
        mU_T_Val[i] = i;
    }

#ifdef NME_GLES
	mVertSrc += "precision mediump float;";
#endif
    mVertSrc += "                                                   \
        uniform mat4 u_m;                                           \
        uniform vec2 u_ts;                                          \
        uniform vec4 u_c;                                           \
                                                                    \
        attribute vec2 a_xy;                                        \
        attribute vec2 a_uv;                                        \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_uv = a_uv * u_ts;                                     \
            v_c = u_c;                                              \
            gl_Position = vec4(a_xy, 0.0, 1.0) * u_m;               \
        }                                                           \
    ";

    std::ostringstream oss;
    oss.clear();
    oss << numTexChannels;
#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
    uniform sampler2D u_t[";
    mFragSrc += oss.str();
    mFragSrc += "];                                                 \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 texel = texture2D(u_t[0], v_uv);                   \
            gl_FragColor = texel * v_c;                             \
        }                                                           \
    ";

    createProgram();

    mA_XY = getAttribLoc("a_xy");
    mA_UV = getAttribLoc("a_uv");
    mU_M = getUniformLoc("u_m");
    mU_T = getUniformLoc("u_t");
    mU_TS = getUniformLoc("u_ts");
    mU_C = getUniformLoc("u_c");
}

}