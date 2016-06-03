
// MainAppDlg.h : ͷ�ļ�
//

#pragma once


// CMainAppDlg �Ի���
class CMainAppDlg : public CDialog
{
// ����
public:
	CMainAppDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MAINAPP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnDevice();
private:
	void EnumDevice(void);
	void ResizeWindow(UINT Width, UINT Height);
	bool IsRendererSupport(void);

	void OnData(BYTE *pData, DWORD DataSize, long lStride, DWORD ts);
};
