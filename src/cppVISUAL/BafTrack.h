#ifndef BAFTRACK_H
#define BAFTRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"


class CPPVISUALSHARED_EXPORT BafTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	BafTrack(QWidget* parent, QString file_path, QString name);

	void setBedFile(QSharedPointer<BedFile> bed_file);
	void setBafIdx(int baf_idx) {baf_idx_ = baf_idx;}

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

	static QString staticType() {return "BAF";}
	QString getType() override {return staticType();}

	static BafTrack* createTrack(QWidget* parent, QString file_path, QString name);

	static int findBafIdx(QSharedPointer<BedFile> bed_file);

protected:
	void paintEvent(QPaintEvent*) override;
// 	void mousePressEvent(QMouseEvent*) override;
// 	void mouseReleaseEvent(QMouseEvent*) override;
	void populateContextMenu(QMenu&) override;
	void handleContextMenuAction(QAction* action) override;

private:
	QSharedPointer<BedFile> bed_file_;
	int baf_idx_;
	QPoint mouse_press_pos_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;

	void drawPlot(QPainter&);
	void drawPoints(QPainter&, const QVector<int>& idxes);
	void drawLinePlot(QPainter&, const QVector<int>& idxes);
	void drawHeatMap(QPainter&, const QVector<int>& idxes);
	void drawBarChart(QPainter&, const QVector<int>& idxes);
	void drawReferenceLine(QPainter&, float baf_value, int x0, int total_width, int draw_height);
	inline int bafToY(float baf, int draw_height);

	enum GraphType
	{
		HEATMAP,
		BAR_CHART,
		POINTS,
		LINE_PLOT
	};

	GraphType graph_mode_ = POINTS;
	QAction* opts[4];
};


#endif // BAFTRACK_H
