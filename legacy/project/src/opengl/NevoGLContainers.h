#ifndef NEVO_GL_CONTAINERS_H
#define NEVO_GL_CONTAINERS_H

#include "NevoVec.h"

namespace nevo
{

class VBO
{
public:
    VBO(int size);
    ~VBO();

    int size();
    int sizePO2();

    unsigned id();
    void update(int offset, int size, void *data);

private:
    unsigned mVBO;
    int mSize;
    int mSizePO2;
};

class VBOPool
{
public:
    VBOPool();
    ~VBOPool();

    VBO* get(int size);
    void refund(VBO *vbo);
private:
    Vec<VBO*> mAlloc;
    Vec<Vec<VBO*>*> mFree;
};

class EBO
{
public:
    EBO(int size);
    ~EBO();

    int size();
    int sizePO2();

    void update(int offset, int size, void *data);
    void draw(unsigned mode, int count, unsigned type, int offset);

private:
    unsigned mEBO;
    int mSize;
    int mSizePO2;
};

class EBOPool
{
public:
    EBOPool();
    ~EBOPool();

    EBO* get(int size);
    void refund(EBO *ebo);
private:
    Vec<EBO*> mAlloc;
    Vec<Vec<EBO*>*> mFree;
};

}

#endif