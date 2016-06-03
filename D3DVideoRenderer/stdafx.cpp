// stdafx.cpp : 只包括标准包含文件的源文件
// D3DVideoRenderer.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"

// TODO: 在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用

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