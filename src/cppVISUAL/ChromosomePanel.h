#ifndef CHROMOSOMEPANEL_H
#define CHROMOSOMEPANEL_H

#include "QtGui/qevent.h"
#include "cppVISUAL_global.h"
#include "BedFile.h"
#include <QWidget>
#include <QVector>
#include <QPair>

//Panel that shows the full chromosome
class CPPVISUALSHARED_EXPORT ChromosomePanel
	: public QWidget
{
	Q_OBJECT

public:
	ChromosomePanel(QWidget* parent);
	//Returns the one-based coordinate under x, or zero outside the chromosome drawing.
	int genomicCoordinateAt(int x) const;

private slots:
	//Updates the region displayed by this widget
	void updateRegion();

private:
	//members needed for paint event - updated when resizing occurs
	double pixels_per_base_;
	float chr_height_;
	float text_height_;
	int band_y_ = 2;
	int padding_ = 2;
	float drag_start_x_;
	float drag_current_x_;
	bool is_dragging_ = false;
	QVector<QPair<QRectF, QString>> bookmark_regions_;
	QVector<QPair<QRectF, QString>> par_regions_;

	void paintEvent(QPaintEvent* event) override;
	bool event(QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;


	void drawBands(QPainter* painter, const BedFile& bands, QVector<int> idxes,
				   int total_width, int total_length);
	void drawRegion(QPainter* painter, int total_length);
	void drawDragRegion(QPainter* painter);
	void drawBookmarks(QPainter& painter, int total_length);
	void drawPseudoAutosomalRegions(QPainter& painter, int total_length);

	using ChromosomeIndexMap = QHash<Chromosome, QVector<int>>;
	ChromosomeIndexMap buildChrIndexMap(const BedFile& bands);
	QColor cytobandColor(QString stain);
};

#endif // CHROMOSOMEPANEL_H
