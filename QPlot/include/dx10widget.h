#ifndef DX10WIDGET_H
#define DX10WIDGET_H
#pragma once

#include <QWidget.h>
#include "dxcommon.h"

class DX10Widget
	: public QWidget
{
	Q_OBJECT

protected:
	//! Our rendering device
	ID3D10Device1*				m_pDevice;
	IDXGISwapChain*				m_pSwapChain;
	DXGI_SWAP_CHAIN_DESC		m_swapDesc;
	ID3D10RenderTargetView*		m_pRenderTargetView;
	ID3D10Texture2D*			m_pDepthStencil;
	ID3D10DepthStencilView*		m_pDepthStencilView;
	D3D10_FEATURE_LEVEL1		featureLevel;
	D3D10_DRIVER_TYPE			driverType;

	bool						standBy;

public:
	DX10Widget(QWidget *parent = 0, Qt::WFlags flags = 0);
	~DX10Widget();
	
	HRESULT			Initialize();

	QString			GetVersion();

protected:
	virtual void	setupScene() = 0;
	virtual void	tearDownScene() = 0;

	virtual void	resize(int width, int height);
	virtual void	render();
	
	void			uninitialize();

	QPaintEngine*	paintEngine() const { return 0; } 
	virtual void	paintEvent(QPaintEvent *e);
	virtual void	resizeEvent(QResizeEvent *p_event);
	
	void			resizeScene(int width, int height);
	void			clearScene(D3DXCOLOR ClearColor, float Z, DWORD Stencil);
	void			clearRenderTarget(D3DXCOLOR ClearColor);
	void			clearDepthStencil(float Z, DWORD Stencil);
	HRESULT			present();

signals:
     void			initialized(int code);
};

#endif // DX10WIDGET_H
