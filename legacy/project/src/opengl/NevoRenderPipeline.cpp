#include "NevoRenderPipeline.h"

#include <Surface.h>
#include <iostream>
#include "OGL.h"

using namespace nme;

namespace nevo
{

static float gMatrixIdentity2x2[4] = {1.0f, 0.0f, 0.0f, 1.0f};
static int gTileIndices[6] = {0, 1, 2, 2, 3, 0};

static void mult(float x, float y, float *m, float tx, float ty, float &_x, float &_y)
{
    _x = x * m[0] + y * m[1] + tx;
    _y = x * m[2] + y * m[3] + ty;
}

void Job::tex(nme::Surface *surface)
{
    mSurface = surface;
    if (mSurface) mSurface->IncRef();
    mTexColor = mSurface ? mSurface->getTextureId() : 0;
    mTexAlpha = mSurface ? mSurface->getAlphaTextureId() : 0;
    mTexW = (float)(mSurface ? mSurface->getTextureWidth() : 0);
    mTexH = (float)(mSurface ? mSurface->getTextureHeight() : 0);
    mTexPixW = (float)(mSurface ? mSurface->Width() : 0);
    mTexPixH = (float)(mSurface ? mSurface->Height() : 0);
}

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

    if (mSurface)
    {
        mUV[0] = 0.0f;
        mUV[1] = 0.0f;
        mUV[2] = 1.0f;
        mUV[3] = 0.0f;
        mUV[4] = 1.0f;
        mUV[5] = 1.0f;
        mUV[6] = 0.0f;
        mUV[7] = 1.0f;
    }

    mType = Job::Type::RECT;
}

void Job::tile(float x, float y, const nme::Rect &inTileRect, float *inTrans, float *inRGBA, int blendMode)
{
    mXY_n = 8;
    mXY = (float*)malloc(mXY_n * sizeof(float));
    mUV_n = 8;
    mUV = (float*)malloc(mUV_n * sizeof(float));
    mInd_n = 6;
    mInd = gTileIndices;

    mult(0, 0, inTrans ? inTrans : gMatrixIdentity2x2, x, y, mXY[0], mXY[1]);
    mult(inTileRect.w, 0, inTrans ? inTrans : gMatrixIdentity2x2, x, y, mXY[2], mXY[3]);
    mult(inTileRect.w, inTileRect.h, inTrans ? inTrans : gMatrixIdentity2x2, x, y, mXY[4], mXY[5]);
    mult(0, inTileRect.h, inTrans ? inTrans : gMatrixIdentity2x2, x, y, mXY[6], mXY[7]);

    if (mSurface)
    {
        mUV[0] = inTileRect.x / mTexW;
        mUV[1] = inTileRect.y / mTexH;
        mUV[2] = (inTileRect.x + inTileRect.w) / mTexW;
        mUV[3] = inTileRect.y / mTexH;
        mUV[4] = (inTileRect.x + inTileRect.w) / mTexW;
        mUV[5] = (inTileRect.y + inTileRect.h) / mTexH;
        mUV[6] = inTileRect.x / mTexW;
        mUV[7] = (inTileRect.y + inTileRect.h) / mTexH;
    }

    mType = Job::Type::TILE;
}

void Job::triangles(int inXYs_n, double *inXYs,
    int inIndixes_n, int *inIndixes, int inUVT_n, double *inUVT,
    int inColours_n, int *inColours, int inCull, int blendMode, unsigned int color, float alpha)
{
    mXY_n = inXYs_n;
    mXY = (float*)malloc(mXY_n * sizeof(float));
    mUV_n = inUVT_n;
    mUV = (float*)malloc(mUV_n * sizeof(float));
    mInd_n = inIndixes_n;
    mInd = (int*)malloc(mInd_n * sizeof(int));;

    //memcpy(mXY, inXYs, mXY_n * sizeof(float));
    //memcpy(mUV, inUVT, mUV_n * sizeof(float));
    for (int i = 0; i < mXY_n; ++i)
        mXY[i] = (float)inXYs[i];
    for (int i = 0; i < mUV_n; ++i)
        mUV[i] = (float)inUVT[i];
    memcpy(mInd, inIndixes, mInd_n * sizeof(int));

    mType = Job::Type::TRIANGLES;
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

class NevoRenderPipeline::DefaultShader : public GPUProgram
{
public:
    DefaultShader()
    {
        //precision mediump float;
        mVertSrc = "                                            \
            uniform mat4 u_m;                                   \
            uniform vec2 u_ts;                                  \
                                                                \
            attribute vec2 a_xy;                                \
            attribute vec2 a_uv;                                \
                                                                \
            varying vec2 v_uv;                                  \
                                                                \
            void main()                                         \
            {                                                   \
                v_uv = a_uv * u_ts;                             \
                gl_Position = vec4(a_xy, 0.0, 1.0) * u_m;       \
            }                                                   \
        ";

        mFragSrc = "                                            \
            uniform sampler2D u_t[8];                           \
                                                                \
            varying vec2 v_uv;                                  \
                                                                \
            void main()                                         \
            {                                                   \
                vec4 texel = texture2D(u_t[0], v_uv);           \
                gl_FragColor = texel;                           \
            }                                                   \
        ";

        createProgram();

        mA_XY = getAttribLoc("a_xy");
        mA_UV = getAttribLoc("a_uv");
        mU_M = getUniformLoc("u_m");
        mU_T = getUniformLoc("u_t");
        mU_TS = getUniformLoc("u_ts");
    }

    GLuint mA_XY;
    GLuint mA_UV;
    GLuint mU_M;
    GLuint mU_T;
    GLuint mU_TS;
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

void NevoRenderPipeline::setNodeParams(float *inTrans, float alpha)
{
    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job &job = (*mJobs)[i];
        if (job.mSurface)
        {
            glBindTexture(GL_TEXTURE_2D, job.mTexColor);
            mDefaultShader->setMatrix4x4fv(mDefaultShader->mU_M, inTrans);
            if (job.mType == Job::Type::TRIANGLES)
                mDefaultShader->setUniform2f(mDefaultShader->mU_TS, job.mTexPixW / job.mTexW, job.mTexPixH / job.mTexH);
            else
                mDefaultShader->setUniform2f(mDefaultShader->mU_TS, 1.0f, 1.0f);
            glVertexAttribPointer(mDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 0, job.mXY);
            glVertexAttribPointer(mDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 0, job.mUV);
            glDrawElements(GL_TRIANGLES, job.mInd_n, GL_UNSIGNED_INT, job.mInd);
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