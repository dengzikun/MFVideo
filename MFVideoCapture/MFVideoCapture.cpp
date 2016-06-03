// MFVideoCapture.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include "tchar.h"
#include "comdef.h"

#include "MFVideoCapture.h"

#include <vector>
#include <string>

#include "bufferlock.h"

#include "initguid.h"
#include "uuids.h"

#include <mutex>

using namespace std;

#pragma comment ( lib, "mfplat.lib" )
#pragma comment ( lib, "mf.lib" )
#pragma comment ( lib, "mfreadwrite.lib" )
#pragma comment ( lib, "mfuuid.lib" )
#pragma comment ( lib, "shlwapi.lib" )

HRESULT LogMediaType(IMFMediaType *pType);
HRESULT GetDefaultStride(IMFMediaType *pType, LONG *plStride) ;
const wchar_t* GetMediaSubtype(const GUID& guid, DWORD &fcc) ;

BYTE buffer[1920 * 1080];

class CMFVideoCapture::Impl : public IMFSourceReaderCallback{
	enum{
		DEVICE_PATH = 0,
		DEVICE_NAME = 1,
		STR_COUNT = 2
	} ;

	class MEDIA_TYPE{
	public:
		MEDIA_TYPE(IMFMediaType *p) :
		  pType(p){
			  pType->AddRef () ;
		  }

		  MEDIA_TYPE(const MEDIA_TYPE& f){
			  pType = f.pType ;
			  pType->AddRef() ;
		  }

		  ~MEDIA_TYPE(){
			  SafeRelease(&pType);
		  }

	public:
		IMFMediaType *pType ;
	} ;

	class DEVICE_INFO{
	public:
		std::wstring DeviceStr[STR_COUNT] ;
		std::vector<MEDIA_TYPE> vecMediaType ;
		IMFActivate *pDevice ;

	public:
		DEVICE_INFO() : 
			pDevice(nullptr){
		  }

		  ~DEVICE_INFO(){
			  SafeRelease(&pDevice);
		  }
	} ;

public:

	Impl(CMFVideoCapture *pOwner) :
		  pOwner_(pOwner),
		  m_nRefCount(1),
		  m_dwCurrentIdx(0),
		  m_pReader(nullptr),
		  m_lDefaultStride(0),
		  m_Height(0)
	  {
		  DWORD Count ;
		  get_DeviceCount( Count ) ;
	  }

	  ~Impl(void)
	  {
		  CloseDevice() ;
		  m_vecDevInfo.clear() ;
	  }

	  // IUnknown methods
	  STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	  {
		  static const QITAB qit[] = 
		  {
			  QITABENT(CMFVideoCapture::Impl, IMFSourceReaderCallback),
			  { 0 },
		  };
		  return QISearch(this, qit, riid, ppv);
	  }

	  STDMETHODIMP_(ULONG) AddRef()
	  {
		  return InterlockedIncrement(&m_nRefCount);
	  }

	  STDMETHODIMP_(ULONG) Release()
	  {
		  ULONG uCount = InterlockedDecrement(&m_nRefCount);
		  if (uCount == 0)
		  {
			  delete this;
		  }
		  // For thread safety, return a temporary variable.
		  return uCount;
	  }

	  // IMFSourceReaderCallback methods
	  STDMETHODIMP OnReadSample(
		  HRESULT hrStatus,
		  DWORD dwStreamIndex,
		  DWORD dwStreamFlags,
		  LONGLONG llTimestamp,
		  IMFSample *pSample
		  )
	  {
		  lock_guard<recursive_mutex> lock(_mtx);

		  HRESULT hr = hrStatus ;
		  if ( FAILED(hr) ) {
			  _com_error err(hr) ;
			  pOwner_->OnMessage(err.ErrorMessage()) ;
			  return hr ;
		  }

		  if (pSample != nullptr){
			  DWORD DataSize = 0;
			  pSample->GetTotalLength(&DataSize);
			  CComPtr<IMFMediaBuffer> pBuffer ;
			  hr = pSample->GetBufferByIndex(0, &pBuffer);
//			  hr = pSample->ConvertToContiguousBuffer(&pBuffer);
			  if (SUCCEEDED(hr)){
				  
				  BYTE *pbScanline0 ;
				  LONG lStride ;
				  VideoBufferLock buffer(pBuffer);
				  hr = buffer.LockBuffer(m_lDefaultStride, m_Height, &pbScanline0, &lStride);
				  
				  if ( SUCCEEDED(hr) ){
					  pOwner_->OnData(pbScanline0, DataSize, m_lDefaultStride, llTimestamp / 10000);
				  }
			  }
		  }

		  hr = m_pReader->ReadSample(
			  (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			  0,
			  nullptr,   // actual
			  nullptr,   // flags
			  nullptr,   // timestamp
			  nullptr    // sample
			  );

		  return hr;
	  }

	  STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *){
		  return S_OK;
	  }

