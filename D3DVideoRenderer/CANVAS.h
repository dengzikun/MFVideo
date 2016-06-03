#pragma once


class CD3DVideoRenderer::CANVAS {
public:

	CANVAS(void) :
		frame(0){
		stream_data.resize(8) ;
		ZeroMemory(&stream_data[0], sizeof(stream_data)*stream_data.size());
	}

	~CANVAS(void){
	}

	HRESULT Create (IDirect3DDevice9Ex *pD3DDevice, IDXVAHD_Device *pDXVA, DWORD Width, DWORD Height){
		CComPtr<IDirect3DSwapChain9> sc ;
		HRESULT hr = pD3DDevice->GetSwapChain(0,&sc) ;
		if ( FAILED(hr) ){
			return hr ;
		}

		hr = sc->GetPresentParameters(&m_d3dpp);
		if ( FAILED(hr) ){
			return hr ;
		}

		m_d3dpp.BackBufferWidth = Width ;
		m_d3dpp.BackBufferHeight = Height ;
		hr = pD3DDevice->CreateAdditionalSwapChain ( &m_d3dpp, &m_pSwapChain ) ;
		if ( FAILED(hr) ){
			return hr ;
		}
		return Create( pDXVA, Width, Height ) ;
	}

	HRESULT SetBackgroundColor( float a, float r, float g, float b){
		DXVAHD_COLOR clr ;
		clr.RGB.A = a ;
		clr.RGB.R = r ;
		clr.RGB.G = g ;
		clr.RGB.B = b ;
		return DXVAHD_SetBackgroundColor(m_pDXVAVP, FALSE, clr); 
	}

	HRESULT SetTargetRect(BOOL bEnable, RECT &rect){
		return DXVAHD_SetTargetRect(m_pDXVAVP, bEnable, rect);
	}

	HRESULT SetSourceRect(DWORD StreamIndex, BOOL bEnable, RECT &rect){
		return DXVAHD_SetSourceRect(m_pDXVAVP, StreamIndex, bEnable, rect) ;
	}

	HRESULT SetDestinationRect(DWORD StreamIndex, BOOL bEnable, RECT &rect){
		return DXVAHD_SetDestinationRect(m_pDXVAVP, StreamIndex, bEnable, rect) ;
	}

	HRESULT SetStreamFormat(DWORD StreamIndex, COLOR_SPACE cs){
		if ( cs == FOURCC_YV12 )
		{
			cs = FOURCC_NV12 ;
		}

		HRESULT hr = SetFilterValue(StreamIndex, 0, 1);
		return DXVAHD_SetStreamFormat(m_pDXVAVP, StreamIndex, (D3DFORMAT)cs) ;
	}

	HRESULT SetFrameFormat(DWORD StreamIndex, FRAME_FORMAT ff){
		return DXVAHD_SetFrameFormat(m_pDXVAVP, StreamIndex, (DXVAHD_FRAME_FORMAT)ff);
	}

	HRESULT SetPlanarAlpha(DWORD StreamIndex, BOOL bEnable, float alpha){
		return DXVAHD_SetPlanarAlpha(m_pDXVAVP, StreamIndex, bEnable, alpha);
	}

	HRESULT SetFilterValue(DWORD StreamIndex, int index, int value){
		return DXVAHD_SetFilterValue(m_pDXVAVP, StreamIndex, DXVAHD_FILTER_NOISE_REDUCTION, TRUE, value);
	}
	
