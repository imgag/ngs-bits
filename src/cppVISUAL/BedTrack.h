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

	static QString staticType() {return "BED";}
	QString getType() override {return staticType();}

	QMap<QString, QVariant> getSettings() override;
	void loadKeyValueFromXml(QString, const QDomElement&) override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void populateContextMenu(QMenu& menu) override;

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
	/*
	 * it is more efficient to have two maps here
	 * one from index of the band to the rows --> this makes painting efficient
	 * because of the use of ChromosomalIndex
	 *
	 * the other map is from rows to vectors of bands stored in that row
	 * this makes look up faster when the user clicks one of the bands
	 */

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
	// handles pop up request and displays the correct information at global pos
	void handlePopupRequest(QPointF local_pos, QPointF global_pos);

	//utility function for calculating the nubmer of rows required/chr
	void calculateNumRows();

	enum DrawMode
	{
		COLLAPSED,
		EXPANDED
	};

	DrawMode draw_mode_ = COLLAPSED;
};

#endif // BEDTRACK_H
