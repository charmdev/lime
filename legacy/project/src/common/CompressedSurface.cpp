#include <Surface.h>
#include "../opengl/OGL.h"

void PVRTGetOGLES2TextureFormat(const PVRTextureHeaderV3& sTextureHeader, PVRTuint32& internalformat)
{
	//Get the last 32 bits of the pixel format.
	//Check for a compressed format (The first 8 bytes will be 0, so the whole thing will be equal to the last 32 bits).
	//Format and type == 0 for compressed textures.

	internalformat = 0;
	EPVRTColourSpace ColourSpace = (EPVRTColourSpace)sTextureHeader.u32ColourSpace;
	PVRTuint64 PixelFormat = sTextureHeader.u64PixelFormat;
	PVRTuint64 PixelFormatPartHigh = PixelFormat & PVRTEX_PFHIGHMASK;

	if (PixelFormatPartHigh == 0)
	{
		
		switch (PixelFormat)
		{
		case ePVRTPF_PVRTCI_2bpp_RGB:
			{	
				return;
			}
		case ePVRTPF_PVRTCI_2bpp_RGBA:
			{
				if (ColourSpace == ePVRTCSpacesRGB) 
				{
					internalformat = GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT;
				}	
				else 
				{
					internalformat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
				}

				return;
			}
		case ePVRTPF_PVRTCI_4bpp_RGB:
			{
				internalformat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
				return;
			}
		case ePVRTPF_PVRTCI_4bpp_RGBA:
			{
				internalformat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
				return;
			}
		case ePVRTPF_PVRTCII_2bpp:
			{
				return;
			}
		case ePVRTPF_PVRTCII_4bpp:
			{
				return;
			}
		case ePVRTPF_ETC1:
			{
				internalformat = GL_ETC1_RGB8_OES;
				return;
			}
		default:
			return;
		}
	}	
	
}

void PVRTGetTextureDataSize(PVRTextureHeaderV3& sTextureHeader, PVRTuint32& compressedSize)
{
	PVRTuint32 uiSmallestWidth=0;
	PVRTuint32 uiSmallestHeight=0;
	PVRTuint32 uiSmallestDepth=0;

	PVRTuint32 bpp=0; //bits per pixel

	switch((EPVRTPixelFormat)sTextureHeader.u64PixelFormat)
	{
		case ePVRTPF_PVRTCI_2bpp_RGBA:
			uiSmallestWidth=16;
			uiSmallestHeight=8;
			uiSmallestDepth=1;
			bpp=2;
			break;
			
		case ePVRTPF_PVRTCI_2bpp_RGB:
			uiSmallestWidth=16;
			uiSmallestHeight=8;
			uiSmallestDepth=1;
			bpp=2;
			break;

		case ePVRTPF_ETC1:
			uiSmallestWidth=4;
			uiSmallestHeight=4;
			uiSmallestDepth=1;
			bpp=4;
			break;
			
		case ePVRTPF_PVRTCI_4bpp_RGB:
			uiSmallestWidth=8;
			uiSmallestHeight=8;
			uiSmallestDepth=1;
			bpp=4;
			break;
			
		case ePVRTPF_PVRTCI_4bpp_RGBA:
			uiSmallestWidth=8;
			uiSmallestHeight=8;
			uiSmallestDepth=1;
			bpp=4;
			break;
	}
	
	PVRTuint32 uiWidth = sTextureHeader.u32Width;
	PVRTuint32 uiHeight = sTextureHeader.u32Height;
	PVRTuint32 uiDepth = sTextureHeader.u32Depth;

	uiWidth = uiWidth + (( -1 * uiWidth) % uiSmallestWidth);
	uiHeight = uiHeight + (( -1 * uiHeight) % uiSmallestHeight);
	uiDepth = uiDepth + (( -1 * uiDepth) % uiSmallestDepth);

	PVRTuint64 uiDataSize = bpp * uiWidth * uiHeight * uiDepth;
	
	compressedSize = (PVRTuint32) (uiDataSize / 8);
}

void Configure(PVRTextureHeaderV3& sTextureHeader, PVRTuint32& internalFormat, PVRTuint32& compressedSize)
{
	GLenum eTextureInternalFormat = 0;
	PVRTGetOGLES2TextureFormat(sTextureHeader, internalFormat);
	PVRTGetTextureDataSize(sTextureHeader, compressedSize);
}

namespace nme
{
	Surface *Surface::LoadCompressed(const OSChar *inFilename)
	{
		ByteArray bytes;
		
//#ifdef ANDROID		
//		bytes = AndroidGetAssetBytes(inFilename);
//#else
		bytes = ByteArray::FromFile(inFilename);
//#endif
	   
	    if (bytes.Ok())
	    {
	    	uint8 *data = bytes.Bytes();
	    	int dataSize = bytes.Size();
	    	
	    	if(!data || !dataSize)
	    		return 0;

	    	//prepare texture header
			PVRTextureHeaderV3 sTextureHeader =* (PVRTextureHeaderV3*) data;
			data = (PVRTuint8*)data + PVRTEX3_HEADERSIZE + sTextureHeader.u32MetaDataSize;

	    	CompressedSurface *result = new CompressedSurface(sTextureHeader.u32Width, sTextureHeader.u32Height);
	    	result -> IncRef();

	    	result -> AttachData(sTextureHeader, data);
	        return result;
	    }

	   return 0;
	}