	  STDMETHODIMP OnFlush(DWORD){
		  return S_OK;
	  } 

	  ////////////////////////////////////////////////////////////////////
	  ////////////////////////////////////////////////////////////////////

	  HRESULT GetDeviceCount(DWORD &Count){
		  lock_guard<recursive_mutex> lock(_mtx);
		  return get_DeviceCount( Count ) ;
	  }

	  HRESULT EnumDevice ( DWORD Idx, wchar_t *DeviceId, wchar_t *DeviceName ){
		  lock_guard<recursive_mutex> lock(_mtx);
		  const size_t devices = m_vecDevInfo.size() ;
		  assert(Idx < devices) ;
		  assert(DeviceId != nullptr);
		  assert(DeviceName != nullptr);
		  if (Idx >= devices || DeviceId == nullptr || DeviceName == nullptr){
			  return E_INVALIDARG ;
		  }
		  swprintf ( DeviceId, m_vecDevInfo[Idx].DeviceStr[DEVICE_PATH].c_str() ) ;
		  swprintf ( DeviceName, m_vecDevInfo[Idx].DeviceStr[DEVICE_NAME].c_str() ) ;
		  return S_OK ;
	  }

	  HRESULT GetCurrentDevice ( wchar_t *DeviceId, wchar_t *DeviceName ){
		  lock_guard<recursive_mutex> lock(_mtx);
		  assert(m_dwCurrentIdx != -1) ;
		  assert(DeviceId != nullptr);
		  assert(DeviceName != nullptr);
		  if (m_dwCurrentIdx == -1 || DeviceId == nullptr || DeviceName == nullptr){
			  return E_INVALIDARG;
		  }
		  swprintf ( DeviceId, m_vecDevInfo[m_dwCurrentIdx].DeviceStr[DEVICE_PATH].c_str() ) ;
		  swprintf ( DeviceName, m_vecDevInfo[m_dwCurrentIdx].DeviceStr[DEVICE_NAME].c_str() ) ;
		  return S_OK ;
	  }

	  HRESULT GetFormatCount(const wchar_t *DeviceId, DWORD &Count){
		  Count = 0;
		  lock_guard<recursive_mutex> lock(_mtx);
		  assert(DeviceId != nullptr);
		  if (DeviceId == nullptr){
			  return E_FAIL ;
		  }

		  for (auto &info : m_vecDevInfo){
			  if ( info.DeviceStr[DEVICE_PATH] == DeviceId ){
				  Count = info.vecMediaType.size();
				  return S_OK ;
			  }
		  }
		  return E_FAIL ;
	  }

	  HRESULT EnumFormat ( const wchar_t *DeviceId, DWORD Idx, VIDEO_FORMAT &VideoFormat ){
		  lock_guard<recursive_mutex> lock(_mtx);

		  assert(DeviceId != nullptr);
		  if (DeviceId == nullptr){
			  return E_INVALIDARG ;
		  }
		  
		  for (auto &info : m_vecDevInfo){
			  if ( info.DeviceStr[DEVICE_PATH] == DeviceId ){
				  if (Idx >= info.vecMediaType.size()){
					  return E_FAIL ;
				  }

				  HRESULT hr = GetMediaInfo ( info.vecMediaType[Idx].pType, VideoFormat ) ;
				  return S_OK ;
			  }
		  }
		  return E_FAIL ;
	  }

