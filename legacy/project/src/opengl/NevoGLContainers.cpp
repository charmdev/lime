#include "NevoGLContainers.h"

#include "OGL.h"

using namespace nme;

namespace nevo
{

static int gMinBufSize = 1024;
static int gMinBufSizePower = 10;
static int gMaxBufSize = 16777216;
static int gMaxBufSizePower = 24;
static int sizePO2(int bytes, int *power)
{
    static int bytes_po2;
    bytes_po2 = gMinBufSize;
    *power = gMinBufSizePower;
    while ((bytes > bytes_po2) && (bytes_po2 < gMaxBufSize))
    {
        bytes_po2 *= 2;
        ++(*power);
    }
    return bytes_po2;
}

//VBO
VBO::VBO(int size, bool staticStorage)
{
    mSize = nevo::sizePO2(size, &mSizePO2);
    mStaticStorage = staticStorage;
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mSize, 0, mStaticStorage ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
}

VBO::~VBO()
{
    glDeleteBuffers(1, &mVBO);
}

int VBO::size()
{
    return mSize;
}

int VBO::sizePO2()
{
    return mSizePO2;
}

bool VBO::isStatic()
{
    return mStaticStorage;
}

unsigned VBO::id()
{
    return mVBO;
}

void VBO::update(int offset, int size, void *data)
{
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

//VBOPool
VBOPool::VBOPool()
{
    mFreeStatic.resize(gMaxBufSizePower);
    mFreeDynamic.resize(gMaxBufSizePower);
    for (int i = 0; i < gMaxBufSizePower; ++i)
    {
        mFreeStatic[i] = new Vec<VBO*>();
        mFreeDynamic[i] = new Vec<VBO*>();
    }
}

VBOPool::~VBOPool()
{
    for (int i = 0; i < mAlloc.size(); ++i)
        delete mAlloc[i];
    for (int i = 0; i < gMaxBufSizePower; ++i)
    {
        delete mFreeStatic[i];
        delete mFreeDynamic[i];
    }
}

VBO* VBOPool::get(int size, bool staticStorage)
{
    if (size > gMaxBufSize)
        return 0;
    
    int sizePO2;
    int sizePO2bytes = nevo::sizePO2(size, &sizePO2);

    if (staticStorage)
    {
        if (mFreeStatic[sizePO2]->size() > 0)
        {
            VBO *vbo = mFreeStatic[sizePO2]->last();
            mFreeStatic[sizePO2]->dec();
            return vbo;
        }
    }
    else
    {
        if (mFreeDynamic[sizePO2]->size() > 0)
        {
            VBO *vbo = mFreeDynamic[sizePO2]->last();
            mFreeDynamic[sizePO2]->dec();
            return vbo;
        }
    }
    return mAlloc.inc() = new VBO(size, staticStorage);
}

void VBOPool::refund(VBO *vbo)
{
    if (vbo->isStatic())
        mFreeStatic[vbo->sizePO2()]->inc() = vbo;
    else
        mFreeDynamic[vbo->sizePO2()]->inc() = vbo;
}

//EBO
EBO::EBO(int size, bool staticStorage)
{
    mSize = nevo::sizePO2(size, &mSizePO2);
    mStaticStorage = staticStorage;
    glGenBuffers(1, &mEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mSize, 0, mStaticStorage ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
}

EBO::~EBO()
{
    glDeleteBuffers(1, &mEBO);
}

int EBO::size()
{
    return mSize;
}

int EBO::sizePO2()
{
    return mSizePO2;
}

bool EBO::isStatic()
{
    return mStaticStorage;
}

void EBO::update(int offset, int size, void *data)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
}

void EBO::draw(unsigned mode, int count, unsigned type, int offset)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glDrawElements(mode, count, type, (const void*)offset);
}

//EBOPool
EBOPool::EBOPool()
{
    mFreeStatic.resize(gMaxBufSizePower);
    mFreeDynamic.resize(gMaxBufSizePower);
    for (int i = 0; i < gMaxBufSizePower; ++i)
    {
        mFreeStatic[i] = new Vec<EBO*>();
        mFreeDynamic[i] = new Vec<EBO*>();
    }
}

EBOPool::~EBOPool()
{
    for (int i = 0; i < mAlloc.size(); ++i)
        delete mAlloc[i];
    for (int i = 0; i < gMaxBufSizePower; ++i)
    {
        delete mFreeStatic[i];
        delete mFreeDynamic[i];
    }
}

EBO* EBOPool::get(int size, bool staticStorage)
{
    if (size > gMaxBufSize)
        return 0;
    
    int sizePO2;
    int sizePO2bytes = nevo::sizePO2(size, &sizePO2);

    if (staticStorage)
    {
        if (mFreeStatic[sizePO2]->size() > 0)
        {
            EBO *ebo = mFreeStatic[sizePO2]->last();
            mFreeStatic[sizePO2]->dec();
            return ebo;
        }
    }
    else
    {
        if (mFreeDynamic[sizePO2]->size() > 0)
        {
            EBO *ebo = mFreeDynamic[sizePO2]->last();
            mFreeDynamic[sizePO2]->dec();
            return ebo;
        }
    }
    return mAlloc.inc() = new EBO(size, staticStorage);
}

void EBOPool::refund(EBO *ebo)
{
    if (ebo->isStatic())
        mFreeStatic[ebo->sizePO2()]->inc() = ebo;
    else
        mFreeDynamic[ebo->sizePO2()]->inc() = ebo;
}

}