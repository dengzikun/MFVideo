// D3DVideoRenderer.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <assert.h>
#include "comdef.h"

#include "D3DVideoRenderer.h"
#include "utils.h"
#include "D3DHelper.h"

#include "IMAGE.h"
#include "CANVAS.h"

class CD3DVideoRenderer::Impl
{
public:
	Impl(HRESULT &hr, HWND hWnd, IDirect3DDeviceManager9 **pManager, DWORD nAdapterNum, bool WaitVsync){
		m_DWM.Initialize() ;

		hr = m_D3D.InitializeD3D9(hWnd, nAdapterNum, WaitVsync) ;
		if (FAILED(hr)){
			return ;
		}

		UINT resetToken = 0;
		hr = DXVA2CreateDirect3DDeviceManager9(&resetToken, pManager);
		if (SUCCEEDED(hr)){
			IDirect3DDeviceManager9 *p = *pManager ;
			CDeviceManager dev(m_D3D) ;
			hr = p->ResetDevice(dev.GetDevice(), resetToken);
			if (SUCCEEDED(hr)){
				_deviceManager = p;
			}
		}

		CDeviceManager dev(m_D3D) ;
		hr = InitializeDXVAHD(dev.GetDevice()) ;
		if (FAILED(hr)){
			return ;
		}
	}

	Impl(HRESULT &hr, IDirect3DDeviceManager9 *pManager){
		m_DWM.Initialize() ;

		hr = m_D3D.InitializeD3D9(pManager) ;
		if (FAILED(hr)){
			return ;
		}

		CDeviceManager dev(m_D3D) ;
		hr = InitializeDXVAHD(dev.GetDevice()) ;
		if (FAILED(hr)){
			return ;
		}
	}

	~Impl(void){
		m_pDXVAHD.Release () ;
	}

	bool IsSupport ( COLOR_SPACE cs ){
		DWORD a = '21VN';
		DWORD b = '21VY';
		DWORD c = 'VUYA';
		auto it = std::find( m_inputFormat.begin(), m_inputFormat.end(), cs ) ;
		if ( it != m_inputFormat.end() ){
			return true ;
		}
		return false ;
	}

	DWORD MaxInputStreams(){
		return caps.MaxInputStreams ;
	}

	HRESULT Create( DWORD Width, DWORD Height, CANVAS **pCanvas ){
		assert(Width > 0);
		assert(Height > 0);
		assert(pCanvas != nullptr);
		assert(m_pDXVAHD != nullptr);

		if (m_pDXVAHD == nullptr || Width == 0 || Height == 0){
			return E_INVALIDARG ;
		}

		CDeviceManager dev(m_D3D) ;
		CANVAS *p = new CANVAS();
		if (p == nullptr){
			return E_OUTOFMEMORY ;
		}

		HRESULT hr = p->Create(dev.GetDevice(), m_pDXVAHD, Width, Height) ;
		if ( FAILED(hr) ){
			delete p ;
			return hr ;
		}
		*pCanvas = p ;
		return S_OK ;
	}

	void Destroy( CANVAS *pCanvas ){
		delete pCanvas ;
	}

