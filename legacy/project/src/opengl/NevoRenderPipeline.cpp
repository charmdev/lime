#include "NevoRenderPipeline.h"

#include "OGL.h"
#include <Surface.h>

#include "NevoTextureBinder.h"
#include "NevoShaders.h"

using namespace nme;

namespace nevo
{

static TextureBinder *gTextureBinder = 0;
static DefaultShader *gDefaultShader = 0;
static float gMatrixIdentity2x2[4] = {1.0f, 0.0f, 0.0f, 1.0f};

static void mult(float x, float y, float *m, float tx, float ty, float *_x, float *_y)
{
    *_x = x * m[0] + y * m[1] + tx;
    *_y = x * m[2] + y * m[3] + ty;
}

//Job
Job::Job()
{
    mSurface = 0;
    clear();
}

Job::~Job()
{
    clear();
}

void Job::mtl(nme::Surface *surface, unsigned int color, int blendMode)
{
    mSurface = surface;
    if (mSurface)
    {
        mSurface->IncRef();
        mTexColor = mSurface->getTextureId();
        mTexAlpha = mSurface->getAlphaTextureId();
        mTexW = mSurface->getTextureWidth();
        mTexH = mSurface->getTextureHeight();
        mTexPixW = mSurface->Width();
        mTexPixH = mSurface->Height();
        if (mSurface->GetFlags() & surfUsePremultipliedAlpha)
            setPremultAlpha();
    }
    mBGRA = color;

    if (blendMode == nme::bmNormal)
        setBlendModeNormal();
    else if (blendMode == nme::bmAdd)
        setBlendModeAdd();
    else
        setBlendModeNone();
}

void Job::rect(float x, float y, float width, float height)
{
    mQ_XY[0].x = x;
    mQ_XY[0].y = y;
    mQ_XY[1].x = x + width;
    mQ_XY[1].y = y;
    mQ_XY[2].x = x + width;
    mQ_XY[2].y = y + height;
    mQ_XY[3].x = x;
    mQ_XY[3].y = y + height;

    if (mSurface)
    {
        mQ_UV[0].u = x / (float)mTexW;
        mQ_UV[0].v = y / (float)mTexH;
        mQ_UV[1].u = (x + width) / (float)mTexW;
        mQ_UV[1].v = y / (float)mTexH;
        mQ_UV[2].u = (x + width) / (float)mTexW;
        mQ_UV[2].v = (y + height) / (float)mTexH;
        mQ_UV[3].u = x / (float)mTexW;
        mQ_UV[3].v = (y + height) / (float)mTexH;
    }

    setTypeRect();
}

void Job::tile(float x, float y, int rectX, int rectY, int rectW, int rectH, float *inTrans)
{
    mult(0, 0, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mQ_XY[0].x, &mQ_XY[0].y);
    mult(rectW, 0, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mQ_XY[1].x, &mQ_XY[1].y);
    mult(rectW, rectH, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mQ_XY[2].x, &mQ_XY[2].y);
    mult(0, rectH, inTrans ? inTrans : gMatrixIdentity2x2, x, y, &mQ_XY[3].x, &mQ_XY[3].y);

    if (mSurface)
    {
        mQ_UV[0].u = rectX / (float)mTexW;
        mQ_UV[0].v = rectY / (float)mTexH;
        mQ_UV[1].u = (rectX + rectW) / (float)mTexW;
        mQ_UV[1].v = rectY / (float)mTexH;
        mQ_UV[2].u = (rectX + rectW) / (float)mTexW;
        mQ_UV[2].v = (rectY + rectH) / (float)mTexH;
        mQ_UV[3].u = rectX / (float)mTexW;
        mQ_UV[3].v = (rectY + rectH) / (float)mTexH;
    }

    setTypeTile();
}

void Job::triangles(int inXYs_n, float *inXYs,
    int inIndixes_n, int *inIndixes, int inUVT_n, float *inUVT,
    int inColours_n, int *inColours)
{
    mT_XY = inXYs;
    mT_UV = inUVT;
    mT_C = inColours;
    mT_I = inIndixes;
    mT_Vn = inXYs_n / 2;
    mT_In = inIndixes_n;

    setTypeTriangles();
}

bool Job::hitTest(float x, float y)
{
    static float a1, a2, a3, x1, x2, x3, y1, y2, y3;

    x1 = mQ_XY[0].x; x2 = mQ_XY[1].x; x3 = mQ_XY[2].x;
    y1 = mQ_XY[0].y; y2 = mQ_XY[1].y; y3 = mQ_XY[2].y;
    a1 = (x1 - x) * (y2 - y1) - (x2 - x1) * (y1 - y);
    a2 = (x2 - x) * (y3 - y2) - (x3 - x2) * (y2 - y);
    a3 = (x3 - x) * (y1 - y3) - (x1 - x3) * (y3 - y);
    if (((a1 >= 0) && (a2 >= 0) && (a3 >= 0)) || ((a1 <= 0) && (a2 <= 0) && (a3 <= 0)))
        return true;

    x1 = mQ_XY[2].x; x2 = mQ_XY[3].x; x3 = mQ_XY[0].x;
    y1 = mQ_XY[2].y; y2 = mQ_XY[3].y; y3 = mQ_XY[0].y;
    a1 = (x1 - x) * (y2 - y1) - (x2 - x1) * (y1 - y);
    a2 = (x2 - x) * (y3 - y2) - (x3 - x2) * (y2 - y);
    a3 = (x3 - x) * (y1 - y3) - (x1 - x3) * (y3 - y);
    return (((a1 >= 0) && (a2 >= 0) && (a3 >= 0)) || ((a1 <= 0) && (a2 <= 0) && (a3 <= 0)));
}

void Job::clear()
{
    if (mSurface) mSurface->DecRef();
    mSurface = 0;
    mTexColor = mTexAlpha = 0;
    mTexW = mTexH = 0;
    mTexPixW = mTexPixH = 0;
    mBGRA = 0x0;
    mFlags = 0x0;
    mT_XY = 0;
    mT_UV = 0;
    mT_C = 0;
    mT_I = 0;
    mT_Vn = 0;
    mT_In = 0;
}

//JobsPool
JobsPool::JobsPool()
{

}

JobsPool::~JobsPool()
{
    for (int i = 0; i < mAllocJobs.size(); ++i)
        delete mAllocJobs[i];
}

Job* JobsPool::get()
{
    if (mFreeJobs.size() > 0)
    {
        Job *job = mFreeJobs.last();
        mFreeJobs.dec();
        return job;
    }
    return mAllocJobs.inc() = new Job();
}

void JobsPool::refund(Job *job)
{
    job->clear();
    mFreeJobs.inc() = job;
}

//NevoRenderPipeline
NevoRenderPipeline::NevoRenderPipeline()
{
    mJobs = 0;

    mI.reserve(cMaxIndex);
    mXY.reserve(cMaxIndex * 2);
    mUV.reserve(cMaxIndex * 2);
    mC.reserve(cMaxIndex);

    mXYvbo = 0;
    mUVvbo = 0;
    mCvbo = 0;
    mIebo = 0;
}

NevoRenderPipeline::~NevoRenderPipeline()
{
    
}

void NevoRenderPipeline::Init()
{
    gTextureBinder = new TextureBinder();
    gDefaultShader = new DefaultShader(gTextureBinder->numChannels());

    glGenBuffers(1, &mXYvbo);
    glBindBuffer(GL_ARRAY_BUFFER, mXYvbo);
    glBufferData(GL_ARRAY_BUFFER, mXY.allocMemSize(), 0, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &mUVvbo);
    glBindBuffer(GL_ARRAY_BUFFER, mUVvbo);
    glBufferData(GL_ARRAY_BUFFER, mUV.allocMemSize(), 0, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &mCvbo);
    glBindBuffer(GL_ARRAY_BUFFER, mCvbo);
    glBufferData(GL_ARRAY_BUFFER, mC.allocMemSize(), 0, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &mIebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mI.allocMemSize(), 0, GL_DYNAMIC_DRAW);
}

void NevoRenderPipeline::Clear()
{
    if (gDefaultShader)
    {
        delete gDefaultShader;
        gDefaultShader = 0;
    }

    if (gTextureBinder)
    {
        delete gTextureBinder;
        gTextureBinder = 0;
    }

    if (mXYvbo)
    {
        glDeleteBuffers(1, &mXYvbo);
        mXYvbo = 0;
    }

    if (mUVvbo)
    {
        glDeleteBuffers(1, &mUVvbo);
        mUVvbo = 0;
    }

    if (mCvbo)
    {
        glDeleteBuffers(1, &mCvbo);
        mCvbo = 0;
    }

    if (mIebo)
    {
        glDeleteBuffers(1, &mIebo);
        mIebo = 0;
    }
}

void NevoRenderPipeline::setJobs(Vec<Job*> *jobs)
{
    mJobs = jobs;
}

void NevoRenderPipeline::setNodeParams(float *inTrans4x4, float r, float g, float b, float a)
{
    gDefaultShader->setMatrix4x4fv(gDefaultShader->mU_M, inTrans4x4);
    gDefaultShader->setUniform4f(gDefaultShader->mU_C, r, g, b, a);

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job &job = *(*mJobs)[i];
        
        if (job.mSurface)
        {
            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, job.mTexColor);
        }

