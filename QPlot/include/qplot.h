#ifndef QPLOT_H
#define QPLOT_H

#include <QtGui/QMainWindow>
#include "ui_qplot.h"
#include <QTimer>

class QPlot
	: public QMainWindow
{
	Q_OBJECT

public:
	QPlot(QWidget *parent = 0, Qt::WFlags flags = 0);
	~QPlot();

private:
	Ui::QPlotClass	ui;
	QTimer			uiUpdateTimer;

protected:
	void changeEvent(QEvent * event);

protected slots:
	void onRendererInitialized(int code);
	void onUpdateUI();
};

#endif // QPLOT_H
