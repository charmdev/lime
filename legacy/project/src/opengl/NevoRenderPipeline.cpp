#include "NevoRenderPipeline.h"

#include "OGL.h"
#include <Surface.h>
#include <iostream>

#include "NevoShaders.h"
#include "NevoGLContainers.h"

using namespace nme;

namespace nevo
{

static DefaultShader *gDefaultShader = 0;
static VBOPool *gVBOPool = 0;
static EBOPool *gEBOPool = 0;
JobsPool *gNevoJobsPool = 0;
NevoRenderPipeline gNevoRender;
static float gMatrixIdentity2x2[4] = { 1.0f, 0.0f,
                                       0.0f, 1.0f };
static float gMatrixIdentity4x4[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f, 0.0f,
                                        0.0f, 0.0f, 0.0f, 1.0f };

static void mult(float x, float y, float *m, float tx, float ty, float *_x, float *_y)
{
    *_x = x * m[0] + y * m[2] + tx;
    *_y = x * m[1] + y * m[3] + ty;
}

static void mult(float x, float y, float m00, float m10, float m01, float m11, float tx, float ty, float *_x, float *_y)
{
    *_x = x * m00 + y * m01 + tx;
    *_y = x * m10 + y * m11 + ty;
}

static bool pointInTriangle(float x, float y, float x1, float y1, float x2, float y2, float x3, float y3)
{
    static float a1, a2, a3;
    a1 = (x1 - x) * (y2 - y1) - (x2 - x1) * (y1 - y);
    a2 = (x2 - x) * (y3 - y2) - (x3 - x2) * (y2 - y);
    a3 = (x3 - x) * (y1 - y3) - (x1 - x3) * (y3 - y);
    return (((a1 >= 0) && (a2 >= 0) && (a3 >= 0)) || ((a1 <= 0) && (a2 <= 0) && (a3 <= 0)));
}

//Job
Job::Job()
{
    mSurface = 0;
    mBGRA = 0;
    mT_XY = 0; mT_UV = 0; mT_C = 0; mT_I = 0; mT_In = 0;
    mFlags = 0;
}

Job::~Job()
{
    clear();
}

void Job::mtl(nme::Surface *surface, unsigned int color, int blendMode)
{
    mSurface = surface;
    if (mSurface) mSurface->IncRef();
    mBGRA = color;

    if (blendMode == nme::bmNormal)
        setBlendModeNormal();
    else if (blendMode == nme::bmAdd)
        setBlendModeAdd();
    else
        setBlendModeNone();
}

bool Job::cmpmtl(Job *job)
{
    return (mSurface == job->mSurface);
}

void Job::rect(float x, float y, float width, float height)
{
    mQ_XY[0].x = x;
    mQ_XY[0].y = y;
    initBB(mQ_XY[0].x, mQ_XY[0].y);
    mQ_XY[1].x = x + width;
    mQ_XY[1].y = y;
    calcBB(mQ_XY[1].x, mQ_XY[1].y);
    mQ_XY[2].x = x + width;
    mQ_XY[2].y = y + height;
    calcBB(mQ_XY[2].x, mQ_XY[2].y);
    mQ_XY[3].x = x;
    mQ_XY[3].y = y + height;
    calcBB(mQ_XY[3].x, mQ_XY[3].y);

    if (mSurface)
    {
        mQ_UV[0].u = x / (float)mSurface->getTextureWidth();
        mQ_UV[0].v = y / (float)mSurface->getTextureHeight();
        mQ_UV[1].u = (x + width) / (float)mSurface->getTextureWidth();
        mQ_UV[1].v = y / (float)mSurface->getTextureHeight();
        mQ_UV[2].u = (x + width) / (float)mSurface->getTextureWidth();
        mQ_UV[2].v = (y + height) / (float)mSurface->getTextureHeight();
        mQ_UV[3].u = x / (float)mSurface->getTextureWidth();
        mQ_UV[3].v = (y + height) / (float)mSurface->getTextureHeight();
    }

    setTypeRect();
}

void Job::tile(float x, float y, int rectX, int rectY, int rectW, int rectH, float *inTrans)
{
    float *m2x2 = inTrans ? inTrans : gMatrixIdentity2x2;
    mult(0.0f, 0.0f, m2x2, x, y, &mQ_XY[0].x, &mQ_XY[0].y);
    initBB(mQ_XY[0].x, mQ_XY[0].y);
    mult(rectW, 0.0f, m2x2, x, y, &mQ_XY[1].x, &mQ_XY[1].y);
    calcBB(mQ_XY[1].x, mQ_XY[1].y);
    mult(rectW, rectH, m2x2, x, y, &mQ_XY[2].x, &mQ_XY[2].y);
    calcBB(mQ_XY[2].x, mQ_XY[2].y);
    mult(0.0f, rectH, m2x2, x, y, &mQ_XY[3].x, &mQ_XY[3].y);
    calcBB(mQ_XY[3].x, mQ_XY[3].y);

    if (mSurface)
    {
        mQ_UV[0].u = rectX / (float)mSurface->getTextureWidth();
        mQ_UV[0].v = rectY / (float)mSurface->getTextureHeight();
        mQ_UV[1].u = (rectX + rectW) / (float)mSurface->getTextureWidth();
        mQ_UV[1].v = rectY / (float)mSurface->getTextureHeight();
        mQ_UV[2].u = (rectX + rectW) / (float)mSurface->getTextureWidth();
        mQ_UV[2].v = (rectY + rectH) / (float)mSurface->getTextureHeight();
        mQ_UV[3].u = rectX / (float)mSurface->getTextureWidth();
        mQ_UV[3].v = (rectY + rectH) / (float)mSurface->getTextureHeight();
    }

    setTypeTile();
}

void Job::triangles(int inXYs_n, float *inXYs,
    int inIndixes_n, short *inIndixes, int inUVT_n, float *inUVT,
    int inColours_n, int *inColours)
{
    mT_XY = 0; mT_UV = 0; mT_C = 0; mT_I = 0; mT_In = 0;
    initBB(0.0f, 0.0f);

    if (inXYs)
    {
        mT_XY = gVBOPool->get(inXYs_n * sizeof(float));
        mT_XY->update(0, inXYs_n * sizeof(float), inXYs);

        initBB(inXYs[0], inXYs[1]);
        for (int i = 0; i < inXYs_n; ++i)
        {
            if ((i + 1) % 2 == 0)
            {
                calcBB(inXYs[i - 1], inXYs[i]);
            }
        }
    }

    if (inUVT)
    {
        mT_UV = gVBOPool->get(inUVT_n * sizeof(float));
        mT_UV->update(0, inUVT_n * sizeof(float), inUVT);
    }

    if (inColours)
    {
        mT_C = gVBOPool->get(inColours_n * sizeof(int));
        mT_C->update(0, inColours_n * sizeof(int), inColours);
    }

    if (inIndixes)
    {
        mT_I = gEBOPool->get(inIndixes_n * sizeof(short));
        mT_I->update(0, inIndixes_n * sizeof(short), inIndixes);
        mT_In = inIndixes_n;
    }

    setTypeTriangles();
}

bool Job::hitTest(float x, float y)
{
    return (mBBminX <= x) && (mBBminY <= y) && (mBBmaxX >= x) && (mBBmaxY >= y);
}

void Job::clear()
{
    if (mSurface) mSurface->DecRef();
    mSurface = 0;
    mBGRA = 0;
    if (isTypeTriangles())
    {
        if (mT_XY) gVBOPool->refund(mT_XY);
        if (mT_UV) gVBOPool->refund(mT_UV);
        if (mT_C) gVBOPool->refund(mT_C);
        if (mT_I) gEBOPool->refund(mT_I);
    }
    mT_XY = 0; mT_UV = 0; mT_C = 0; mT_I = 0; mT_In = 0;
    mFlags = 0;
}

//JobsPool
JobsPool::JobsPool()
{

}

JobsPool::~JobsPool()
{
    for (int i = 0; i < mAlloc.size(); ++i)
        delete mAlloc[i];
}

Job* JobsPool::get()
{
    if (mFree.size() > 0)
    {
        Job *job = mFree.last();
        mFree.dec();
        return job;
    }
    return mAlloc.inc() = new Job();
}

void JobsPool::refund(Job *job)
{
    job->clear();
    mFree.inc() = job;
}

//NevoRenderPipeline
NevoRenderPipeline::NevoRenderPipeline()
{
    mJobs = 0;
    mI_n = 0;
    mPrevJob = 0;
}

NevoRenderPipeline::~NevoRenderPipeline()
{
    
}

void NevoRenderPipeline::Init()
{
    Clear();

    gDefaultShader = new DefaultShader();
    gVBOPool = new VBOPool();
    gEBOPool = new EBOPool();
    gNevoJobsPool = new JobsPool();

    mXYvbo = gVBOPool->get(cMaxVerts * sizeof(float) * 2);
    mUVvbo = gVBOPool->get(cMaxVerts * sizeof(float) * 2);
    mCvbo = gVBOPool->get(cMaxVerts * sizeof(int));
    mQIebo = gEBOPool->get(cMaxVerts * sizeof(unsigned short), true);
    unsigned short qIndex[cMaxVerts];
    qIndex[0] = 0; qIndex[1] = 1; qIndex[2] = 2;
    qIndex[3] = 2; qIndex[4] = 3; qIndex[5] = 0;
    for (int i = 6; i < cMaxVerts; ++i)
        qIndex[i] = qIndex[i - 6] + 4;
    mQIebo->update(0, cMaxVerts * sizeof(unsigned short), qIndex);

    mXY.reserve(cMaxVerts * 2);
    mUV.reserve(cMaxVerts * 2);
    mC.reserve(cMaxVerts);
    mI_n = 0;
}

void NevoRenderPipeline::Clear()
{
    if (gDefaultShader)
    {
        delete gDefaultShader;
        gDefaultShader = 0;
    }

    if (gNevoJobsPool)
    {
        delete gNevoJobsPool;
        gNevoJobsPool = 0;
    }

    if (gVBOPool)
    {
        delete gVBOPool;
        gVBOPool = 0;
    }

    if (gEBOPool)
    {
        delete gEBOPool;
        gEBOPool = 0;
    }
}

void NevoRenderPipeline::setJobs(Vec<Job*> *jobs)
{
    mJobs = jobs;
}

void NevoRenderPipeline::setNodeParams(float *inTrans4x4, float r, float g, float b, float a)
{
    gDefaultShader->setMatrix4x4fv(gDefaultShader->mU_M, inTrans4x4);
    gDefaultShader->setUniform4f(gDefaultShader->mU_C, b, g, r, a);

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job *job = (*mJobs)[i];

        if (mPrevJob ? !mPrevJob->cmpmtl(job) : true)
        {
            flushQuads();

            bool mtlC = false;
            bool mtlA = false;
            bool mtlPremultA = false;
            bool mtlBlendAdd = job->isBlendModeAdd();
            if (job->mSurface)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, job->mSurface->getTextureId());
                mtlC = true;
                if (job->mSurface->getAlphaTextureId())
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, job->mSurface->getAlphaTextureId());
                    mtlA = true;
                }
                mtlPremultA = job->mSurface->alphaIsPremultiply();
            }
            gDefaultShader->setUniform4i(gDefaultShader->mU_MTL, mtlC, mtlA, mtlPremultA, mtlBlendAdd);

            float aspectW = 1.0f;
            float aspectH = 1.0f;
            if (job->isTypeTriangles())
            {
                aspectW = job->mSurface ? job->mSurface->Width() / (float)job->mSurface->getTextureWidth() : 1.0f;
                aspectH = job->mSurface ? job->mSurface->Height() / (float)job->mSurface->getTextureHeight() : 1.0f;  
            }
            gDefaultShader->setUniform2f(gDefaultShader->mU_TS, aspectW, aspectH);
        }

        if (job->isTypeTriangles())
        {
            flushQuads();

            gDefaultShader->setAttribPointer(job->mT_XY->id(), gDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, 0);
            gDefaultShader->setAttribPointer(job->mT_UV->id(), gDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, 0);
            if (job->mT_C)
                gDefaultShader->setAttribPointer(job->mT_C->id(), gDefaultShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, 0);
            else
                gDefaultShader->setAttrib4f(gDefaultShader->mA_C, job->b(), job->g(), job->r(), job->a());
            job->mT_I->draw(GL_TRIANGLES, job->mT_In, GL_UNSIGNED_SHORT, 0);
        }
        else 
        {
            if (!pushQuad(job))
            {
                flushQuads();
                pushQuad(job);
            }
        }
    }
    flushQuads();
}

