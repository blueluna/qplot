#ifndef QPLOT_H
#define QPLOT_H

#include <QtGui/QMainWindow>
#include "ui_qplot.h"

class QPlot
	: public QMainWindow
{
	Q_OBJECT

public:
	QPlot(QWidget *parent = 0, Qt::WFlags flags = 0);
	~QPlot();

private:
	Ui::QPlotClass ui;

protected:
	void changeEvent(QEvent * event);
	
};

#endif // QPLOT_H
