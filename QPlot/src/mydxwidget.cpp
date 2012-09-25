#include "include\mydxwidget.h"

MyDxWidget::MyDxWidget(QWidget *parent)
	: DX10Widget(parent)
	, clearColor(Qt::black)
	, index(0.0f)
	, vertexLayout(0)
	, vertexBuffer(0)
	, indexBuffer(0)
	, vertexShader(0)
	, pixelShader(0)
	, shaderBuffer(0)
	, redrawTimer(this)
{
	initialize();
	connect(&redrawTimer, SIGNAL(timeout()), this, SLOT(repaint()));
	redrawTimer.setInterval(60.0);
}

MyDxWidget::~MyDxWidget()
{
	tearDownScene();
}

void MyDxWidget::setupScene()
{
	HRESULT hr;

	index = 0.0f;

	DWORD VERTS_PER_EDGE = 64;
	DWORD dwNumVertices = VERTS_PER_EDGE * VERTS_PER_EDGE;
	DWORD dwNumIndices = 6 * (VERTS_PER_EDGE - 1) * (VERTS_PER_EDGE - 1);
	
	float offset = float(D3DX_PI);
    projectionMatrix.ortho(-offset, offset, -offset, offset, -100.0f, 100.0f);

	// Create a constant buffer
	D3D10_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(ShaderVariables);
	cbDesc.Usage = D3D10_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	hr = m_pDevice->CreateBuffer(&cbDesc, NULL, &shaderBuffer);
	if (hr != S_OK) {
		return;
	}

	// Create an Index Buffer
	D3D10_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = dwNumIndices * sizeof(WORD);
	ibDesc.Usage = D3D10_USAGE_DEFAULT;
	ibDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;

	WORD* pIndexData = new WORD [dwNumIndices];
	if (!pIndexData) { return; }
	WORD* pIndices = pIndexData;
	for( DWORD y = 1; y < VERTS_PER_EDGE; y++ ) {
		for( DWORD x = 1; x < VERTS_PER_EDGE; x++ ) {
			*pIndices++ = (WORD)(( y - 1 ) * VERTS_PER_EDGE + (x - 1));
			*pIndices++ = (WORD)(( y - 0 ) * VERTS_PER_EDGE + (x - 1));
			*pIndices++ = (WORD)(( y - 1 ) * VERTS_PER_EDGE + (x - 0));

			*pIndices++ = (WORD)(( y - 1 ) * VERTS_PER_EDGE + (x - 0));
			*pIndices++ = (WORD)(( y - 0 ) * VERTS_PER_EDGE + (x - 1));
			*pIndices++ = (WORD)(( y - 0 ) * VERTS_PER_EDGE + (x - 0));
		}
	}

	D3D10_SUBRESOURCE_DATA ibInitData;
	ZeroMemory(&ibInitData, sizeof(D3D10_SUBRESOURCE_DATA));
	ibInitData.pSysMem = pIndexData;
	hr = m_pDevice->CreateBuffer(&ibDesc, &ibInitData, &indexBuffer);
	if (hr != S_OK) { return; }
	SAFE_DELETE_ARRAY(pIndexData);

	// Create a Vertex Buffer
	D3D10_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = dwNumVertices * sizeof(Vertex);
	vbDesc.Usage = D3D10_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;

	Vertex* pVertData = new Vertex[dwNumVertices];
	if (!pVertData) { return; }
	Vertex* pVertices = pVertData;
	for( DWORD y = 0; y < VERTS_PER_EDGE; y++ )
	{
		for( DWORD x = 0; x < VERTS_PER_EDGE; x++ )
		{
			pVertices->pos.x = ((float)x / (float)(VERTS_PER_EDGE - 1) - 0.5f) * D3DX_PI;
			pVertices->pos.y = ((float)y / (float)(VERTS_PER_EDGE - 1) - 0.5f) * D3DX_PI;
			*pVertices++;
		}
	}

	D3D10_SUBRESOURCE_DATA vbInitData;
	ZeroMemory(&vbInitData, sizeof(D3D10_SUBRESOURCE_DATA));
	vbInitData.pSysMem = pVertData;
	hr = m_pDevice->CreateBuffer(&vbDesc, &vbInitData, &vertexBuffer);
	if (hr != S_OK) { return; }
	SAFE_DELETE_ARRAY( pVertData );

	// Compile the VS from the file
	ID3D10Blob* pVSBuf = NULL;

	DWORD dwShaderFlags = 0;
	#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3D10_SHADER_DEBUG;
	#endif

	char* vsFunc = 0;
	char* psFunc = 0;

	switch(featureLevel) {
	case D3D10_FEATURE_LEVEL_10_1:
		vsFunc = "vs_4_1";
		psFunc = "ps_4_1";
		break;
	case D3D10_FEATURE_LEVEL_10_0:
		vsFunc = "vs_4_0";
		psFunc = "ps_4_0";
		break;
	case D3D10_FEATURE_LEVEL_9_3:
		vsFunc = "vs_4_0_level_9_3";
		psFunc = "ps_4_0_level_9_3";
		break;
	default:
	case D3D10_FEATURE_LEVEL_9_2: // Shader model 2 fits feature level 9_1
	case D3D10_FEATURE_LEVEL_9_1:
		vsFunc = "vs_4_0_level_9_1";
		psFunc = "ps_4_0_level_9_1";
		break;
	}

	hr = D3DX10CompileFromFile(L"HLSLwithoutFX10.vsh", NULL, NULL, "Ripple", vsFunc, dwShaderFlags, NULL, NULL, &pVSBuf, NULL, NULL);
	if (hr != S_OK) { return; }
	hr = m_pDevice->CreateVertexShader((DWORD*)pVSBuf->GetBufferPointer(), pVSBuf->GetBufferSize(), &vertexShader);
	if (hr != S_OK) { return; }

	// Compile the PS from the file
	ID3D10Blob* pPSBuf = NULL;
	hr = D3DX10CompileFromFile(L"HLSLwithoutFX.psh", NULL, NULL, "main", psFunc, dwShaderFlags, NULL, NULL, &pPSBuf, NULL, NULL);
	if (hr != S_OK) { return; }
	hr = m_pDevice->CreatePixelShader((DWORD*)pPSBuf->GetBufferPointer(), pPSBuf->GetBufferSize(), &pixelShader);
	if (hr != S_OK) { return; }

	// Create our vertex input layout
	const D3D10_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
	
	hr = m_pDevice->CreateInputLayout(layout, 1, pVSBuf->GetBufferPointer(), pVSBuf->GetBufferSize(), &vertexLayout);
	if (hr != S_OK) { return; }

	SAFE_RELEASE(pVSBuf);
	SAFE_RELEASE(pPSBuf);

	// Create a blend state to disable alpha blending
	D3D10_BLEND_DESC BlendState;
	ZeroMemory( &BlendState, sizeof(D3D10_BLEND_DESC));
	BlendState.BlendEnable[0] = FALSE;
	BlendState.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
	hr = m_pDevice->CreateBlendState(&BlendState, &blendState);
	if (hr != S_OK) { return; }

	// Create a rasterizer state to disable culling
	D3D10_RASTERIZER_DESC RSDesc;
	RSDesc.FillMode = D3D10_FILL_WIREFRAME;
	RSDesc.CullMode = D3D10_CULL_NONE;
	RSDesc.FrontCounterClockwise = TRUE;
	RSDesc.DepthBias = 0;
	RSDesc.DepthBiasClamp = 0;
	RSDesc.SlopeScaledDepthBias = 0;
	RSDesc.ScissorEnable = FALSE;
	RSDesc.MultisampleEnable = FALSE;
	RSDesc.AntialiasedLineEnable = FALSE;
	hr = m_pDevice->CreateRasterizerState(&RSDesc, &rasterizerState);
	if (hr != S_OK) { return; }

	// Create a depth stencil state to enable less-equal depth testing
	D3D10_DEPTH_STENCIL_DESC DSDesc;
	ZeroMemory( &DSDesc, sizeof(D3D10_DEPTH_STENCIL_DESC));
	DSDesc.DepthEnable = TRUE;
	DSDesc.DepthFunc = D3D10_COMPARISON_ALWAYS;
	DSDesc.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ALL;
	hr = m_pDevice->CreateDepthStencilState(&DSDesc, &stencilState);
	if (hr != S_OK) { return; }
	else {
		redrawTimer.start();
	}
}

