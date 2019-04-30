#ifndef NEVO_SHADERS_H
#define NEVO_SHADERS_H

#include "NevoVec.h"
#include "OGL.h"

#include <string>
#include <vector>

namespace nevo
{

class GPUProgram
{
public:
    GPUProgram();
    ~GPUProgram();

    void bind();
    void unbind();

    void setUniform1f(GLint loc, GLfloat v);
    void setUniform2f(GLint loc, GLfloat v0, GLfloat v1);
    void setUniform3f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2);
    void setUniform4f(GLint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void setUniform1i(GLint loc, GLfloat v);
    void setUniform4i(GLint loc, GLint v0, GLint v1, GLint v2, GLint v3);
    void setUniform1iv(GLint loc, GLsizei count, const GLint* v);
    void setMatrix4x4fv(GLint loc, const GLfloat *v);
    void setAttribPointer(GLint bufId, GLuint attrib, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *ptr);
    void setAttrib4f(GLint attrib, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void setAttrib1f(GLint attrib, GLfloat v0);
    void draw(GLint bufId, GLenum mode, GLsizei count, GLenum type, const void *indices);
    void draw(GLenum mode, GLint first, GLsizei count);
    void disableAttribs();

    GLint mA_XY;
    GLint mA_UV;
    GLint mA_C;
    GLint mU_M;
    GLint mU_C;
    GLint mU_TC;
    GLint mU_TA;
    GLint mU_TA_MULT;
    GLint mU_BLEND_F;

protected:
    char log[512];
    std::string mVertSrc;
    std::string mFragSrc;
    GLuint mProg;

    void createProgram(std::vector<std::string> attribs);
    GLint getUniformLoc(const char *uniformName);
    GLint getAttribLoc(const char *attribName);
};

class ColorShader : public GPUProgram
{
public:
    ColorShader();
};

class TexShader : public GPUProgram
{
public:
    TexShader();
};

class TexAShader : public GPUProgram
{
public:
    TexAShader();
};

}

#endif