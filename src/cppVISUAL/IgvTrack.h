#ifndef IGVTRACK_H
#define IGVTRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "IgvFile.h"
#include "ChromosomalIndex.h"

//Track that shows the IGV data file
class CPPVISUALSHARED_EXPORT IgvTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	IgvTrack(QWidget* parent, QString file_path, QString name);
	static QByteArray type() { return "IgvTrack"; }

	void setIgvFile(QSharedPointer<IgvFile> igv_file);
	void reloadTrack() override;

	QSize sizeHint() const override;

	static IgvTrack* createTrack(QWidget* parent, QString file_path, QString name = "");


protected:
	QList<Parameter> getParameters() const override;
	QHash<QByteArray, QVariant> getParameterDefaults() const override;
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;

private:
	IgvTrack(QWidget* parent, QString file_path, QString name, QSharedPointer<IgvFile> igv_file);

	QSharedPointer<IgvFile> igv_file_;
	QPoint mouse_press_pos_;
	std::unique_ptr<ChromosomalIndex<IgvFile>> chr_index_;

	// draw functions
	struct PlotScale
	{
		double minimum;
		double maximum;
		int height;
	};
	PlotScale plotScale() const;
	void drawPlot(QPainter&);
	void drawPoints(QPainter&, const QVector<int>& idxes, const PlotScale& scale);
	void drawLinePlot(QPainter&, const QVector<int>& idxes, const PlotScale& scale);
	void drawHeatMap(QPainter&, const QVector<int>& idxes, const PlotScale& scale);
	void drawBarChart(QPainter&, const QVector<int>& idxes, const PlotScale& scale);
	void drawReferenceLine(QPainter&, double value, const PlotScale& scale);
	void drawScaleText(QPainter&);

	// handles right click by user
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);

	// utility functons
	static int valueToY(double value, const PlotScale& scale);
	static QRectF barRect(const IgvFileLine& line, const Viewport& viewport, const PlotScale& scale);

	// file functions
	static QString getTrackNameFromIgvFile(QSharedPointer<IgvFile> igv_file);

};




#endif // IGVTRACK_H