	HRESULT Create( DWORD Width, DWORD Height, D3DFORMAT cs, IMAGE **pImage ){
		assert(m_pDXVAHD != nullptr);
		assert(pImage != nullptr);
		assert(Width > 0) ;
		assert(Height > 0) ;
//		assert(cs == FOURCC_NV12 || cs == FOURCC_YV12 || cs == FOURCC_YUY2 || cs == FOURCC_A8R8G8B8);

		if (m_pDXVAHD == nullptr || pImage == nullptr){
			return E_INVALIDARG ;
		}
		if ( cs != FOURCC_NV12 && cs != FOURCC_YV12 && cs != FOURCC_YUY2 && cs != FOURCC_A8R8G8B8 ){
			return E_INVALIDARG ;
		}

		if ( Width == 0 || Height == 0 ){
			return E_INVALIDARG ;
		}

		*pImage = nullptr;
		HRESULT hr = S_OK ;
		if ( cs == FOURCC_YUY2 ){	
			*pImage = new IMAGE_YUY2(Width, Height, caps.InputPool, m_pDXVAHD, hr);
		}
		else if ( cs == FOURCC_YV12 ){
			*pImage = new IMAGE_YV12(Width, Height, caps.InputPool, m_pDXVAHD, hr);
		}
		else if ( cs == FOURCC_NV12 ){
			*pImage = new IMAGE_NV12(Width, Height, caps.InputPool, m_pDXVAHD, hr);
		}
		else if ( cs == FOURCC_A8R8G8B8 ){
			*pImage = new IMAGE_A8R8G8B8(Width, Height, caps.InputPool, m_pDXVAHD, hr);
		}else{
			return E_INVALIDARG ;
		}

		if (*pImage == nullptr){
			return E_OUTOFMEMORY ;
		}

		if ( FAILED(hr) ){
			delete *pImage ;
			return hr ;
		}
		return S_OK ;
	}

	HRESULT Create( wchar_t* str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height, IMAGE **pImage ){
		assert(str != nullptr);
		assert(pImage != nullptr);
		assert(Width > 0);
		assert(Height > 0);
		assert(pImage != nullptr);
		
		if (str == nullptr || pImage == nullptr || Width == 0 || Height == 0){
			return E_INVALIDARG ;
		}

		HRESULT hr ;
		*pImage = new IMAGE_TEXT ( str, LogFont, color, caps.InputPool, m_pDXVAHD, Width, Height, hr ) ;
		if (*pImage == nullptr){
			return E_OUTOFMEMORY ;
		}
		if ( FAILED(hr) ){
			delete *pImage ;
			return hr ;
		}
		return S_OK ;
	}

	void Destroy( IMAGE *pImage ){
		delete pImage ;
	}

	HRESULT SetBackgroundColor(CANVAS &Canvas, float a, float r, float g, float b){
		return Canvas.SetBackgroundColor(a, r, g, b);
	}

	HRESULT SetTargetRect(CANVAS &Canvas, BOOL bEnable, RECT &rect){
		return Canvas.SetTargetRect(bEnable, rect);
	}

	HRESULT SetSourceRect(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, RECT &rect){
		return Canvas.SetSourceRect(StreamIndex, bEnable, rect) ;
	}

	HRESULT SetDestinationRect(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, RECT &rect){
		return Canvas.SetDestinationRect(StreamIndex, bEnable, rect) ;
	}

	HRESULT SetStreamFormat(CANVAS &Canvas, DWORD StreamIndex, COLOR_SPACE cs){
		return Canvas.SetStreamFormat(StreamIndex, cs) ;
	}

	HRESULT SetFrameFormat(CANVAS &Canvas, DWORD StreamIndex, FRAME_FORMAT ff){
		return Canvas.SetFrameFormat(StreamIndex, ff) ;
	}

	HRESULT SetPlanarAlpha(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, float alpha){
		return Canvas.SetPlanarAlpha(StreamIndex, bEnable, alpha) ;
	}

	HRESULT Update( IMAGE &Image, BYTE *pFrame, long Stride ){
		return Image.Update(pFrame, Stride) ;
	}

	HRESULT Update( IMAGE &Image, YUV_FRAME_INFO *pInfo ){
		return Image.Update(Image, pInfo) ;
	}

	HRESULT Update( IMAGE &Image, IDirect3DSurface9 *pSur ){
		CDeviceManager dev(m_D3D) ;
		return Image.Update(dev.GetDevice(), m_pDXVAHD, pSur) ;
	}

	HRESULT Update( IMAGE &Image, wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height ){
		return Image.Update ( str, LogFont, color, Width, Height ) ;
	}

