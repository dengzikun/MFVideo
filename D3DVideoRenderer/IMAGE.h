#pragma once


class CD3DVideoRenderer::IMAGE{
public:

	IMAGE(DWORD Width, DWORD Height, D3DFORMAT cs, D3DPOOL pool, IDXVAHD_Device *pDXVAHD, HRESULT &hr) :
		  m_Width(Width),
		  m_Height(Height),
		  m_cs(cs),
		  m_dwFrameNum(0)
	  {
		  hr = pDXVAHD->CreateVideoSurface(
			  Width,
			  Height,
			  cs,
			  pool,
			  0,
			  DXVAHD_SURFACE_TYPE_VIDEO_OUTPUT,
			  1,
			  &m_pSur,
			  nullptr
			  );
	  }

	  IMAGE() :
		  m_pSur(nullptr),
		  m_Width(0),
		  m_Height(0),
		  m_cs(D3DFMT_A8R8G8B8),
		  m_dwFrameNum(0)
	  {
	  }

	  virtual ~IMAGE(void){
	  }

	  HRESULT Update( BYTE *pFrame, long Stride ){
		  D3DLOCKED_RECT lr;
		  HRESULT hr = m_pSur->LockRect(&lr, nullptr, D3DLOCK_NOSYSLOCK);
		  if (FAILED(hr)){ 
			  return hr ;
		  }

		  Update( pFrame, Stride, &lr );

		  hr = m_pSur->UnlockRect();
		  return hr ;
	  }

	  HRESULT Update( IMAGE &Image, YUV_FRAME_INFO *pInfo ){
		  D3DLOCKED_RECT lr;
		  HRESULT hr = m_pSur->LockRect(&lr, nullptr, D3DLOCK_NOSYSLOCK);
		  if (FAILED(hr)){ 
			  return hr ;
		  }

		  Update( pInfo, &lr );

		  hr = m_pSur->UnlockRect();
		  return hr ;
	  }

	  virtual HRESULT Update(BYTE *pFrame, long Stride, D3DLOCKED_RECT *lr){
		  return E_NOTIMPL ;
	  }

	  virtual HRESULT Update(YUV_FRAME_INFO *pInfo, D3DLOCKED_RECT *lr){
		  return E_NOTIMPL ;
	  }
	  

	  virtual HRESULT Update( wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height ){
		  return E_NOTIMPL ;
	  }

	  HRESULT Update( IDirect3DDevice9Ex *pDevice, IDXVAHD_Device *pDXVA, IDirect3DSurface9 *pSur ){
		  RECT rect={0,0,m_Width,m_Height};
		  HRESULT hr = pDevice->StretchRect(pSur, &rect, m_pSur, nullptr, D3DTEXF_POINT);
		  return hr;
	  }

public:
	CComPtr<IDirect3DSurface9> m_pSur ;
	DWORD m_dwFrameNum ;

	DWORD m_Width, m_Height ;
	D3DFORMAT m_cs ;
};


class IMAGE_YUY2 : public CD3DVideoRenderer::IMAGE
{
public:
	IMAGE_YUY2(DWORD Width, DWORD Height, D3DPOOL pool, IDXVAHD_Device *pDXVAHD, HRESULT &hr ) :
	  IMAGE(Width, Height, D3DFMT_YUY2, pool, pDXVAHD, hr ){
	  }
	  ~IMAGE_YUY2(){
	  }

	  HRESULT Update(BYTE *pFrame, long Stride, D3DLOCKED_RECT *lr){
		  BYTE* pDst = (BYTE*)lr->pBits;
		  INT   Pitch = lr->Pitch ;
		  BYTE *pSrc = pFrame ;
		  DWORD w = Stride ;
		  DWORD h = m_Height ;

		  if ( Pitch == w ){
			  CopyMemory ( pDst, pSrc, w*h ) ;
		  }else{
			  for (DWORD i = 0 ; i < h ; ++i){
				  CopyMemory ( pDst, pSrc, w ) ;
				  pSrc += w ;
				  pDst += Pitch ;
			  }
		  }
		  return S_OK ;
	  }
} ;

