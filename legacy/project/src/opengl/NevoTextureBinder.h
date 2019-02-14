#ifndef NEVO_TEXTURE_BINDER_H
#define NEVO_TEXTURE_BINDER_H

#include "NevoVec.h"

namespace nevo
{

class TextureBinder
{
private:
    int mMaxCh;
    int mCurCh;
    Vec<bool> mChUse;
    Vec<int> mChTex;
    Vec<int> mTexCh;

    void incCurCh()
    {
        while (mChUse[mCurCh] && (mCurCh < mMaxCh))
            ++mCurCh;
    }

public:
    TextureBinder();
    ~TextureBinder();

    int numChannels();
    int bindTexture(int glTexId);
    void qickReset();
    void reset();
};

}

#endif