	Surface *Surface::LoadCompressed(const OSChar *inFilename, const OSChar *inAlphaname)
	{

		//__android_log_print(ANDROID_LOG_VERBOSE, "CompressedSurface", "image name: %s", inFilename);
		//__android_log_print(ANDROID_LOG_VERBOSE, "CompressedSurface", "alpha name: %s", inAlphaname);

		/*std::string path(inFilename);
		int pos = path.find_first_of(":");
		if(pos != std::string::npos)
		{
			std::string obb(path.substr(0, pos));
			__android_log_print(ANDROID_LOG_VERBOSE, "CompressedSurface", "path file name: %s", obb.c_str());

			int err_code = 0;
			zip *za = zip_open(obb.c_str(), 0, &err_code);

			//uint8* buff = (uint8*)gzopen(obb.c_str(), "rb");
			switch(err_code)
			{
				case 21: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "Zip archive inconsistent"); break;
				case 18: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "Invalid argument"); break;
				case 9: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "No such file"); break;
				case 14: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "Malloc failure"); break;
				case 19: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "Not a zip archive"); break;
				case 11: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "Can't open file "); break;
				case 5: __android_log_write(ANDROID_LOG_VERBOSE, "CompressedSurface", "Read error"); break;
				case 0:

					const char *fname = path.c_str() + pos + 1;
					__android_log_print(ANDROID_LOG_VERBOSE, "CompressedSurface", "file name: %s", fname);

					zip_file *zfile = zip_fopen(za, fname, 0);
					if (zfile != 0)
					{
						__android_log_print(ANDROID_LOG_VERBOSE, "CompressedSurface", "file bytes left to read: %i", zfile -> bytes_left);
						__android_log_print(ANDROID_LOG_VERBOSE, "CompressedSurface", "file compressed bytes left to read: %i", zfile -> bytes_left);
					}

				break;
			}

			return 0;
		}*/

	    ByteArray bytes;
		ByteArray bytes_alpha;
		
//#ifdef ANDROID		
//		bytes = AndroidGetAssetBytes(inFilename);
//	    bytes_alpha = AndroidGetAssetBytes(inAlphaname);
//#else
		bytes = ByteArray::FromFile(inFilename);
	    bytes_alpha = ByteArray::FromFile(inAlphaname);
//#endif

	    if (bytes.Ok() && bytes_alpha.Ok())
	    {
	    	uint8 *data = bytes.Bytes();
	    	uint8 *alpha = bytes_alpha.Bytes();

	    	int dataSize = bytes.Size();
	    	int alphaSize = bytes_alpha.Size();

	    	if(!data || !dataSize || !alpha || !alphaSize)
	    		return 0;

	    	//prepare texture header
			PVRTextureHeaderV3 sTextureHeader =* (PVRTextureHeaderV3*) data;
			data = (PVRTuint8*)data + PVRTEX3_HEADERSIZE + sTextureHeader.u32MetaDataSize;

			PVRTextureHeaderV3 sAlphaHeader =* (PVRTextureHeaderV3*) alpha;
			alpha = (PVRTuint8*)alpha + PVRTEX3_HEADERSIZE + sAlphaHeader.u32MetaDataSize;

	    	CompressedSurface *result = new CompressedSurface(sTextureHeader.u32Width, sTextureHeader.u32Height);
	    	result -> IncRef();

	    	result -> AttachData(sTextureHeader, data);
			result -> AttachAlpha(sAlphaHeader, alpha);

	        return result;
	    }

	   return 0;
	}
	
	void CompressedSurface::AttachData(PVRTextureHeaderV3& sTextureHeader, uint8* data)
	{
		Configure(sTextureHeader, (PVRTuint32&) mDataFormat, (PVRTuint32&) mDataSize);
		mData = new unsigned char[mDataSize];
	    memcpy(mData, data, mDataSize);
	}

	void CompressedSurface::AttachAlpha(PVRTextureHeaderV3& sAlphaHeader, uint8* alpha)
	{
		Configure(sAlphaHeader, (PVRTuint32&) mAlphaFormat, (PVRTuint32&) mAlphaSize);
		mAlpha = new unsigned char[mAlphaSize];
	    memcpy(mAlpha, alpha, mAlphaSize);

	    mSepAlpha = true;
	}

	CompressedSurface::CompressedSurface(int inWidth, int inHeight)
	{
		mWidth = inWidth;
   		mHeight = inHeight;

   		mData = NULL;
   		mAlpha = NULL;

   		mDataSize = 0;
   		mAlphaSize = 0;

   		mDataFormat = 0;
   		mAlphaFormat = 0;

   		mCompressed = true;
   		mSepAlpha = false;
	}

	CompressedSurface::~CompressedSurface()
	{
  		ClearBuffs();
	}

	void CompressedSurface::dispose()
	{
		if (mTexture)
		{
			delete mTexture;
			mTexture = 0;
		}

		ClearBuffs();
	}

	void CompressedSurface::MakeTextureOnly()
	{ 
		if (mData)
			GetTexture( nme::HardwareRenderer::current );

		ClearBuffs();
	}

	void CompressedSurface::ClearBuffs()
	{
		if (mData)
		{
			delete [] mData;
			mData = NULL;
		}

		if (mAlpha)
		{
			delete [] mAlpha;
			mAlpha = NULL;
		}
	}
}