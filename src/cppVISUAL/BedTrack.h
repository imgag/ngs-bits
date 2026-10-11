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
	static QByteArray type() { return "BedTrack"; }

	// creates a BedTrack, same as the constructor above but
	// tries to load file_path first into a bed file if that fails
	// returns nullptr.
	static BedTrack* createTrack(QWidget* parent, QString file_path, QString name);

	//Sets a non-null, sorted BED file and rebuilds the index and visible rows.
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
	QSharedPointer<BedFile> bedfile_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;

	QPointF mouse_press_pos_;
	//BED indices and packed row numbers for the current view.
	QVector<int> visible_indices_;
	QHash<int, int> row_idxes_;
	int num_rows_ = 0;
	// reloads bedfile_ from file_path_, returns true if successful.
	bool load();
	//Row spacing shared by drawing, sizing and popup hit testing.
	int rowHeight() const;
	QFont regionFont() const;

	// calls the appropriate function depending on the current draw mode
	QString getRegionText(const BedLine& region, int row, int x);
	// returns the location plus annonations for the bedline as a string
	QString regionTooltip(const BedLine&region);
	// handles pop up request and displays the correct information at global pos
	void handlePopupRequest(QPointF local_pos, QPointF global_pos);

	//Packs only regions overlapping the current view.
	void calculateRows();

};

#endif // BEDTRACK_H
