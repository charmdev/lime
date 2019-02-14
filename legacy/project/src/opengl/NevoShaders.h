#ifndef NEVO_SHADERS_H
#define NEVO_SHADERS_H

#include "NevoVec.h"
#include "OGL.h"

#include <string>

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
    void setUniform1iv(GLuint loc, GLsizei count, const GLint* v);
    void setMatrix4x4fv(GLuint loc, const GLfloat *v);

protected:
    char log[512];
    std::string mVertSrc;
    std::string mFragSrc;
    GLuint mProg;

    void createProgram();
    GLuint getUniformLoc(const char *uniformName);
    GLuint getAttribLoc(const char *attribName);
};

class DefaultShader : public GPUProgram
{
public:
    DefaultShader(int numTexChannels);

    GLuint mA_XY;
    GLuint mA_UV;
    GLuint mU_M;
    GLuint mU_T;
    GLuint mU_TS;
    GLuint mU_C;

    Vec<int> mU_T_Val;

private:

};

}

#endif