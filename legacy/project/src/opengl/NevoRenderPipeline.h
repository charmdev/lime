#ifndef NEVO_RENDER_PIPELINE_H
#define NEVO_RENDER_PIPELINE_H

#include "NevoVec.h"

namespace nme
{

class Surface;

}

namespace nevo
{

class Job
{
public:
    Job();
    ~Job();
    
    void mtl(nme::Surface *surface, unsigned int color, int blendMode);
    void rect(float x, float y, float width, float height);
    void tile(float x, float y, int rectX, int rectY, int rectW, int rectH, float *inTrans);
    void triangles(int inXYs_n, float *inXYs,
        int inIndixes_n, int *inIndixes, int inUVT_n, float *inUVT,
        int inColours_n, int *inColours);
    bool hitTest(float x, float y);
    void clear();

    void setTypeRect() { mFlags |= 1; }
    void setTypeTile() { mFlags |= 2; }
    void setTypeTriangles() { mFlags |= 4; }
    bool isTypeRect() { return mFlags & 1; }
    bool isTypeTile() { return mFlags & 2; }
    bool isTypeTriangles() { return mFlags & 4; }

    void setBlendModeNone() { mFlags |= 8; }
    void setBlendModeNormal() { mFlags |= 16; }
    void setBlendModeAdd() { mFlags |= 32; }
    bool isBlendModeNone() { return mFlags & 8; }
    bool isBlendModeNormal() { return mFlags & 16; }
    bool isBlendModeAdd() { return mFlags & 32; }

    void setPremultAlpha() { mFlags |= 64; }
    bool isPremultAlpha() { return mFlags & 64; }

    float r() { return mR / 255.0f; }
    float g() { return mG / 255.0f; }
    float b() { return mB / 255.0f; }
    float a() { return mA / 255.0f; }

public:
    nme::Surface *mSurface;
    unsigned short int mTexColor, mTexAlpha;
    unsigned short int mTexW, mTexH;
    unsigned short int mTexPixW, mTexPixH;
    union {
        unsigned int mBGRA;
        struct { unsigned char mB, mG, mR, mA; };
    };
    unsigned char mFlags;

    union
    {
        struct
        {
            float *mT_XY;
            float *mT_UV;
            int *mT_C;
            int *mT_I;
            int mT_Vn;
            int mT_In;
        };
        struct
        {
            struct { float x, y; } mQ_XY[4];
            struct { float u, v; } mQ_UV[4];
        };
    };
};

class JobsPool
{
public:
    JobsPool();
    ~JobsPool();

    Job* get();
    void refund(Job *job);

private:
    Vec<Job*> mAllocJobs;
    Vec<Job*> mFreeJobs;
};

class NevoRenderPipeline
{
public:
    NevoRenderPipeline();
    ~NevoRenderPipeline();

    void Init();
    void Clear();

    void setJobs(Vec<Job*> *jobs);
    void setNodeParams(float *inTrans4x4, float r, float g, float b, float a);
    void begin();
    void end();

private:
    Vec<Job*> *mJobs;

    static const int cMaxIndex = 65535;
    Vec<float> mXY;
    Vec<float> mUV;
    Vec<int> mC;
    Vec<unsigned short int> mI;

    unsigned int mXYvbo;
    unsigned int mUVvbo;
    unsigned int mCvbo;
    unsigned int mIebo;
};

extern JobsPool gNevoJobsPool;
extern NevoRenderPipeline gNevoRender;

}

#endif