	HRESULT Render(CANVAS &Canvas, STREAM_INFO *pInfo, DWORD StreamCount, HWND hWnd, RECT *src, RECT *dst){
		HRESULT hr = m_D3D.TestCooperativeLevel();

		switch (hr){
		case D3D_OK :
			break;

		case D3DERR_DEVICELOST :
			return hr ;
			break ;

		case D3DERR_DEVICENOTRESET :
			return hr ;
			break;

		default :
			return E_FAIL ;
		}

		Canvas.Draw(pInfo, StreamCount);
		/*
		if ( hWnd == nullptr )
		{
			m_DWM.EnableDwmQueuing(m_D3D.m_d3dpp.hDeviceWindow);
		}
		else
		{
			m_DWM.EnableDwmQueuing(hWnd);
		}*/

		return Canvas.Render(hWnd, src, dst);
	}

private:
	HRESULT InitializeDXVAHD(IDirect3DDevice9Ex *pD3DDevice){
		HRESULT hr = S_OK;
		DWORD index = 0;

		DXVAHD_CONTENT_DESC desc;

		desc.InputFrameFormat = DXVAHD_FRAME_FORMAT_PROGRESSIVE;
		desc.InputFrameRate.Numerator = VIDEO_FPS;
		desc.InputFrameRate.Denominator = 1;
		desc.InputWidth = VIDEO_MAIN_WIDTH;
		desc.InputHeight = VIDEO_MAIN_HEIGHT;
		desc.OutputFrameRate.Numerator = VIDEO_FPS;
		desc.OutputFrameRate.Denominator = 1;
		desc.OutputWidth = VIDEO_MAIN_WIDTH;
		desc.OutputHeight = VIDEO_MAIN_HEIGHT;

		PDXVAHDSW_Plugin pSWPlugin = nullptr;

		hr = DXVAHD_CreateDevice(
			pD3DDevice,
			&desc,
			DXVAHD_DEVICE_USAGE_PLAYBACK_NORMAL,
			pSWPlugin,
			&m_pDXVAHD
			);
		if (FAILED(hr)){ 
			return hr ; 
		}

		// Get the DXVA-HD device caps.

		ZeroMemory(&caps, sizeof(caps));

		hr = m_pDXVAHD->GetVideoProcessorDeviceCaps(&caps);
		if (FAILED(hr)){ 
			return hr ; 
		}

		if (caps.MaxInputStreams < 1 + SUB_STREAM_COUNT){
			DBGMSG(L"Device only supports %d input streams.\n", caps.MaxInputStreams);
			return E_UNEXPECTED ;
		}

		// Check the output format.

		m_inputFormat.clear() ;
		m_inputFormat.resize(caps.OutputFormatCount) ;

		hr = m_pDXVAHD->GetVideoProcessorOutputFormats(caps.OutputFormatCount, &m_inputFormat[0]);
		if (FAILED(hr)){ 
			return hr ; 
		}

		for (index = 0; index < caps.OutputFormatCount; index++){
			if (m_inputFormat[index] == VIDEO_RENDER_TARGET_FORMAT){
				break;
			}
		}
		if (index == caps.OutputFormatCount){
			return E_UNEXPECTED ;
		}

		// Check the input formats.

		m_inputFormat.clear() ;
		m_inputFormat.resize(caps.InputFormatCount) ;

		hr = m_pDXVAHD->GetVideoProcessorInputFormats(caps.InputFormatCount, &m_inputFormat[0]);
		if (FAILED(hr)){ 
			return hr ; 
		}

		D3DFORMAT inputFormats[] = { VIDEO_MAIN_FORMAT, VIDEO_SUB_FORMAT };

		for (DWORD j = 0; j < 2; j++){
			for (index = 0; index < caps.InputFormatCount; index++){
				if (m_inputFormat[index] == inputFormats[j]){
					break;
				}
			}
			if (index == caps.InputFormatCount){
				return E_UNEXPECTED ;
			}
		}

		return S_OK ;
	}
	
private:
	DwmHelper           m_DWM;
	D3DHelper           m_D3D;

