#ifndef GENEPANEL_H
#define GENEPANEL_H

#include <QScrollArea>
#include "BedFile.h"
#include "FastaFileIndex.h"
#include "Transcript.h"

namespace Ui {
class GenePanel;
}

//Panel that shows gene transcripts and nucleotides
class GenePanel
	: public QScrollArea
{
	Q_OBJECT

public:
	GenePanel(QWidget* parent, const FastaFileIndex& genome_idx, const TranscriptList& transcripts);

public slots:
	void setRegion(const BedLine& region);

private:
	Ui::GenePanel* ui_;
	const FastaFileIndex& genome_idx_;
	const TranscriptList& transcripts_;
};

#endif // GENEPANEL_H
