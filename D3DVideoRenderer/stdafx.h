// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ���ų�����ʹ�õ�����
// Windows ͷ�ļ�:
#include <windows.h>



// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�


#include <dwmapi.h>
#include <d3d9.h>


#pragma warning(push)
#pragma warning(disable:4201)  // Disable warning C4201: nonstandard extension used

#include <Dxva2api.h>
#include <dxvahd.h>

#pragma warning(pop)

#include "dxvahd_utils.h"

#include "atlbase.h"

#include <vector>
#include <string>
#include <algorithm>

void DisplayError(HWND hwnd, WCHAR *pszMessage);
void DBGMSG(PCWSTR format, ...);

const UINT DWM_BUFFER_COUNT  = 4;

// Frame rate
const UINT VIDEO_FPS     = 60;
const UINT VIDEO_MSPF    = (1000 + VIDEO_FPS / 2) / VIDEO_FPS;
const UINT VIDEO_100NSPF = VIDEO_MSPF * 10000;

// Formats
const D3DFORMAT VIDEO_RENDER_TARGET_FORMAT = D3DFMT_X8R8G8B8;
const D3DFORMAT VIDEO_MAIN_FORMAT          = D3DFMT_YUY2;
const D3DFORMAT VIDEO_SUB_FORMAT           = D3DFORMAT('VUYA'); // AYUV

// Primary stream
const UINT VIDEO_MAIN_WIDTH  = 1920;
const UINT VIDEO_MAIN_HEIGHT = 1080;
const RECT VIDEO_MAIN_RECT   = {0, 0, VIDEO_MAIN_WIDTH, VIDEO_MAIN_HEIGHT};

const UINT DEST_BORDER = 20;
const RECT VIDEO_MAIN_DEST_RECT = { DEST_BORDER, DEST_BORDER, VIDEO_MAIN_WIDTH - DEST_BORDER, VIDEO_MAIN_HEIGHT - DEST_BORDER }; 

// Substreams
const UINT SUB_STREAM_COUNT  = 1;

const UINT VIDEO_SUB_WIDTH   = 128;
const UINT VIDEO_SUB_HEIGHT  = 128;

const UINT VIDEO_SUB_SURF_WIDTH = VIDEO_SUB_WIDTH * 3;
const UINT VIDEO_SUB_SURF_HEIGHT = VIDEO_SUB_HEIGHT * 3;
const RECT VIDEO_SUB_RECT = { VIDEO_SUB_WIDTH, VIDEO_SUB_HEIGHT };

const UINT VIDEO_SUB_VX  = 3;   // horizontal velocity
const UINT VIDEO_SUB_VY  = 2;   // vertical velocity

// Default alpha values
const BYTE DEFAULT_PLANAR_ALPHA_VALUE = 0xFF;
const BYTE DEFAULT_PIXEL_ALPHA_VALUE  = 0x80;


