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

	// creates a BedTrack, same as the constructor above but
	// tries to load file_path first into a bed file if that fails
	// returns nullptr.
	static BedTrack* createTrack(QWidget* parent, QString file_path, QString name);

	// sets bedFile, re creates chromosome index, re calculates row numbers, updates geometry
	void setBedFile(QSharedPointer<BedFile> bedfile);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();};

	// reloads the file, if failed deletes the track
	void reloadTrack() override;

	QString getType() override {return "BED";}

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	// band data in row
	struct BandData
	{
		int start; // band start
		int end; // band end
		int ind; // idx in bedfile
	};


	QSharedPointer<BedFile> bedfile_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;
	QColor color_ = QColor(0, 0, 178);

	QPointF mouse_press_pos_;

	// pre count of num rows required per chromosome
	QHash<Chromosome, int> num_rows_;

	//hash map from row to vector of data of bands stored in that row
	using RowInfoType = QHash<int, QVector<BandData>>;
	RowInfoType row_store_;

	// array for storing the draw index of each band
	QVector<int> row_idxes_;
	// reloads bedfile_ from file_path_, returns true if successful.
	bool load();

	// calls the appropriate function depending on the current draw mode
	QString getBandText(const BedLine& region, int row, int x);
	// gets the information of the band at position x in row 0 if row is not 0, returns empty string
	QString getBandTextCollapsedMode(const BedLine& region, int row, int x);
	// same as collapsed mode but returns text based on row
	QString getBandTextExpandedMode(const BedLine& region, int row, int x);
	// returns the location plus annonations for the bedline as a string
	QString getBandString(const BedLine&);

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
