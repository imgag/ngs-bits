#ifndef CHROMOSOMEPANEL_H
#define CHROMOSOMEPANEL_H

#include "cppVISUAL_global.h"
#include <QWidget>

//Panel that shows gene transcripts and nucleotides
class CPPVISUALSHARED_EXPORT ChromosomePanel
	: public QWidget
{
	Q_OBJECT

public:
	ChromosomePanel(QWidget* parent);

signals:
	void mouseCoordinate(QString);

private slots:
	//Updates the region displayed by this widget
	void updateRegion();

private:
	//members needed for paint event - updated when resizing occurs
	double pixels_per_base_;

	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

};

#endif // CHROMOSOMEPANEL_H
