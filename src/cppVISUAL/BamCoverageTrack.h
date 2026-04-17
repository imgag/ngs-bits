#ifndef BAMCOVERAGETRACK_H
#define BAMCOVERAGETRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "TrackData.h"

#include <QSharedPointer>

struct BaseCoverage
{
	int a =0;
	int c =0;
	int g =0;
	int t =0;
	bool is_variant =false;

	int total() const {return a + c + g + t;}

	int max() const {return std::max({a, c, g, t});}
};

class CPPVISUALSHARED_EXPORT BamCoverageTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	explicit BamCoverageTrack(QWidget* parent, QString file_path, QString name);
	void setTrackData(QSharedPointer<BamTrackData> track_data);
	QString getType() override {return "BAM/CRAM_COVERAGE";}

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

protected:
	void paintEvent(QPaintEvent*) override;

private:
	QSharedPointer<BamTrackData> track_data_;
	// QSharedPointer<BamReader> reader_;
	// vector for stroing the coverage everytime the region changes
	// so that we don't have to recalulate every coverage paintEvent
	QVector<BaseCoverage> coverage_;
	long long max_coverage_;

	void storeCoverage();

	void drawZoomInText(QPainter&);
	void drawCoverage(QPainter&);

private slots:
	void dataReady();
};

#endif // BAMCOVERAGETRACK_H
