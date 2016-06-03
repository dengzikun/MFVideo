
// MainAppDlg.cpp : 实现文件
//

#include "stdafx.h"
#include <memory>
#include "MainApp.h"
#include "MainAppDlg.h"
#include "DeviceDlg.h"
#include "FF_Codec.h"

extern "C"
{
#include "libswscale/swscale.h"
}

#pragma comment ( lib, "swscale.lib" )
#pragma comment ( lib, "FF_Codec.lib" )

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


unique_ptr<CD3DVideoRenderer> pRenderer;
CD3DVideoRenderer::CANVAS *pCanvas ;
CD3DVideoRenderer::IMAGE  *pImage ;

SwsContext *sws_ctx;

#define C_STYLE




bool MJPEG = false;

//CFile f;

class Cap : public CMFVideoCapture
{
public:
	Cap(HRESULT &hr) :
	  CMFVideoCapture(hr),
	  pDec(nullptr, FFVideoDecoder_Destroy)
	{
//		f.Open(L"b.yuv", CFile::modeCreate | CFile::modeWrite);

		
	}
public:
	virtual void OnMessage(const char* Msg) override
	{
		printf ( "Capture: %s\n", Msg ) ;
	}
public:
	virtual void OnData(BYTE *pData, DWORD DataSize, long lStride, DWORD ts) override
	{
		if (MJPEG){
			ff_input.pBitStream = pData;
			ff_input.bitStreamLen = DataSize;
			int ret = FFVideoDecoder_Decode(&ff_input);
			return;
		}

		pRenderer->Update(*pImage, pData, lStride);

		CD3DVideoRenderer::STREAM_INFO info[1] ;
		memset ( info, 0, sizeof(CD3DVideoRenderer::STREAM_INFO) ) ;

		info[0].pInputImage = pImage ;
		info[0].bEnable = TRUE ;
		info[0].InputFrameOrField = 0 ;

		pRenderer->Render ( *pCanvas, info, 1 ) ;
		
		DWORD t2 = GetTickCount () ;
		static DWORD t1 = GetTickCount () ;

		DWORD diff = t2 - t1 ;
		static DWORD count = 0 ;
		++count ;
		if ( diff >= 5000 )
		{
			printf ( "frame_rate: %.2f\n", count * 1000.0f / diff ) ;
			t1 = t2 ;
			count = 0 ;
		}
	}

	int ffmpeg_callback(FFVIDEODECODER_OUTPUT *output){

		CD3DVideoRenderer::YUV_FRAME_INFO frame_info;
		frame_info.yuv[0] = output->yuv[0];
		frame_info.yuv[1] = output->yuv[1];
		frame_info.yuv[2] = output->yuv[2];

		frame_info.stride[0] = output->stride[0];
		frame_info.stride[1] = output->stride[1];
		frame_info.stride[2] = output->stride[2];
		pRenderer->Update(*pImage, &frame_info);//*/

		CD3DVideoRenderer::STREAM_INFO info[1];
		memset(info, 0, sizeof(CD3DVideoRenderer::STREAM_INFO));

		info[0].pInputImage = pImage;
		info[0].bEnable = TRUE;
		info[0].InputFrameOrField = 0;

		pRenderer->Render(*pCanvas, info, 1);
		return 0;
	}

	void Reset(){
		auto callback = [](FFVIDEODECODER_OUTPUT *output){return ((Cap*)output->userData)->ffmpeg_callback(output); };

		//8: MJPEG
		pDec.reset(FFVideoDecoder_Create(8, 1, callback, this));

		ff_input.hDecoder = pDec.get();
		ff_input.corrupted = 0;

		MJPEG = true;
	}

private:
	unique_ptr<std::remove_pointer<HFFVIDEODECODER>::type, void(*)(HFFVIDEODECODER)> pDec;
	FFVIDEODECODER_INPUT ff_input;
} ;

