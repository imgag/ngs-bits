#ifndef IGVTRACK_H
#define IGVTRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "IgvFile.h"
#include "ChromosomalIndex.h"

//Fast read-only parameters access; populated and maintained by ParameterList::link().
struct CPPVISUALSHARED_EXPORT IgvTrackSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QByteArray draw_mode MEMBER draw_mode)
	Q_PROPERTY(double min MEMBER min)
	Q_PROPERTY(double max MEMBER max)
	Q_PROPERTY(int track_height MEMBER track_height)

public:
	QByteArray draw_mode{};
	double min{};
	double max{};
	int track_height{};
};

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
	IgvTrackSettings fast_settings_;
	IgvTrack(QWidget* parent, QString file_path, QString name, QSharedPointer<IgvFile> igv_file);

	QSharedPointer<IgvFile> igv_file_;
	QPoint mouse_press_pos_;
	std::unique_ptr<ChromosomalIndex<IgvFile>> chr_index_;

	// draw functions
	void drawPlot(QPainter&);
	void drawPoints(QPainter&, const QVector<int>& idxes);
	void drawLinePlot(QPainter&, const QVector<int>& idxes);
	void drawHeatMap(QPainter&, const QVector<int>& idxes);
	void drawBarChart(QPainter&, const QVector<int>& idxes);
	void drawReferenceLine(QPainter&, double value);
	void drawScaleText(QPainter&);

	// handles right click by user
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);

	// utility functons
	int valueToY(double value) const;
	QRectF barRect(const IgvFileLine& line, const Viewport& viewport) const;

	// file functions
	static QString getTrackNameFromIgvFile(QSharedPointer<IgvFile> igv_file);

};




#endif // IGVTRACK_H