	HRESULT Draw(STREAM_INFO *pInfo, DWORD StreamCount){
		if ( stream_data.size() < StreamCount )
		{
			stream_data.clear();
			stream_data.resize(StreamCount);
			ZeroMemory(&stream_data[0], sizeof(stream_data)*stream_data.size());
		}

		HRESULT hr ;

		for ( DWORD i = 0, j = 0 ; i < StreamCount ; ++i ){
			if (pInfo[i].pInputImage->m_pSur == nullptr){
				continue ;
			}
			stream_data[j].Enable			 = pInfo[i].bEnable;
			stream_data[j].OutputIndex		 = pInfo[i].OutputIndex;
			stream_data[j].InputFrameOrField = pInfo[i].InputFrameOrField;
			stream_data[j].pInputSurface	 = pInfo[i].pInputImage->m_pSur;
			stream_data[j].PastFrames		 = pInfo[i].PastFrames ;
			stream_data[j].FutureFrames		 = pInfo[i].FutureFrames ;

			if ( pInfo[i].PastFrames > 0 ){
				if ( pInfo[i].PastFrames > m_pPastSurfaces.size() ){
					m_pPastSurfaces.resize(pInfo[i].PastFrames) ;
				}

				for ( UINT k = 0 ; k < pInfo[i].PastFrames ; ++k ){
					m_pPastSurfaces[k] = pInfo[i].ppPastImages[k]->m_pSur ;
				}
				stream_data[j].ppPastSurfaces = &m_pPastSurfaces[0] ;
			}

			if ( pInfo[i].FutureFrames > 0 ){
				if ( pInfo[i].FutureFrames > m_pFutureSurfaces.size() ){
					m_pFutureSurfaces.resize(pInfo[i].FutureFrames) ;
				}

				for ( UINT k = 0 ; k < pInfo[i].FutureFrames ; ++k ){
					m_pFutureSurfaces[k] = pInfo[i].ppFutureImages[k]->m_pSur ;
				}
				stream_data[j].ppFutureSurfaces = &m_pFutureSurfaces[0];
			}
			++j ;
		}

		CComPtr<IDirect3DSurface9> pRT ;
		hr = m_pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pRT);
		if ( FAILED(hr) ){
			return hr ;
		}
		hr = m_pDXVAVP->VideoProcessBltHD( pRT, frame, StreamCount, &stream_data[0] );
		return hr ;
	}

	HRESULT Render(HWND hWnd, RECT *src, RECT *dst){
		HRESULT hr = m_pSwapChain->Present(src, dst, hWnd, nullptr, 0);
		return hr ;
	}


private:

	HRESULT Create( IDXVAHD_Device *pDXVA, DWORD Width, DWORD Height ){
		ZeroMemory(&caps, sizeof(caps));

		HRESULT hr = pDXVA->GetVideoProcessorDeviceCaps(&caps);
		if ( FAILED(hr) ){
			return hr ;
		}

		DXVAHD_FILTER_RANGE_DATA data ;
		if ( caps.FilterCaps & DXVAHD_FILTER_CAPS_NOISE_REDUCTION ){
			hr = pDXVA->GetVideoProcessorFilterRange(DXVAHD_FILTER_NOISE_REDUCTION, &data);
		}
		if ( caps.FilterCaps & DXVAHD_FILTER_CAPS_EDGE_ENHANCEMENT ){
			hr = pDXVA->GetVideoProcessorFilterRange(DXVAHD_FILTER_EDGE_ENHANCEMENT, &data);
		}

		std::vector<DXVAHD_VPCAPS> pVPCaps ;
		pVPCaps.resize(caps.VideoProcessorCount) ;

		hr = pDXVA->GetVideoProcessorCaps(caps.VideoProcessorCount, &pVPCaps[0]);
		if (FAILED(hr)){
			return hr ;
		}

		hr = pDXVA->CreateVideoProcessor(&pVPCaps[0].VPGuid, &m_pDXVAVP);
		return hr;
	}


private:
	CComPtr<IDXVAHD_VideoProcessor> m_pDXVAVP;
	CComPtr<IDirect3DSwapChain9>    m_pSwapChain ;
	D3DPRESENT_PARAMETERS m_d3dpp ;
	DXVAHD_VPDEVCAPS caps;

	DWORD frame ;

	std::vector<DXVAHD_STREAM_DATA> stream_data ;

	std::vector<IDirect3DSurface9*> m_pPastSurfaces ;
	std::vector<IDirect3DSurface9*> m_pFutureSurfaces ;

	wchar_t m_strText[256] ;
	LOGFONTW m_LogFont ;
	RECT  m_textRect ;
	DWORD   m_dwTexWidth;                 // Texture dimensions
	DWORD   m_dwTexHeight;
	IDirect3DSurface9 *pSur;
};
