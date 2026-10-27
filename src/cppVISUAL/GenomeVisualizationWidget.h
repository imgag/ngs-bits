#ifndef GENOMEVISUALIZATIONWIDGET_H
#define GENOMEVISUALIZATIONWIDGET_H

#include "cppVISUAL_global.h"
#include "BedFile.h"
#include <QWidget>
#include <QPointer>
#include "DelayedInitializationTimer.h"
class BamAlignmentTrack;

namespace Ui
{
class GenomeVisualizationWidget;
}

class ParameterEditor;
class TrackWidget;

//Widget for genome visaulization, similar to IGV
class CPPVISUALSHARED_EXPORT GenomeVisualizationWidget
	: public QWidget
{
	Q_OBJECT

public:
	//Default constructor
	GenomeVisualizationWidget(QWidget* parent);
	~GenomeVisualizationWidget() override;
	//Adds a generated BED track with a backing file for reload/session persistence.
	void addBedTrack(QSharedPointer<BedFile> regions, QString file_path, QString name);

	QList<BamAlignmentTrack*> bamTracks();
	//Returns the owning viewer of a panel/track. A viewer parent is required.
	static GenomeVisualizationWidget* fromWidget(QWidget* widget);
	//Invalid in the whole-genome overview. Each viewer owns its position.
	const BedLine& region() const { return region_; }

public slots:
	//An invalid chromosome selects the whole-genome overview.
	void setRegion(const Chromosome& chr, int start, int end);
	//Shows 'open file' dialog
	void openFileDialog();
	//Shows 'open URL' dialog
	void openUrlDialog();
	//Loads a file
	void loadFile(QString filename);
	//Triggers reload tracks for all tracks
	void reloadTracks();
	//Clears everything
	void clearSession();
	//Save current session
	void saveSession();
	// load session from file, triggers 'open file dialog' and loads the session
	void loadSession();
	// One-time synchronisation between two instances
	void synchroniseFrom(GenomeVisualizationWidget* widget);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;
	//set up debug button
	void setupDebugButton();
	// handles zoom in/out
	void wheelEvent(QWheelEvent* event) override;

protected slots:
	//Delayed initialization
	void delayedInitialization();
	//Perform search based on input field (chromosome, region, gene, transcript, ...)
	void search();
	//Zoom in
	void zoomIn();
	//Zoom out
	void zoomOut();
	//Zoom in centered at genome pos x
	void zoomIn(int x);
	//Zoom out centered at genome pos x
	void zoomOut(int x);
	//Updates the region displayed by this widget
	void updateRegion();
	//Update the label that shows the genomic coordinate under the cursor
	void updateCoordinateLabel(QString text);
	//Updates indices (called when transcripts changed)
	void updateIndices();
	//Sets the region of the whole chromosome
	void setChromosomeRegion(QString chromsome);
	//Debugging method
	//Toggle settings visbilty
	void toggleSettingsVisibility();

signals:
	//Emitted when the displayed region has changed.
	void regionChanged(const BedLine& reg);
	void updateGenomicCoordinate(QString coordinate);

private:
	BedLine region_;
	void mouseMoved(int x, int width);
	Ui::GenomeVisualizationWidget* ui_;
	DelayedInitializationTimer timer_;
	QPointer<TrackWidget> selected_track_;
	ParameterEditor* parameter_editor_ = nullptr;
	QMetaObject::Connection selected_track_destroyed_;
	void selectTrack(TrackWidget* track);
	void clearParameterEditor();
	void setupBookmarkMenu();
	void storeBookmark();

	QStringList valid_chrs_; //chromosome list (normalized)
	QHash<QByteArray, QSet<int>> gene_to_trans_indices_;
	QHash<QByteArray, int> trans_to_index_;
};

#endif // GENOMEVISUALIZATIONWIDGET_H
