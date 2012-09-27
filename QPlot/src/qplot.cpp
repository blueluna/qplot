#include "qplot.h"

QPlot::QPlot(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
	, uiUpdateTimer(this)
{
	ui.setupUi(this);
	connect(ui.widget, SIGNAL(initialized(int)), this, SLOT(onRendererInitialized(int)));

	ui.widget->Initialize();

	bool ok = connect(&uiUpdateTimer, SIGNAL(timeout()), this, SLOT(onUpdateUI()));

	uiUpdateTimer.setInterval(1000.0);
	uiUpdateTimer.start();
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

void QPlot::onRendererInitialized(int code)
{
	if (code == S_OK) {
		QString version = ui.widget->GetVersion();
		ui.directXVersionLineEdit->setText(version);
	}
}

void QPlot::onUpdateUI()
{
	float interval = ui.widget->GetRedrawInterval();
	ui.drawIntervalLineEdit->setText(QString::number(interval) + tr(" ms"));
}
