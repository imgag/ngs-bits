#ifndef GENOMEVISUALIZATIONWIDGET_H
#define GENOMEVISUALIZATIONWIDGET_H

#include "cppVISUAL_global.h"
#include "GenomeData.h"
#include "BedFile.h"
#include <QWidget>

namespace Ui {
class GenomeVisualizationWidget;
}

//Settings for GenomeVisualizationWidget
struct CPPVISUALSHARED_EXPORT GenomeVisualizationSettings
{
	int min_window_size = 40;
	int transcript_padding = 2000;
};

//TODO Marc: make struct with genome, transcript data and all indices that can be used in all panels

//Widget for genome visaulization, similar to IGV
class CPPVISUALSHARED_EXPORT GenomeVisualizationWidget
	: public QWidget
{
	Q_OBJECT

public:
	//Defaukt constructor. Make sure to call setTranscrips before doing anything else!
	GenomeVisualizationWidget(QWidget* parent);

    //Sets genome data
    void setGenomeData(QSharedPointer<GenomeData> data);

	//Sets visualized region (1-based)
	void setRegion(const Chromosome& chr, int start, int end);

protected slots:
	//Sets the region of the whole chromosome.
	void setChromosomeRegion(QString chromsome);
	//Perform search based on input field (chromosome, region, gene, transcript, ...)
	void search();
	//Zoom in
	void zoomIn();
	//Zoom out
	void zoomOut();
	//Update widgets that show the current region
	void updateRegionWidgets(const BedLine& reg);
	//
	void updateCoordinateLabel(QString text);

signals:
	//Emitted when the displayed region has changed.
	void regionChanged(const BedLine& reg);

private:
	Ui::GenomeVisualizationWidget* ui_;
    GenomeVisualizationSettings settings_;
    QSharedPointer<GenomeData> genome_data_;

	QStringList valid_chrs_; //chromosome list (normalized)
	QHash<QByteArray, QSet<int>> gene_to_trans_indices_;
	QHash<QByteArray, int> trans_to_index_;
	BedLine current_reg_;
};

#endif // GENOMEVISUALIZATIONWIDGET_H
