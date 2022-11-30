#ifndef GENEPANEL_H
#define GENEPANEL_H

#include "cppVISUAL_global.h"
#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include "BedFile.h"
#include "FastaFileIndex.h"
#include "Transcript.h"
#include "ChromosomalIndex.h"

//Settings struct for gene panel
struct CPPVISUALSHARED_EXPORT GenePanelSettings
{
	bool strand_forward = true;
	bool show_translation = false;
	bool show_only_ensembl = true;
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
	//transcript positions
	struct TranscriptPosition
	{
		Transcript trans;
		int row; //row index (0=topmost)
		QRectF rect; //bounding rectangle
	};
	QList<TranscriptPosition> trans_positions_;

	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	bool event(QEvent *event) override; //tooltip

	//Returns the character size.
	static QSize characterSize(QFont font);
	//Returs the color to draw the given base.
	static QColor baseColor(QChar base);
	//Returns the colos to draw the given amino acid.
	static QColor aaColor(int start_index, QChar aa);
	//Returns the number of pixels per base.
	double pixelsPerBase() const;
	//Draws a transcript at the topmost free position
	void drawTranscript(QPainter& painter, const Transcript& trans, int y_content_start, QColor color);
	//Calculates the y position where there is space for the transcript with the given start/end x coordinate
	int transcriptY(double x_start, double x_end, int y_content_start, int trans_height, const Transcript& trans);
	//Returns the start x coordinate of the given chromosomal coordinate.
	double baseStartX(int pos, bool restrict_to_content_area) const;
	//Returns the end x coordinate of the given chromosomal coordinate.
	double baseEndX(int pos, bool restrict_to_content_area) const;
};

#endif // GENEPANEL_H
