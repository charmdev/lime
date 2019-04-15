#include "NevoRenderPipeline.h"

#include "OGL.h"
#include <Surface.h>
#include <iostream>

#include "NevoShaders.h"
#include "NevoGLContainers.h"
#include "NevoMemory.h"

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
    return (mSurface == job->mSurface) && (getBlendMode() == job->getBlendMode());
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
        mT_XY = gMemoryPool.get(inXYs_n * sizeof(float));
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
        mT_UV = gMemoryPool.get(inUVT_n * sizeof(float));
        mT_UV->update(0, inUVT_n * sizeof(float), inUVT);
    }

    if (inColours)
    {
        mT_C = gMemoryPool.get(inColours_n * sizeof(int));
        mT_C->update(0, inColours_n * sizeof(int), inColours);
    }

    if (inIndixes)
    {
        mT_I = gMemoryPool.get(inIndixes_n * sizeof(short));
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
        if (mT_XY) gMemoryPool.refund(mT_XY);
        if (mT_UV) gMemoryPool.refund(mT_UV);
        if (mT_C) gMemoryPool.refund(mT_C);
        if (mT_I) gMemoryPool.refund(mT_I);
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

    mXY.reserve(cMaxVerts * 2);
    mUV.reserve(cMaxVerts * 2);
    mC.reserve(cMaxVerts);
    mI.reserve(cMaxVerts);
    mMTL.reserve(cMaxVerts * 4);

    mChU.resize(cMaxCh);
    mChTex.resize(cMaxCh);
    mTexCh.resize(1024);
    for (int i = 0; i < cMaxCh; ++i)
    {
        mChU[i] = i;
        mChTex[i] = -1;
    }
    for (int i = 0; i < mTexCh.size(); ++i)
        mTexCh[i] = -1;
    clrMtl();
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
    /*gDefaultShader->setMatrix4x4fv(gDefaultShader->mU_M, inTrans4x4);
    gDefaultShader->setUniform4f(gDefaultShader->mU_C, b, g, r, a);

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job *job = (*mJobs)[i];

        if (mPrevJob ? !mPrevJob->cmpmtl(job) : true)
        {
            flushGeometry();

            float mtlC = 10.0f;
            float mtlA = 0.0f;
            float mtlPremultA = 0.0f;
            float mtlBlendAdd = job->isBlendModeAdd() ? 0.0f : 1.0f;
            if (job->mSurface)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, job->mSurface->getTextureId());
                mtlC = 0.0f;
                if (job->mSurface->getAlphaTextureId())
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, job->mSurface->getAlphaTextureId());
                    mtlA = 1.0f;
                }
                mtlPremultA = job->mSurface->alphaIsPremultiply() ? 1.0f : 0.0f;
            }
            gDefaultShader->setAttrib4f(gDefaultShader->mA_MTL, mtlC, mtlA, mtlPremultA, mtlBlendAdd);
        }

        if (job->isTypeTriangles())
        {
            flushGeometry();

            float Us = job->mSurface ? job->mSurface->Width() / (float)job->mSurface->getTextureWidth() : 1.0f;
            float Vs = job->mSurface ? job->mSurface->Height() / (float)job->mSurface->getTextureHeight() : 1.0f;
            gDefaultShader->setUniform2f(gDefaultShader->mU_UV_S, Us, Vs);
            gDefaultShader->setAttribPointer(0, gDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, job->mT_XY->ptr());
            gDefaultShader->setAttribPointer(0, gDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, job->mT_UV->ptr());
            if (job->mT_C)
                gDefaultShader->setAttribPointer(0, gDefaultShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, job->mT_C->ptr());
            else
                gDefaultShader->setAttrib4f(gDefaultShader->mA_C, job->b(), job->g(), job->r(), job->a());
            gDefaultShader->draw(0, GL_TRIANGLES, job->mT_In, GL_UNSIGNED_SHORT, job->mT_I->ptr());
        }
        else 
        {
            if (!pushQuad(job))
            {
                flushGeometry();
                pushQuad(job);
            }
        }
    }
    flushGeometry();*/
}

void NevoRenderPipeline::setNodeParams2(float *inTrans4x4, float r, float g, float b, float a)
{
    mT00 = inTrans4x4[0]; mT01 = inTrans4x4[1];
    mT10 = inTrans4x4[4]; mT11 = inTrans4x4[5];
    mTTX = inTrans4x4[3]; mTTY = inTrans4x4[7];
    mR = r; mG = g; mB = b; mA = a;

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job *job = (*mJobs)[i];

        if (!setMtl(job))
        {
            flushGeometry();
            clrMtl();
            setMtl(job);
        }

        if (job->isTypeTriangles())
        {
            pushTriangles(job);
        }
        else
        {
            if (!pushQuad(job))
            {
                flushGeometry();
                pushQuad(job);
            }
        }
    }
}

void NevoRenderPipeline::begin()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    for (int i = 0; i < cMaxCh; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glEnable(GL_TEXTURE_2D);
    }

    gDefaultShader->bind();
    gDefaultShader->setUniform1iv(gDefaultShader->mU_T, cMaxCh, (GLint*)mChU.ptr());
}

