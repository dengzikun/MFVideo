#ifndef __D3DRENDERER_H__
#define __D3DRENDERER_H__

// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 D3DVIDEORENDERER_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// D3DVIDEORENDERER_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef D3DVIDEORENDERER_EXPORTS
#define D3DVIDEORENDERER_API __declspec(dllexport)
#else
#define D3DVIDEORENDERER_API __declspec(dllimport)
#endif

struct IDirect3DDeviceManager9 ;
struct IDirect3DSurface9 ;

// 此类是从 D3DVideoRenderer.dll 导出的
class D3DVIDEORENDERER_API CD3DVideoRenderer 
{
public:
	class CANVAS ;
	class IMAGE  ;

	/* FourCC */
	typedef enum {
		FOURCC_NV12         = '21VN', 
		FOURCC_YV12         = '21VY',
		FOURCC_YUY2         = '2YUY',
		FOURCC_A8R8G8B8     = 21,
	} COLOR_SPACE;

	typedef enum {
		FRAME_FORMAT_PROGRESSIVE	= 0,
		FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST	= 1,
		FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST	= 2
	} 	FRAME_FORMAT;

#pragma pack(push,8)
	typedef struct {
		BYTE *yuv[3] ;
		long  stride[3] ;
	} YUV_FRAME_INFO;

	typedef struct {
		BOOL bEnable ;
		UINT OutputIndex;
		UINT InputFrameOrField;
		UINT PastFrames;
		UINT FutureFrames;
		IMAGE **ppPastImages;
		IMAGE *pInputImage;
		IMAGE **ppFutureImages;
	} STREAM_INFO;
#pragma pack(pop,8)

	static UINT GetAdapterCount();
	static const char* GetAdapterDescription(UINT index) ;

	CD3DVideoRenderer(HRESULT &hr, HWND hWnd, IDirect3DDeviceManager9 **pManager, DWORD nAdapterNum = 0, bool WaitVsync = true);
	CD3DVideoRenderer(HRESULT &hr, IDirect3DDeviceManager9 *pManager);
	~CD3DVideoRenderer(void);

	bool  IsSupport ( COLOR_SPACE cs ) ;
	DWORD MaxInputStreams( CANVAS *pCanvas ) ;

	HRESULT Create( DWORD Width, DWORD Height, CANVAS **pCanvas );
	void Destroy( CANVAS *pCanvas );

	HRESULT Create( DWORD Width, DWORD Height, COLOR_SPACE cs, IMAGE **pImage );
	HRESULT Create( wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height, IMAGE **pImage );
	void Destroy( IMAGE *pImage );

	HRESULT SetBackgroundColor(CANVAS &Canvas, float a, float r, float g, float b);
	HRESULT SetTargetRect(CANVAS &Canvas, BOOL bEnable, RECT &rect) ;

	HRESULT SetSourceRect(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, RECT &rect);
	HRESULT SetDestinationRect(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, RECT &rect);
	HRESULT SetStreamFormat(CANVAS &Canvas, DWORD StreamIndex, COLOR_SPACE cs) ;
	HRESULT SetFrameFormat(CANVAS &Canvas, DWORD StreamIndex, FRAME_FORMAT ff) ;
	HRESULT SetPlanarAlpha(CANVAS &Canvas, DWORD StreamIndex, BOOL bEnable, float alpha) ;
	

	HRESULT Update( IMAGE &Image, BYTE *pFrame, long Stride );
	HRESULT Update( IMAGE &Image, YUV_FRAME_INFO *pInfo );
	HRESULT Update( IMAGE &Image, IDirect3DSurface9 *pSur ) ;
	HRESULT Update( IMAGE &Image, wchar_t *str, LOGFONTW &LogFont, COLORREF color, DWORD &Width, DWORD &Height );
	HRESULT Render(CANVAS &Canvas, STREAM_INFO *pInfo, DWORD StreamCount, HWND hWnd = NULL, RECT *src = NULL, RECT *dst = NULL );

private:
	class Impl ;
	Impl *pImpl ;
};

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


	D3DVIDEORENDERER_API CD3DVideoRenderer* D3DRendererInit(HWND hWnd, IDirect3DDeviceManager9 **pManager, DWORD nAdapterNum) ;
	D3DVIDEORENDERER_API CD3DVideoRenderer* D3DRendererInit_Ex(IDirect3DDeviceManager9 *pManager) ;
	D3DVIDEORENDERER_API void D3DRendererUninit(CD3DVideoRenderer* pRenderer) ;

	D3DVIDEORENDERER_API int  D3DRendererIsSupport ( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::COLOR_SPACE cs ) ;
	D3DVIDEORENDERER_API DWORD D3DRendererMaxInputStreams( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *pCanvas ) ;


	D3DVIDEORENDERER_API HRESULT D3DRendererCreateCanvas( CD3DVideoRenderer* pRenderer, DWORD Width, DWORD Height, CD3DVideoRenderer::CANVAS **pCanvas );
	D3DVIDEORENDERER_API void D3DRendererDestroyCanvas( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *pCanvas );

	D3DVIDEORENDERER_API HRESULT D3DRendererCreateImage( CD3DVideoRenderer* pRenderer, DWORD Width, DWORD Height, CD3DVideoRenderer::COLOR_SPACE cs, CD3DVideoRenderer::IMAGE **pImage );
	D3DVIDEORENDERER_API HRESULT D3DRendererCreateText( CD3DVideoRenderer* pRenderer, wchar_t *str, LOGFONTW *LogFont, COLORREF color, DWORD *Width, DWORD *Height, CD3DVideoRenderer::IMAGE **pImage );
	D3DVIDEORENDERER_API void D3DRendererDestroyImage( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *pImage );

	D3DVIDEORENDERER_API HRESULT D3DRendererSetBackgroundColor(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, float a, float r, float g, float b);
	D3DVIDEORENDERER_API HRESULT D3DRendererSetCanvasTargetRect(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, BOOL bEnable, RECT *rect) ;

	D3DVIDEORENDERER_API HRESULT D3DRendererSetCanvasSourceRect(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, BOOL bEnable, RECT *rect);
	D3DVIDEORENDERER_API HRESULT D3DRendererSetCanvasDestinationRect(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, BOOL bEnable, RECT *rect);
	D3DVIDEORENDERER_API HRESULT D3DRendererSetStreamFormat(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, CD3DVideoRenderer::COLOR_SPACE cs) ;
	D3DVIDEORENDERER_API HRESULT D3DRendererSetFrameFormat(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, CD3DVideoRenderer::FRAME_FORMAT ff) ;
	D3DVIDEORENDERER_API HRESULT D3DRendererSetPlanarAlpha(CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, DWORD StreamIndex, BOOL bEnable, float alpha) ;

	D3DVIDEORENDERER_API HRESULT D3DRendererUpdateImage( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *Image, BYTE *pFrame, long Stride );
	D3DVIDEORENDERER_API HRESULT D3DRendererUpdateSurface( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *Image, IDirect3DSurface9 *pSur ) ;
	D3DVIDEORENDERER_API HRESULT D3DRendererUpdateText( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::IMAGE *Image, wchar_t *str, LOGFONTW *LogFont, COLORREF color, DWORD *Width, DWORD *Height );
	D3DVIDEORENDERER_API HRESULT D3DRendererRender( CD3DVideoRenderer* pRenderer, CD3DVideoRenderer::CANVAS *Canvas, CD3DVideoRenderer::STREAM_INFO *pInfo, DWORD StreamCount, HWND hWnd, RECT *src, RECT *dst );


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif