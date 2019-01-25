#ifndef NEVO_RENDER_PIPELINE_H
#define NEVO_RENDER_PIPELINE_H

#include <memory.h>
#include <stdlib.h>

namespace nevo
{

template <typename T>
class Vec
{
public:
    Vec()
    {
        mSize = 0;
        mAlloc = 16;
        mPtr = (T*)malloc(mAlloc * sizeof(T));
    }

    ~Vec()
    {
        free(mPtr);
    }

    T& last()
    {
        return mPtr[mSize - 1];
    }

    T& inc()
    {
        ++mSize;
        if (mSize == mAlloc)
        {
            mAlloc += mAlloc;
            mPtr = (T*)realloc(mPtr, mAlloc * sizeof(T));
        }
        return mPtr[mSize - 1];
    }

    void resize(int size)
    {
        mSize = size;
        if (mSize > mAlloc)
        {
            mAlloc = mSize;
            mPtr = (T*)realloc(mPtr, mAlloc * sizeof(T));
        }
    }

    void reserve(int size)
    {
        if (size > mAlloc)
        {
            mAlloc = size;
            mPtr = (T*)realloc(mPtr, mAlloc * sizeof(T));
        }
    }

    int size()
    {
        return mSize;
    }

    T& operator[](int index)
    {
        return mPtr[index];
    }
    
private:
    int mSize;
    int mAlloc;
    T *mPtr;
};

template <typename T>
class List
{
public:


private:
    Vec<T*> mTPtr;
};

struct Job
{
    Job() {}

    unsigned short int mTexColor, mTexAlpha;
    unsigned short int mTexW, mTexH;
    unsigned short int mTexPixW, mTexPixH;
    union {
        unsigned int mBGRA;
        struct { unsigned char mB, mG, mR, mA; };
    };

    float *mXY;
    unsigned short int mXY_n;
    float *mUV;
    unsigned short int mUV_n;
    unsigned short int *mInd;
    unsigned short int mInd_n;
    
    void rect(float x, float y, float width, float height);
    void tile(float x, float y, int rectX, int rectY, int rectW, int rectH, float *inTrans);
    void triangles(int inXYs_n, double *inXYs,
        int inIndixes_n, int *inIndixes, int inUVT_n, double *inUVT,
        int inColours_n, int *inColours);
    bool hitTest(float x, float y);
    void free_mem();

    //
    unsigned char mFlags;
    void setTypeRect() { mFlags |= 1; }
    void setTypeTile() { mFlags |= 2; }
    void setTypeTriangles() { mFlags |= 4; }
    bool isTypeRect() { return mFlags & 1; }
    bool isTypeTile() { return mFlags & 2; }
    bool isTypeTriangles() { return mFlags & 4; }
    //
    void setBlendModeNone() { mFlags |= 8; }
    void setBlendModeNormal() { mFlags |= 16; }
    void setBlendModeAdd() { mFlags |= 32; }
    bool isBlendModeNone() { return mFlags & 8; }
    bool isBlendModeNormal() { return mFlags & 16; }
    bool isBlendModeAdd() { return mFlags & 32; }
    //
    void setPremultAlpha() { mFlags |= 64; }
    bool isPremultAlpha() { return mFlags & 64; }
};

class NevoRenderPipeline
{
public:
    NevoRenderPipeline();
    ~NevoRenderPipeline();

    void Init();
    void Clear();

    void setJobs(Vec<Job> *jobs);
    void setNodeParams(float *inTrans4x4, float r, float g, float b, float a);
    void begin();
    void end();

private:
    class DefaultShader;
    DefaultShader *mDefaultShader;

    Vec<Job> *mJobs;

    Vec<int> mChTex;
    Vec<int> mTexCh;
    Vec<int> mChUVal;
};

extern NevoRenderPipeline gNevoRender;

}

#endif