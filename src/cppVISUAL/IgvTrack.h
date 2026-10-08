#ifndef IGVTRACK_H
#define IGVTRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "IgvTrackSettings.h"

//Track that shows the IGV data file
class CPPVISUALSHARED_EXPORT IgvTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	IgvTrack(QWidget* parent, QString file_path, QString name);

	void setBedFile(QSharedPointer<BedFile> bed_file);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

	static QString staticType() {return "IGV";}
	QString getType() override {return staticType();}

	static IgvTrack* createTrack(QWidget* parent, QString file_path, QString name = "");

	QMap<QString, QVariant> getSettings() override;
	void loadKeyValueFromXml(QString key, QString value) override;

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void populateContextMenu(QMenu&, const QPoint&) override;

private:
	QSharedPointer<BedFile> bed_file_;
	QPoint mouse_press_pos_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;

	// draw functions
	void drawPlot(QPainter&);
	void drawPoints(QPainter&, const QVector<int>& idxes);
	void drawLinePlot(QPainter&, const QVector<int>& idxes);
	void drawHeatMap(QPainter&, const QVector<int>& idxes);
	void drawBarChart(QPainter&, const QVector<int>& idxes);
	void drawReferenceLine(QPainter&, float baf_value);
	void drawScaleText(QPainter&);

	// handles right click by user
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);

	// utility funcitons
	inline int valueToY(float value);
	// converts a BedLine to text for the pop up info box
	QString getIgvText(const BedLine& bd);

	// file functions
	static QString getTrackNameFromIgvFile(QSharedPointer<BedFile> bed_file);

	QSharedPointer<IgvTrackSettings> settings;
};




#endif // IGVTRACK_H
