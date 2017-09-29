#ifndef _PVRTGLOBAL_H_
#define _PVRTGLOBAL_H_

#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG	0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG	0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG	0x8C03
#define GL_ETC1_RGB8_OES 0x8D64

#define PVRTSIZEASSERT(T, size) typedef int (sizeof_##T)[sizeof(T) == (size)]

typedef char				PVRTchar8;
typedef signed char			PVRTint8;
typedef signed short		PVRTint16;
typedef signed int			PVRTint32;
typedef unsigned char		PVRTuint8;
typedef unsigned short		PVRTuint16;
typedef unsigned int		PVRTuint32;

typedef float				PVRTfloat32;

#if (defined(__int64) || defined(_WIN32))
typedef signed __int64     PVRTint64;
typedef unsigned __int64   PVRTuint64;
#elif defined(__GNUC__)
__extension__ typedef signed long long PVRTint64;
__extension__ typedef unsigned long long PVRTuint64;
#else
typedef signed long long   PVRTint64;
typedef unsigned long long PVRTuint64;
#endif

PVRTSIZEASSERT(PVRTuint64, 8);

#endif //_PVRTGLOBAL_H_