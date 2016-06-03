#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CDeviceDlg 对话框

class CDeviceDlg : public CDialog
{
	DECLARE_DYNAMIC(CDeviceDlg)

public:
	CDeviceDlg(vector<DEVICE_INFO> &dev, DWORD DeviceId, DWORD CSId, DWORD FSId, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDeviceDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_DEVICE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_ComboDevice;
	virtual BOOL OnInitDialog();
	CComboBox m_ComboCS;
	CComboBox m_ComboFS;
	afx_msg void OnCbnSelchangeComboFramesize();
	afx_msg void OnCbnSelchangeComboColorspace();
	afx_msg void OnCbnSelchangeComboDevice();
private:
	void InitDevice(void);
	void InitCS(void);
	void InitFS(void);

public:
	DWORD m_DeviceId ;
	DWORD m_FormatId ;

	DWORD m_CSId ;
	DWORD m_FSId ;

private:
	void InitFR(void);
public:
	DWORD m_FrameRateN;
	DWORD m_FrameRateD;
	CString m_FrameRate;
	CString m_FrameRateMin;
	CString m_FrameRateMax;
	vector<DEVICE_INFO> &vec_Device ;
private:
	void InitFRMax(void);
public:
	void InitFRMin(void);
	afx_msg void OnEnChangeEditFrameRateN();
	afx_msg void OnEnChangeEditFrameRateD();
};
