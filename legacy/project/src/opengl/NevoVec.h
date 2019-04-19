#ifndef NEVO_VEC_H
#define NEVO_VEC_H

#include <memory.h>
#include <stdlib.h>

namespace nevo
{

template <typename T>
class Vec
{
public:
    Vec(int alloc = 16)
    {
        mSize = 0;
        mAlloc = alloc;
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

    void dec()
    {
        if (mSize > 0)
            --mSize;
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

    int allocMemSize()
    {
        return mAlloc * sizeof(T);
    }

    int sizeMemSize()
    {
        return mSize * sizeof(T);
    }

    inline int size()
    {
        return mSize;
    }

    inline T& operator[](int index)
    {
        return mPtr[index];
    }

    inline void* ptr()
    {
        return (void*)mPtr;
    }
    
private:
    int mSize;
    int mAlloc;
    T *mPtr;
};

}

#endif