void MyDxWidget::tearDownScene()
{
	redrawTimer.stop();

	SAFE_RELEASE(vertexLayout);
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);
	SAFE_RELEASE(vertexShader);
	SAFE_RELEASE(pixelShader);
	SAFE_RELEASE(shaderBuffer);
	SAFE_RELEASE(blendState);
	SAFE_RELEASE(rasterizerState);
	SAFE_RELEASE(stencilState);
}

void MyDxWidget::resize(int width, int height)
{
	const double *projd = reinterpret_cast<const double*>(projectionMatrix.constData());
	float projf[16] = {0};
	for (int n = 0; n < 16; n++) {
		projf[n] = static_cast<float>(projd[n]);
	}
	dxProjection = D3DXMATRIXA16(projf);
}

void MyDxWidget::render()
{
	HRESULT hr = S_OK;
	clearScene(D3DXCOLOR(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), 1.0f), 1.0f, 0);
	// DX10 spec only guarantees Sincos function from -100 * Pi to 100 * Pi
	index += 0.01;
	float indexClamp = (float)index - (floor(index / (2.0f * D3DX_PI)) * 2.0f * D3DX_PI);

	DWORD VERTS_PER_EDGE = 64;
	DWORD dwNumIndices = 6 * (VERTS_PER_EDGE - 1) * (VERTS_PER_EDGE - 1);

	ShaderVariables* variables;
	shaderBuffer->Map(D3D10_MAP_WRITE_DISCARD, NULL, (void**)&variables);
	variables->projection = dxProjection;
	variables->index = indexClamp;
	shaderBuffer->Unmap();

	// Set IA parameters
	UINT strides[1] = {sizeof(Vertex)};
	UINT offsets[1] = {0};
	ID3D10Buffer* pBuffers[1] = {vertexBuffer};

	m_pDevice->IASetVertexBuffers(0, 1, pBuffers, strides, offsets);
	m_pDevice->IASetInputLayout(vertexLayout);
	m_pDevice->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind the constant buffer to the device
	pBuffers[0] = shaderBuffer;
	m_pDevice->VSSetConstantBuffers(0, 1, pBuffers);
	m_pDevice->OMSetBlendState(blendState, 0, 0xffffffff);
	m_pDevice->RSSetState(rasterizerState);
	m_pDevice->OMSetDepthStencilState(stencilState, 0);
	m_pDevice->VSSetShader(vertexShader);
	m_pDevice->GSSetShader(NULL);
	m_pDevice->PSSetShader(pixelShader);
	m_pDevice->DrawIndexed(dwNumIndices, 0, 0);
}
