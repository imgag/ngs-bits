#ifndef CHROMOSOMEPANEL_H
#define CHROMOSOMEPANEL_H

#include "cppVISUAL_global.h"
#include <QWidget>
#include "BedFile.h"

//Panel that shows gene transcripts and nucleotides
class CPPVISUALSHARED_EXPORT ChromosomePanel
	: public QWidget
{
	Q_OBJECT

public:
	ChromosomePanel(QWidget* parent);

public slots:
	void setRegion(const BedLine& region);

signals:
	void mouseCoordinate(QString);

private:
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

	BedLine reg_;
};

#endif // CHROMOSOMEPANEL_H
