#include "dx10widget.h"
#include <QtGui/QResizeEvent>

DX10Widget::DX10Widget(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags)
	, m_pDevice(0)
	, m_pSwapChain(0)
	, m_pRenderTargetView(0)
	, m_pDepthStencil(0)
	, m_pDepthStencilView(0)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);
}

DX10Widget::~DX10Widget()
{
	uninitialize();
}

//-----------------------------------------------------------------------------
// Name: initialize()
// Desc: This function will only be called once during the application's 
//       initialization phase. Therefore, it can't contain any resources that 
//       need to be restored every time the Direct3D device is lost or the 
//       window is resized.
//-----------------------------------------------------------------------------
HRESULT DX10Widget::initialize()
{
	HRESULT hr = S_OK;

	m_pDevice = 0;
	m_pSwapChain = 0;
	m_pRenderTargetView = 0;
	m_pDepthStencil = 0;
	m_pDepthStencilView = 0;
	featureLevel = D3D10_FEATURE_LEVEL_9_1;

	static const D3D10_DRIVER_TYPE driverAttempts[] =
	{
		D3D10_DRIVER_TYPE_HARDWARE,
		D3D10_DRIVER_TYPE_WARP,
	};

	static const D3D10_FEATURE_LEVEL1 levelAttempts[] =
	{
		D3D10_FEATURE_LEVEL_10_1,  // Direct3D 10.1 SM 4
		D3D10_FEATURE_LEVEL_10_0,  // Direct3D 10.0 SM 4
		D3D10_FEATURE_LEVEL_9_3,   // Direct3D 9.3  SM 3
		D3D10_FEATURE_LEVEL_9_2,   // Direct3D 9.2  SM 2
		D3D10_FEATURE_LEVEL_9_1,   // Direct3D 9.1  SM 2
	};

	UINT deviceFlags=0;
#ifdef _DEBUG
	deviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif
#ifdef USE_D2D
	deviceFlags |= D3D10_CREATE_DEVICE_BGRA_SUPPORT;
#endif

	for (UINT driver = 0; driver < ARRAYSIZE(driverAttempts); driver++) {
		for (UINT level = 0; level < ARRAYSIZE(levelAttempts); level++) {
			hr = D3D10CreateDevice1(
				0,
				driverAttempts[driver],
				NULL,
				deviceFlags,
				levelAttempts[level],
				D3D10_1_SDK_VERSION,
				&m_pDevice
				);
			if (SUCCEEDED(hr)) {
				featureLevel = levelAttempts[level];
				driverType = driverAttempts[driver];
				break;
			}
		}
		if (SUCCEEDED(hr)) {
			break;
		}
	}

	IDXGIDevice *pDXGIDevice = NULL;
	IDXGIAdapter *pAdapter = NULL;
	IDXGIFactory *pDXGIFactory = NULL;

	if (SUCCEEDED(hr)) {
		hr = m_pDevice->QueryInterface(&pDXGIDevice);
	}
	if (SUCCEEDED(hr)) {
		hr = pDXGIDevice->GetAdapter(&pAdapter);
	}
	if (SUCCEEDED(hr)) {
		hr = pAdapter->GetParent(IID_PPV_ARGS(&pDXGIFactory));
	}
	if (SUCCEEDED(hr)) {
		::ZeroMemory(&m_swapDesc, sizeof(m_swapDesc));

		m_swapDesc.SampleDesc.Count = 1;		//The Number of Multisamples per Level
		m_swapDesc.SampleDesc.Quality = 0;		//between 0(lowest Quality) and one lesser than m_pDevice->CheckMultisampleQualityLevels

		for(UINT i=1; i <= 4/*D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT*/; i++) {
			UINT Quality;
			if (SUCCEEDED(m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, i, &Quality))) {
				if (Quality > 0) {
					DXGI_SAMPLE_DESC Desc = { i, Quality - 1 };
					m_swapDesc.SampleDesc = Desc;
				}
			}
		}

		m_swapDesc.BufferDesc.Width = width();
		m_swapDesc.BufferDesc.Height = height();
		m_swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		m_swapDesc.BufferDesc.RefreshRate.Numerator = 60;
		m_swapDesc.BufferDesc.RefreshRate.Denominator = 1;
		m_swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		m_swapDesc.BufferCount = 1;
		m_swapDesc.OutputWindow = winId();
		m_swapDesc.Windowed = TRUE;

		hr = pDXGIFactory->CreateSwapChain(m_pDevice, &m_swapDesc, &m_pSwapChain);

		if (FAILED(hr)) {
			m_swapDesc.SampleDesc.Count = 1;
			hr = pDXGIFactory->CreateSwapChain(m_pDevice, &m_swapDesc, &m_pSwapChain);
		}
	}

	SAFE_RELEASE(pDXGIDevice);
	SAFE_RELEASE(pAdapter);
	SAFE_RELEASE(pDXGIFactory);

	if (SUCCEEDED(hr)) {
		setupScene();
	}
	return hr;
}

//-----------------------------------------------------------------------------
// Name: uninitialize()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
void DX10Widget::uninitialize()
{
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pDepthStencil);
	SAFE_RELEASE(m_pDepthStencilView);
}

