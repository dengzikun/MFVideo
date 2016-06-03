// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� FF_CODEC_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// FF_CODEC_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�

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