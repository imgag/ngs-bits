#ifndef GENEPANEL_H
#define GENEPANEL_H

#include "cppVISUAL_global.h"
#include <QWidget>
#include <QLabel>
#include "BedFile.h"
#include "FastaFileIndex.h"
#include "Transcript.h"

//Panel that shows gene transcripts and nucleotides
class CPPVISUALSHARED_EXPORT GenePanel
	: public QWidget
{
	Q_OBJECT

public:
	GenePanel(QWidget* parent);

public slots:
	void setDependencies(const FastaFileIndex& genome_idx, const TranscriptList& transcripts);
	void setRegion(const BedLine& region);

private slots:
	void contextMenu(QPoint pos);

private:
	const FastaFileIndex* genome_idx_ = nullptr;
	const TranscriptList* transcripts_ = nullptr;
	BedLine reg_;
	bool strand_forward_;
	bool show_translation_;

	void paintEvent(QPaintEvent* event) override;

	//Returns the character size.
	static QSize characterSize(QFont font);
	//Returs the color to draw the given base.
	static QColor baseColor(QChar base);
	//Returns the colos to draw the given amino acid.
	static QColor aaColor(int start_index, QChar aa);
};

#endif // GENEPANEL_H
