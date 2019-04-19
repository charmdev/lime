#ifndef NEVO_RENDER_PIPELINE_H
#define NEVO_RENDER_PIPELINE_H

#include "NevoVec.h"

namespace nme
{

class Surface;

}

namespace nevo
{

class VBO;
class EBO;
class Memory;

struct Color
{
    Color() {}

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

    void set(nme::Surface *surface, Color color, int blendMode);
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

    inline bool operator==(const Material &rhs)
    {
        return (mSurface ==  rhs.mSurface) && (mBlend == rhs.mBlend);
    }
    inline bool operator!=(const Material &rhs) { return !(*this == rhs); }

    nme::Surface *mSurface;
    Color mColor;
    BlendType mBlend;    
};

struct Job
{
    enum JobType { JobType_NONE, JobType_RECT, JobType_TILE, JobType_TRIANGLES };

    Job::Job()
    {
        mT_XY = 0; mT_UV = 0; mT_C = 0; mT_I = 0; mT_In = 0;
        mType = JobType_NONE;
    }

    Job::~Job()
    {
        clear();
    }
    
    void rect(float x, float y, float width, float height);
    void tile(float x, float y, int rectX, int rectY, int rectW, int rectH, float *inTrans);
    void triangles(int inXYs_n, float *inXYs,
        int inIndixes_n, short *inIndixes, int inUVT_n, float *inUVT,
        int inColours_n, int *inColours);
    bool hitTest(float x, float y);
    void clear();

    bool isTypeRect() { return (mType == JobType_RECT); }
    bool isTypeTile() { return (mType == JobType_TILE); }
    bool isTypeTriangles() { return (mType == JobType_TRIANGLES); }

    Material mMtl;
    JobType mType;

    union
    {
        struct
        {
            Memory *mT_XY;
            Memory *mT_UV;
            Memory *mT_C;
            Memory *mT_I;
            int mT_In;
        };
        
        struct
        {
            struct { float x, y; } mQ_XY[4];
            struct { float u, v; } mQ_UV[4];
        };
    };

    float mBBminX, mBBminY, mBBmaxX, mBBmaxY;
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
};

class JobsPool
{
public:
    JobsPool();
    ~JobsPool();

    Job* get();
    void refund(Job *job);
private:
    Vec<Job*> mAlloc;
    Vec<Job*> mFree;
};

class NevoRenderPipeline
{
public:
    NevoRenderPipeline();
    ~NevoRenderPipeline();

    void Init();
    void Clear();

    void setJobs(Vec<Job*> *jobs);
    void setNodeParams(float *inTrans4x4, Color color);
    void begin();
    void end();

private:
    bool pushQuad(Job *job);
    void flushGeometry();

    Vec<Job*> *mJobs;

    static const int cMaxVerts = 65535;
    Vec<float> mXY;
    Vec<float> mUV;
    Vec<int> mC;
    Vec<unsigned short> mI;
};

extern JobsPool *gNevoJobsPool;
extern NevoRenderPipeline gNevoRender;

}

#endif