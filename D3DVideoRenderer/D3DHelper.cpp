
#include "stdafx.h"
#include "D3DHelper.h"


HRESULT LockDevice(
    IDirect3DDeviceManager9 *pDeviceManager,
    BOOL fBlock,
    IDirect3DDevice9 **ppDevice, // Receives a pointer to the device.
    HANDLE *pHandle              // Receives a device handle.   
    ){
	*pHandle = nullptr;
	*ppDevice = nullptr;

    HANDLE hDevice = 0;

    HRESULT hr = pDeviceManager->OpenDeviceHandle(&hDevice);

    if (SUCCEEDED(hr)){
        hr = pDeviceManager->LockDevice(hDevice, ppDevice, fBlock);
    }

    if (hr == DXVA2_E_NEW_VIDEO_DEVICE){
        // Invalid device handle. Try to open a new device handle.
        hr = pDeviceManager->CloseDeviceHandle(hDevice);

        if (SUCCEEDED(hr)){
            hr = pDeviceManager->OpenDeviceHandle(&hDevice);
        }

        // Try to lock the device again.
        if (SUCCEEDED(hr)){
            hr = pDeviceManager->LockDevice(hDevice, ppDevice, TRUE); 
        }
    }

    if (SUCCEEDED(hr)){
        *pHandle = hDevice;
    }
    return hr;
}

HRESULT D3DHelper::InitializeD3D9(HWND hwnd, DWORD nAdapterNum, bool WaitVsync){
	HRESULT hr;
	DestroyD3D9();

	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);

	if (FAILED(hr)){
		DBGMSG(L"Direct3DCreate9Ex failed.\n");
		return hr;
	}

	ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));

	m_d3dpp.BackBufferWidth  = VIDEO_MAIN_WIDTH;
	m_d3dpp.BackBufferHeight = VIDEO_MAIN_HEIGHT;

	m_d3dpp.BackBufferFormat           = VIDEO_RENDER_TARGET_FORMAT;
	m_d3dpp.BackBufferCount            = BACK_BUFFER_COUNT;
	m_d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
	m_d3dpp.hDeviceWindow              = hwnd;
	m_d3dpp.Windowed                   = TRUE;
	m_d3dpp.Flags                      = D3DPRESENTFLAG_VIDEO;
	m_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	if ( WaitVsync ){
		m_d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_ONE;
	}else{
		m_d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;
	}

	/*
	D3DMULTISAMPLE_TYPE msType[15] = {
		D3DMULTISAMPLE_16_SAMPLES, D3DMULTISAMPLE_15_SAMPLES, D3DMULTISAMPLE_14_SAMPLES,
		D3DMULTISAMPLE_13_SAMPLES, D3DMULTISAMPLE_12_SAMPLES, D3DMULTISAMPLE_11_SAMPLES, 
		D3DMULTISAMPLE_10_SAMPLES, D3DMULTISAMPLE_9_SAMPLES, D3DMULTISAMPLE_8_SAMPLES,
		D3DMULTISAMPLE_7_SAMPLES, D3DMULTISAMPLE_6_SAMPLES, D3DMULTISAMPLE_5_SAMPLES,
		D3DMULTISAMPLE_4_SAMPLES, D3DMULTISAMPLE_3_SAMPLES, D3DMULTISAMPLE_2_SAMPLES
	};
	for ( int i = 0 ; i < sizeof(msType)/sizeof(D3DMULTISAMPLE_TYPE) ; ++i )
	{
		hr = m_pD3D->CheckDeviceMultiSampleType(nAdapterNum, D3DDEVTYPE_HAL, VIDEO_RENDER_TARGET_FORMAT, TRUE, msType[i], nullptr ) ;
		if ( SUCCEEDED(hr) )
		{
			m_d3dpp.MultiSampleType = msType[i] ;
			break ;
		}
	}*/

//	m_d3dpp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

/*
	//////////////////////////////////////////////////////////
	D3DCAPS9                d3d9caps;
    D3DOVERLAYCAPS          d3doverlaycaps = {0};
    CComPtr<IDirect3D9ExOverlayExtension> d3d9overlay = nullptr;
    bool overlaySupported = false;

    memset(&d3d9caps, 0, sizeof(d3d9caps));
    hr = m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3d9caps);
    if (FAILED(hr) || !(d3d9caps.Caps & D3DCAPS_OVERLAY))
    {
        overlaySupported = false;
    }
    else
    {
        hr = m_pD3D->QueryInterface(IID_PPV_ARGS(&d3d9overlay));
        if (FAILED(hr) || (d3d9overlay == nullptr))
        {
            overlaySupported = false;
        }
        else
        {
            hr = d3d9overlay->CheckDeviceOverlayType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                m_d3dpp.BackBufferWidth,
                m_d3dpp.BackBufferHeight,
                m_d3dpp.BackBufferFormat, nullptr,
                D3DDISPLAYROTATION_IDENTITY, &d3doverlaycaps);
            if (FAILED(hr))
            {
                overlaySupported = false;
            }
            else
            {
                overlaySupported = true;
            }
        }
    }
	///////////////////////////////////////////////////////////
	*/
	
	hr = m_pD3D->CreateDeviceEx(
		nAdapterNum, //D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hwnd,
		D3DCREATE_FPU_PRESERVE |
		D3DCREATE_MULTITHREADED |
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&m_d3dpp,
		nullptr,
		&m_pDevice
		);

	if (FAILED(hr)){
		DBGMSG(L"CreateDevice(HAL) failed with error 0x%x.\n", hr);
		return hr ;
	}
	
	if(hwnd){
		hr = m_pDevice->ResetEx(&m_d3dpp, nullptr);
        if (FAILED(hr))
            return hr;
		hr = m_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        if (FAILED(hr))
            return hr;
    }

	return hr;
}

HRESULT D3DHelper::InitializeD3D9(IDirect3DDeviceManager9 *pManager){
	DestroyD3D9();
	m_pManager = pManager ;
	return S_OK ;
}

BOOL D3DHelper::ResetDevice(){
	HRESULT hr;
	if (m_pDevice)
	{
		// Reset will change the parameters, so use a copy instead.

		D3DPRESENT_PARAMETERS d3dpp = m_d3dpp;

		hr = m_pDevice->Reset(&d3dpp);

		if (FAILED(hr)){
			DBGMSG(L"Reset failed with error 0x%x.\n", hr);
		}

		if (SUCCEEDED(hr)){
			return TRUE;
		}
	}

	return FALSE;
}

HRESULT D3DHelper::TestCooperativeLevel(){
	CDeviceManager dev(*this);
	HRESULT hr = dev.GetDevice()->TestCooperativeLevel();

	if (hr == D3DERR_DEVICENOTRESET){
		if (ResetDevice()){
			hr = D3D_OK;
		}
	}

	return hr;
}