#include "NevoRenderPipeline.h"

#include "OGL.h"
#include <iostream>
#include <Surface.h>
#include "NevoShaders.h"

using namespace nme;

namespace nevo
{

NevoRenderPipeline gNevoRender;
static ColorShader *gColorShader = 0;
static TexShader *gTexShader = 0;
static TexAShader *gTexAShader = 0;
static GPUProgram *gCurShader = 0;

//Material
Material::Material()
{
    mSurface = 0;
    mBlend = BlendType_NORMAL;
}

Material::~Material()
{
    if (mSurface) mSurface->DecRef();
}

void Material::setSurface(nme::Surface *surface)
{
    if (mSurface) mSurface->DecRef();
    mSurface = surface;
    if (mSurface) mSurface->IncRef();
}

void Material::setBlendMode(int blendMode)
{
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
    setSurface(0);
    mColor.mBGRA = 0xFFFFFFFF;
    mBlend = BlendType_NORMAL;
}

//NevoRenderPipeline
NevoRenderPipeline::NevoRenderPipeline()
{

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
}

void NevoRenderPipeline::setGraphicsData(Vec<Job*> *jobs, Vec<float> *xy, Vec<float> *uv, Vec<int> *c)
{
    mJobs = jobs;
    mXY = xy;
    mUV = uv;
    mC = c;
}

void NevoRenderPipeline::drawGraphicsData(float *inTrans4x4, Color color)
{
    if (color.isTransparent())
       return;

    bool refreshOnceParams = true;

    for (int i = 0; i < mJobs->size(); ++i)
    {
        Job *job = (*mJobs)[i];

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
            gCurShader->bind();
            refreshOnceParams = true;
        }

        if (refreshOnceParams)
        {
            gCurShader->setMatrix4x4fv(gCurShader->mU_M, inTrans4x4);
            gCurShader->setUniform4f(gCurShader->mU_C, color.b(), color.g(), color.r(), color.a());
            gCurShader->setAttribPointer(0, gCurShader->mA_XY, 2, GL_FLOAT, GL_FALSE, 8, mXY->ptr());
            gCurShader->setAttribPointer(0, gCurShader->mA_UV, 2, GL_FLOAT, GL_FALSE, 8, mUV->ptr());
            gCurShader->setAttribPointer(0, gCurShader->mA_C, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4, mC->ptr());
            refreshOnceParams = false;
        }

        gCurShader->setUniform1i(gCurShader->mU_TC, 0);
        gCurShader->setUniform1i(gCurShader->mU_TA, 1);
        gCurShader->setUniform1f(gCurShader->mU_TA_MULT, job->mMtl.alphaIsPremultiply());
        gCurShader->setUniform1f(gCurShader->mU_BLEND_F, job->mMtl.getBlendFactor());

        gCurShader->draw(GL_TRIANGLES, job->mVfirst, job->mVcount);
    }
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

}