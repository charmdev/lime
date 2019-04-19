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
    if (loc == -1) return;
    glUniform1f(loc, v);
}

void GPUProgram::setUniform2f(GLuint loc, GLfloat v0, GLfloat v1)
{
    if (loc == -1) return;
    glUniform2f(loc, v0, v1);
}

void GPUProgram::setUniform3f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2)
{
    if (loc == -1) return;
    glUniform3f(loc, v0, v1, v2);
}

void GPUProgram::setUniform4f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    if (loc == -1) return;
    glUniform4f(loc, v0, v1, v2, v3);
}

void GPUProgram::setUniform1i(GLuint loc, GLfloat v)
{
    if (loc == -1) return;
    glUniform1i(loc, v);
}

void GPUProgram::setUniform4i(GLuint loc, GLint v0, GLint v1, GLint v2, GLint v3)
{
    if (loc == -1) return;
    glUniform4i(loc, v0, v1, v2, v3);
}

void GPUProgram::setUniform1iv(GLuint loc, GLsizei count, const GLint* v)
{
    if (loc == -1) return;
    glUniform1iv(loc, count, v);
}

void GPUProgram::setMatrix4x4fv(GLuint loc, const GLfloat *v)
{
    if (loc == -1) return;
    glUniformMatrix4fv(loc, 1, 0, v);
}

void GPUProgram::setAttribPointer(GLuint bufId, GLuint attrib, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *ptr)
{
    if (attrib == -1) return;
    glBindBuffer(GL_ARRAY_BUFFER, bufId);
    glVertexAttribPointer(attrib, size, type, normalized, stride, ptr);
    glEnableVertexAttribArray(attrib);
}

void GPUProgram::setAttrib4f(GLuint attrib, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    if (attrib == -1) return;
    glDisableVertexAttribArray(attrib);
    glVertexAttrib4f(attrib, v0, v1, v2, v3);
}

void GPUProgram::setAttrib1f(GLuint attrib, GLfloat v0)
{
    if (attrib == -1) return;
    glDisableVertexAttribArray(attrib);
    glVertexAttrib1f(attrib, v0);
}

void GPUProgram::draw(GLuint bufId, GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufId);
    glDrawElements(mode, count, type, indices);
}

void GPUProgram::disableAttribs()
{
    if (mA_XY != -1) glDisableVertexAttribArray(mA_XY);
    if (mA_UV != -1) glDisableVertexAttribArray(mA_UV);
    if (mA_C != -1) glDisableVertexAttribArray(mA_C);
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

    mA_XY = getAttribLoc("a_xy");
    mA_UV = getAttribLoc("a_uv");
    mA_C = getAttribLoc("a_c");
    mU_M = getUniformLoc("u_m");
    mU_UV_S = getUniformLoc("u_uv_s");
    mU_C = getUniformLoc("u_c");
    mU_TC = getUniformLoc("u_tC");
    mU_TA = getUniformLoc("u_tA");
    mU_MTL = getUniformLoc("u_mtl");
    mU_TA_MULT = getUniformLoc("u_tA_mult");
    mU_BLEND_F = getUniformLoc("u_blend_f");
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
#ifdef NME_GLES
	mVertSrc += "precision highp float;";
#endif
    mVertSrc += "                                                   \
        uniform mat4 u_m;                                           \
        uniform vec2 u_uv_s;                                        \
        uniform vec4 u_c;                                           \
                                                                    \
        attribute vec2 a_xy;                                        \
        attribute vec2 a_uv;                                        \
        attribute vec4 a_c;                                         \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_uv = a_uv * u_uv_s;                                   \
            v_c = u_c.bgra * a_c.bgra;                              \
            gl_Position = vec4(a_xy, 0.0, 1.0) * u_m;               \
        }                                                           \
    ";

#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
        uniform sampler2D u_tC;                                     \
        uniform sampler2D u_tA;                                     \
        uniform vec4 u_mtl;                                         \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 texel = vec4(1.0, 1.0, 1.0, 1.0);                  \
            if (u_mtl[0] != 0.0)                                    \
            {                                                       \
                texel = texture2D(u_tC, v_uv);                      \
                if (u_mtl[1] != 0.0)                                \
                    texel.a = texture2D(u_tA, v_uv).r;              \
                texel.rgb *= mix(1.0, 1.0 / texel.a, u_mtl[2]);     \
            }                                                       \
            texel *= v_c;                                           \
            texel.rgb *= texel.a;                                   \
            texel.a *= u_mtl[3];                                    \
            gl_FragColor = texel;                                   \
        }                                                           \
    ";

    std::vector<std::string> attribs;
    attribs.push_back("a_xy");
    attribs.push_back("a_uv");
    attribs.push_back("a_c");
    createProgram(attribs);
}