	  HRESULT GetCurrentFormat ( VIDEO_FORMAT &VideoFormat ){
		  lock_guard<recursive_mutex> lock(_mtx);

		  assert(m_dwCurrentIdx != -1) ;
		  if ( m_dwCurrentIdx == -1 ){
			  return E_FAIL ;
		  }
		  CComPtr<IMFMediaSource> pSource ;
		  HRESULT hr = m_vecDevInfo[m_dwCurrentIdx].pDevice->ActivateObject(IID_PPV_ARGS(&pSource));
		  if (FAILED(hr)){
			  return E_FAIL ;
		  }
		  return GetCurrentFormat(pSource, VideoFormat) ;
	  }

	  HRESULT CloseDevice (){
		  lock_guard<recursive_mutex> lock(_mtx);
		  SafeRelease(&m_pReader);
		  return S_OK ;
	  }

	  HRESULT OpenDevice ( const wchar_t *DeviceId ){
		  lock_guard<recursive_mutex> lock(_mtx);

		  assert(DeviceId != nullptr);
		  if (DeviceId == nullptr){
			  return E_INVALIDARG ;
		  }

		  SafeRelease(&m_pReader);
		  m_dwCurrentIdx = -1 ;

		  int i = -1 ;
		  for (auto &info : m_vecDevInfo){
			  ++i ;
			  if ( info.DeviceStr[DEVICE_PATH] == DeviceId ){
				  CComPtr<IMFAttributes> pAttributes ;
				  HRESULT hr = MFCreateAttributes(&pAttributes, 2);
				  if ( FAILED(hr) ){
					  return hr ;
				  }

				  hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
				  if ( FAILED(hr) ){
					  return hr ;
				  }

				  hr = pAttributes->SetUnknown(
					  MF_SOURCE_READER_ASYNC_CALLBACK,
					  this
					  );
				  if ( FAILED(hr) ){
					  return hr ;
				  }

				  CComPtr<IMFMediaSource> pSource ;
				  hr = m_vecDevInfo[i].pDevice->ActivateObject(IID_PPV_ARGS(&pSource));
				  if (FAILED(hr)){
					  return hr ;
				  }
				  hr = MFCreateSourceReaderFromMediaSource(
					  pSource,
					  pAttributes,
					  &m_pReader
					  );
				  if ( SUCCEEDED(hr) ){			
					  m_dwCurrentIdx = i ;

					  VIDEO_FORMAT vf ;
					  hr = GetCurrentFormat(vf) ;
					  if ( FAILED(hr) ){
						  return hr ;
					  }
					  m_Height = vf.Height ;

					  CComPtr<IMFMediaType> pType ;
					  hr = GetCurrentMediaType( pSource, &pType ) ;
					  if ( FAILED(hr) ){
						  return hr ;
					  }
					  hr = GetDefaultStride ( pType, &m_lDefaultStride) ;
					  if ( FAILED(hr) ){
						  return hr ;
					  }
					  return S_OK ;
				  }
				  break ;
			  }
		  }
		  return E_FAIL ;
	  }

	  HRESULT SetFormat ( DWORD Idx, RATIO &FrameRate ){
		  lock_guard<recursive_mutex> lock(_mtx);
		  
		  const size_t formats = m_vecDevInfo[m_dwCurrentIdx].vecMediaType.size() ;

		  assert (m_dwCurrentIdx != -1) ;
		  assert (Idx < formats) ;
		  if ( m_dwCurrentIdx == -1 ){
			  return E_FAIL ;
		  }
		  if ( Idx >= formats ) {
			  return E_FAIL ;
		  }

		  IMFMediaType *pType = m_vecDevInfo[m_dwCurrentIdx].vecMediaType[Idx].pType ;
		  HRESULT hr = MFSetAttributeRatio(pType, MF_MT_FRAME_RATE, FrameRate.Numerator, FrameRate.Denominator) ;
		  if ( FAILED(hr) ){
			  return hr ;
		  }

		  CComPtr<IMFMediaSource> pSource ;
		  hr = m_vecDevInfo[m_dwCurrentIdx].pDevice->ActivateObject(IID_PPV_ARGS(&pSource));
		  if (FAILED(hr)){
			  return hr ;
		  }
		  hr = SetCurrentMediaType( pSource, pType ) ;
		  if ( FAILED(hr) ){
			  return hr ;
		  }

		  VIDEO_FORMAT vf ;
		  hr = GetCurrentFormat(vf) ;
		  if ( FAILED(hr) ){
			  return hr ;
		  }
		  m_Height = vf.Height ;

		  return GetDefaultStride ( pType, &m_lDefaultStride) ;
	  }