	CComPtr<IDXVAHD_Device>	m_pDXVAHD;
//	IDXVAHD_Device	*m_pDXVAHD;
	DXVAHD_VPDEVCAPS caps;

	std::vector<D3DFORMAT> m_inputFormat ;

	CComPtr<IDirect3DDeviceManager9> _deviceManager ;
};

static long g_get = 0 ;
static std::vector<std::string> vec_des ;

UINT CD3DVideoRenderer::GetAdapterCount(){
	if ( InterlockedCompareExchange(&g_get, 1, 0) == 1 ){
		return vec_des.size() ;
	}

	CComPtr<IDirect3D9Ex> pD3D;
	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D);
	if (FAILED(hr)){
		DBGMSG(L"Direct3DCreate9Ex failed.\n");
		return 0;
	}
	UINT count = pD3D->GetAdapterCount() ;
	for ( UINT i = 0 ; i < count ; ++i ){
		D3DADAPTER_IDENTIFIER9 id ;
		hr = pD3D->GetAdapterIdentifier(i, 0, &id) ;
		if ( SUCCEEDED(hr)){
			vec_des.push_back(id.Description);
		}
	}
	return vec_des.size();
}
const char* CD3DVideoRenderer::GetAdapterDescription(UINT index){
	if ( index < vec_des.size() ){
		return vec_des[index].c_str() ;
	}
	return nullptr;
}

CD3DVideoRenderer::CD3DVideoRenderer(HRESULT &hr, HWND hWnd, IDirect3DDeviceManager9 **pManager, DWORD nAdapterNum, bool WaitVsync ){
	pImpl = new Impl(hr, hWnd, pManager, nAdapterNum, WaitVsync);
}

CD3DVideoRenderer::CD3DVideoRenderer(HRESULT &hr, IDirect3DDeviceManager9 *pManager ){
	pImpl = new Impl(hr, pManager);
}

CD3DVideoRenderer::~CD3DVideoRenderer(void){
	delete pImpl ;
}

bool CD3DVideoRenderer::IsSupport ( COLOR_SPACE cs ){
	return pImpl->IsSupport ( cs ) ;
}

DWORD CD3DVideoRenderer::MaxInputStreams(CANVAS *pCanvas){
	return pImpl->MaxInputStreams() ;
}

HRESULT CD3DVideoRenderer::Create( DWORD Width, DWORD Height, CANVAS **pCanvas ){
	return pImpl->Create(Width, Height, pCanvas) ;
}

void CD3DVideoRenderer::Destroy( CANVAS *pCanvas ){
	return pImpl->Destroy(pCanvas);
}

HRESULT CD3DVideoRenderer::Create( DWORD Width, DWORD Height, COLOR_SPACE cs, IMAGE **pImage ){
	return pImpl->Create(Width, Height, (D3DFORMAT)cs, pImage) ;
}

HRESULT CD3DVideoRenderer::Create( wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height, IMAGE **pImage ){
	return pImpl->Create(str, LogFont, color, Width, Height, pImage) ;
}

void CD3DVideoRenderer::Destroy( IMAGE *pImage ){
	return pImpl->Destroy(pImage);
}

HRESULT CD3DVideoRenderer::SetBackgroundColor(CANVAS &Canvas, float a, float r, float g, float b){
	return pImpl->SetBackgroundColor(Canvas, a, r, g, b);
}

HRESULT CD3DVideoRenderer::SetTargetRect(CANVAS &Canvas, BOOL bEnable, RECT &rect){
	return pImpl->SetTargetRect(Canvas, bEnable, rect);
}

HRESULT CD3DVideoRenderer::SetSourceRect(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, RECT &rect){
	return pImpl->SetSourceRect( Canvas, StreamIndex, bEnable, rect );
}

HRESULT CD3DVideoRenderer::SetDestinationRect(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, RECT &rect){
	return pImpl->SetDestinationRect( Canvas, StreamIndex, bEnable, rect );
}

