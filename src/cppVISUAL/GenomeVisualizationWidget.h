#ifndef GENOMEVISUALIZATIONWIDGET_H
#define GENOMEVISUALIZATIONWIDGET_H

#include "cppVISUAL_global.h"
#include "BedFile.h"
#include <QWidget>

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

	//display errors to user
	static void displayError(QString msg);

public slots:
	//Triggers the 'open file' dialog
	void loadFile();
	//Triggers reload tracks for all tracks
	void reloadTracks();

protected slots:
	//Perform search based on input field (chromosome, region, gene, transcript, ...)
	void search();
	//Zoom in
	void zoomIn();
	//Zoom out
	void zoomOut();
	//Updates the region displayed by this widget
	void updateRegion();
	//Update the label that shows the genomic coordinate under the cursor
	void updateCoordinateLabel(QString text);
	//Updates indices (called when transcripts changed)
	void updateIndices();
	//Sets the region of the whole chromosome
	void setChromosomeRegion(QString chromsome);

signals:
	//Emitted when the displayed region has changed.
	void regionChanged(const BedLine& reg);

private:
	Ui::GenomeVisualizationWidget* ui_;

	QStringList valid_chrs_; //chromosome list (normalized)
	QHash<QByteArray, QSet<int>> gene_to_trans_indices_;
	QHash<QByteArray, int> trans_to_index_;
};

#endif // GENOMEVISUALIZATIONWIDGET_H
