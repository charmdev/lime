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

    void setUniform1f(GLuint loc, GLfloat v);
    void setUniform2f(GLuint loc, GLfloat v0, GLfloat v1);
    void setUniform3f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2);
    void setUniform4f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void setUniform1i(GLuint loc, GLfloat v);
    void setUniform4i(GLuint loc, GLint v0, GLint v1, GLint v2, GLint v3);
    void setUniform1iv(GLuint loc, GLsizei count, const GLint* v);
    void setMatrix4x4fv(GLuint loc, const GLfloat *v);
    void setAttribPointer(GLuint bufId, GLuint attrib, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *ptr);
    void setAttrib4f(GLuint attrib, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void setAttrib1f(GLuint attrib, GLfloat v0);
    void draw(GLuint bufId, GLenum mode, GLsizei count, GLenum type, const void *indices);

protected:
    char log[512];
    std::string mVertSrc;
    std::string mFragSrc;
    GLuint mProg;

    void createProgram(std::vector<std::string> attribs);
    GLuint getUniformLoc(const char *uniformName);
    GLuint getAttribLoc(const char *attribName);
};

class DefaultShader : public GPUProgram
{
public:
    DefaultShader();

    // GLuint mA_XY;
    // GLuint mA_UV;
    // GLuint mA_C;
    // GLuint mA_MTL;
    // GLuint mU_M;
    // GLuint mU_C;
    // GLuint mU_UV_S;
    // GLuint mU_T;

    GLuint mA_XY;
    GLuint mA_UV;
    GLuint mA_C;
    GLuint mA_MTL;
    GLuint mU_T;

private:

};

}

#endif