//#ifdef C_STYLE
unique_ptr<std::remove_pointer<HMFVIDEODCAPTURE>::type, void(*)(HMFVIDEODCAPTURE)> pCapture1(nullptr, MFVideoCapture_Destroy);
//#else
unique_ptr<Cap> pCapture;
//#endif


vector<DEVICE_INFO> vec_Device ;

DWORD m_DeviceId ;
DWORD m_FormatId ;
DWORD m_CSId ;
DWORD m_FSId ;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMainAppDlg 对话框




CMainAppDlg::CMainAppDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainAppDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMainAppDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_COMMAND(ID_DEVICE, &CMainAppDlg::OnDevice)
END_MESSAGE_MAP()


// CMainAppDlg 消息处理程序

BOOL CMainAppDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	AllocConsole () ;
	freopen( "CONOUT$", "a", stdout ) ;

	enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P;
	enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;
	int src_w = 1920, src_h = 1080, dst_w=1280, dst_h=720;
	sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
		dst_w, dst_h, dst_pix_fmt,
		SWS_BILINEAR, NULL, NULL, NULL);


	HRESULT hr ;
//#ifdef C_STYLE
	MF_VIDEOCAPTURE_INIT params;
	auto callback = [](MF_VIDEOCAPTURE_OUTPUT *output){pCapture->OnData(output->data, output->dataSize, output->stride, output->timeStamp); return 0; };
	params.dataCallBack = callback;
	params.msgCallBack = nullptr;
	params.userData = this;
	pCapture1.reset(MFVideoCapture_Create(&params));
	if (pCapture1 == nullptr){
		MessageBox("MFVIDEOCAPTURE_Create failed", NULL, MB_ICONERROR);
		return FALSE;
	}
//#else
	pCapture.reset(new Cap (hr)) ;
	if ( FAILED(hr) )
	{
		_com_error err(hr) ;
		MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
		return FALSE;
	}
//#endif
	
	UINT count = CD3DVideoRenderer::GetAdapterCount();
	for ( UINT i = 0 ; i < count ; ++i )
	{
		const char* p = CD3DVideoRenderer::GetAdapterDescription(i);
		if ( p != NULL )
		{
			string str = p ;
			string::size_type index = str.find("AMD") ;
			if ( index != string::npos )
			printf ("%s\n", p);
		}
	}

	IDirect3DDeviceManager9 *pManager ;
	pRenderer.reset( new CD3DVideoRenderer ( hr, this->GetSafeHwnd(), &pManager, 0 ) ) ;
	if ( FAILED(hr) )
	{
		_com_error err(hr) ;
		MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
		return FALSE;
	}
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}



void CMainAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMainAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMainAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMainAppDlg::ResizeWindow(UINT Width, UINT Height)
{
	RECT rect ;
	GetClientRect(&rect ) ;

	RECT rect1 ;
	GetWindowRect(&rect1 ) ;
	long CX = rect1.right - rect1.left ;
	long CY = rect1.bottom - rect1.top ;
	long cx = rect.right - rect.left ;
	long cy = rect.bottom - rect.top ;
	CX = CX - cx + Width ;
	CY = CY - cy + Height ;
	::SetWindowPos ( this->GetSafeHwnd(), HWND_TOP, 0, 0, CX, CY, SWP_NOMOVE ) ;

	InvalidateRect ( NULL ) ;
}

