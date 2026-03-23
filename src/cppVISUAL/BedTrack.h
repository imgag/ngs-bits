#ifndef BEDTRACK_H
#define BEDTRACK_H


#include "cppVISUAL_global.h"
#include "ChromosomalIndex.h"
#include "TrackData.h"
#include "TrackWidget.h"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>


class CPPVISUALSHARED_EXPORT BedTrack
	: public TrackWidget
{
	Q_OBJECT

public:
	BedTrack(QWidget* parent, QSharedPointer<BedFileTrackData> tack);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();};

private:
	QSharedPointer<BedFileTrackData> bed_track_;
	ChromosomalIndex<BedFile> chr_index_;

	// pre count of num rows required per chromosome
	QHash<Chromosome, int> num_rows_;
	QVector<int> row_idxes_;

	static const int BLOCK_HEIGHT = 10;
	static const int BLOCK_PADDING = 5;
	static const int SPACING_BELOW = 20;

	void paintEvent(QPaintEvent* event) override;

	//utility function for calculating the nubmer of rows required/chr
	void calculateNumRows();

	//utility function for mapping a \in [min_, max_] to [c, d]
	inline float map(float a, float min_, float max_, float c, float d)
	{
		float p = (float)(a - min_)/(float)(max_ - min_);
		p = std::clamp(p, 0.f, 1.f);
		return c + (d - c) * p;
	}
};

#endif // BEDTRACK_H