	  HRESULT Start (){
		  lock_guard<recursive_mutex> lock(_mtx);

		  assert(m_pReader != nullptr);
		  if (m_pReader != nullptr){
			  HRESULT hr = m_pReader->ReadSample(
				  (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
				  0,
				  nullptr,
				  nullptr,
				  nullptr,
				  nullptr
				  );
			  if ( FAILED(hr) ){
				  return hr ;
			  }
			  return S_OK ;
		  }
		  return E_FAIL ;
	  }

	  HRESULT Stop  (){
		  lock_guard<recursive_mutex> lock(_mtx);
		  return E_FAIL ;
	  }

private:

	HRESULT get_DeviceCount( DWORD &Count ){
		HRESULT hr = S_OK ;
		Count = 0 ;
		m_vecDevInfo.clear() ;
		
		CComPtr<IMFAttributes> pAttributes ;
		CComHeapPtr<IMFActivate*> ppDevices ;
		UINT32 count = 0 ;
		do
		{
			hr = MFCreateAttributes(&pAttributes, 1);
			if ( FAILED(hr) ){
				break ;
			}

			hr = pAttributes->SetGUID(
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, 
				MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
				);
			if (FAILED(hr)) { 
				break ; 
			}

			hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
			if (FAILED(hr)){ 
				break ; 
			}

			GUID guid[STR_COUNT] = {MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
				MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME} ;
			for (DWORD i = 0; i < count; ++i){
				DEVICE_INFO info ;
				for ( int j = 0 ; j < STR_COUNT ; ++j ){
					CComHeapPtr<wchar_t> str ;
					UINT size = 0 ;
					hr = ppDevices[i]->GetAllocatedString(
						guid[j], 
						&str, 
						&size
						);
					if ( SUCCEEDED(hr) ){
						info.DeviceStr[j] = str ;
					}
				}

				if ( info.DeviceStr[DEVICE_PATH] != L"" && info.DeviceStr[DEVICE_NAME] != L"" ){ 
					CComPtr<IMFMediaSource> pSource ;
					hr = ppDevices[i]->ActivateObject(IID_PPV_ARGS(&pSource));
					if (FAILED(hr)){
						continue ;
					}
					EnumerateCaptureFormats( pSource, info ) ;
					info.pDevice = ppDevices[i] ;
					ppDevices[i]->AddRef () ;
					m_vecDevInfo.push_back(info) ;
				}
			}

		} while (false ) ;

		Count = m_vecDevInfo.size() ;
		return hr ;
	}

	HRESULT EnumerateCaptureFormats(IMFMediaSource *pSource, DEVICE_INFO &info){
		CComPtr<IMFPresentationDescriptor> pPD ;
		CComPtr<IMFStreamDescriptor> pSD ;
		CComPtr<IMFMediaTypeHandler> pHandler ;

		HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
		if (FAILED(hr)){
			return hr ;
		}

		BOOL fSelected;
		hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pSD->GetMediaTypeHandler(&pHandler);
		if (FAILED(hr)){
			return hr ;
		}

		DWORD cTypes = 0;
		hr = pHandler->GetMediaTypeCount(&cTypes);
		if (FAILED(hr)){
			return hr ;
		}

		for (DWORD i = 0; i < cTypes; i++){
			CComPtr<IMFMediaType> pType ;
			hr = pHandler->GetMediaTypeByIndex(i, &pType);
			if (FAILED(hr)){
				return hr ;
			}

			bool Equal = false ;
			auto it = info.vecMediaType.begin() ;
			for ( ; it != info.vecMediaType.end() ; ++it ){
				DWORD Flag ;
				hr = pType->IsEqual((*it).pType, &Flag) ;
				if ( hr == S_OK ){
					Equal = true ;
					break ;
				}

				if ( Flag == 11 ){
					GUID guid1, guid2 ;
					hr = pType->GetGUID(MF_MT_AM_FORMAT_TYPE, &guid1) ;
					if ( FAILED(hr) ){
						continue ;
					}
					hr = (*it).pType->GetGUID(MF_MT_AM_FORMAT_TYPE, &guid2) ;
					if ( FAILED(hr) ){
						continue ;
					}
					if ( guid1 == guid2 ){
						continue ;
					}

					if ( FORMAT_VideoInfo == guid1 && FORMAT_VideoInfo2 == guid2 ){
						Equal = true ;
						break ;
					}
					else if ( FORMAT_VideoInfo == guid2 && FORMAT_VideoInfo2 == guid1 ){
						info.vecMediaType.erase(it) ;
						break ;
					}
				}
				/*
				if ( FAILED(hr) )
				{
					BOOL Ret ;
					HRESULT hr = pType->Compare((*it).pType, MF_ATTRIBUTES_MATCH_INTERSECTION, &Ret ) ;
				}*/
			}

			if ( !Equal ){
				MEDIA_TYPE vf(pType) ;
				info.vecMediaType.push_back(vf) ;
				hr = S_OK ;
			}
		}

		return hr;
	}

	HRESULT GetMediaInfo ( IMFMediaType *pType, VIDEO_FORMAT &VideoFormat ){
		LogMediaType(pType) ;

		GUID guid ;
		HRESULT hr = pType->GetGUID(MF_MT_SUBTYPE, &guid) ;
		if ( FAILED(hr) ){
			return hr ;
		}

		const wchar_t *str = GetMediaSubtype (guid, VideoFormat.Fcc) ;
		if (str == nullptr){
			return E_FAIL ;
		}

		int len = WideCharToMultiByte(0, 0, str, wcslen(str), VideoFormat.ColorSpace, 255, NULL, NULL);
		VideoFormat.ColorSpace[len] = 0;
//		wcscpy_s(VideoFormat.ColorSpace, str);

		hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &VideoFormat.Width, &VideoFormat.Height) ;
		if ( FAILED(hr) ){
			return hr ;
		}

		hr = MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, (UINT32*)&VideoFormat.FrameRate.Numerator, (UINT32*)&VideoFormat.FrameRate.Denominator) ;
		if ( FAILED(hr) ){
			return hr ;
		}