//ColorShader
ColorShader::ColorShader()
{
#ifdef NME_GLES
	mVertSrc += "precision highp float;";
#endif
    mVertSrc += "                                                   \
        uniform mat4 u_m;                                           \
        uniform vec4 u_c;                                           \
                                                                    \
        attribute vec2 a_xy;                                        \
        attribute vec4 a_c;                                         \
                                                                    \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_c = u_c.bgra * a_c.bgra;                              \
            gl_Position = vec4(a_xy, 0.0, 1.0) * u_m;               \
        }                                                           \
    ";

#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
        uniform float u_blend_f;                                    \
                                                                    \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 color = v_c;                                       \
            color.rgb *= color.a;                                   \
            color.a *= u_blend_f;                                   \
            gl_FragColor = color;                                   \
        }                                                           \
    ";

    std::vector<std::string> attribs;
    attribs.push_back("a_xy");
    attribs.push_back("a_c");
    createProgram(attribs);
}

//TexShader
TexShader::TexShader()
{
#ifdef NME_GLES
	mVertSrc += "precision highp float;";
#endif
    mVertSrc += "                                                   \
        uniform mat4 u_m;                                           \
        uniform vec2 u_uv_s;                                        \
        uniform vec4 u_c;                                           \
                                                                    \
        attribute vec2 a_xy;                                        \
        attribute vec2 a_uv;                                        \
        attribute vec4 a_c;                                         \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_uv = a_uv * u_uv_s;                                   \
            v_c = u_c.bgra * a_c.bgra;                              \
            gl_Position = vec4(a_xy, 0.0, 1.0) * u_m;               \
        }                                                           \
    ";

#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
        uniform sampler2D u_tC;                                     \
        uniform float u_tA_mult;                                    \
        uniform float u_blend_f;                                    \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 texel = texture2D(u_tC, v_uv);                     \
            texel.rgb *= mix(1.0, 1.0 / texel.a, u_tA_mult);        \
            texel *= v_c;                                           \
            texel.rgb *= texel.a;                                   \
            texel.a *= u_blend_f;                                   \
            gl_FragColor = texel;                                   \
        }                                                           \
    ";

    std::vector<std::string> attribs;
    attribs.push_back("a_xy");
    attribs.push_back("a_uv");
    attribs.push_back("a_c");
    createProgram(attribs);
}

//TexAShader
TexAShader::TexAShader()
{
#ifdef NME_GLES
	mVertSrc += "precision highp float;";
#endif
    mVertSrc += "                                                   \
        uniform mat4 u_m;                                           \
        uniform vec2 u_uv_s;                                        \
        uniform vec4 u_c;                                           \
                                                                    \
        attribute vec2 a_xy;                                        \
        attribute vec2 a_uv;                                        \
        attribute vec4 a_c;                                         \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            v_uv = a_uv * u_uv_s;                                   \
            v_c = u_c.bgra * a_c.bgra;                              \
            gl_Position = vec4(a_xy, 0.0, 1.0) * u_m;               \
        }                                                           \
    ";

#ifdef NME_GLES
	mFragSrc += "precision mediump float;";
#endif
    mFragSrc += "                                                   \
        uniform sampler2D u_tC;                                     \
        uniform sampler2D u_tA;                                     \
        uniform float u_tA_mult;                                    \
        uniform float u_blend_f;                                    \
                                                                    \
        varying vec2 v_uv;                                          \
        varying vec4 v_c;                                           \
                                                                    \
        void main()                                                 \
        {                                                           \
            vec4 texel = texture2D(u_tC, v_uv).rgb;                 \
            texel.a = texture2D(u_tA, v_uv).r;                      \
            texel.rgb *= mix(1.0, 1.0 / texel.a, u_tA_mult);        \
            texel *= v_c;                                           \
            texel.rgb *= texel.a;                                   \
            texel.a *= u_blend_f;                                   \
            gl_FragColor = texel;                                   \
        }                                                           \
    ";

    std::vector<std::string> attribs;
    attribs.push_back("a_xy");
    attribs.push_back("a_uv");
    attribs.push_back("a_c");
    createProgram(attribs);
}

}