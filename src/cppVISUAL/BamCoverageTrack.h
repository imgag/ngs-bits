#ifndef BAMCOVERAGETRACK_H
#define BAMCOVERAGETRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "TrackData.h"

#include <QSharedPointer>

struct BaseCoverage
{
	int forward_a =0;
	int forward_c =0;
	int forward_g =0;
	int forward_t =0;

	int reverse_a =0;
	int reverse_c =0;
	int reverse_g =0;
	int reverse_t =0;

	int a() const {return forward_a + reverse_a;}
	int c() const {return forward_c + reverse_c;}
	int g() const {return forward_g + reverse_g;}
	int t() const {return forward_t + reverse_t;}

	bool is_variant =false;

	int total() const {return a() + c() + g() + t();}

	int max() const {return std::max({a(), c(), g(), t()});}
};

class CPPVISUALSHARED_EXPORT BamCoverageTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	explicit BamCoverageTrack(QWidget* parent, QString file_path, QString name);
	void setTrackData(QSharedPointer<BamTrackData> track_data);
	virtual void reloadTrack() override;

	static QString staticType() {return "BAM/CRAM_COVERAGE";}
	QString getType() override {return staticType();}

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

	static BamCoverageTrack* createTrack(QWidget* parent, QString file_path, QString name);

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;

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
	void handlePopupRequest(QPointF local_pos, QPointF global_pos);
	QString getCoverageText(const BaseCoverage& coverage, int coverage_idx);

	QPoint mouse_press_pos_;

private slots:
	void dataReady();
};

#endif // BAMCOVERAGETRACK_H
