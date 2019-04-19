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

static ColorShader *gColorShader = 0;
static TexShader *gTexShader = 0;
static TexAShader *gTexAShader = 0;
static GPUProgram *gCurShader = 0;

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

//Material
Material::Material()
{
    mSurface = 0;
    mColor.mBGRA = 0;
    mBlend = BlendType_NORMAL;
}

void Material::set(nme::Surface *surface, Color color, int blendMode)
{
    mSurface = surface;
    if (mSurface) mSurface->IncRef();
    mColor = color;
    mBlend = (blendMode == nme::bmAdd) ? BlendType_ADD : BlendType_NORMAL;
}

float Material::getBlendFactor()
{
    return (mBlend == BlendType_ADD) ? 0.0f : 1.0f;
}

bool Material::hasTexture()
{
    return mSurface;
}

bool Material::hasAlphaTexture()
{
    return mSurface ? mSurface->getAlphaTextureId() : false;
}

bool Material::alphaIsPremultiply()
{
    return mSurface ? mSurface->alphaIsPremultiply() : false;
}

int Material::getTextureId()
{
    return mSurface ? mSurface->getTextureId() : 0;
}

int Material::getAlphaTextureId()
{
    return mSurface ? mSurface->getAlphaTextureId() : 0;
}

int Material::getTextureWidth()
{
    return mSurface ? mSurface->Width() : 1;
}

int Material::getTextureHeight()
{
    return mSurface ? mSurface->Height() : 1;
}

int Material::getHardwareTextureWidth()
{
    return mSurface ? mSurface->getTextureWidth() : 1;
}

int Material::getHardwareTextureHeight()
{
    return mSurface ? mSurface->getTextureHeight() : 1;
}

float Material::getWidthRatio()
{
    return getTextureWidth() / (float)getHardwareTextureWidth();
}

float Material::getHeightRatio()
{
    return getTextureHeight() / (float)getHardwareTextureHeight();
}

void Material::clear()
{
    if (mSurface) mSurface->DecRef();
    mSurface = 0;
    mColor.mBGRA = 0;
    mBlend = BlendType_NORMAL;
}

//Job
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

    if (mMtl.hasTexture())
    {
        mQ_UV[0].u = x / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[0].v = y / (float)mMtl.getHardwareTextureHeight();
        mQ_UV[1].u = (x + width) / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[1].v = y / (float)mMtl.getHardwareTextureHeight();
        mQ_UV[2].u = (x + width) / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[2].v = (y + height) / (float)mMtl.getHardwareTextureHeight();
        mQ_UV[3].u = x / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[3].v = (y + height) / (float)mMtl.getHardwareTextureHeight();
    }

    mType = JobType_RECT;
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

    if (mMtl.hasTexture())
    {
        mQ_UV[0].u = rectX / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[0].v = rectY / (float)mMtl.getHardwareTextureHeight();
        mQ_UV[1].u = (rectX + rectW) / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[1].v = rectY / (float)mMtl.getHardwareTextureHeight();
        mQ_UV[2].u = (rectX + rectW) / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[2].v = (rectY + rectH) / (float)mMtl.getHardwareTextureHeight();
        mQ_UV[3].u = rectX / (float)mMtl.getHardwareTextureWidth();
        mQ_UV[3].v = (rectY + rectH) / (float)mMtl.getHardwareTextureHeight();
    }

    mType = JobType_TILE;
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

    mType = JobType_TRIANGLES;
}

bool Job::hitTest(float x, float y)
{
    return (mBBminX <= x) && (mBBminY <= y) && (mBBmaxX >= x) && (mBBmaxY >= y);
}

void Job::clear()
{
    mMtl.clear();
    if (isTypeTriangles())
    {
        if (mT_XY) gMemoryPool.refund(mT_XY);
        if (mT_UV) gMemoryPool.refund(mT_UV);
        if (mT_C) gMemoryPool.refund(mT_C);
        if (mT_I) gMemoryPool.refund(mT_I);
    }
    mT_XY = 0; mT_UV = 0; mT_C = 0; mT_I = 0; mT_In = 0;
    mType = JobType_NONE;
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
}

NevoRenderPipeline::~NevoRenderPipeline()
{
    
}

void NevoRenderPipeline::Init()
{
    Clear();

    gColorShader = new ColorShader();
    gTexShader = new TexShader();
    gTexAShader = new TexAShader();
    gVBOPool = new VBOPool();
    gEBOPool = new EBOPool();
    gNevoJobsPool = new JobsPool();

    mXY.reserve(cMaxVerts * 2);
    mUV.reserve(cMaxVerts * 2);
    mC.reserve(cMaxVerts);
    mI.reserve(cMaxVerts);
}

