#include "NevoRenderPipeline.h"

#include <iostream>
#include "OGL.h"

using namespace nme;

namespace nevo
{

static float gMatrixIdentity2x2[4] = {1.0f, 0.0f, 0.0f, 1.0f};
static unsigned short int gTileIndices[6] = {0, 1, 2, 2, 3, 0};

static void mult(float x, float y, float *m, float tx, float ty, float *_x, float *_y)
{
    *_x = x * m[0] + y * m[1] + tx;
    *_y = x * m[2] + y * m[3] + ty;
}

//Job
void Job::rect(float x, float y, float width, float height)
{
    mXY_n = 8;
    mXY = (float*)malloc(mXY_n * sizeof(float));
    mUV_n = 8;
    mUV = (float*)malloc(mUV_n * sizeof(float));
    mInd_n = 6;
    mInd = gTileIndices;

    mXY[0] = x;
    mXY[1] = y;
    mXY[2] = x + width;
    mXY[3] = y;
    mXY[4] = x + width;
    mXY[5] = y + height;
    mXY[6] = x;
    mXY[7] = y + height;

    if (mTexColor)
    {
        mUV[0] = x / (float)mTexW;
        mUV[1] = y / (float)mTexH;
        mUV[2] = (x + width) / (float)mTexW;
        mUV[3] = y / (float)mTexH;
        mUV[4] = (x + width) / (float)mTexW;
        mUV[5] = (y + height) / (float)mTexH;
        mUV[6] = x / (float)mTexW;
        mUV[7] = (y + height) / (float)mTexH;
    }
}

void Job::tile(float x, float y, int rectX, int rectY, int rectW, int rectH, float *inTrans)
{
    mXY_n = 8;
    mXY = (float*)malloc(mXY_n * sizeof(float));
    mUV_n = 8;
    mUV = (float*)malloc(mUV_n * sizeof(float));
    mInd_n = 6;
    mInd = gTileIndices;

    mult(0, 0, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mXY[0], &mXY[1]);
    mult(inTileRect.w, 0, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mXY[2], &mXY[3]);
    mult(inTileRect.w, inTileRect.h, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mXY[4], &mXY[5]);
    mult(0, inTileRect.h, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mXY[6], &mXY[7]);

    if (mTexColor)
    {
        mUV[0] = rectX / (float)mTexW;
        mUV[1] = rectY / (float)mTexH;
        mUV[2] = (rectX + rectW) / (float)mTexW;
        mUV[3] = rectY / (float)mTexH;
        mUV[4] = (rectX + rectW) / (float)mTexW;
        mUV[5] = (rectY + rectH) / (float)mTexH;
        mUV[6] = rectX / (float)mTexW;
        mUV[7] = (rectY + rectH) / (float)mTexH;
    }
}

void Job::triangles(int inXYs_n, double *inXYs,
    int inIndixes_n, int *inIndixes, int inUVT_n, double *inUVT,
    int inColours_n, int *inColours)
{
    mXY_n = inXYs_n;
    mXY = (float*)malloc(mXY_n * sizeof(float));
    mUV_n = inUVT_n;
    mUV = (float*)malloc(mUV_n * sizeof(float));
    mInd_n = inIndixes_n;
    mInd = (int*)malloc(mInd_n * sizeof(unsigned short int));

    for (int i = 0; i < mXY_n; ++i)
        mXY[i] = (float)inXYs[i];
    for (int i = 0; i < mUV_n; ++i)
        mUV[i] = (float)inUVT[i];
    for (int i = 0; i < mInd_n; ++i)
        mInd[i] = inIndixes[i];
}

bool Job::hitTest(float x, float y)
{
    static float a1, a2, a3, x1, x2, x3, y1, y2, y3;
    for (int i = 0; i < mInd_n; ++i)
    {
        if (((i + 1) % 3) == 0)
        {
            x1 = mXY[mInd[i - 2] * 2]; y1 = mXY[mInd[i - 2] * 2 + 1];
            x2 = mXY[mInd[i - 1] * 2]; y2 = mXY[mInd[i - 1] * 2 + 1];
            x3 = mXY[mInd[i] * 2]; y3 = mXY[mInd[i] * 2 + 1];
            a1 = (x1 - x) * (y2 - y1) - (x2 - x1) * (y1 - y);
            a2 = (x2 - x) * (y3 - y2) - (x3 - x2) * (y2 - y);
            a3 = (x3 - x) * (y1 - y3) - (x1 - x3) * (y3 - y);
            if (((a1 >= 0) && (a2 >= 0) && (a3 >= 0)) || ((a1 <= 0) && (a2 <= 0) && (a3 <= 0)))
                return true;
        }
    }

    return false;
}

void Job::free_mem()
{
    if (mXY && mXY_n) free(mXY);
    if (mUV && mUV_n) free(mUV);
    if (mInd && mInd_n && mInd != gTileIndices) free(mInd);
    mXY = 0; mUV = 0; mInd = 0;
    mXY_n = mUV_n = mInd_n = 0;
}

//GPUProgram
class GPUProgram
{
public:
    GPUProgram()
    {
        mProg = 0;
        mVertSrc = 0;
        mFragSrc = 0;
    }

