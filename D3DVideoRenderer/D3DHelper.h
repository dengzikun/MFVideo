//////////////////////////////////////////////////////////////////////
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#pragma once

const UINT BACK_BUFFER_COUNT = 1;


//-------------------------------------------------------------------
// D3DHelper
//
// Manages the Direct3D device.
//-------------------------------------------------------------------

class D3DHelper
{
	IDirect3D9Ex*       m_pD3D;
    	
	void DestroyD3D9()
    {
		if (m_pManager == nullptr)
		{
			if (m_pDevice != nullptr)
			{
				m_pDevice->Release () ;
				m_pDevice = nullptr;
			}

			if (m_pD3D != nullptr)
			{
				m_pD3D->Release () ;
				m_pD3D = nullptr;
			}
		}
    }

public:
	D3DPRESENT_PARAMETERS   m_d3dpp;
	IDirect3DDeviceManager9 *m_pManager ;
	IDirect3DDevice9Ex* m_pDevice;

public:
    D3DHelper() : 
		m_pD3D(nullptr),
		m_pDevice(nullptr),
		m_pManager(nullptr)
    {
        ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
    }

    ~D3DHelper()
    {
        DestroyD3D9();
    }

    HRESULT    InitializeD3D9(HWND hwnd, DWORD nAdapterNum, bool WaitVsync);
	HRESULT    InitializeD3D9(IDirect3DDeviceManager9 *pManager);
    BOOL    ResetDevice();
    HRESULT TestCooperativeLevel();

};

HRESULT LockDevice(
    IDirect3DDeviceManager9 *pDeviceManager,
    BOOL fBlock,
    IDirect3DDevice9 **ppDevice, // Receives a pointer to the device.
    HANDLE *pHandle              // Receives a device handle.   
    ) ;

class CDeviceManager
{
public:
	CDeviceManager( D3DHelper &D3D ) :
			m_pManager(D3D.m_pManager)
	{
		if (m_pManager != nullptr){
			LockDevice(m_pManager, TRUE, (IDirect3DDevice9**)&m_pDevice, &m_hDevice) ;
		}else{
			m_pDevice = D3D.m_pDevice ;
		}
	}

	~CDeviceManager()
	{
		if (m_pManager != nullptr){
			m_pManager->UnlockDevice(m_pDevice, TRUE); 
			m_pManager->CloseDeviceHandle(m_hDevice) ;
		}
	}

	IDirect3DDevice9Ex* GetDevice(){return m_pDevice;}

private:
	IDirect3DDeviceManager9 *m_pManager ;
	IDirect3DDevice9Ex* m_pDevice;
	HANDLE m_hDevice ;
} ;