        if (job.isTypeTriangles())
        {
            gDefaultShader->setUniform2f(gDefaultShader->mU_TS, job.mTexPixW / (float)job.mTexW, job.mTexPixH / (float)job.mTexH);
        }
        else 
        {
            gDefaultShader->setUniform2f(gDefaultShader->mU_TS, 1.0f, 1.0f);

            // mXY.resize(8);
            // mXY[0] = job.mQ_XY[0].x; mXY[1] = job.mQ_XY[0].y;
            // mXY[2] = job.mQ_XY[1].x; mXY[3] = job.mQ_XY[1].y;
            // mXY[4] = job.mQ_XY[2].x; mXY[5] = job.mQ_XY[2].y;
            // mXY[6] = job.mQ_XY[3].x; mXY[7] = job.mQ_XY[3].y;

            // mUV.resize(8);
            // mUV[0] = job.mQ_UV[0].u; mUV[1] = job.mQ_UV[0].v;
            // mUV[2] = job.mQ_UV[1].u; mUV[3] = job.mQ_UV[1].v;
            // mUV[4] = job.mQ_UV[2].u; mUV[5] = job.mQ_UV[2].v;
            // mUV[6] = job.mQ_UV[3].u; mUV[7] = job.mQ_UV[3].v;

            mI.resize(6);
            mI[0] = 0; mI[1] = 1; mI[2] = 2;
            mI[3] = 2; mI[4] = 3; mI[5] = 0;

            glBindBuffer(GL_ARRAY_BUFFER, mXYvbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 32, job.mQ_XY);
            glEnableVertexAttribArray(gDefaultShader->mA_XY);
            glVertexAttribPointer(gDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, 0);

            glBindBuffer(GL_ARRAY_BUFFER, mUVvbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 32, job.mQ_UV);
            glEnableVertexAttribArray(gDefaultShader->mA_UV);
            glVertexAttribPointer(gDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, 0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mI.sizeMemSize(), &mI[0]);
            glDrawElements(GL_TRIANGLES, mI.size(), GL_UNSIGNED_SHORT, 0);
        }
    }
}

void NevoRenderPipeline::begin()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /*glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);*/

    gDefaultShader->bind();
    gDefaultShader->setUniform1iv(gDefaultShader->mU_T, gDefaultShader->mU_T_Val.size(), &gDefaultShader->mU_T_Val[0]);
}

void NevoRenderPipeline::end()
{
    glDisableVertexAttribArray(gDefaultShader->mA_XY);
    glDisableVertexAttribArray(gDefaultShader->mA_UV);
    gDefaultShader->unbind();
}

JobsPool gNevoJobsPool;
NevoRenderPipeline gNevoRender;

}