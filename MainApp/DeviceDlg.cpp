// DeviceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MainApp.h"
#include "DeviceDlg.h"


// CDeviceDlg 对话框

IMPLEMENT_DYNAMIC(CDeviceDlg, CDialog)

CDeviceDlg::CDeviceDlg(vector<DEVICE_INFO> &dev,DWORD DeviceId, DWORD CSId, DWORD FSId, CWnd* pParent /*=NULL*/)
	: CDialog(CDeviceDlg::IDD, pParent),
		vec_Device(dev),
		m_DeviceId(DeviceId),
		m_CSId(CSId),
		m_FSId(FSId)
		, m_FrameRateN(1)
		, m_FrameRateD(1)
		, m_FrameRate(_T(""))
		, m_FrameRateMin(_T(""))
		, m_FrameRateMax(_T(""))
{

}

CDeviceDlg::~CDeviceDlg()
{
}

void CDeviceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DEVICE, m_ComboDevice);
	DDX_Control(pDX, IDC_COMBO_COLORSPACE, m_ComboCS);
	DDX_Control(pDX, IDC_COMBO_FRAMESIZE, m_ComboFS);
	DDX_Text(pDX, IDC_EDIT_FRAME_RATE_N, m_FrameRateN);
	DDX_Text(pDX, IDC_EDIT_FRAME_RATE_D, m_FrameRateD);
	DDX_Text(pDX, IDC_STATIC_FRAME_RATE, m_FrameRate);
	DDX_Text(pDX, IDC_STATIC_FRAMERATE_MIN, m_FrameRateMin);
	DDX_Text(pDX, IDC_STATIC_FRAMERATE_MAX, m_FrameRateMax);
	DDV_MinMaxUInt(pDX, m_FrameRateN, 1, ULONG_MAX);
	DDV_MinMaxUInt(pDX, m_FrameRateD, 1, ULONG_MAX);
}


BEGIN_MESSAGE_MAP(CDeviceDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_FRAMESIZE, &CDeviceDlg::OnCbnSelchangeComboFramesize)
	ON_CBN_SELCHANGE(IDC_COMBO_COLORSPACE, &CDeviceDlg::OnCbnSelchangeComboColorspace)
	ON_CBN_SELCHANGE(IDC_COMBO_DEVICE, &CDeviceDlg::OnCbnSelchangeComboDevice)
	ON_EN_CHANGE(IDC_EDIT_FRAME_RATE_N, &CDeviceDlg::OnEnChangeEditFrameRateN)
	ON_EN_CHANGE(IDC_EDIT_FRAME_RATE_D, &CDeviceDlg::OnEnChangeEditFrameRateD)
END_MESSAGE_MAP()


// CDeviceDlg 消息处理程序

BOOL CDeviceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化

	InitDevice() ;
	InitCS() ;
	InitFS() ;
	InitFR() ;
	InitFRMin() ;
	InitFRMax() ;

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CDeviceDlg::InitDevice(void)
{
	m_ComboDevice.ResetContent() ;
	for ( size_t i = 0 ; i < vec_Device.size() ; ++i )
	{
		m_ComboDevice.AddString ( vec_Device[i].name.c_str() ) ;
	}

	m_ComboDevice.SetCurSel ( m_DeviceId ) ;
}


void CDeviceDlg::InitCS(void)
{
	if ( m_DeviceId >= vec_Device.size() )
	{
		return ;
	}

	m_ComboCS.ResetContent() ;
	string str ;
	for ( size_t i = 0 ; i < vec_Device[m_DeviceId].vecFormat.size() ; ++i )
	{
		if ( str != vec_Device[m_DeviceId].vecFormat[i].ColorSpace )
		{
			int idx = m_ComboCS.AddString (vec_Device[m_DeviceId].vecFormat[i].ColorSpace ) ;
			if ( idx >= 0 )
			{
				str = vec_Device[m_DeviceId].vecFormat[i].ColorSpace ;
			}
		}
	}

	m_ComboCS.SetCurSel ( m_CSId ) ;
}

void CDeviceDlg::InitFS(void)
{
	if ( m_DeviceId >= vec_Device.size() )
	{
		return ;
	}

	m_ComboFS.ResetContent() ;
	char buf[512] = { 0 };
	m_ComboCS.GetLBText(m_CSId, buf ) ;
	string str = buf ;

	for ( size_t i = 0 ; i < vec_Device[m_DeviceId].vecFormat.size() ; ++i )
	{
		if ( str == vec_Device[m_DeviceId].vecFormat[i].ColorSpace )
		{
			CMFVideoCapture::VIDEO_FORMAT &vf = vec_Device[m_DeviceId].vecFormat[i] ;
			CString s ;
			s.Format ( " %-6uX %-6u  %6.2f ~ %6.2f", vf.Width, vf.Height, vf.FrameRateMin.Numerator *1.0f / vf.FrameRateMin.Denominator, vf.FrameRateMax.Numerator *1.0f / vf.FrameRateMax.Denominator ) ;
			int idx = m_ComboFS.AddString ( s ) ;
			if ( idx >= 0 )
			{
				m_ComboFS.SetItemData ( idx, i ) ;
			}
		}
	}

	m_ComboFS.SetCurSel ( m_FSId ) ;
	m_FormatId = m_ComboFS.GetItemData ( m_FSId ) ;
}

