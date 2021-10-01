#ifndef GENOMEVISUALIZATIONWIDGET_H
#define GENOMEVISUALIZATIONWIDGET_H

#include "cppVISUAL_global.h"
#include "FastaFileIndex.h"
#include "BedFile.h"
#include <QWidget>

namespace Ui {
class GenomeVisualizationWidget;
}

struct CPPVISUALSHARED_EXPORT GenomeVisualizationSettings
{
	int min_window_size = 40;
};

class CPPVISUALSHARED_EXPORT GenomeVisualizationWidget
	: public QWidget
{
	Q_OBJECT

public:
	GenomeVisualizationWidget(QWidget* parent, const FastaFileIndex& genome_idx);

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

signals:
	//Emitted when the displayed region has changed.
	void regionChanged(const BedLine& reg);

private:
	Ui::GenomeVisualizationWidget* ui_;
	GenomeVisualizationSettings settings_;
	const FastaFileIndex& genome_idx_;
	QStringList valid_chrs_; //chromosome list (normalized)
	BedLine current_reg_;
};

#endif // GENOMEVISUALIZATIONWIDGET_H
