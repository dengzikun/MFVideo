// stdafx.cpp : ֻ������׼�����ļ���Դ�ļ�
// D3DVideoRenderer.pch ����ΪԤ����ͷ
// stdafx.obj ������Ԥ����������Ϣ

#include "stdafx.h"

// TODO: �� STDAFX.H ��
// �����κ�����ĸ���ͷ�ļ����������ڴ��ļ�������

#include <assert.h>
#include <strsafe.h>

void DBGMSG(PCWSTR format, ...)
{
    va_list args;
    va_start(args, format);

    WCHAR string[MAX_PATH];

    if (SUCCEEDED(StringCbVPrintf(string, sizeof(string), format, args)))
    {
        OutputDebugString(string);
    }
    else
    {
        DebugBreak();
    }
}

void DisplayError(HWND hwnd, WCHAR *pszMessage)
{
    MessageBox(hwnd, pszMessage, L"Error", MB_OK | MB_ICONERROR);
}