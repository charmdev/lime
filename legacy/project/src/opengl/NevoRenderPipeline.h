#ifndef NEVO_RENDER_PIPELINE_H
#define NEVO_RENDER_PIPELINE_H

#include "NevoVec.h"

namespace nme
{

class Surface;

}

namespace nevo
{

struct Color
{
    Color() { mBGRA = 0xFFFFFFFF; }

    float r() { return mR / 255.0f; }
    float g() { return mG / 255.0f; }
    float b() { return mB / 255.0f; }
    float a() { return mA / 255.0f; }

    void set(float nR, float nG, float nB, float nA)
    {
        mR = (unsigned char)(nR * 255.0f);
        mG = (unsigned char)(nG * 255.0f);
        mB = (unsigned char)(nB * 255.0f);
        mA = (unsigned char)(nA * 255.0f);
    }

    void set(unsigned int bgr, float nA)
    {
        mBGRA = bgr;
        mA = (unsigned char)(nA * 255.0f);
    }

    void set(float *inRGBA)
    {
        if (inRGBA)
            set(inRGBA[0], inRGBA[1], inRGBA[2], inRGBA[3]);
        else
            mBGRA = 0xFFFFFFFF;
    }

    void mult(float nR, float nG, float nB, float nA)
    {
        mR = (unsigned char)(r() * nR * 255.0f);
        mG = (unsigned char)(g() * nG * 255.0f);
        mB = (unsigned char)(b() * nB * 255.0f);
        mA = (unsigned char)(a() * nA * 255.0f);
    }

    bool isTransparent() { return (mA < 10); }

    union
    {
        unsigned int mBGRA;
        struct { unsigned char mB, mG, mR, mA; };
    };
};

struct Material
{
    enum BlendType { BlendType_NORMAL, BlendType_ADD };

    Material();
    ~Material();

    void setSurface(nme::Surface *surface);
    void setBlendMode(int blendMode);
    float getBlendFactor();
    bool isTransparent() { return mColor.isTransparent(); }
    bool hasTexture();
    bool hasAlphaTexture();
    bool alphaIsPremultiply();
    int getTextureId();
    int getAlphaTextureId();
    int getTextureWidth();
    int getTextureHeight();
    int getHardwareTextureWidth();
    int getHardwareTextureHeight();
    float getWidthRatio();
    float getHeightRatio();

    void clear();

    bool operator==(const Material &rhs)
    {
        return (mSurface ==  rhs.mSurface) && (mBlend == rhs.mBlend);
    }

    bool operator!=(const Material &rhs) { return !(*this == rhs); }
    
    Material& operator=(const Material &rhs)
    {
        setSurface(rhs.mSurface);
        mColor = rhs.mColor;
        mBlend = rhs.mBlend;
        return *this;
    }

    nme::Surface *mSurface;
    Color mColor;
    BlendType mBlend;    
};

struct Job
{
    Job() {}

    void clear()
    {
        mMtl.clear();
    }

    Material mMtl;
    int mVfirst, mVcount;
};

struct BB
{
    BB() {}

    bool hitTest(float x, float y)
    {
        return (mBBminX <= x) && (mBBminY <= y) && (mBBmaxX >= x) && (mBBmaxY >= y);
    }

    void initBB(float x, float y)
    {
        mBBminX = mBBmaxX = x;
        mBBminY = mBBmaxY = y;
    }

    void calcBB(float x, float y)
    {
        if (x < mBBminX) mBBminX = x;
        if (x > mBBmaxX) mBBmaxX = x;
        if (y < mBBminY) mBBminY = y;
        if (y > mBBmaxY) mBBmaxY = y;
    }

    float mBBminX, mBBminY, mBBmaxX, mBBmaxY;
};

class NevoRenderPipeline
{
public:
    NevoRenderPipeline();
    ~NevoRenderPipeline();

    void Init();
    void Clear();

    void setGraphicsData(Vec<Job*> *jobs, Vec<float> *xy, Vec<float> *uv, Vec<int> *c);
    void drawGraphicsData(float *inTrans4x4, Color color);
    void begin();
    void end();

private:
    Vec<Job*> *mJobs;
    Vec<float> *mXY;
    Vec<float> *mUV;
    Vec<int> *mC;
};

extern NevoRenderPipeline gNevoRender;

}

#endif