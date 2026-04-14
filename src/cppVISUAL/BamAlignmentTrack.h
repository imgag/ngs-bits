#ifndef BAMALIGNMENTTRACK_H
#define BAMALIGNMENTTRACK_H

// #include "BamReader.h"
#include "TrackData.h"
#include "cppVISUAL_global.h"
#include "TrackWidget.h"

#include <QSharedPointer>

class CPPVISUALSHARED_EXPORT BamAlignmentTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	explicit BamAlignmentTrack(QWidget* parent, QString file_path, QString name);
	void setTrackData(QSharedPointer<BamTrackData> track_data);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

	QString getType() override {return "BAM/CRAM";}

protected:
	void paintEvent(QPaintEvent*) override;

private:
	//constants
	static constexpr int ROW_HEIGHT = 10;
	static constexpr int ROW_PADDING = 2;
	static constexpr int SPACING_BELOW = 4;

	static constexpr float CIGAR_DETAIL_SCALE = 20.0f;
	static constexpr float BASE_DETAIL_SCALE  = 1.0f;

	// QSharedPointer<BamReader> reader_;
	QSharedPointer<BamTrackData> track_data_;

	void drawZoomInText(QPainter&);
	void calculateRows();
	void drawAlignment(QPainter&, const BamAlignment& al, int row_y,
					   float scale, int x0, int x_max);

	static QColor baseColor(char base);
	static QColor strandColor(bool is_reverse);

	QVector<int> row_idxes_;
	int num_rows_ = 1;

	//utility function for mapping a \in [min_, max_] to [c, d]
	inline float map(float a, float min_, float max_, float c, float d)
	{
		float p = (float)(a - min_)/(float)(max_ - min_);
		p = std::clamp(p, 0.f, 1.f);
		return c + (d - c) * p;
	}

private slots:
	void dataReady();
};

#endif // BAMALIGNMENTTRACK_H