		hr = MFGetAttributeRatio(pType, MF_MT_FRAME_RATE_RANGE_MAX, (UINT32*)&VideoFormat.FrameRateMax.Numerator, (UINT32*)&VideoFormat.FrameRateMax.Denominator) ;
		if ( FAILED(hr) ){
			return hr ;
		}

		hr = MFGetAttributeRatio(pType, MF_MT_FRAME_RATE_RANGE_MIN, (UINT32*)&VideoFormat.FrameRateMin.Numerator, (UINT32*)&VideoFormat.FrameRateMin.Denominator) ;
		if ( FAILED(hr) ){
			return hr ;
		}

		hr = MFGetAttributeRatio(pType, MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&VideoFormat.PAR.Numerator, (UINT32*)&VideoFormat.PAR.Denominator) ;
		if ( FAILED(hr) ){
			return hr ;
		}
//		UINT32 interlace = MFGetAttributeUINT32 ( pType, MF_MT_DEFAULT_STRIDE, 1000 ) ;
		return hr ;
	}

	HRESULT GetCurrentFormat(IMFMediaSource *pSource, VIDEO_FORMAT &VideoFormat){
		CComPtr<IMFPresentationDescriptor> pPD ;
		CComPtr<IMFStreamDescriptor> pSD ;
		CComPtr<IMFMediaTypeHandler> pHandler ;
		CComPtr<IMFMediaType> pType ;

		HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
		if (FAILED(hr)){
			return hr ;
		}

		BOOL fSelected;
		hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pSD->GetMediaTypeHandler(&pHandler);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pHandler->GetCurrentMediaType(&pType);
		if (FAILED(hr)){
			return hr ;
		}

		hr = GetMediaInfo ( pType, VideoFormat ) ;
		return hr ;
	}

	HRESULT GetCurrentMediaType(IMFMediaSource *pSource, IMFMediaType **pType){
		CComPtr<IMFPresentationDescriptor> pPD ;
		CComPtr<IMFStreamDescriptor> pSD ;
		CComPtr<IMFMediaTypeHandler> pHandler ;

		HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
		if (FAILED(hr)){
			return hr ;
		}

		BOOL fSelected;
		hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pSD->GetMediaTypeHandler(&pHandler);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pHandler->GetCurrentMediaType(pType);
		return hr ;
	}

	HRESULT SetCurrentMediaType(IMFMediaSource *pSource, IMFMediaType *pType ){
		CComPtr<IMFPresentationDescriptor> pPD ;
		CComPtr<IMFStreamDescriptor> pSD ;
		CComPtr<IMFMediaTypeHandler> pHandler ;

		HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
		if (FAILED(hr)){
			return hr ;
		}

		BOOL fSelected;
		hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pSD->GetMediaTypeHandler(&pHandler);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pHandler->SetCurrentMediaType(pType);
		return hr ;
	}


