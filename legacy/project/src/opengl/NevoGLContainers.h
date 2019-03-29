#ifndef NEVO_GL_CONTAINERS_H
#define NEVO_GL_CONTAINERS_H

#include "NevoVec.h"

namespace nevo
{

class MemoryBuffer
{
public:
    MemoryBuffer(int size);
    ~MemoryBuffer();

    void* ptr();
    void update(int offset, int size, void *data);
    int size();
    int sizePO2();

private:
    void *mPtr;
    int mSize;
    int mSizePO2;
};

class MemoryBufferPool
{
public:
    MemoryBufferPool();
    ~MemoryBufferPool();

    MemoryBuffer* get(int size);
    void refund(MemoryBuffer *mb);

private:
    Vec<MemoryBuffer*> mAlloc;
    Vec<Vec<MemoryBuffer*>*> mFree;
};

extern MemoryBufferPool gMemoryBufferPool;

class VBO
{
public:
    VBO(int size, bool staticStorage = false);
    ~VBO();

    int size();
    int sizePO2();
    bool isStatic();

    unsigned id();
    void update(int offset, int size, void *data);

private:
    unsigned mVBO;
    int mSize;
    int mSizePO2;
    bool mStaticStorage;
};

class VBOPool
{
public:
    VBOPool();
    ~VBOPool();

    VBO* get(int size, bool staticStorage = false);
    void refund(VBO *vbo);
private:
    Vec<VBO*> mAlloc;
    Vec<Vec<VBO*>*> mFreeStatic;
    Vec<Vec<VBO*>*> mFreeDynamic;
};

class EBO
{
public:
    EBO(int size, bool staticStorage = false);
    ~EBO();

    int size();
    int sizePO2();
    bool isStatic();

    void update(int offset, int size, void *data);
    void draw(unsigned mode, int count, unsigned type, int offset);

private:
    unsigned mEBO;
    int mSize;
    int mSizePO2;
    bool mStaticStorage;
};

class EBOPool
{
public:
    EBOPool();
    ~EBOPool();

    EBO* get(int size, bool staticStorage = false);
    void refund(EBO *ebo);
private:
    Vec<EBO*> mAlloc;
    Vec<Vec<EBO*>*> mFreeStatic;
    Vec<Vec<EBO*>*> mFreeDynamic;
};

}

#endif