#ifndef BEDTRACK_H
#define BEDTRACK_H


#include "cppVISUAL_global.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "TrackWidget.h"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>

//Track that displays a BedFile
class CPPVISUALSHARED_EXPORT BedTrack
	: public TrackWidget
{
	Q_OBJECT

public:
	BedTrack(QWidget* parent, QString file_path, QString name);
	~BedTrack();
	static QByteArray type() { return "BedTrack"; }

	// creates a BedTrack, same as the constructor above but
	// tries to load file_path first into a bed file if that fails
	// returns nullptr.
	static BedTrack* createTrack(QWidget* parent, QString file_path, QString name);

	// sets bedFile, re creates chromosome index, re calculates row numbers, updates geometry
	void setBedFile(QSharedPointer<BedFile> bedfile);

	QSize sizeHint() const override;

	// reloads the file, if failed deletes the track
	void reloadTrack() override;


protected:
	QList<Parameter> getParameters() const override;
	QHash<QByteArray, QVariant> getParameterDefaults() const override;
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	// region data in row
	struct RegionData
	{
		int start; // region start
		int end; // region end
		int ind; // idx in bedfile
	};


	QSharedPointer<BedFile> bedfile_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;

	QPointF mouse_press_pos_;
	/*
	 * it is more efficient to have two maps here
	 * one from index of the region to the rows --> this makes painting efficient
	 * because of the use of ChromosomalIndex
	 *
	 * the other map is from rows to vectors of regions stored in that row
	 * this makes look up faster when the user clicks one of the regions
	 */

	// pre count of num rows required per chromosome
	QHash<Chromosome, int> num_rows_;

	//hash map from row to vector of data of regions stored in that row
	using RowInfoType = QHash<int, QVector<RegionData>>;
	RowInfoType row_store_;

	// array for storing the draw index of each region
	QVector<int> row_idxes_;
	// reloads bedfile_ from file_path_, returns true if successful.
	bool load();
	//Row spacing shared by drawing, sizing and popup hit testing.
	int rowHeight() const;

	// calls the appropriate function depending on the current draw mode
	QString getRegionText(const BedLine& region, int row, int x);
	// gets the information of the region at position x in row 0 if row is not 0, returns empty string
	QString getRegionTextCollapsedMode(const BedLine& region, int row, int x);
	// same as collapsed mode but returns text based on row
	QString getRegionTextExpandedMode(const BedLine& region, int row, int x);
	// returns the location plus annonations for the bedline as a string
	QString regionTooltip(const BedLine&region);
	// handles pop up request and displays the correct information at global pos
	void handlePopupRequest(QPointF local_pos, QPointF global_pos);

	//utility function for calculating the nubmer of rows required/chr
	void calculateNumRows();

};

#endif // BEDTRACK_H
