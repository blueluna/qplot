#ifndef MYDXWIDGET_H
#define MYDXWIDGET_H

#include <dx10widget.h>
#include <QColor>
#include <QMatrix4x4>
#include <QTimer>

class MyDxWidget
	: public DX10Widget
{
public:
	struct Vertex
	{
		D3DXVECTOR2 pos;
	};

	struct ShaderVariables
	{
		D3DXMATRIX		projection;
		D3DXVECTOR4		_1;
		float			_2;
		float			index;
		float			_4;
		float			_5;
	};

protected:
	QColor						clearColor;

    QMatrix4x4					projectionMatrix;
	D3DXMATRIXA16				dxProjection;

	QTimer						redrawTimer;

	float						index;

	ID3D10InputLayout*          vertexLayout;
	ID3D10Buffer*               vertexBuffer;
	ID3D10Buffer*               indexBuffer;
	ID3D10VertexShader*         vertexShader;
	ID3D10PixelShader*          pixelShader;
	ID3D10Buffer*               shaderBuffer;
	ID3D10BlendState*           blendState;
	ID3D10RasterizerState*      rasterizerState;
	ID3D10DepthStencilState*	stencilState;

public:
	MyDxWidget(QWidget *parent);
	~MyDxWidget();

	void	SetClearColor(QColor color) { clearColor = color; }

private:
	virtual void	setupScene();
	virtual void	tearDownScene();

	virtual void	resize(int width, int height);
	virtual void	render();
};

#endif // MYDXWIDGET_H