class IMAGE_YV12 : public CD3DVideoRenderer::IMAGE{
public:
	IMAGE_YV12(DWORD Width, DWORD Height, D3DPOOL pool, IDXVAHD_Device *pDXVAHD, HRESULT &hr ) :
	  IMAGE(Width, Height, (D3DFORMAT)CD3DVideoRenderer::FOURCC_NV12, pool, pDXVAHD, hr ){
		  m_cs = (D3DFORMAT)CD3DVideoRenderer::FOURCC_YV12 ;
	  }
	  ~IMAGE_YV12(){
	  }

	  HRESULT Update(BYTE *pFrame, long Stride, D3DLOCKED_RECT *lr){
		  BYTE *pSrc = pFrame ;
		  BYTE* pDst = (BYTE*)lr->pBits;
		  INT   Pitch = lr->Pitch ;

		  DWORD w = Stride ;
		  DWORD h = m_Height ;

		  if ( Pitch == w ){
			  CopyMemory ( pDst, pSrc, w*h ) ;
			  BYTE *pSrcV = pFrame + w*h ;
			  BYTE *pSrcU = pSrcV + w*h/4 ;
			  pDst += (w*h) ;

			  for (DWORD j = 0 ; j < h / 2 ; ++j){
				  for (DWORD i = 0 ; i < w / 2 ; ++i){
					  *pDst++ = *pSrcU++ ;
					  *pDst++ = *pSrcV++ ;
				  }
			  }
		  }else{
			  for ( DWORD i = 0 ; i < h ; ++i ){
				  CopyMemory ( pDst, pSrc, w ) ;
				  pSrc += w ;
				  pDst += Pitch ;
			  }

			  BYTE *pSrcV = pFrame + w*h ;
			  BYTE *pSrcU = pSrcV + w*h/4 ;
			  
			  for (DWORD j = 0 ; j < h / 2 ; ++j){
				  for (DWORD i = 0 ; i < w / 2 ; ++i){
					  *pDst++ = *pSrcU++ ;
					  *pDst++ = *pSrcV++ ;
				  }
				  pDst += (Pitch-w) ;
			  }
		  }
		  return S_OK ;
	  }

	  virtual HRESULT Update(CD3DVideoRenderer::YUV_FRAME_INFO *pInfo, D3DLOCKED_RECT *lr){
		  BYTE *pSrcY = pInfo->yuv[0] ;
		  BYTE *pDst = (BYTE*)lr->pBits ;
		  INT   Pitch = lr->Pitch ;

		  DWORD h = m_Height ;
		  DWORD w = m_Width ;

		  for ( int i = 0 ; i < h ; ++i ){
			  CopyMemory ( pDst, pSrcY, w ) ;
			  pSrcY += pInfo->stride[0] ;
			  pDst  += Pitch ;
		  }

		  BYTE *pSrcU = pInfo->yuv[1] ;
		  BYTE *pSrcV = pInfo->yuv[2] ;

		  DWORD h_h = h >> 1 ;
		  DWORD h_w = w >> 1 ;

		  for (DWORD j = 0 ; j < h_h ; ++j){
			  for (DWORD i = 0 ; i < h_w ; ++i){
				  *pDst++ = *pSrcU++ ;
				  *pDst++ = *pSrcV++ ;
			  }
			  pDst += (Pitch-w) ;
			  pSrcU +=(pInfo->stride[1]-h_w);
			  pSrcV +=(pInfo->stride[2]-h_w);
		  }
		  return S_OK ;
	  }
} ;

class IMAGE_NV12 : public CD3DVideoRenderer::IMAGE{
public:
	IMAGE_NV12(DWORD Width, DWORD Height, D3DPOOL pool, IDXVAHD_Device *pDXVAHD, HRESULT &hr ) :
	  IMAGE(Width, Height, (D3DFORMAT)CD3DVideoRenderer::FOURCC_NV12, pool, pDXVAHD, hr ){
	  }
	  ~IMAGE_NV12(){
	  }

