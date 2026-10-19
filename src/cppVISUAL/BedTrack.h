#ifndef BEDTRACK_H
#define BEDTRACK_H

#include "cppVISUAL_global.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "TrackWidget.h"
#include "TabixIndexedFile.h"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>
#include <memory>

//Fast read-only parameters access; populated and maintained by ParameterList::link().
struct CPPVISUALSHARED_EXPORT BedTrackSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QByteArray color MEMBER color)
	Q_PROPERTY(QByteArray draw_mode MEMBER draw_mode)
	Q_PROPERTY(bool show_region_names MEMBER show_region_names)

public:
	QByteArray color{};
	QByteArray draw_mode{};
	bool show_region_names{};
};

//Track that displays a BED file
class CPPVISUALSHARED_EXPORT BedTrack
	: public TrackWidget
{
	Q_OBJECT

public:
	BedTrack(QWidget* parent, QString file_path, QString name);
	static QByteArray type() { return "BedTrack"; }

	//Loads a BED or opens a tabix-indexed BED.GZ. Returns nullptr on failure.
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
	BedTrackSettings fast_settings_;
	QSharedPointer<BedFile> bedfile_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;
	std::unique_ptr<TabixIndexedFile> tabix_;
	QString load_error_;
	//Installs either an in-memory BED or the current tabix query result.
	void setBedData(QSharedPointer<BedFile> bedfile);

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

	// handles pop up request and displays the correct information at global pos
	void handlePopupRequest(QPointF local_pos, QPointF global_pos);

	//Packs only regions overlapping the current view.
	void calculateRows();

};

#endif // BEDTRACK_H