private:
	long m_nRefCount;
	vector<DEVICE_INFO> m_vecDevInfo ;
	DWORD m_dwCurrentIdx ;
	IMFSourceReader *m_pReader;

	recursive_mutex _mtx;
	long m_lDefaultStride ;
	UINT m_Height ;

	CMFVideoCapture *pOwner_ ;
};


CMFVideoCapture::CMFVideoCapture(HRESULT &hr){
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)){
        return ;
    }
	hr = MFStartup(MF_VERSION);
	if (FAILED(hr)){
        return ;
    }
	pImpl = new Impl(this) ;
	return;
}

CMFVideoCapture::~CMFVideoCapture(void)
{
	if (pImpl != nullptr){
		pImpl->CloseDevice () ;
	}
	SafeRelease (&pImpl) ;
	MFShutdown();
	CoUninitialize();
}

HRESULT CMFVideoCapture::GetDeviceCount(DWORD &Count){
	return pImpl->GetDeviceCount(Count) ;
}

HRESULT CMFVideoCapture::EnumDevice ( DWORD Idx, char *DeviceId, char *DeviceName ){
	wchar_t Device_Id[256], Device_Name[256];
	HRESULT hr = pImpl->EnumDevice(Idx, Device_Id, Device_Name);
	if (FAILED(hr)){
		return hr;
	}
	int len = WideCharToMultiByte(0, 0, Device_Id, wcslen(Device_Id), DeviceId, 255, NULL, NULL);
	DeviceId[len] = 0;
	len = WideCharToMultiByte(0, 0, Device_Name, wcslen(Device_Name), DeviceName, 255, NULL, NULL);
	DeviceName[len] = 0;
	return hr;
}

HRESULT CMFVideoCapture::GetCurrentDevice ( char *DeviceId, char *DeviceName ){
	wchar_t Device_Id[256], Device_Name[256];
	HRESULT hr = pImpl->GetCurrentDevice(Device_Id, Device_Name);
	if (FAILED(hr)){
		return hr;
	}
	int len = WideCharToMultiByte(0, 0, Device_Id, wcslen(Device_Id), DeviceId, 255, NULL, NULL);
	DeviceId[len] = 0;
	len = WideCharToMultiByte(0, 0, Device_Name, wcslen(Device_Name), DeviceName, 255, NULL, NULL);
	DeviceName[len] = 0;
	return hr;
}


HRESULT CMFVideoCapture::GetFormatCount(const char *DeviceId, DWORD &Count){
	wchar_t Device_Id[256] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, DeviceId, strlen(DeviceId), Device_Id, 255);
	return pImpl->GetFormatCount(Device_Id, Count);
}

HRESULT CMFVideoCapture::EnumFormat ( const char *DeviceId, DWORD Idx, VIDEO_FORMAT &VideoFormat ){
	wchar_t Device_Id[256] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, DeviceId, strlen(DeviceId), Device_Id, 255);
	return pImpl->EnumFormat ( Device_Id, Idx, VideoFormat ) ;
}

HRESULT CMFVideoCapture::GetCurrentFormat ( VIDEO_FORMAT &VideoFormat ){
	return pImpl->GetCurrentFormat ( VideoFormat ) ;
}

HRESULT CMFVideoCapture::OpenDevice ( const char *DeviceId ){
	wchar_t Device_Id[256] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, DeviceId, strlen(DeviceId), Device_Id, 255);
	return pImpl->OpenDevice(Device_Id) ;
}

HRESULT CMFVideoCapture::CloseDevice (){
	return pImpl->CloseDevice() ;
}