    ~GPUProgram()
    {
        glDeleteProgram(mProg);
    }

    void bind()
    {
        glUseProgram(mProg);
    }

    void unbind()
    {
         glUseProgram(0);
    }

    void setUniform1f(GLuint loc, GLfloat v)
    {
        glUniform1f(loc, v);
    }

    void setUniform2f(GLuint loc, GLfloat v0, GLfloat v1)
    {
        glUniform2f(loc, v0, v1);
    }

    void setUniform3f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2)
    {
        glUniform3f(loc, v0, v1, v2);
    }

    void setUniform4f(GLuint loc, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
    {
        glUniform4f(loc, v0, v1, v2, v3);
    }

    void setUniform1i(GLuint loc, GLfloat v)
    {
        glUniform1i(loc, v);
    }

    void setUniform1iv(GLuint loc, GLsizei count, const GLint* v)
    {
        glUniform1iv(loc, count, v);
    }

    void setMatrix4x4fv(GLuint loc, const GLfloat *v)
    {
        glUniformMatrix4fv(loc, 1, 0, v);
    }

protected:
    char log[512];
    const char *mVertSrc;
    const char *mFragSrc;
    GLuint mProg;

    void createProgram()
    {
        GLint success;
        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &mVertSrc, NULL);
        glCompileShader(vert);
        glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vert, 512, NULL, log);
            std::cout << "Vertex Compilation Error: " << log << std::endl;
            return;
        }

        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &mFragSrc, NULL);
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

    GLuint getUniformLoc(const char *uniformName)
    {
        return glGetUniformLocation(mProg, uniformName);
    }

    GLuint getAttribLoc(const char *attribName)
    {
        return glGetAttribLocation(mProg, attribName);
    }
};

//DefaultShader
class NevoRenderPipeline::DefaultShader : public GPUProgram
{
public:
    DefaultShader()
    {
        //precision mediump float;
        mVertSrc = "                                                    \
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

        mFragSrc = "                                                    \
            uniform sampler2D u_t[8];                                   \
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

    GLuint mA_XY;
    GLuint mA_UV;
    GLuint mU_M;
    GLuint mU_T;
    GLuint mU_TS;
    GLuint mU_C;
private:

};

//NevoRenderPipeline
NevoRenderPipeline::NevoRenderPipeline()
{
    mDefaultShader = 0;
    mJobs = 0;
    mChTex.resize(8);
    for (int i = 0; i < mChTex.size(); ++i) mChTex[i] = -1;
    mTexCh.resize(1024);
    for (int i = 0; i < mTexCh.size(); ++i) mTexCh[i] = -1;
    mChUVal.resize(8);
    for (int i = 0; i < mChUVal.size(); ++i) mChUVal[i] = i;
}

NevoRenderPipeline::~NevoRenderPipeline()
{
    
}

void NevoRenderPipeline::Init()
{
    mDefaultShader = new DefaultShader();
}

void NevoRenderPipeline::Clear()
{
    if (mDefaultShader)
    {
        delete mDefaultShader;
    }
}

void NevoRenderPipeline::setJobs(Vec<Job> *jobs)
{
    mJobs = jobs;
}

void NevoRenderPipeline::setNodeParams(float *inTrans4x4, float r, float g, float b, float a)
{
    mDefaultShader->setMatrix4x4fv(mDefaultShader->mU_M, inTrans4x4);
    mDefaultShader->setUniform4f(mDefaultShader->mU_C, r, g, b, a);

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job &job = (*mJobs)[i];
        if (job.mTexColor)
        {
            glBindTexture(GL_TEXTURE_2D, job.mTexColor);
            if (job.isTypeTriangles())
                mDefaultShader->setUniform2f(mDefaultShader->mU_TS, job.mTexPixW / (float)job.mTexW, job.mTexPixH / (float)job.mTexH);
            else
                mDefaultShader->setUniform2f(mDefaultShader->mU_TS, 1.0f, 1.0f);
            glVertexAttribPointer(mDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 0, job.mXY);
            glVertexAttribPointer(mDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 0, job.mUV);
            glDrawElements(GL_TRIANGLES, job.mInd_n, GL_UNSIGNED_SHORT, job.mInd);
        }
    }
}

void NevoRenderPipeline::begin()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    mDefaultShader->bind();
    mDefaultShader->setUniform1iv(mDefaultShader->mU_T, mChUVal.size(), &mChUVal[0]);
    glEnableVertexAttribArray(mDefaultShader->mA_XY);
    glEnableVertexAttribArray(mDefaultShader->mA_UV);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
}

void NevoRenderPipeline::end()
{
    glDisableVertexAttribArray(mDefaultShader->mA_XY);
    glDisableVertexAttribArray(mDefaultShader->mA_UV);
    mDefaultShader->unbind();
}

NevoRenderPipeline gNevoRender;

}