#ifndef NEVO_RENDER_PIPELINE_H
#define NEVO_RENDER_PIPELINE_H

#include <memory.h>
#include <stdlib.h>
#include <nme/Rect.h>

namespace nme 
{

class Surface;

}

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

class Job
{
public:
    enum Type {RECT, TILE, TRIANGLES};
    enum BlendMode {NONE = -1, NORMAL = 0, ADD = 7};

    Job() {}

    nme::Surface *mSurface;
    bool mPremultAlpha;
    int mTexColor;
    int mTexAlpha;
    float mTexPixW;
    float mTexPixH;
    float mTexW;
    float mTexH;
  
    float *mXY;
    int mXY_n;
    float *mUV;
    int mUV_n;
    int *mInd;
    int mInd_n;

    Type mType;
    BlendMode mBlendMode;
    
    void tex(nme::Surface *surface);
    void rect(float x, float y, float width, float height, int bgra, int blendMode);
    void tile(float x, float y, const nme::Rect &inTileRect, float *inTrans, int bgra, int blendMode);
    void triangles(int inXYs_n, double *inXYs,
        int inIndixes_n, int *inIndixes, int inUVT_n, double *inUVT,
        int inColours_n, int *inColours, int bgra, int blendMode);
    bool hitTest(float x, float y);
    void free_mem();

private:
    BlendMode toBlendMode(int blendMode);
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

    Vec<float> mXY;
    Vec<float> mUV;
    Vec<int> mInd;
};

extern NevoRenderPipeline gNevoRender;

}

#endif