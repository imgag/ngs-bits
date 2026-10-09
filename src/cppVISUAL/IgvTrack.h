#ifndef IGVTRACK_H
#define IGVTRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"

//Track that shows the IGV data file
class CPPVISUALSHARED_EXPORT IgvTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	IgvTrack(QWidget* parent, QString file_path, QString name);
	~IgvTrack();
	static QByteArray type() { return "IgvTrack"; }

	void setBedFile(QSharedPointer<BedFile> bed_file);

	QSize sizeHint() const override;

	static IgvTrack* createTrack(QWidget* parent, QString file_path, QString name = "");

	QMap<QString, QVariant> getSettings() override;
	void loadKeyValueFromXml(QString key, QString value) override;

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void populateContextMenu(QMenu&, const QPoint&) override;

private:
	IgvTrack(QWidget* parent, QString file_path, QString name, const QHash<QByteArray, QVariant>& defaults);
	static QList<Parameter> parameterConfig();
	static QHash<QByteArray, QVariant> defaultsFromFile(QSharedPointer<BedFile> bed_file);

	QSharedPointer<BedFile> bed_file_;
	QPoint mouse_press_pos_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;

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
	void drawReferenceLine(QPainter&, float baf_value, const PlotScale& scale);
	void drawScaleText(QPainter&);

	// handles right click by user
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);

	// utility funcitons
	static int valueToY(float value, const PlotScale& scale);
	// converts a BedLine to text for the pop up info box
	QString getIgvText(const BedLine& bd);

	// file functions
	static QString getTrackNameFromIgvFile(QSharedPointer<BedFile> bed_file);

};




#endif // IGVTRACK_H
