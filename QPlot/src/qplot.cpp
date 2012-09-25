#include "qplot.h"

QPlot::QPlot(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
}

QPlot::~QPlot()
{
}

void QPlot::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::PaletteChange) {
		ui.widget->SetClearColor(palette().color(QPalette::Active, QPalette::Window));
	}
}