	  HRESULT Update(BYTE *pFrame, long Stride, D3DLOCKED_RECT *lr){
		  BYTE* pDst = (BYTE*)lr->pBits;
		  INT   Pitch = lr->Pitch ;
		  BYTE *pSrc = pFrame ;
		  DWORD w = Stride ;
		  DWORD h = m_Height * 3 / 2 ;

		  if ( Pitch == w ){
			  CopyMemory ( pDst, pSrc, w*h ) ;
		  } else {
			  for (DWORD i = 0 ; i < h ; ++i){
				  CopyMemory ( pDst, pSrc, w ) ;
				  pSrc += w ;
				  pDst += Pitch ;
			  }
		  }
		  return S_OK ;
	  }
} ;

class IMAGE_A8R8G8B8 : public CD3DVideoRenderer::IMAGE{
public:
	IMAGE_A8R8G8B8(DWORD Width, DWORD Height, D3DPOOL pool, IDXVAHD_Device *pDXVAHD, HRESULT &hr ) :
	  IMAGE(Width, Height, (D3DFORMAT)CD3DVideoRenderer::FOURCC_A8R8G8B8, pool, pDXVAHD, hr ){
	  }
	  ~IMAGE_A8R8G8B8(){
	  }

	  HRESULT Update(BYTE *pFrame, long Stride, D3DLOCKED_RECT *lr){
		  BYTE* pDst = (BYTE*)lr->pBits;
		  INT   Pitch = lr->Pitch ;
		  BYTE *pSrc = pFrame ;
		  DWORD w = Stride ;
		  DWORD h = m_Height ;

		  if ( Pitch == w ){
			  CopyMemory ( pDst, pSrc, w*h ) ;
		  } else {
			  for (DWORD i = 0 ; i < h ; ++i) {
				  CopyMemory ( pDst, pSrc, w ) ;
				  pSrc += w ;
				  pDst += Pitch ;
			  }
		  }
		  return S_OK ;
	  }
} ;

class IMAGE_TEXT : public CD3DVideoRenderer::IMAGE{
public:
	IMAGE_TEXT( wchar_t *str, LOGFONTW &LogFont, COLORREF color, D3DPOOL pool, IDXVAHD_Device *pDXVAHD, DWORD &Width, DWORD &Height, HRESULT &hr) :
			m_strText(str),
			m_pDXVAHD(pDXVAHD),
			m_pool(pool)
	  {
			hr = Create(LogFont, color );
			if ( FAILED(hr) ){
				return ;
			}
			Width  = m_Width ;
			Height = m_Height ;
	  }

	  ~IMAGE_TEXT(){
	  }

	  void Destroy(){
		  m_pSur.Release();
	  }

