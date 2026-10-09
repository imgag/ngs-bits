#ifndef GENOMEVISUALIZATIONWIDGET_H
#define GENOMEVISUALIZATIONWIDGET_H

#include "cppVISUAL_global.h"
#include "BedFile.h"
#include <QWidget>
#include "DelayedInitializationTimer.h"

namespace Ui {
class GenomeVisualizationWidget;
}

//Widget for genome visaulization, similar to IGV
class CPPVISUALSHARED_EXPORT GenomeVisualizationWidget
	: public QWidget
{
	Q_OBJECT

public:
	//Default constructor
	GenomeVisualizationWidget(QWidget* parent);

public slots:
	//Shows 'open file' dialog
	void openFileDialog();
	//Showse 'open URL' dialog
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

protected:
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
	void debugMethod();

signals:
	//Emitted when the displayed region has changed.
	void regionChanged(const BedLine& reg);

private:
	Ui::GenomeVisualizationWidget* ui_;
	DelayedInitializationTimer timer_;

	QStringList valid_chrs_; //chromosome list (normalized)
	QHash<QByteArray, QSet<int>> gene_to_trans_indices_;
	QHash<QByteArray, int> trans_to_index_;
};

#endif // GENOMEVISUALIZATIONWIDGET_H
