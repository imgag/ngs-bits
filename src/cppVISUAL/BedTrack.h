#ifndef BEDTRACK_H
#define BEDTRACK_H


#include "cppVISUAL_global.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "TrackWidget.h"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>


class CPPVISUALSHARED_EXPORT BedTrack
	: public TrackWidget
{
	Q_OBJECT

public:
	explicit BedTrack(QWidget* parent, QString file_path, QString name);

	bool load();
	void setBedFile(QSharedPointer<BedFile> bedfile);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();};

	void reloadTrack() override;

	QString getType() override {return "BED";}

private:
	QSharedPointer<BedFile> bedfile_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;
	QColor color_ = QColor(0, 0, 178);

	// pre count of num rows required per chromosome
	QHash<Chromosome, int> num_rows_;
	QVector<int> row_idxes_;

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