void NevoRenderPipeline::end()
{
    flushGeometry();
    clrMtl();

    glDisableVertexAttribArray(gDefaultShader->mA_XY);
    glDisableVertexAttribArray(gDefaultShader->mA_UV);
    glDisableVertexAttribArray(gDefaultShader->mA_C);
    glDisableVertexAttribArray(gDefaultShader->mA_MTL);
    gDefaultShader->unbind();
}

bool NevoRenderPipeline::pushQuad(Job *job)
{
    static float x, y;
    static Color c;

    if ((mI.size() + 6) > cMaxVerts)
        return false;

    c.mBGRA = job->mBGRA;
    c.mult(mR, mG, mB, mA);

    mI.inc() = mC.size() + 0;
    mI.inc() = mC.size() + 1;
    mI.inc() = mC.size() + 2;
    mI.inc() = mC.size() + 2;
    mI.inc() = mC.size() + 3;
    mI.inc() = mC.size() + 0;
    for (int i = 0; i < 4; ++i)
    {
        mult(job->mQ_XY[i].x, job->mQ_XY[i].y, mT00, mT10, mT01, mT11, mTTX, mTTY, &x, &y);

        //mXY.inc() = job->mQ_XY[i].x;
        //mXY.inc() = job->mQ_XY[i].y;
        mXY.inc() = x;
        mXY.inc() = y;
        mUV.inc() = job->mQ_UV[i].u;
        mUV.inc() = job->mQ_UV[i].v;
        //mC.inc() = job->mBGRA;
        mC.inc() = c.mBGRA;
        mMTL.inc() = mMtlC;
        mMTL.inc() = mMtlA;
        mMTL.inc() = mMtlPremA;
        mMTL.inc() = mMtlAddBlend;
    }
    mPrevJob = job;

    return true;
}

void NevoRenderPipeline::pushTriangles(Job *job)
{
    /*float *xy = (float*)job->mT_XY->ptr();
    float *uv = (float*)job->mT_UV->ptr();
    unsigned int *c = job->mT_C ? (unsigned int*)job->mT_C->ptr() : 0;
    short *i = (short*)job->mT_I->ptr();
    int i_n = job->mT_In;*/


}

void NevoRenderPipeline::flushGeometry()
{
    if (mI.size() > 0)
    {
        //gDefaultShader->setUniform2f(gDefaultShader->mU_UV_S, 1.0f, 1.0f);
        gDefaultShader->setAttribPointer(0, gDefaultShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, mXY.ptr());
        gDefaultShader->setAttribPointer(0, gDefaultShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, mUV.ptr());
        gDefaultShader->setAttribPointer(0, gDefaultShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, mC.ptr());
        gDefaultShader->setAttribPointer(0, gDefaultShader->mA_MTL, 4, GL_FLOAT, GL_FALSE, 16, mMTL.ptr());
        gDefaultShader->draw(0, GL_TRIANGLES, mI.size(), GL_UNSIGNED_SHORT, mI.ptr());

        mXY.resize(0);
        mUV.resize(0);
        mC.resize(0);
        mMTL.resize(0);
        mI.resize(0);
    }
    mPrevJob = 0;
}

bool NevoRenderPipeline::setMtl(Job *job)
{
    if (job->mSurface)
    {
        int cId = job->mSurface->getTextureId();
        if (mTexCh[cId] == -1)
        {
            if (mCurCh < cMaxCh)
            {
                glActiveTexture(GL_TEXTURE0 + mCurCh);
                glBindTexture(GL_TEXTURE_2D, cId);
                mMtlC = mCurCh;
                mTexCh[cId] = mCurCh;
                mChTex[mCurCh] = cId;
                ++mCurCh;
            }
            else
            {
                return false;
            }
        }
        else
        {
            mMtlC = mTexCh[cId];
        }

        int aId = job->mSurface->getAlphaTextureId();
        if (aId != 0)
        {
            if (mTexCh[aId] == -1)
            {
                if (mCurCh < cMaxCh)
                {
                    glActiveTexture(GL_TEXTURE0 + mCurCh);
                    glBindTexture(GL_TEXTURE_2D, aId);
                    mMtlA = mCurCh;
                    mTexCh[aId] = mCurCh;
                    mChTex[mCurCh] = aId;
                    ++mCurCh;
                }
                else 
                {
                    return false;
                }
            }
            else
            {
                mMtlA = mTexCh[aId];
            }
        }
        else
        {
            mMtlA = cMaxCh;
        }
        mMtlPremA = job->mSurface->alphaIsPremultiply() ? 1.0f : 0.0f;
    }
    else
    {
        mMtlC = cMaxCh;
        mMtlA = cMaxCh;
        mMtlPremA = 0.0f;
    }
    mMtlAddBlend = job->isBlendModeAdd() ? 0.0f : 1.0f;
    return true;
}

void NevoRenderPipeline::clrMtl()
{
    mCurCh = 0;
    for (int i = 0; i < cMaxCh; ++i)
    {
        if (mChTex[i] != -1)
        {
            mTexCh[mChTex[i]] = -1;
        }
        mChTex[i] = -1;
    }
    mMtlC = cMaxCh;
    mMtlA = cMaxCh;
    mMtlPremA = 0.0f;
    mMtlAddBlend = 1.0f;
}

}