HRESULT CD3DVideoRenderer::SetStreamFormat(CANVAS &Canvas, DWORD StreamIndex, COLOR_SPACE cs){
	return pImpl->SetStreamFormat( Canvas, StreamIndex, cs );
}

HRESULT CD3DVideoRenderer::SetFrameFormat(CANVAS &Canvas, DWORD StreamIndex, FRAME_FORMAT ff){
	return pImpl->SetFrameFormat( Canvas, StreamIndex, ff );
}

HRESULT CD3DVideoRenderer::SetPlanarAlpha(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, float alpha){
	return pImpl->SetPlanarAlpha( Canvas, StreamIndex, bEnable, alpha );
}

HRESULT CD3DVideoRenderer::Update( IMAGE &Image, BYTE *pFrame, long Stride ){
	return pImpl->Update( Image, pFrame, Stride );
}

HRESULT CD3DVideoRenderer::Update( IMAGE &Image, YUV_FRAME_INFO *pInfo ){
	return pImpl->Update( Image, pInfo );
}

HRESULT CD3DVideoRenderer::Update( IMAGE &Image, IDirect3DSurface9 *pSur ){
	return pImpl->Update( Image, pSur ) ;
}

HRESULT CD3DVideoRenderer::Update( IMAGE &Image, wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height ){
	return pImpl->Update( Image, str, LogFont, color, Width, Height );
}