void CDeviceDlg::InitFR(void)
{
	if ( m_DeviceId >= vec_Device.size() )
	{
		return ;
	}

	if ( m_FormatId >= vec_Device[m_DeviceId].vecFormat.size() )
	{
		return ;
	}

	DWORD idx = m_ComboFS.GetItemData ( m_FSId ) ;
	m_FrameRateN = vec_Device[m_DeviceId].vecFormat[idx].FrameRate.Numerator ;
	m_FrameRateD = vec_Device[m_DeviceId].vecFormat[idx].FrameRate.Denominator ;

	m_FrameRate.Format( "= %.2f fps", m_FrameRateN*1.0f/m_FrameRateD ) ;
	UpdateData ( FALSE ) ;
}

void CDeviceDlg::InitFRMax(void)
{
	if ( m_DeviceId >= vec_Device.size() )
	{
		return ;
	}

	if ( m_FormatId >= vec_Device[m_DeviceId].vecFormat.size() )
	{
		return ;
	}

	DWORD idx = m_ComboFS.GetItemData ( m_FSId ) ;
	DWORD n = vec_Device[m_DeviceId].vecFormat[idx].FrameRateMax.Numerator ;
	DWORD d = vec_Device[m_DeviceId].vecFormat[idx].FrameRateMax.Denominator ;
	m_FrameRateMax.Format( "%u ÷ %u = %.2f fps", n, d, n*1.0f/d ) ;
	UpdateData ( FALSE ) ;
}

void CDeviceDlg::InitFRMin(void)
{
	if ( m_DeviceId >= vec_Device.size() )
	{
		return ;
	}

	if ( m_FormatId >= vec_Device[m_DeviceId].vecFormat.size() )
	{
		return ;
	}

	DWORD idx = m_ComboFS.GetItemData ( m_FSId ) ;
	DWORD n = vec_Device[m_DeviceId].vecFormat[idx].FrameRateMin.Numerator ;
	DWORD d = vec_Device[m_DeviceId].vecFormat[idx].FrameRateMin.Denominator ;
	m_FrameRateMin.Format( "%u ÷ %u = %.2f fps", n, d, n*1.0f/d ) ;
	UpdateData ( FALSE ) ;
}


void CDeviceDlg::OnCbnSelchangeComboDevice()
{
	// TODO: 在此添加控件通知处理程序代码

	m_DeviceId = m_ComboDevice.GetCurSel () ;
	m_CSId = 0 ;
	m_FSId = 0 ;

	InitCS() ;
	InitFS();
	InitFR() ;
	InitFRMin() ;
	InitFRMax() ;
}


void CDeviceDlg::OnCbnSelchangeComboColorspace()
{
	// TODO: 在此添加控件通知处理程序代码

	m_CSId = m_ComboCS.GetCurSel () ;
	m_FSId = 0 ;
	InitFS();
	InitFR() ;
	InitFRMin() ;
	InitFRMax() ;
}

void CDeviceDlg::OnCbnSelchangeComboFramesize()
{
	// TODO: 在此添加控件通知处理程序代码
	m_FSId = m_ComboFS.GetCurSel () ;
	m_FormatId = m_ComboFS.GetItemData ( m_FSId ) ;
	InitFR() ;
	InitFRMin() ;
	InitFRMax() ;
}
void CDeviceDlg::OnEnChangeEditFrameRateN()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	DWORD n = m_FrameRateN ;
	UpdateData() ;
	if ( n != m_FrameRateN )
	{
		m_FrameRate.Format( "= %.2f fps", m_FrameRateN*1.0f/m_FrameRateD ) ;
		UpdateData (FALSE) ;

		if ( m_DeviceId >= vec_Device.size() )
		{
			return ;
		}

		if ( m_FormatId >= vec_Device[m_DeviceId].vecFormat.size() )
		{
			return ;
		}

		DWORD idx = m_ComboFS.GetItemData ( m_FSId ) ;
		vec_Device[m_DeviceId].vecFormat[idx].FrameRateMin.Numerator = m_FrameRateN ;
	}
}

void CDeviceDlg::OnEnChangeEditFrameRateD()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

	DWORD d = m_FrameRateD ;
	UpdateData() ;
	if ( d != m_FrameRateD )
	{
		m_FrameRate.Format( "= %.2f fps", m_FrameRateN*1.0f/m_FrameRateD ) ;
		UpdateData (FALSE) ;

		if ( m_DeviceId >= vec_Device.size() )
		{
			return ;
		}

		if ( m_FormatId >= vec_Device[m_DeviceId].vecFormat.size() )
		{
			return ;
		}

		DWORD idx = m_ComboFS.GetItemData ( m_FSId ) ;
		vec_Device[m_DeviceId].vecFormat[idx].FrameRateMin.Denominator = m_FrameRateD ;
	}
}