void DX10Widget::resizeEvent(QResizeEvent *p_event)
{
	QSize newSize = size();
	if (p_event) {
		newSize = p_event->size();
		QWidget::resizeEvent(p_event);
	}
	resizeScene(newSize.width(), newSize.height());
}

void DX10Widget::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e);
	
	if (!m_pDevice) return;
	if (!m_pRenderTargetView || !m_pDepthStencilView) return;

	HRESULT hr = S_OK;

	if (!standBy) {
		render();
	}
	hr = present();
}

void DX10Widget::resizeScene(int width, int height)
{
	HRESULT hr = S_OK;

	ID3D10Resource *pBackBufferResource = NULL;
	ID3D10RenderTargetView *viewList[1] = {NULL};

	m_pDevice->ClearState();
	m_pDevice->OMSetRenderTargets(1, viewList, NULL);

	// Ensure that nobody is holding onto one of the old resources
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pDepthStencilView);

	// Resize render target buffers
	hr = m_pSwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

	if (SUCCEEDED(hr))
	{
		D3D10_TEXTURE2D_DESC texDesc;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
		texDesc.CPUAccessFlags = 0;
		texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = m_swapDesc.SampleDesc.Count;
		texDesc.SampleDesc.Quality = m_swapDesc.SampleDesc.Quality;
		texDesc.Usage = D3D10_USAGE_DEFAULT;

		SAFE_RELEASE(m_pDepthStencil);
		hr = m_pDevice->CreateTexture2D(&texDesc, NULL, &m_pDepthStencil);
	}

	if (SUCCEEDED(hr))
	{
		// Create the render target view and set it on the device
		hr = m_pSwapChain->GetBuffer(
			0,
			IID_PPV_ARGS(&pBackBufferResource)
			);
	}
	if (SUCCEEDED(hr))
	{
		D3D10_RENDER_TARGET_VIEW_DESC renderDesc;
		renderDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		renderDesc.ViewDimension = (m_swapDesc.SampleDesc.Count>1) ? D3D10_RTV_DIMENSION_TEXTURE2DMS : D3D10_RTV_DIMENSION_TEXTURE2D;
		renderDesc.Texture2D.MipSlice = 0;

		hr = m_pDevice->CreateRenderTargetView(pBackBufferResource, &renderDesc, &m_pRenderTargetView);
	}
	if (SUCCEEDED(hr))
	{
		D3D10_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
		depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthViewDesc.ViewDimension = (m_swapDesc.SampleDesc.Count>1) ? D3D10_DSV_DIMENSION_TEXTURE2DMS : D3D10_DSV_DIMENSION_TEXTURE2D;
		depthViewDesc.Texture2D.MipSlice = 0;

		hr = m_pDevice->CreateDepthStencilView(m_pDepthStencil, &depthViewDesc, &m_pDepthStencilView);
	}
	if (SUCCEEDED(hr))
	{
		viewList[0] = m_pRenderTargetView;
		m_pDevice->OMSetRenderTargets(1, viewList, m_pDepthStencilView);

		// Set a new viewport based on the new dimensions
		D3D10_VIEWPORT viewport;
		viewport.Width = width;
		viewport.Height = height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;
		m_pDevice->RSSetViewports(1, &viewport);
	}
	SAFE_RELEASE(pBackBufferResource);
	
	resize(width, height);
}

void DX10Widget::resize(int width, int height)
{
}

void DX10Widget::render()
{
	clearScene(D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
}

//-----------------------------------------------------------------------------
// Name: clearScene()
// Desc: Clear the render target and depth stencil
//-----------------------------------------------------------------------------
void DX10Widget::clearScene(D3DXCOLOR ClearColor, float Z, DWORD Stencil)
{
	clearRenderTarget(ClearColor);
	clearDepthStencil(Z, Stencil);
}

//-----------------------------------------------------------------------------
// Name: clearRenderTarget()
// Desc: Clear the render target
//-----------------------------------------------------------------------------
void DX10Widget::clearRenderTarget(D3DXCOLOR ClearColor)
{
	m_pDevice->ClearRenderTargetView( m_pRenderTargetView, (const float*)ClearColor );
}

//-----------------------------------------------------------------------------
// Name: clearScene()
// Desc: Clear the render target
//-----------------------------------------------------------------------------
void DX10Widget::clearDepthStencil(float Z, DWORD Stencil)
{
	m_pDevice->ClearDepthStencilView( m_pDepthStencilView, D3D10_CLEAR_DEPTH, Z, Stencil );
}

//-----------------------------------------------------------------------------
// Name: present()
// Desc: Present the backbuffer contents to the display
//-----------------------------------------------------------------------------
HRESULT DX10Widget::present()
{
	HRESULT hr;

	hr = m_pSwapChain->Present(0, 0);

	if (hr == DXGI_STATUS_OCCLUDED) {
		standBy = true;
	}
	else {
		standBy = false;
	}
		
	if (hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_REMOVED) {
		uninitialize();
		hr = initialize();
	}

	return hr;
}
