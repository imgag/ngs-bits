#ifndef BAMCOVERAGETRACK_H
#define BAMCOVERAGETRACK_H

#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "BamTrackData.h"

#include <QSharedPointer>

struct BaseCoverage // stores coverage at a given genome pos
{
	// bases read in forward strands
	int forward_a =0;
	int forward_c =0;
	int forward_g =0;
	int forward_t =0;

	// bases read in reverse strands
	int reverse_a =0;
	int reverse_c =0;
	int reverse_g =0;
	int reverse_t =0;

	int insertions =0;
	int deletions =0;

	int a() const {return forward_a + reverse_a;}
	int c() const {return forward_c + reverse_c;}
	int g() const {return forward_g + reverse_g;}
	int t() const {return forward_t + reverse_t;}

	bool is_variant =false; // set to true when a significant number of bases differ from the reference base

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
	// track_data shared b/w coverage/alignment tracks
	QSharedPointer<BamTrackData> track_data_;
	// vector for stroing the coverage everytime the region changes
	QVector<BaseCoverage> coverage_;
	int max_coverage_;

	// calculates coverage per base and caches it in coverage_
	void storeCoverage();

	void drawZoomInText(QPainter&);
	void drawCoverage(QPainter&);

	// displays info at global_pos that was request at local_pos
	void handlePopupRequest(QPointF local_pos, QPointF global_pos);

	// converts BaseCoverage into a string for the pop up info (TODO: this can also be HTML)
	QString getCoverageText(const BaseCoverage& coverage, int coverage_idx);

	// used for checking if user is dragging or not
	QPoint mouse_press_pos_;

private slots:
	void dataReady();
};

#endif // BAMCOVERAGETRACK_H