	  HRESULT Create( LOGFONTW &LogFont, COLORREF color ){
		  Destroy() ;

		  HRESULT hr = S_OK;
		  HFONT hFont = nullptr;
		  HFONT hFontOld = nullptr;
		  HDC hDC = nullptr;
		  HBITMAP hbmBitmap = nullptr;
		  HGDIOBJ hbmOld = nullptr;

		  do
		  {
			  hDC = CreateCompatibleDC(nullptr);
			  if (hDC == nullptr){
				  hr = HRESULT_FROM_WIN32(GetLastError()) ;
				  break ;
			  }

			  SetMapMode( hDC, MM_TEXT ) ;

			  hFont = CreateFontIndirectW ( &LogFont ) ;
			  if (hFont == nullptr){
				  hr = HRESULT_FROM_WIN32(GetLastError()) ;
				  break ;
			  }

			  hFontOld = (HFONT) SelectObject( hDC, hFont );

			  hr = CalcuTextureSize ( hDC, m_strText.c_str() ) ;

			  if( FAILED(hr) ){
				  break ;
			  }

			  DWORD*      pBitmapBits;
			  BITMAPINFO bmi;
			  ZeroMemory( &bmi.bmiHeader, sizeof(BITMAPINFOHEADER) );
			  bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			  bmi.bmiHeader.biWidth       =  (int)m_Width;
			  bmi.bmiHeader.biHeight      = -(int)m_Height;
			  bmi.bmiHeader.biPlanes      = 1;
			  bmi.bmiHeader.biCompression = BI_RGB;
			  bmi.bmiHeader.biBitCount    = 32;

			  hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS,
				  (void**)&pBitmapBits, nullptr, 0);
			  if (hbmBitmap == nullptr){
				  hr = HRESULT_FROM_WIN32(GetLastError()) ;
				  break ;
			  }

			  hbmOld = SelectObject( hDC, hbmBitmap );

			  SetTextColor( hDC, color ) ;
			  SetBkColor ( hDC, 0x00101010 ) ;	
			  SetTextAlign( hDC, TA_TOP );

			  hr = PaintText( hDC, m_strText.c_str() );
			  if( FAILED(hr) ){
				  break ;
			  }

			  HRESULT hr = m_pDXVAHD->CreateVideoSurface(
				  m_Width,
				  m_Height,
				  m_cs,
				  m_pool,
				  0,
				  DXVAHD_SURFACE_TYPE_VIDEO_INPUT,
				  1,
				  &m_pSur,
				  nullptr
				  );

			  if (FAILED(hr)){ 
				  break ; 
			  }

			  D3DLOCKED_RECT d3dlr;
			  hr = m_pSur->LockRect(&d3dlr, nullptr, D3DLOCK_NOSYSLOCK);
			  if( FAILED(hr) ){
				  break ;
			  }

			  BYTE* pDstRow;
			  pDstRow = (BYTE*)d3dlr.pBits;
			  DWORD* pDst;
			  DWORD x, y;

			  for( y=0; y < m_Height; y++ ){
				  pDst = (DWORD*)pDstRow;
				  for( x=0; x < m_Width; x++ ){
					  *pDst = pBitmapBits[m_Width*y + x] ;

					  if ( *pDst != 0x00101010 ){
						  *pDst |= 0xff000000 ;
					  }
					  pDst++ ;
				  }
				  pDstRow += d3dlr.Pitch;
			  }


			  hr = m_pSur->UnlockRect();
			  if( FAILED(hr) ){
				  break ;
			  }

		  } while ( false ) ;

		  SelectObject( hDC, hbmOld );
		  SelectObject( hDC, hFontOld );
		  DeleteObject( hbmBitmap );
		  DeleteObject( hFont );
		  DeleteDC( hDC );

		  return hr ;
	  }

	  HRESULT Update( wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height ){
		  m_strText = str ;
		  HRESULT hr = Create( LogFont, color );
		  if ( FAILED(hr) )
		  {
			  return hr ;
		  }
		  Width = m_Width ;
		  Height = m_Height ;
		  return hr ;
	  }

public:
	std::basic_string<wchar_t> m_strText ;

//	LOGFONTW m_LogFont ;
//	COLORREF m_dwColor ;

	D3DPOOL m_pool ;
	IDXVAHD_Device *m_pDXVAHD ;
	
private:
	HRESULT PaintText( HDC hDC, const wchar_t *strText ){
		RECT rect ;
		rect.left = rect.top = 0 ;
		rect.right = m_Width ;
		rect.bottom = m_Height ;

		if (0 == ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &rect, strText, wcslen(strText), nullptr)){
			return HRESULT_FROM_WIN32(GetLastError()) ;
		}

		return S_OK;
	}

	HRESULT CalcuTextureSize ( HDC hDC, const wchar_t *strText ){
		SIZE size ;
		if( 0 == GetTextExtentPoint32W( hDC, strText, wcslen(strText), &size ) ){
			return HRESULT_FROM_WIN32(GetLastError()) ;
		}

		m_Width  = ( size.cx / 16 + 1 ) * 16 ;
		m_Height = ( size.cy / 16 + 1 ) * 16 ;

		return S_OK;
	}
} ;
