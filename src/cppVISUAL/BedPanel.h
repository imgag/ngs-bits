#ifndef BEDPANEL_H
#define BEDPANEL_H

#include "cppVISUAL_global.h"
#include "BedFile.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QMouseEvent>

class CPPVISUALSHARED_EXPORT BedPanel
	: public QWidget
{
	Q_OBJECT

public:
	BedPanel(QWidget*);

public slots:
	void tracksChanged();

private:
	QVBoxLayout* layout_;
	void clearLayout();

	void paintEvent(QPaintEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void wheelEvent(QWheelEvent*) override;

	float drag_start_x_;
	float current_mouse_x_;
	BedLine drag_start_region_;
	float is_dragging_ = false;
};


#endif // BEDPANEL_H