void NevoRenderPipeline::Clear()
{
    if (gColorShader)
    {
        delete gColorShader;
        gColorShader = 0;
    }

    if (gTexShader)
    {
        delete gTexShader;
        gTexShader = 0;
    }

    if (gTexAShader)
    {
        delete gTexAShader;
        gTexAShader = 0;
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

void NevoRenderPipeline::setNodeParams(float *inTrans4x4, Color color)
{
    if (color.isTransparent())
        return;

    Material *curMtl = 0;

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job *job = (*mJobs)[i];

        if (!job->mMtl.isTransparent())
        {
            if (curMtl ? (job->mMtl != *curMtl) : true)
            {
                curMtl = &job->mMtl;
                flushGeometry();

                GPUProgram *shader = gColorShader;
                if (job->mMtl.hasTexture())
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, job->mMtl.getTextureId());
                    shader = gTexShader;
                    if (job->mMtl.hasAlphaTexture())
                    {
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, job->mMtl.getAlphaTextureId());
                        shader = gTexAShader;
                    }
                }
                
                if (shader != gCurShader)
                {
                    gCurShader = shader;
                    if (gCurShader)
                    {
                        gCurShader->bind();
                        gCurShader->setUniform1f(gCurShader->mU_BLEND_F, job->mMtl.getBlendFactor());
                    }
                }

                if (gCurShader == gColorShader)
                {
                    
                }
                else if (gCurShader == gTexShader)
                {
                    gCurShader->setUniform1i(gCurShader->mU_TC, 0);
                    gCurShader->setUniform1f(gCurShader->mU_TA_MULT, job->mMtl.alphaIsPremultiply());
                }
                else if (gCurShader == gTexAShader)
                {
                    gCurShader->setUniform1i(gCurShader->mU_TC, 0);
                    gCurShader->setUniform1i(gCurShader->mU_TA, 1);
                    gCurShader->setUniform1f(gCurShader->mU_TA_MULT, job->mMtl.alphaIsPremultiply());
                }
            }

            gCurShader->setMatrix4x4fv(gCurShader->mU_M, inTrans4x4);
            gCurShader->setUniform4f(gCurShader->mU_C, color.b(), color.g(), color.r(), color.a());

            if (job->isTypeTriangles())
            {
                flushGeometry();

                gCurShader->setUniform2f(gCurShader->mU_UV_S, job->mMtl.getWidthRatio(), job->mMtl.getHeightRatio());
                gCurShader->setAttribPointer(0, gCurShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, job->mT_XY->ptr());
                gCurShader->setAttribPointer(0, gCurShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, job->mT_UV->ptr());
                if (job->mT_C)
                    gCurShader->setAttribPointer(0, gCurShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, job->mT_C->ptr());
                else
                    gCurShader->setAttrib4f(gCurShader->mA_C, job->mMtl.mColor.b(), job->mMtl.mColor.g(), job->mMtl.mColor.r(), job->mMtl.mColor.a());
                gCurShader->draw(0, GL_TRIANGLES, job->mT_In, GL_UNSIGNED_SHORT, job->mT_I->ptr());
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
    flushGeometry();
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

    gCurShader = 0;
}

void NevoRenderPipeline::end()
{
    if (gCurShader)
    {
        gCurShader->disableAttribs();
        gCurShader->unbind();
    }
}

bool NevoRenderPipeline::pushQuad(Job *job)
{
    if ((mI.size() + 6) > cMaxVerts)
        return false;

    mI.inc() = mC.size() + 0;
    mI.inc() = mC.size() + 1;
    mI.inc() = mC.size() + 2;
    mI.inc() = mC.size() + 2;
    mI.inc() = mC.size() + 3;
    mI.inc() = mC.size() + 0;
    for (int i = 0; i < 4; ++i)
    {
        mXY.inc() = job->mQ_XY[i].x;
        mXY.inc() = job->mQ_XY[i].y;
        mUV.inc() = job->mQ_UV[i].u;
        mUV.inc() = job->mQ_UV[i].v;
        mC.inc() = job->mMtl.mColor.mBGRA;
    }

    return true;
}

void NevoRenderPipeline::flushGeometry()
{
    if (mI.size() > 0)
    {
        gCurShader->setUniform2f(gCurShader->mU_UV_S, 1.0f, 1.0f);
        gCurShader->setAttribPointer(0, gCurShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, mXY.ptr());
        gCurShader->setAttribPointer(0, gCurShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, mUV.ptr());
        gCurShader->setAttribPointer(0, gCurShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, mC.ptr());
        gCurShader->draw(0, GL_TRIANGLES, mI.size(), GL_UNSIGNED_SHORT, mI.ptr());

        mXY.resize(0);
        mUV.resize(0);
        mC.resize(0);
        mI.resize(0);
    }
}

}