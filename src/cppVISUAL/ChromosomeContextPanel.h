#ifndef CHROMOSOMECONTEXTPANEL_H
#define CHROMOSOMECONTEXTPANEL_H

#include "cppVISUAL_global.h"

#include <QPainter>
#include <QWidget>

class CPPVISUALSHARED_EXPORT ChromosomeContextPanel
	: public QWidget
{
	Q_OBJECT

public:
	ChromosomeContextPanel(QWidget* parent);

private slots:
	void updateRegion();

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;

private:
	void drawLineWithArrow(QPainter*, QPoint start, QPoint end);
	void drawArrowsAndSize(QPainter*, int label_width, int total_width);
	void drawTicks(QPainter*, int label_width, int total_width);
	void drawDragRegion(QPainter*);

	QString formatRegionSize(int length);
	QString formatCoordinateLabel(int coord, int size);
	int tickInterval(int range, int max_ticks);


	float drag_start_x_;
	float drag_current_x_;
	bool is_dragging_ = false;
};

#endif // CHROMOSOMECONTEXTPANEL_H
