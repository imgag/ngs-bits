#ifndef CIRCOSPLOTWIDGET_H
#define CIRCOSPLOTWIDGET_H

#include <QWidget>

namespace Ui {
class CircosPlotWidget;
}

///Widget for visualization of the Circos plot
class CircosPlotWidget
		: public QWidget
{
	Q_OBJECT

public:
	explicit CircosPlotWidget(QWidget *parent = 0);
	~CircosPlotWidget();

protected:
	void resizeEvent(QResizeEvent*);

private:
	void loadCircosPlot(QString filename);
	Ui::CircosPlotWidget *ui_;
	QPixmap image_;
};

#endif // CIRCOSPLOTWIDGET_H
