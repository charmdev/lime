#ifndef NEVO_MEMORY_H
#define NEVO_MEMORY_H

#include "NevoVec.h"

namespace nevo
{

class Memory
{
public:
    Memory(int size);
    ~Memory();

    void* ptr();
    void update(int offset, int size, void *data);
    int size();
    int sizePO2();

private:
    void *mPtr;
    int mSize;
    int mSizePO2;
};

class MemoryPool
{
public:
    MemoryPool();
    ~MemoryPool();

    Memory* get(int size);
    void refund(Memory *m);

private:
    Vec<Memory*> mAlloc;
    Vec<Vec<Memory*>*> mFree;
};

extern MemoryPool gMemoryPool;

}

#endif