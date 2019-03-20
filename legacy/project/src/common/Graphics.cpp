#include <Graphics.h>
#include <Surface.h>
#include <Display.h>

namespace nme
{

void Graphics::OnChanged()
{
   mVersion++;
   if (mOwner && !(mOwner->mDirtyFlags & dirtExtent))
      mOwner->DirtyExtent();
}

// TODO: invlidate/cache extents (do for whole lot at once)

Graphics::Graphics(DisplayObject *inOwner,bool inInitRef) : Object(inInitRef)
{
   mRotation0 = 0;
   mCursor = UserPoint(0,0);
   mHardwareData = 0;
   mPathData = new GraphicsPath;
   mBuiltHardware = 0;
   mTileJob.mIsTileJob = true;
   mMeasuredJobs = 0;
   mVersion = 0;
   mOwner = inOwner;

#ifdef NEVO_RENDER
   mTileTexture = 0;
   mFillTexture = 0;
   mFillBGRA = 0xFFFFFFFF;
   mTileBlendMode = bmNormal;
#endif
}


Graphics::~Graphics()
{
   mOwner = 0;
   clear();
   mPathData->DecRef();
}


void Graphics::clear()
{
#ifdef NEVO_RENDER
   for (int i = 0; i < mNevoJobs.size(); ++i)
      nevo::gNevoJobsPool->refund(mNevoJobs[i]);
   mNevoJobs.resize(0);
   if (mTileTexture) mTileTexture->DecRef();
   if (mFillTexture) mFillTexture->DecRef();
   mTileTexture = 0;
   mFillTexture = 0;
   mFillBGRA = 0xFFFFFFFF;
   mTileBlendMode = bmNormal;
#else
   mFillJob.clear();
   mLineJob.clear();
   mTileJob.clear();

   // clear jobs
   for(int i=0;i<mJobs.size();i++)
      mJobs[i].clear();
   mJobs.resize(0);

   if (mHardwareData)
   {
      delete mHardwareData;
      mHardwareData = 0;
   }
   mPathData->clear();

   mExtent0 = Extent2DF();
   mRotation0 = 0;
   mBuiltHardware = 0;
   mMeasuredJobs = 0;
   mCursor = UserPoint(0,0);
   OnChanged();
#endif
}

int Graphics::Version() const
{
   int result = mVersion;
	for(int i=0;i<mJobs.size();i++)
		result += mJobs[i].Version();
	return result;
}

#define SIN45 0.70710678118654752440084436210485
#define TAN22 0.4142135623730950488016887242097

void Graphics::drawEllipse(float x, float y, float width, float height)
{
#ifndef NEVO_RENDER
   x += width/2;
   y += height/2;
   float w = width*0.5;
   float w_ = w*SIN45;
   float cw_ = w*TAN22;
   float h = height*0.5;
   float h_ = h*SIN45;
   float ch_ = h*TAN22;

   Flush();

   mPathData->moveTo(x+w,y);
   mPathData->curveTo(x+w,  y+ch_, x+w_, y+h_);
   mPathData->curveTo(x+cw_,y+h,   x,    y+h);
   mPathData->curveTo(x-cw_,y+h,   x-w_, y+h_);
   mPathData->curveTo(x-w,  y+ch_, x-w,  y);
   mPathData->curveTo(x-w,  y-ch_, x-w_, y-h_);
   mPathData->curveTo(x-cw_,y-h,   x,    y-h);
   mPathData->curveTo(x+cw_,y-h,   x+w_, y-h_);
   mPathData->curveTo(x+w,  y-ch_, x+w,  y);

   Flush();
   OnChanged();
#endif
}

void Graphics::drawCircle(float x,float y, float radius)
{
   drawEllipse(x,y,radius,radius);
}

void Graphics::drawRect(float x, float y, float width, float height)
{
#ifdef NEVO_RENDER
   nevo::Job *job = mNevoJobs.inc() = nevo::gNevoJobsPool->get();
   job->mtl(mFillTexture, mFillBGRA, mOwner->getBlendMode());
   job->rect(x, y, width, height);
#else
   Flush();
   moveTo(x,y);
   lineTo(x+width,y);
   lineTo(x+width,y+height);
   lineTo(x,y+height);
   lineTo(x,y);
   Flush();
   mVersion++;
#endif
}

/*

   < ------------ w ----->
      < -------- w_ ----->
       < ------ cw_ ----->
             < --- lw ---> 
        c   --------------+
         222 |            x
        2    |
       p
    c 1 ..   ry
     1    . 
     1     ..|
    | - rx --
    |
    |

*/

void Graphics::drawRoundRect(float x,float  y,float  width,float  height,float  rx,float  ry)
{
#ifndef NEVO_RENDER
   rx *= 0.5;
   ry *= 0.5;
   float w = width*0.5;
   x+=w;
   if (rx>w) rx = w;
   float lw = w - rx;
   float w_ = lw + rx*SIN45;
   float cw_ = lw + rx*TAN22;
   float h = height*0.5;
   y+=h;
   if (ry>h) ry = h;
   float lh = h - ry;
   float h_ = lh + ry*SIN45;
   float ch_ = lh + ry*TAN22;

   Flush();

   mPathData->moveTo(x+w,y+lh);
   mPathData->curveTo(x+w,  y+ch_, x+w_, y+h_);
   mPathData->curveTo(x+cw_,y+h,   x+lw,    y+h);
   mPathData->lineTo(x-lw,    y+h);
   mPathData->curveTo(x-cw_,y+h,   x-w_, y+h_);
   mPathData->curveTo(x-w,  y+ch_, x-w,  y+lh);
   mPathData->lineTo( x-w, y-lh);
   mPathData->curveTo(x-w,  y-ch_, x-w_, y-h_);
   mPathData->curveTo(x-cw_,y-h,   x-lw,    y-h);
   mPathData->lineTo(x+lw,    y-h);
   mPathData->curveTo(x+cw_,y-h,   x+w_, y-h_);
   mPathData->curveTo(x+w,  y-ch_, x+w,  y-lh);
   mPathData->lineTo(x+w,  y+lh);

   Flush();
   OnChanged();
#endif
}

void Graphics::drawPath(const QuickVec<uint8> &inCommands, const QuickVec<float> &inData,
           WindingRule inWinding )
{
#ifndef NEVO_RENDER
   int n = inCommands.size();
   if (n==0 || inData.size()<2)
      return;

   const UserPoint *point = (UserPoint *)&inData[0];
   const UserPoint *last =  point + inData.size()/2;

   if ( (mFillJob.mFill && mFillJob.mCommand0==mPathData->commands.size()) ||
        (mLineJob.mStroke && mLineJob.mCommand0==mPathData->commands.size()) )
     mPathData->initPosition(mCursor);

   for(int i=0;i<n && point<last;i++)
   {
      switch(inCommands[i])
      {
         case pcWideMoveTo:
            point++;
            if (point==last) break;
         case pcMoveTo:
            mPathData->moveTo(point->x,point->y);
            mCursor = *point++;
            break;

         case pcWideLineTo:
            point++;
            if (point==last) break;
         case pcLineTo:
            mPathData->lineTo(point->x,point->y);
            mCursor = *point++;
            break;

         case pcCurveTo:
            if (point+1==last) break;
            mPathData->curveTo(point->x,point->y,point[1].x,point[1].y);
            mCursor = point[1];
            point += 2;
      }
   }
   OnChanged();
#endif
}



void Graphics::drawGraphicsDatum(IGraphicsData *inData)
{
#ifdef NEVO_RENDER
   switch(inData->GetType())
   {
      case gdtBitmapFill:
         {
            GraphicsBitmapFill *bf = inData->AsBitmapFill();
            if (mFillTexture) mFillTexture->DecRef();
            mFillTexture = bf->bitmapData;
            if (mFillTexture) mFillTexture->IncRef();
            mFillBGRA = 0xFFFFFFFF;
         }
         break;

      case gdtSolidFill:
         {
            GraphicsSolidFill *sf = inData->AsSolidFill();
            if (mFillTexture) mFillTexture->DecRef();
            mFillTexture = 0;
            mFillBGRA = sf->mRGB.ival;
         }
         break;
   }
#else
   switch(inData->GetType())
   {
      case gdtUnknown: break;
      case gdtTrianglePath: break;
      case gdtPath:
         {
         GraphicsPath *path = inData->AsPath();
         drawPath(path->commands, path->data, path->winding);
         break;
         }
      case gdtEndFill:
         endFill();
         break;
      case gdtSolidFill:
      case gdtGradientFill:
      case gdtBitmapFill:
         {
         IGraphicsFill *fill = inData->AsIFill();
         if (fill->isSolidStyle())
         {
            Flush(false,true);
            endTiles();
            if (mFillJob.mFill)
               mFillJob.mFill->DecRef();
            mFillJob.mFill = fill;
            mFillJob.mFill->IncRef();
            if (mFillJob.mCommand0 == mPathData->commands.size())
               mPathData->initPosition(mCursor);
         }
         else if (mLineJob.mStroke)
         {
            Flush(true,false);
            mLineJob.mStroke = mLineJob.mStroke->CloneWithFill(fill);
         }
         }
         break;
      case gdtStroke:
         {
         Flush(true,false);
         if (mLineJob.mStroke)
         {
            mLineJob.mStroke->DecRef();
            mLineJob.mStroke = 0;
         }
         GraphicsStroke *stroke = inData->AsStroke();
         if (stroke->thickness>=0 && stroke->fill)
         {
            mLineJob.mStroke = stroke;
            mLineJob.mStroke->IncRef();
            if (mLineJob.mCommand0 == mPathData->commands.size())
               mPathData->initPosition(mCursor);
         }
         }
         break;

   }
   OnChanged();
#endif
}

void Graphics::drawGraphicsData(IGraphicsData **graphicsData,int inN)
{
#ifndef NEVO_RENDER
   for(int i=0;i<inN;i++)
      drawGraphicsDatum(graphicsData[i]);
   OnChanged();
#endif
}

void Graphics::beginFill(unsigned int color, float alpha)
{
#ifdef NEVO_RENDER
   if (mFillTexture) mFillTexture->DecRef();
   mFillTexture = 0;
   mFillBGRA = color;
   mFillBGRA |= ((unsigned int)(255 * alpha) << 24);
#else
   Flush(false,true,true);
   endTiles();
   if (mFillJob.mFill)
      mFillJob.mFill->DecRef();
   mFillJob.mFill = new GraphicsSolidFill(color,alpha);
   mFillJob.mFill->IncRef();
   if (mFillJob.mCommand0 == mPathData->commands.size())
      mPathData->initPosition(mCursor);
#endif
}

void Graphics::endFill()
{
#ifndef NEVO_RENDER
   Flush(true,true);
   if (mFillJob.mFill)
   {
      mFillJob.mFill->DecRef();
      mFillJob.mFill = 0;
   }
#endif
}

void Graphics::beginBitmapFill(Surface *bitmapData, const Matrix &inMatrix,
   bool inRepeat, bool inSmooth)
{
#ifdef NEVO_RENDER
   if (mFillTexture) mFillTexture->DecRef();
   mFillTexture = bitmapData;
   if (mFillTexture) mFillTexture->IncRef();
   mFillBGRA = 0xFFFFFFFF;
#else
   Flush(false,true,true);
   endTiles();
   if (mFillJob.mFill)
      mFillJob.mFill->DecRef();
   mFillJob.mFill = new GraphicsBitmapFill(bitmapData,inMatrix,inRepeat,inSmooth);
   mFillJob.mFill->IncRef();
   if (mFillJob.mCommand0 == mPathData->commands.size())
      mPathData->initPosition(mCursor);
#endif
}

void Graphics::endTiles()
{
#ifndef NEVO_RENDER
   if (mTileJob.mFill)
   {
      mTileJob.mFill->DecRef();
      mTileJob.mFill = 0;

      OnChanged();
   }
#endif
}

void Graphics::beginTiles(Surface *bitmapData,bool inSmooth,int inBlendMode)
{
#ifdef NEVO_RENDER
   if (mTileTexture) mTileTexture->DecRef();
   mTileTexture = bitmapData;
   if (mTileTexture) mTileTexture->IncRef();
   mTileBlendMode = inBlendMode;
#else
   endFill();
   lineStyle(-1);
   Flush();
   if (mTileJob.mFill)
      mTileJob.mFill->DecRef();
   mTileJob.mFill = new GraphicsBitmapFill(bitmapData,Matrix(),false,inSmooth);
   mTileJob.mFill->IncRef();
   mPathData->elementBlendMode(inBlendMode);
#endif
}

void Graphics::lineStyle(double thickness, unsigned int color, double alpha,
                  bool pixelHinting, StrokeScaleMode scaleMode,
                  StrokeCaps caps,
                  StrokeJoints joints, double miterLimit)
{
#ifndef NEVO_RENDER
   Flush(true,false,true);
   endTiles();
   if (mLineJob.mStroke)
   {
      mLineJob.mStroke->DecRef();
      mLineJob.mStroke = 0;
   }
   if (thickness>=0)
   {
      IGraphicsFill *solid = new GraphicsSolidFill(color,alpha);
      mLineJob.mStroke = new GraphicsStroke(solid,thickness,pixelHinting,
          scaleMode,caps,joints,miterLimit);
      mLineJob.mStroke->IncRef();
      if (mLineJob.mCommand0 == mPathData->commands.size())
         mPathData->initPosition(mCursor);
   }
#endif
}



void Graphics::lineTo(float x, float y)
{
#ifndef NEVO_RENDER
   if ( (mFillJob.mFill && mFillJob.mCommand0==mPathData->commands.size()) ||
        (mLineJob.mStroke && mLineJob.mCommand0==mPathData->commands.size()) )
     mPathData->initPosition(mCursor);

   mPathData->lineTo(x,y);
   mCursor = UserPoint(x,y);
   OnChanged();
#endif
}

void Graphics::moveTo(float x, float y)
{
#ifndef NEVO_RENDER
   mPathData->moveTo(x,y);
   mCursor = UserPoint(x,y);
   OnChanged();
#endif
}

void Graphics::curveTo(float cx, float cy, float x, float y)
{
#ifndef NEVO_RENDER
   if ( (mFillJob.mFill && mFillJob.mCommand0==mPathData->commands.size()) ||
        (mLineJob.mStroke && mLineJob.mCommand0==mPathData->commands.size()) )
     mPathData->initPosition(mCursor);

   if ( (fabs(mCursor.x-cx)<0.00001 && fabs(mCursor.y-cy)<0.00001) ||
        (fabs(x-cx)<0.00001 && fabs(y-cy)<0.00001)  )
   {
      mPathData->lineTo(x,y);
   }
   else
      mPathData->curveTo(cx,cy,x,y);
   mCursor = UserPoint(x,y);
   OnChanged();
#endif
}

void Graphics::arcTo(float cx, float cy, float x, float y)
{
#ifndef NEVO_RENDER
   if ( (mFillJob.mFill && mFillJob.mCommand0==mPathData->commands.size()) ||
        (mLineJob.mStroke && mLineJob.mCommand0==mPathData->commands.size()) )
     mPathData->initPosition(mCursor);

   mPathData->arcTo(cx,cy,x,y);
   mCursor = UserPoint(x,y);
   OnChanged();
#endif
}

void Graphics::tile(float x, float y, const Rect &inTileRect,float *inTrans,float *inRGBA)
{
#ifdef NEVO_RENDER
   nevo::Job *job = mNevoJobs.inc() = nevo::gNevoJobsPool->get();
   unsigned int bgra = 0xFFFFFFFF;
   if (inRGBA)
   {
      bgra = (unsigned int)(255 * inRGBA[2]);
      bgra |= ((unsigned int)(255 * inRGBA[1]) << 8);
      bgra |= ((unsigned int)(255 * inRGBA[0]) << 16);
      bgra |= ((unsigned int)(255 * inRGBA[3]) << 24);
   }
   job->mtl(mTileTexture, bgra, mTileBlendMode);
   job->tile(x, y, inTileRect.x, inTileRect.y, inTileRect.w, inTileRect.h, inTrans);
#else
   mPathData->tile(x,y,inTileRect,inTrans,inRGBA);
#endif
}


void Graphics::drawPoints(QuickVec<float> inXYs, QuickVec<int> inRGBAs, unsigned int inDefaultRGBA,
								  double inSize)
{
#ifndef NEVO_RENDER
   endFill();
   lineStyle(-1);
   Flush();

   GraphicsJob job;
   job.mCommand0 = mPathData->commands.size();
   job.mCommandCount = 1;
   job.mData0 = mPathData->data.size();
   job.mIsPointJob = true;
   mPathData->drawPoints(inXYs,inRGBAs);
   job.mDataCount = mPathData->data.size() - job.mData0;
   if (mPathData->commands[job.mCommand0]==pcPointsXY)
   {
      job.mFill = new GraphicsSolidFill(inDefaultRGBA&0xffffff,(inDefaultRGBA>>24)/255.0);
      job.mFill->IncRef();
   }
	if (inSize>0)
	{
		job.mStroke = new GraphicsStroke(0,inSize);
		job.mStroke->IncRef();
	}

   mJobs.push_back(job);
#endif
}

void Graphics::drawTriangles(const QuickVec<float> &inXYs,
            const QuickVec<int> &inIndices,
            const QuickVec<float> &inUVT, int inCull,
            const QuickVec<int> &inColours,
            int blendMode)
{
#ifndef NEVO_RENDER
	Flush( );
	
	if (!mFillJob.mFill)
	{
		beginFill (0, 0);
	}
	
	IGraphicsFill *fill = mFillJob.mFill;

   GraphicsTrianglePath *path = new GraphicsTrianglePath(inXYs,
           inIndices, inUVT, inCull, inColours, blendMode );
   GraphicsJob job;
   path->IncRef();

	if (!fill || !fill->AsBitmapFill())
		path->mUVT.resize(0);

   job.mFill = fill ? fill->IncRef() : 0;
   job.mStroke = mLineJob.mStroke ? mLineJob.mStroke->IncRef() : 0;
   job.mTriangles = path;

   mJobs.push_back(job);
#endif
}

#ifdef NEVO_RENDER
void Graphics::drawTrianglesNevo(int inXYs_n, float *inXYs,
            int inIndixes_n, short *inIndixes, int inUVT_n, float *inUVT,
            int inColours_n, int *inColours, int inCull, int blendMode)
{
   nevo::Job *job = mNevoJobs.inc() = nevo::gNevoJobsPool->get();
   job->mtl(mFillTexture, mFillBGRA, blendMode);
   job->triangles(inXYs_n, inXYs,
      inIndixes_n, inIndixes, inUVT_n, inUVT,
      inColours_n, inColours);
}
#endif

// This routine converts a list of "GraphicsPaths" (mItems) into a list
//  of LineData and SolidData.
// The items intermix fill-styles and line-stypes with move/draw/triangle
//  geometry data - this routine separates them out.

void Graphics::Flush(bool inLine, bool inFill, bool inTile)
{
#ifndef NEVO_RENDER
   int n = mPathData->commands.size();
   int d = mPathData->data.size();
   bool wasFilled = false;

   if (inTile)
   {
      if (mTileJob.mFill && mTileJob.mCommand0 <n)
      {
         mTileJob.mFill->IncRef();
         mTileJob.mDataCount = d-mTileJob.mData0;
         mTileJob.mCommandCount = n-mTileJob.mCommand0;
         mTileJob.mIsTileJob = true;
         mJobs.push_back(mTileJob);
      }
   }


   // Do fill first, so lines go over top.
   if (inFill)
   {
      if (mFillJob.mFill && mFillJob.mCommand0 <n)
      {
         mFillJob.mFill->IncRef();
         mFillJob.mCommandCount = n-mFillJob.mCommand0;
         mFillJob.mDataCount = d-mFillJob.mData0;
         wasFilled = true;

         // Move the fill job up the list so it is "below" lines that start at the same
         // (or later) data point
         int pos = mJobs.size()-1;
         while(pos>=0)
         {
            if (mJobs[pos].mData0 < mFillJob.mData0)
               break;
            pos--;
         }
         pos++;
         if (pos==mJobs.size())
         {
            mJobs.push_back(mFillJob);
         }
         else
         {
            mJobs.InsertAt(0,mFillJob);
         }
         mFillJob.mCommand0 = n;
         mFillJob.mData0 = d;
      }
   }


   if (inLine)
   {
      if (mLineJob.mStroke && mLineJob.mCommand0 <n-1)
      {
         mLineJob.mStroke->IncRef();

         // Add closing segment...
         if (wasFilled)
         {
            mPathData->closeLine(mLineJob.mCommand0,mLineJob.mData0);
            n = mPathData->commands.size();
            d = mPathData->data.size();
         }
         mLineJob.mCommandCount = n-mLineJob.mCommand0;
         mLineJob.mDataCount = d-mLineJob.mData0;
         mJobs.push_back(mLineJob);
      }
      mLineJob.mCommand0 = n;
      mLineJob.mData0 = d;
   }


   if (inTile)
   {
      mTileJob.mCommand0 = n;
      mTileJob.mData0 = d;
   }

   if (inFill)
   {
      mFillJob.mCommand0 = n;
      mFillJob.mData0 = d;
   }
#endif
}


Extent2DF Graphics::GetSoftwareExtent(const Transform &inTransform, bool inIncludeStroke)
{
#ifdef NEVO_RENDER
   static const Matrix *m;
   static float x, y;
   static nevo::Job *job;
   static int i, j;
   Extent2DF result;

   m = inTransform.mMatrix;
   for (i = 0; i < mNevoJobs.size(); ++i)
   {
      job = mNevoJobs[i];
      x = job->mBBminX; y = job->mBBminY;
      result.Add(m->m00 * x + m->m01 * y + m->mtx, m->m10 * x + m->m11 * y + m->mty);
      x = job->mBBmaxX; y = job->mBBminY;
      result.Add(m->m00 * x + m->m01 * y + m->mtx, m->m10 * x + m->m11 * y + m->mty);
      x = job->mBBmaxX; y = job->mBBmaxY;
      result.Add(m->m00 * x + m->m01 * y + m->mtx, m->m10 * x + m->m11 * y + m->mty);
      x = job->mBBminX; y = job->mBBmaxY;
      result.Add(m->m00 * x + m->m01 * y + m->mtx, m->m10 * x + m->m11 * y + m->mty);
   }

   return result;
#else
   Extent2DF result;
   Flush();

   for(int i=0;i<mJobs.size();i++)
   {
      GraphicsJob &job = mJobs[i];
      if (!job.mSoftwareRenderer)
         job.mSoftwareRenderer = Renderer::CreateSoftware(job,*mPathData);

      job.mSoftwareRenderer->GetExtent(inTransform,result,inIncludeStroke);
   }

   return result;
#endif
}

const Extent2DF &Graphics::GetExtent0(double inRotation)
{
   if ( mMeasuredJobs<mJobs.size() || inRotation!=mRotation0)
   {
      Transform trans;
      Matrix  m;
      trans.mMatrix = &m;
      if (inRotation)
         m.Rotate(inRotation);
      mExtent0 = GetSoftwareExtent(trans,true);
      mRotation0 = inRotation;
      mMeasuredJobs = mJobs.size();
   }
   return mExtent0;
}


bool Graphics::Render( const RenderTarget &inTarget, const RenderState &inState )
{
#ifdef NEVO_RENDER
   if (inTarget.IsHardware())
   {
      if (inState.mPhase==rpHitTest)
      {
         if (inState.mClipRect.w!=1 || inState.mClipRect.h!=1)
            return false;
         UserPoint screen(inState.mClipRect.x, inState.mClipRect.y);
         UserPoint pos = inState.mTransform.mMatrix->ApplyInverse(screen);
         for (int i = 0; i < mNevoJobs.size(); ++i)
         {
            if (mNevoJobs[i]->hitTest(pos.x, pos.y))
               return true;
         }
         return false;
      }
      else
      {
         nevo::gNevoRender.setJobs(&mNevoJobs);
         inTarget.mHardware->Render(inState,*mHardwareData);
      }
   }
#else
   Flush();
   
   #ifdef NME_DIRECTFB
   
   for(int i=0;i<mJobs.size();i++)
   {
      GraphicsJob &job = mJobs[i];
      
      if (!job.mHardwareRenderer /*&& !job.mSoftwareRenderer*/)
         job.mHardwareRenderer = Renderer::CreateHardware(job,*mPathData,*inTarget.mHardware);
      
      //if (!job.mSoftwareRenderer)
         //job.mSoftwareRenderer = Renderer::CreateSoftware(job,*mPathData);
      
      if (inState.mPhase==rpHitTest)
      {
         if (job.mHardwareRenderer && job.mSoftwareRenderer->Hits(inState))
         {
            return true;
         }
         /*else if (job.mSoftwareRenderer && job.mSoftwareRenderer->Hits(inState))
         {
            return true;
         }*/
      }
      else
      {
         if (job.mHardwareRenderer)
            job.mHardwareRenderer->Render(inTarget,inState);
         //else
            //job.mSoftwareRenderer->Render(inTarget,inState);
      }
   }
   
   #else
   
   if (inTarget.IsHardware())
   {
      if (!mHardwareData)
         mHardwareData = new HardwareData();
      else if (!mHardwareData->isScaleOk(inState))
      {
         mHardwareData->clear();
         mBuiltHardware = 0;
      }
      
      while(mBuiltHardware<mJobs.size())
      {
         BuildHardwareJob(mJobs[mBuiltHardware++],*mPathData,*mHardwareData,*inTarget.mHardware,inState);
      }
      
      if (mHardwareData && !mHardwareData->mElements.empty())
      {
         if (inState.mPhase==rpHitTest)
            return inTarget.mHardware->Hits(inState,*mHardwareData);
         else
            inTarget.mHardware->Render(inState,*mHardwareData);
      }
   }
   else
   {
      for(int i=0;i<mJobs.size();i++)
      {
         GraphicsJob &job = mJobs[i];
         if (!job.mSoftwareRenderer)
            job.mSoftwareRenderer = Renderer::CreateSoftware(job,*mPathData);

         if (inState.mPhase==rpHitTest)
         {
            if (job.mSoftwareRenderer->Hits(inState))
               return true;
         }
         else
            job.mSoftwareRenderer->Render(inTarget,inState);
      }
   }
   
   #endif

#endif //NEVO_RENDER

   return false;
}


// --- RenderState -------------------------------------------------------------------

void GraphicsJob::clear()
{
   if (mStroke) mStroke->DecRef();
   if (mFill) mFill->DecRef();
   if (mTriangles) mTriangles->DecRef();
   if (mSoftwareRenderer) mSoftwareRenderer->Destroy();
   bool was_tile = mIsTileJob;
   memset(this,0,sizeof(GraphicsJob));
   mIsTileJob = was_tile;
}

// --- RenderState -------------------------------------------------------------------

ColorTransform sgIdentityColourTransform;

RenderState::RenderState(Surface *inSurface,int inAA)
{
   mTransform.mAAFactor = inAA;
   mMask = 0;
   mPhase = rpRender;
   mAlpha_LUT = 0;
   mR_LUT = 0;
   mG_LUT = 0;
   mB_LUT = 0;
   mColourTransform = &sgIdentityColourTransform;
   mRoundSizeToPOW2 = false;
   mHitResult = 0;
   mRecurse = true;
   mTargetOffset = ImagePoint(0,0);
   mWasDirtyPtr = 0;
   if (inSurface)
   {
      mClipRect = Rect(inSurface->Width(),inSurface->Height());
   }
   else
      mClipRect = Rect(0,0);
}



void RenderState::CombineColourTransform(const RenderState &inState,
                                         const ColorTransform *inObjTrans,
                                         ColorTransform *inBuf)
{
   mAlpha_LUT = mColourTransform->IsIdentityAlpha() ? 0 : mColourTransform->GetAlphaLUT();
   if (inObjTrans->IsIdentity())
   {
      mColourTransform = inState.mColourTransform;
      mAlpha_LUT = inState.mAlpha_LUT;
      mR_LUT = inState.mR_LUT;
      mG_LUT = inState.mG_LUT;
      mB_LUT = inState.mB_LUT;
      return;
   }

   mColourTransform = inBuf;
   inBuf->Combine(*(inState.mColourTransform),*inObjTrans);

   if (mColourTransform->IsIdentityColour())
   {
      mR_LUT = 0;
      mG_LUT = 0;
      mB_LUT = 0;
   }
   else
   {
      mR_LUT = mColourTransform->GetRLUT();
      mG_LUT = mColourTransform->GetGLUT();
      mB_LUT = mColourTransform->GetBLUT();
   }

   if (mColourTransform->IsIdentityAlpha())
      mAlpha_LUT = 0;
   else
      mAlpha_LUT = mColourTransform->GetAlphaLUT();
}



// --- RenderTarget -------------------------------------------------------------------

RenderTarget::RenderTarget(const Rect &inRect,PixelFormat inFormat,uint8 *inPtr, int inStride)
{
   mRect = inRect;
   mPixelFormat = inFormat;
   mSoftPtr = inPtr;
   mSoftStride = inStride;
   mHardware = 0;
}

RenderTarget::RenderTarget(const Rect &inRect,HardwareRenderer *inHardware)
{
   mRect = inRect;
   mPixelFormat = pfHardware;
   mSoftPtr = 0;
   mSoftStride = 0;
   mHardware = inHardware;
}

RenderTarget::RenderTarget() : mRect(0,0)
{
   mPixelFormat = pfAlpha;
   mSoftPtr = 0;
   mSoftStride = 0;
   mHardware = 0;
}


RenderTarget RenderTarget::ClipRect(const Rect &inRect) const
{
   RenderTarget result = *this;
   result.mRect = result.mRect.Intersect(inRect);
   return result;
}

void RenderTarget::Clear(uint32 inColour, const Rect &inRect) const
{
   if (IsHardware())
   {
      mHardware->Clear(inColour,&inRect);
      return;
   }

   if (mPixelFormat==pfAlpha)
   {
      int val = inColour>>24;
      for(int y=inRect.y;y<inRect.y1();y++)
      {
         uint8 *alpha = (uint8 *)Row(y) + inRect.x;
         memset(alpha,val,inRect.w);
      }
   }
   else
   {
      ARGB rgb(inColour);
      if (!(mPixelFormat & pfHasAlpha))
         rgb.a = 255;

      for(int y=inRect.y;y<inRect.y1();y++)
      {
         int *ptr = (int *)Row(y) + inRect.x;
         for(int x=0;x<inRect.w;x++)
            *ptr++ = rgb.ival;
      }
 
   }
}




// --- GraphicsBitmapFill -------------------------------------------------------------------

GraphicsBitmapFill::GraphicsBitmapFill(Surface *inBitmapData,
      const Matrix &inMatrix, bool inRepeat, bool inSmooth) : bitmapData(inBitmapData),
         matrix(inMatrix),  repeat(inRepeat), smooth(inSmooth)
{
   if (bitmapData)
      bitmapData->IncRef();
}

GraphicsBitmapFill::~GraphicsBitmapFill()
{
   if (bitmapData)
      bitmapData->DecRef();
}

int GraphicsBitmapFill::Version() const
{
	return bitmapData->Version();
}
// --- GraphicsStroke -------------------------------------------------------------------

GraphicsStroke::GraphicsStroke(IGraphicsFill *inFill, double inThickness,
                  bool inPixelHinting, StrokeScaleMode inScaleMode,
                  StrokeCaps inCaps,
                  StrokeJoints inJoints, double inMiterLimit)
      : fill(inFill), thickness(inThickness), pixelHinting(inPixelHinting),
        scaleMode(inScaleMode), caps(inCaps), joints(inJoints), miterLimit(inMiterLimit)
   {
      if (fill)
         fill->IncRef();
   }


GraphicsStroke::~GraphicsStroke()
{
   if (fill)
      fill->DecRef();
}

GraphicsStroke *GraphicsStroke::CloneWithFill(IGraphicsFill *inFill)
{
   if (mRefCount < 2)
   {
      inFill->IncRef();
      if (fill)
         fill->DecRef();
      fill = inFill;
      return this;
   }

   GraphicsStroke *clone = new GraphicsStroke(inFill,thickness,pixelHinting,scaleMode,caps,joints,miterLimit);
   DecRef();
   clone->IncRef();
   return clone;
}



// --- Gradient ---------------------------------------------------------------------


static void GetLinearLookups(int **outToLinear, int **outFromLinear)
{
   static int *to = 0;
   static int *from = 0;

   if (!to)
   {
      double a = 0.055;
      to = new int[256];
      from = new int[4096];

      for(int i=0;i<4096;i++)
      {
         double t = i / 4095.0;
         from[i] = 255.0 * (t<=0.0031308 ? t*12.92 : (a+1)*pow(t,1/2.4)-a) + 0.5;
      }

      for(int i=0;i<256;i++)
      {
         double t = i / 255.0;
         to[i] = 4095.0 * ( t<=0.04045 ? t/12.92 : pow( (t+a)/(1+a), 2.4 ) ) + 0.5;
      }
   }

   *outToLinear = to;
   *outFromLinear = from;
}


void GraphicsGradientFill::FillArray(ARGB *outColours)
{
   int *ToLinear = 0;
   int *FromLinear = 0;

   if (interpolationMethod==imLinearRGB)
      GetLinearLookups(&ToLinear,&FromLinear);
    
   bool reflect = spreadMethod==smReflect;
   int n = mStops.size();
   if (n==0)
      memset(outColours,0,sizeof(ARGB)*(reflect?512:256));
   else
   {
      int i;
      int last = mStops[0].mPos;
      if (last>255) last = 255;

      for(i=0;i<=last;i++)
         outColours[i] = mStops[0].mARGB;
      for(int k=0;k<n-1;k++)
      {
         ARGB c0 = mStops[k].mARGB;
         int p0 = mStops[k].mPos;
         int p1 = mStops[k+1].mPos;
         int diff = p1 - p0;
         if (diff>0)
         {
            if (p0<0) p0 = 0;
            if (p1>256) p1 = 256;

            int da = mStops[k+1].mARGB.a - c0.a;
            if (ToLinear)
            {
               int dr = ToLinear[mStops[k+1].mARGB.r] - ToLinear[c0.r];
               int dg = ToLinear[mStops[k+1].mARGB.g] - ToLinear[c0.g];
               int db = ToLinear[mStops[k+1].mARGB.b] - ToLinear[c0.b];
               for(i=p0;i<p1;i++)
               {
                  outColours[i].r= FromLinear[ ToLinear[c0.r] + dr*(i-p0)/diff];
                  outColours[i].g= FromLinear[ ToLinear[c0.g] + dg*(i-p0)/diff];
                  outColours[i].b= FromLinear[ ToLinear[c0.b] + db*(i-p0)/diff];
                  outColours[i].a = FromLinear[ ToLinear[c0.a] + da*(i-p0)/diff];
               }
            }
            else
            {
               int dr = mStops[k+1].mARGB.r - c0.r;
               int dg = mStops[k+1].mARGB.g - c0.g;
               int db = mStops[k+1].mARGB.b - c0.b;
               for(i=p0;i<p1;i++)
               {
                  outColours[i].r = c0.r + dr*(i-p0)/diff;
                  outColours[i].g = c0.g + dg*(i-p0)/diff;
                  outColours[i].b = c0.b + db*(i-p0)/diff;
                  outColours[i].a = c0.a + da*(i-p0)/diff;
               }
            }
         }
      }
      for(;i<256;i++)
         outColours[i] = mStops[n-1].mARGB;

      if (reflect)
      {
         for(;i<512;i++)
            outColours[i] = outColours[511-i];
      }
   }
}

// --- Helper ----------------------------------------

int UpToPower2(int inX)
{
   int result = 1;
   while(result<inX) result<<=1;
   return result;
}

}