bool CMainAppDlg::IsRendererSupport(void)
{
	MJPEG = false;
	UINT Width  = vec_Device[m_DeviceId].vecFormat[m_FormatId].Width ;
	UINT Height = vec_Device[m_DeviceId].vecFormat[m_FormatId].Height ;

	CString s = "renderer module not support " ;
	s += vec_Device[m_DeviceId].vecFormat[m_FormatId].ColorSpace ;
	bool b = false ;
	if ( vec_Device[m_DeviceId].vecFormat[m_FormatId].Fcc == CD3DVideoRenderer::FOURCC_YUY2 )
	{
		b = pRenderer->IsSupport ( CD3DVideoRenderer::FOURCC_YUY2 ) ;

		if ( !b )
		{
			MessageBox ( s, NULL, MB_ICONERROR ) ;
			return b;
		}
	}
	else if ( vec_Device[m_DeviceId].vecFormat[m_FormatId].Fcc == CD3DVideoRenderer::FOURCC_NV12 )
	{
		b = pRenderer->IsSupport ( CD3DVideoRenderer::FOURCC_NV12 ) ;

		if ( !b )
		{
			MessageBox ( s, NULL, MB_ICONERROR ) ;
			return b;
		}
	}
	else if ( vec_Device[m_DeviceId].vecFormat[m_FormatId].Fcc == CD3DVideoRenderer::FOURCC_YV12 )
	{
		b = pRenderer->IsSupport ( CD3DVideoRenderer::FOURCC_YV12 ) ;

		if ( !b )
		{
			MessageBox ( s, NULL, MB_ICONERROR ) ;
			return b;
		}
	}
	else //MJPEG
	{
//		MessageBox ( s, NULL, MB_ICONERROR ) ;
//		return false;
//		pDec.reset(FFVideoDecoder_Create(8, 1)); // 8: MJPEG
//		ff_info.hDecoder = pDec.get();

		pCapture->Reset();
		return true;
	}
	return true ;
}

void CMainAppDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 在此处添加消息处理程序代码

	
#ifdef C_STYLE
	if (pCapture1){
		MFVideoCapture_CloseDevice(pCapture1.get());
	}
#else
	if (pCapture){
		pCapture->CloseDevice() ;
	}
#endif

	if ( pRenderer ){
		pRenderer->Destroy ( pImage ) ;
		pRenderer->Destroy ( pCanvas ) ;
	}
}

void CMainAppDlg::EnumDevice(void)
{
	vec_Device.clear() ; 

	DWORD Count ;

#ifndef C_STYLE
	HRESULT hr = pCapture->GetDeviceCount ( Count ) ;
	if ( FAILED(hr) )
	{
		_com_error err(hr) ;
		MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
		return  ;
	}

	char id[256], name[256];
	for ( DWORD i = 0 ; i < Count ; ++i )
	{
		hr = pCapture->EnumDevice ( i, id, name ) ;
		if ( FAILED(hr) )
		{
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			continue ;
		}
		DEVICE_INFO info ;
		info.id   = id ;
		info.name = name ;

		DWORD FormatCount ;
		hr = pCapture->GetFormatCount(id, FormatCount ) ;
		if ( FAILED(hr) )
		{
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			continue ;
		}
		
		for ( DWORD j = 0 ; j < FormatCount ; ++j )
		{
			CMFVideoCapture::VIDEO_FORMAT vf ;
			hr = pCapture->EnumFormat ( id, j, vf ) ;
			if ( FAILED(hr) )
			{
				_com_error err(hr) ;
				MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
				continue ;
			}
			info.vecFormat.push_back (vf) ;
		}

		vec_Device.push_back (info) ;
	}
#else
	HRESULT hr = MFVideoCapture_GetDeviceCount(pCapture1.get(), &Count);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
		return;
	}

	char id[256], name[256];
	for (DWORD i = 0; i < Count; ++i)
	{
		hr = MFVideoCapture_EnumDevice(pCapture1.get(), i, id, name);
		if (FAILED(hr))
		{
			_com_error err(hr);
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			continue;
		}
		DEVICE_INFO info;
		info.id = id;
		info.name = name;

		DWORD FormatCount;
		hr = MFVideoCapture_GetFormatCount(pCapture1.get(), id, &FormatCount);
		if (FAILED(hr))
		{
			_com_error err(hr);
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			continue;
		}

		for (DWORD j = 0; j < FormatCount; ++j)
		{
			CMFVideoCapture::VIDEO_FORMAT vf;
			hr = MFVideoCapture_EnumFormat(pCapture1.get(), id, j, &vf);
			if (FAILED(hr))
			{
				_com_error err(hr);
				MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
				continue;
			}
			info.vecFormat.push_back(vf);
		}

		vec_Device.push_back(info);
	}
