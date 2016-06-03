// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 FF_CODEC_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// FF_CODEC_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#ifndef FF_CODEC_H__
#define FF_CODEC_H__

#ifdef FF_CODEC_EXPORTS
#define FF_CODEC_API __declspec(dllexport)
#else
#define FF_CODEC_API __declspec(dllimport)
#endif

#include "windows.h"

extern "C"
{
	DECLARE_HANDLE(HFFVIDEODECODER);

#pragma pack(push,8)
	typedef struct{
		HFFVIDEODECODER hDecoder; // in
		BYTE *pBitStream; // in
		DWORD bitStreamLen; // in
		DWORD corrupted;
	} FFVIDEODECODER_INPUT;

	typedef struct{
		void *userData;
		BYTE *yuv[3];       // out
		DWORD stride[3];    // out
		DWORD width; // out
		DWORD height; // out
	} FFVIDEODECODER_OUTPUT;
#pragma pack(pop,8)

	typedef int(*FF_OUTPUTCALLBACK_PTR)(FFVIDEODECODER_OUTPUT *output);

	// decoder_id: 
	// 28     : H264
	// 'H265' : H265
	// 8 : MJPEG
	FF_CODEC_API HFFVIDEODECODER FFVideoDecoder_Create(DWORD decoder_id, DWORD thread_num, FF_OUTPUTCALLBACK_PTR callBack, void *userData);
	FF_CODEC_API void FFVideoDecoder_Destroy(HFFVIDEODECODER hDecoder);

	// return 0 succeeded.
	FF_CODEC_API int FFVideoDecoder_Decode(FFVIDEODECODER_INPUT *input);
}

#endif