void NevoRenderPipeline::begin()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);

    gDefaultShader->bind();
    gDefaultShader->setUniform1i(gDefaultShader->mU_TC, 0);
    gDefaultShader->setUniform1i(gDefaultShader->mU_TA, 1);
}

void NevoRenderPipeline::end()
{
    glDisableVertexAttribArray(gDefaultShader->mA_XY);
    glDisableVertexAttribArray(gDefaultShader->mA_UV);
    glDisableVertexAttribArray(gDefaultShader->mA_C);
    gDefaultShader->unbind();
}

bool NevoRenderPipeline::pushQuad(Job *job)
{
    if ((mI_n + 6) > cMaxVerts)
        return false;

    for (int i = 0; i < 4; ++i)
    {
        mXY.inc() = job->mQ_XY[i].x;
        mXY.inc() = job->mQ_XY[i].y;
        mUV.inc() = job->mQ_UV[i].u;
        mUV.inc() = job->mQ_UV[i].v;
        mC.inc() = job->mBGRA;
    }
    mI_n += 6;
    mPrevJob = job;

    return true;
}

void NevoRenderPipeline::flushQuads()
{
    if (mI_n > 0)
    {
        mXYvbo->update(0, mXY.sizeMemSize(), &mXY[0]);
        gDefaultShader->setAttribPointer(mXYvbo->id(), gDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, 0);
        mUVvbo->update(0, mUV.sizeMemSize(), &mUV[0]);
        gDefaultShader->setAttribPointer(mUVvbo->id(), gDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, 0);
        mCvbo->update(0, mC.sizeMemSize(), &mC[0]);
        gDefaultShader->setAttribPointer(mCvbo->id(), gDefaultShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, 0);
        mQIebo->draw(GL_TRIANGLES, mI_n, GL_UNSIGNED_SHORT, 0);

        mI_n = 0;
        mXY.resize(0);
        mUV.resize(0);
        mC.resize(0);
    }
    mPrevJob = 0;
}

}