HRESULT CD3DVideoRenderer::Render(CANVAS &Canvas, STREAM_INFO *pInfo, DWORD StreamCount, HWND hWnd, RECT *src, RECT *dst){
	return pImpl->Render( Canvas, pInfo, StreamCount, hWnd, src, dst );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

D3DVIDEORENDERER_API CD3DVideoRenderer* D3DRendererInit(HWND hWnd, IDirect3DDeviceManager9 **pManager,  DWORD nAdapterNum){
	HRESULT hr ;
	CD3DVideoRenderer *p = new CD3DVideoRenderer(hr, hWnd, pManager, nAdapterNum) ;
	if (p == nullptr || FAILED(hr)){
		delete p ;
		p = nullptr ;
	}
	return p ;
}

D3DVIDEORENDERER_API CD3DVideoRenderer* D3DRendererInit_Ex(IDirect3DDeviceManager9 *pManager){
	HRESULT hr ;
	CD3DVideoRenderer *p = new CD3DVideoRenderer(hr, pManager) ;
	if (p == nullptr || FAILED(hr)){
		delete p ;
		p = nullptr;
	}
	return p ;
}
D3DVIDEORENDERER_API void D3DRendererUninit(CD3DVideoRenderer* pRenderer){
	delete pRenderer ;
}

D3DVIDEORENDERER_API int  D3DRendererIsSupport ( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::COLOR_SPACE cs ){
	if (pRenderer == nullptr){
		return 0 ;
	}
	return pRenderer->IsSupport(cs);
}
D3DVIDEORENDERER_API DWORD D3DRendererMaxInputStreams( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *pCanvas ){
	if (pCanvas == nullptr || pRenderer == nullptr){
		return 0 ;
	}
	return pRenderer->MaxInputStreams(pCanvas);
}


D3DVIDEORENDERER_API HRESULT D3DRendererCreateCanvas( CD3DVideoRenderer* pRenderer, DWORD Width, DWORD Height, CD3DVideoRenderer::CANVAS **pCanvas){
	if (pRenderer == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Create( Width, Height, pCanvas ) ;
}

D3DVIDEORENDERER_API void D3DRendererDestroyCanvas( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *pCanvas ){
	if (pRenderer == nullptr){
		return ;
	}
	pRenderer->Destroy(pCanvas);
}

D3DVIDEORENDERER_API HRESULT D3DRendererCreateImage( CD3DVideoRenderer* pRenderer, DWORD Width, DWORD Height, CD3DVideoRenderer::COLOR_SPACE cs, CD3DVideoRenderer::IMAGE **pImage ){
	if (pRenderer == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Create( Width, Height, cs, pImage ) ;
}

D3DVIDEORENDERER_API HRESULT D3DRendererCreateText( CD3DVideoRenderer* pRenderer, wchar_t *str, LOGFONTW *LogFont, COLORREF color, DWORD *Width, DWORD *Height, CD3DVideoRenderer::IMAGE **pImage ){
	if (pRenderer == nullptr || LogFont == nullptr || Width == nullptr || Height == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Create( str, *LogFont, color, *Width, *Height, pImage ) ;
}

D3DVIDEORENDERER_API void D3DRendererDestroyImage( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *pImage ){
	if (pRenderer == nullptr){
		return ;
	}
	pRenderer->Destroy(pImage) ;
}

D3DVIDEORENDERER_API HRESULT D3DRendererSetBackgroundColor(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, float a, float r, float g, float b){
	if (pRenderer == nullptr || Canvas == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetBackgroundColor(*Canvas, a, r,g,b);
}

D3DVIDEORENDERER_API HRESULT D3DRendererSetCanvasTargetRect(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, BOOL bEnable, RECT *rect){
	if (pRenderer == nullptr || Canvas == nullptr || rect == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetTargetRect(*Canvas, bEnable, *rect);
}

D3DVIDEORENDERER_API HRESULT D3DRendererSetCanvasSourceRect(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, BOOL bEnable, RECT *rect){
	if (pRenderer == nullptr || Canvas == nullptr || rect == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetSourceRect(*Canvas, StreamIndex, bEnable, *rect);
}

D3DVIDEORENDERER_API HRESULT D3DRendererSetCanvasDestinationRect(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, BOOL bEnable, RECT *rect){
	if (pRenderer == nullptr || Canvas == nullptr || rect == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetDestinationRect(*Canvas, StreamIndex, bEnable, *rect);
}

D3DVIDEORENDERER_API HRESULT D3DRendererSetStreamFormat(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, CD3DVideoRenderer::COLOR_SPACE cs){
	if (pRenderer == nullptr || Canvas == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetStreamFormat(*Canvas, StreamIndex, cs);
}
	
D3DVIDEORENDERER_API HRESULT D3DRendererSetFrameFormat(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, CD3DVideoRenderer::FRAME_FORMAT ff){
	if (pRenderer == nullptr || Canvas == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetFrameFormat(*Canvas, StreamIndex, ff);
}

D3DVIDEORENDERER_API HRESULT D3DRendererSetPlanarAlpha(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, BOOL bEnable, float alpha){
	if (pRenderer == nullptr || Canvas == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->SetPlanarAlpha(*Canvas, StreamIndex, bEnable, alpha);
}

D3DVIDEORENDERER_API HRESULT D3DRendererUpdateImage( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *Image, BYTE *pFrame, long Stride ){
	if (pRenderer == nullptr || Image == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Update(*Image, pFrame, Stride);
}

D3DVIDEORENDERER_API HRESULT D3DRendererUpdateSurface( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *Image, IDirect3DSurface9 *pSur ){
	if (pRenderer == nullptr || Image == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Update(*Image, pSur);
}

D3DVIDEORENDERER_API HRESULT D3DRendererUpdateText( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *Image, wchar_t *str, LOGFONTW *LogFont, COLORREF color, DWORD *Width, DWORD *Height ){
	if (pRenderer == nullptr || Image == nullptr || str == nullptr || LogFont == nullptr || Width == nullptr || Height == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Update(*Image, str, *LogFont, color, *Width, *Height);
}

D3DVIDEORENDERER_API HRESULT D3DRendererRender( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, CD3DVideoRenderer::STREAM_INFO *pInfo, DWORD StreamCount, HWND hWnd, RECT *src, RECT *dst ){
	if (pRenderer == nullptr || Canvas == nullptr){
		return E_INVALIDARG ;
	}
	return pRenderer->Render(*Canvas, pInfo, StreamCount, hWnd, src, dst);
}