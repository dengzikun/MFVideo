// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 MFVIDEOCAPTURE_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MFVIDEOCAPTURE_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#ifndef MF_VIDEO_CAPTURE_H__
#define MF_VIDEO_CAPTURE_H__

#ifdef MFVIDEOCAPTURE_EXPORTS
#define MFVIDEOCAPTURE_API __declspec(dllexport)
#else
#define MFVIDEOCAPTURE_API __declspec(dllimport)
#endif

// 此类是从 MFVideoCapture.dll 导出的
class MFVIDEOCAPTURE_API CMFVideoCapture
{
public:
#pragma pack(push,8)
	typedef struct 
	{
		DWORD Numerator   ;
		DWORD Denominator ;
	} RATIO ;

	typedef struct
	{
		char  ColorSpace[256] ;
		DWORD Fcc ;
		UINT  Width   ;
		UINT  Height  ;
		RATIO FrameRate ;
		RATIO FrameRateMax ;
		RATIO FrameRateMin ;
		RATIO PAR ; 

	} VIDEO_FORMAT ;
#pragma pack(pop,8)

public:
	CMFVideoCapture(HRESULT &hr);
	virtual ~CMFVideoCapture(void);

public:
	HRESULT GetDeviceCount(DWORD &Count) ;
	HRESULT EnumDevice ( DWORD Idx, char *DeviceId, char *DeviceName ) ;
	HRESULT GetCurrentDevice ( char *DeviceId, char *DeviceName ) ;
	HRESULT GetFormatCount(const char *DeviceId, DWORD &Count);
	HRESULT EnumFormat ( const char *DeviceId, DWORD Idx, VIDEO_FORMAT &VideoFormat ) ;
	HRESULT GetCurrentFormat ( VIDEO_FORMAT &VideoFormat ) ;
	HRESULT OpenDevice ( const char *DeviceId ) ;
	HRESULT CloseDevice () ;
	HRESULT SetFormat ( DWORD Idx, RATIO &FrameRate ) ;

	HRESULT Start () ;
	HRESULT Stop  () ;

public:
	virtual void OnData ( BYTE *pData, DWORD DataSize, long lStride, DWORD ts ) = 0 ;
	virtual void OnMessage(const char *Msg ) = 0 ;

private:
	class Impl ;
	Impl *pImpl ;
};

/////////////////////////////////////////////////////////////////////////////////////////////////

#include "windows.h"

extern "C"
{
	DECLARE_HANDLE(HMFVIDEODCAPTURE);

#pragma pack(push,8)

	typedef struct
	{
		BYTE  *data;
		DWORD dataSize;
		long  stride;
		DWORD timeStamp;
		void *userData;
	} MF_VIDEOCAPTURE_OUTPUT;

	// callback
	typedef int(*MF_VIDEOCAPTURE_OUTPUTCALLBACK_PTR)(MF_VIDEOCAPTURE_OUTPUT *output);
	typedef int(*MF_VIDEOCAPTURE_MSGCALLBACK_PTR)(const char *msg);

	typedef struct
	{
		MF_VIDEOCAPTURE_OUTPUTCALLBACK_PTR dataCallBack;
		MF_VIDEOCAPTURE_MSGCALLBACK_PTR    msgCallBack; // 可为NULL.
		void *userData;
	} MF_VIDEOCAPTURE_INIT;

#pragma pack(pop,8)

	
	MFVIDEOCAPTURE_API HMFVIDEODCAPTURE MFVideoCapture_Create(MF_VIDEOCAPTURE_INIT *params);
	MFVIDEOCAPTURE_API void MFVideoCapture_Destroy(HMFVIDEODCAPTURE capture);

	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetDeviceCount(HMFVIDEODCAPTURE capture, DWORD *Count);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_EnumDevice(HMFVIDEODCAPTURE capture, DWORD Idx, char *DeviceId, char *DeviceName);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetCurrentDevice(HMFVIDEODCAPTURE capture, char *DeviceId, char *DeviceName);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetFormatCount(HMFVIDEODCAPTURE capture, const char *DeviceId, DWORD *Count);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_EnumFormat(HMFVIDEODCAPTURE capture, const char *DeviceId, DWORD Idx, CMFVideoCapture::VIDEO_FORMAT *VideoFormat);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetCurrentFormat(HMFVIDEODCAPTURE capture, CMFVideoCapture::VIDEO_FORMAT *VideoFormat);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_OpenDevice(HMFVIDEODCAPTURE capture, const char *DeviceId);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_CloseDevice(HMFVIDEODCAPTURE capture);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_SetFormat(HMFVIDEODCAPTURE capture, DWORD Idx, CMFVideoCapture::RATIO *FrameRate);

	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_Start(HMFVIDEODCAPTURE capture);
	MFVIDEOCAPTURE_API HRESULT MFVideoCapture_Stop(HMFVIDEODCAPTURE capture);
}

#endif