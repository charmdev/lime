#include "NevoMemory.h"

#include <memory.h>

namespace nevo
{

static int gMinSize = 1024;
static int gMinSizePower = 10;
static int gMaxSize = 16777216;
static int gMaxSizePower = 24;
static int sizePO2(int bytes, int *power)
{
    static int bytes_po2;
    bytes_po2 = gMinSize;
    *power = gMinSizePower;
    while ((bytes > bytes_po2) && (bytes_po2 < gMaxSize))
    {
        bytes_po2 = bytes_po2 << 1;
        ++(*power);
    }
    return bytes_po2;
}

//Memory
Memory::Memory(int size)
{
    mSize = nevo::sizePO2(size, &mSizePO2);
    mPtr = (void*)malloc(mSize);
}

Memory::~Memory()
{
    free(mPtr);
}

void* Memory::ptr()
{
    return mPtr;
}

void Memory::update(int offset, int size, void *data)
{
    memcpy((char*)mPtr + offset, data, size);
}

int Memory::size()
{
    return mSize;
}

int Memory::sizePO2()
{
    return mSizePO2;
}

//MemoryPool
MemoryPool::MemoryPool()
{
    mFree.resize(gMaxSizePower);
    for (int i = 0; i < gMaxSizePower; ++i)
        mFree[i] = new Vec<Memory*>();
}

MemoryPool::~MemoryPool()
{
    for (int i = 0; i < mAlloc.size(); ++i)
        delete mAlloc[i];
    for (int i = 0; i < gMaxSizePower; ++i)
        delete mFree[i];
}

Memory* MemoryPool::get(int size)
{
    if (size > gMaxSize)
        return 0;
    
    int sizePO2;
    int sizePO2bytes = nevo::sizePO2(size, &sizePO2);

    if (mFree[sizePO2]->size() > 0)
    {
        Memory *m = mFree[sizePO2]->last();
        mFree[sizePO2]->dec();
        return m;
    }
    return mAlloc.inc() = new Memory(size);
}

void MemoryPool::refund(Memory *m)
{
    mFree[m->sizePO2()]->inc() = m;
}

MemoryPool gMemoryPool;

}