#endif
}

void CMainAppDlg::OnDevice()
{
	// TODO: 在此添加命令处理程序代码

	EnumDevice () ;

	HRESULT hr ;

	CDeviceDlg dlg(vec_Device, m_DeviceId, m_CSId, m_FSId ) ;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK){
		if ( m_DeviceId >= vec_Device.size() ){
			return ;
		}

		if ( m_FormatId >= vec_Device[m_DeviceId].vecFormat.size() ){
			return ;
		}

		m_DeviceId = dlg.m_DeviceId ;
		m_FormatId = dlg.m_FormatId ;
		vec_Device[m_DeviceId].vecFormat[m_FormatId].FrameRate.Numerator = dlg.m_FrameRateN ;
		vec_Device[m_DeviceId].vecFormat[m_FormatId].FrameRate.Denominator = dlg.m_FrameRateD ;

		m_CSId     = dlg.m_CSId ;
		m_FSId     = dlg.m_FSId ;

#ifndef C_STYLE
		hr = pCapture->OpenDevice ( vec_Device[m_DeviceId].id.c_str() ) ;
		if ( FAILED(hr) ){
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return ;
		}
		hr = pCapture->SetFormat ( m_FormatId, vec_Device[m_DeviceId].vecFormat[m_FormatId].FrameRate ) ;
		if ( FAILED(hr) ){
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return ;
		}
#else
		hr = MFVideoCapture_OpenDevice(pCapture1.get(), vec_Device[m_DeviceId].id.c_str());
		if (FAILED(hr)){
			_com_error err(hr);
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return;
		}
		hr = MFVideoCapture_SetFormat(pCapture1.get(), m_FormatId, &vec_Device[m_DeviceId].vecFormat[m_FormatId].FrameRate);
		if (FAILED(hr)){
			_com_error err(hr);
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return;
		}
#endif

		if ( !IsRendererSupport() ){
			return ;
		}

		UINT Width  = vec_Device[m_DeviceId].vecFormat[m_FormatId].Width ;
		UINT Height = vec_Device[m_DeviceId].vecFormat[m_FormatId].Height ;

		hr = pRenderer->Create ( Width, Height, &pCanvas ) ;


		RECT dstrect={300,300, 160+300, 90+300};
		RECT targetrect={100,100, 640+300, 360+300};

//		pRenderer->SetTargetRect(*pCanvas, TRUE, targetrect);
//		pRenderer->SetDestinationRect(*pCanvas, 0, TRUE, dstrect);
//		pRenderer->SetBackgroundColor(*pCanvas, 1.0f, 0, 0, 1.0f);

		if ( FAILED(hr) ){
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return ;
		}

		CD3DVideoRenderer::COLOR_SPACE cs = CD3DVideoRenderer::FOURCC_YUY2;
		if (MJPEG){
			cs = CD3DVideoRenderer::FOURCC_YV12;
		}

		hr = pRenderer->Create ( Width, Height, cs, &pImage ) ;

		if ( FAILED(hr) ){
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return ;
		}

		hr = pRenderer->SetStreamFormat(*pCanvas, 0, cs);

		if ( FAILED(hr) ){
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return ;
		}
		{
			//		RECT rect = {0, 0, 1920, 1080} ;
			//		hr = pD3DV->SetDestinationRect ( *pCanvas[1], 0, TRUE, rect ) ;
		}

		ResizeWindow(Width, Height);

#ifndef C_STYLE
		hr = pCapture->Start() ;
#else
		MFVideoCapture_Start(pCapture1.get());
#endif
		if ( FAILED(hr) ){
			_com_error err(hr) ;
			MessageBox(err.ErrorMessage(), NULL, MB_ICONERROR);
			return ;
		}
	}
	else if (nResponse == IDCANCEL){
	}
}

void CMainAppDlg::OnData(BYTE *pData, DWORD DataSize, long lStride, DWORD ts){

}




