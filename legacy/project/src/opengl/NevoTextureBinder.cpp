#include "NevoTextureBinder.h"

#include "OGL.h"

using namespace nme;

namespace nevo
{

TextureBinder::TextureBinder()
{
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mMaxCh);

    mChTex.resize(mMaxCh);
    mChUse.resize(mMaxCh);
    for (int i = 0; i < mMaxCh; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glEnable(GL_TEXTURE_2D);
    }

    mTexCh.resize(1024);

    reset();
}

TextureBinder::~TextureBinder()
{

}

void TextureBinder::reset()
{
    mCurCh = 0;

    for (int i = 0; i < mMaxCh; ++i)
    {
        mChTex[i] = -1;
        mChUse[i] = false;
    }

    for (int i = 0; i < mTexCh.size(); ++i)
    {
        mTexCh[i] = -1;
    }
}

int TextureBinder::numChannels()
{
    return mMaxCh;
}

int TextureBinder::bindTexture(int glTexId)
{
    if (mCurCh < mMaxCh)
    {
        if (mTexCh[glTexId] != -1)
        {
            mChUse[mTexCh[glTexId]] = true;
        }
        else 
        {
            if (mChTex[mCurCh] != -1)
            {
                mTexCh[mChTex[mCurCh]] = -1;
            }
            mTexCh[glTexId] = mCurCh;
            mChTex[mCurCh] = glTexId;
            mChUse[mCurCh] = true;

            glActiveTexture(GL_TEXTURE0 + mCurCh);
            glBindTexture(GL_TEXTURE_2D, glTexId);
        }
        incCurCh();
        return mTexCh[glTexId];
    }

    return -1;
}

void TextureBinder::qickReset()
{
    mCurCh = 0;
    for (int i = 0; i < mMaxCh; ++i)
    {
        mChUse[i] = false;
    }
}

}