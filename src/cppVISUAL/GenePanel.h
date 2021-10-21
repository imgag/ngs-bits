#ifndef GENEPANEL_H
#define GENEPANEL_H

#include "cppVISUAL_global.h"
#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include "BedFile.h"
#include "FastaFileIndex.h"
#include "Transcript.h"

//Settings struct for gene panel
struct CPPVISUALSHARED_EXPORT GenePanelSettings
{
	bool strand_forward_ = true;
	bool show_translation_ = false;
	int label_width  = 165;
};

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

signals:
	void mouseCoordinate(QString);

private slots:
	void contextMenu(QPoint pos);

private:
	//general members
	GenePanelSettings settings_;
	const FastaFileIndex* genome_idx_ = nullptr;
	const TranscriptList* transcripts_ = nullptr;
	ChromosomalIndex<TranscriptList>* transcripts_idx_;
	BedLine reg_;
	//members needed for paint event - updated when resizing occurs
	double pixels_per_base_;
	QSize char_size_;


	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

	//Returns the character size.
	static QSize characterSize(QFont font);
	//Returs the color to draw the given base.
	static QColor baseColor(QChar base);
	//Returns the colos to draw the given amino acid.
	static QColor aaColor(int start_index, QChar aa);
	//Returns the number of pixels per base.
	double pixelsPerBase() const;
	//Draws a transcript at the topmost free position
	void drawTranscript(QPainter& painter, const Transcript& trans, int y, QColor color);
	//Returns the start x coordinate of the given chromosomal coordinate.
	double baseStartX(int pos, bool restrict_to_content_area) const;
	//Returns the end x coordinate of the given chromosomal coordinate.
	double baseEndX(int pos, bool restrict_to_content_area) const;
};

#endif // GENEPANEL_H
