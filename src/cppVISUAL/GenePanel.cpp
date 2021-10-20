#include "GenePanel.h"
#include "Helper.h"
#include <QDebug>

#include "ui_GenePanel.h"

GenePanel::GenePanel(QWidget *parent, const FastaFileIndex& genome_idx, const TranscriptList& transcripts)
	: QScrollArea(parent)
	, ui_(new Ui::GenePanel())
	, genome_idx_(genome_idx)
	, transcripts_(transcripts)
{
}

void GenePanel::setRegion(const BedLine& region)
{
	qDebug() << "GENE PANEL:" << region.toString(true) << region.length();

	//paint name pane (165px)

	//paint sequence and translation
	Sequence seq = genome_idx_.seq(region.chr(), region.start(), region.length());

	//paint genes
}