HRESULT CMFVideoCapture::SetFormat ( DWORD Idx, RATIO &FrameRate ){
	return pImpl->SetFormat(Idx, FrameRate) ;
}

HRESULT CMFVideoCapture::Start () {
	return pImpl->Start() ;
}

HRESULT CMFVideoCapture::Stop  (){
	return pImpl->Stop() ;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// c style

class Cap : public CMFVideoCapture
{
public:
	Cap(HRESULT &hr, MF_VIDEOCAPTURE_INIT *params) :
		CMFVideoCapture(hr),
		_dataCallBack(params->dataCallBack),
		_msgCallBack(params->msgCallBack)	
	{
		_output.userData = params->userData;
	}
public:
	virtual void OnMessage(const char* Msg) override{
		if (_msgCallBack != nullptr){
			_msgCallBack(Msg);
		}
	}
public:
	virtual void OnData(BYTE *pData, DWORD DataSize, long lStride, DWORD ts) override{
		_output.data = pData;
		_output.dataSize = DataSize;
		_output.stride = lStride;
		_output.timeStamp = ts;
		_dataCallBack(&_output);
	}

private:
	MF_VIDEOCAPTURE_OUTPUTCALLBACK_PTR _dataCallBack;
	MF_VIDEOCAPTURE_MSGCALLBACK_PTR    _msgCallBack;
	MF_VIDEOCAPTURE_OUTPUT _output;
};

MFVIDEOCAPTURE_API HMFVIDEODCAPTURE MFVideoCapture_Create(MF_VIDEOCAPTURE_INIT *params){
	HRESULT hr;
	Cap *p = new Cap(hr, params);
	if (p == nullptr || FAILED(hr)){
		delete p;
		p = nullptr;
	}
	return (HMFVIDEODCAPTURE)p;
}

MFVIDEOCAPTURE_API void MFVideoCapture_Destroy(HMFVIDEODCAPTURE capture){
	delete ((Cap*)capture);
}


MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetDeviceCount(HMFVIDEODCAPTURE capture, DWORD *Count){
	if (capture == nullptr || Count == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->GetDeviceCount(*Count);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_EnumDevice(HMFVIDEODCAPTURE capture, DWORD Idx, char *DeviceId, char *DeviceName){
	if (capture == nullptr || DeviceId == nullptr || DeviceName == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->EnumDevice(Idx, DeviceId, DeviceName);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetCurrentDevice(HMFVIDEODCAPTURE capture, char *DeviceId, char *DeviceName){
	if (capture == nullptr || DeviceId == nullptr || DeviceName == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->GetCurrentDevice(DeviceId, DeviceName);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetFormatCount(HMFVIDEODCAPTURE capture, const char *DeviceId, DWORD *Count){
	if (capture == nullptr || DeviceId == nullptr || Count == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->GetFormatCount(DeviceId, *Count);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_EnumFormat(HMFVIDEODCAPTURE capture, const char *DeviceId, DWORD Idx, CMFVideoCapture::VIDEO_FORMAT *VideoFormat){
	if (capture == nullptr || DeviceId == nullptr || VideoFormat == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->EnumFormat(DeviceId, Idx, *VideoFormat);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_GetCurrentFormat(HMFVIDEODCAPTURE capture, CMFVideoCapture::VIDEO_FORMAT *VideoFormat){
	if (capture == nullptr || VideoFormat == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->GetCurrentFormat(*VideoFormat);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_OpenDevice(HMFVIDEODCAPTURE capture, const char *DeviceId){
	if (capture == nullptr || DeviceId == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->OpenDevice(DeviceId);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_CloseDevice(HMFVIDEODCAPTURE capture){
	if (capture == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->CloseDevice();
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_SetFormat(HMFVIDEODCAPTURE capture, DWORD Idx, CMFVideoCapture::RATIO *FrameRate){
	if (capture == nullptr || FrameRate == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->SetFormat(Idx, *FrameRate);
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_Start(HMFVIDEODCAPTURE capture){
	if (capture == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->Start();
}

MFVIDEOCAPTURE_API HRESULT MFVideoCapture_Stop(HMFVIDEODCAPTURE capture){
	if (capture == nullptr){
		return E_INVALIDARG;
	}
	Cap *p = (Cap*)capture;
	return p->Stop();
}
