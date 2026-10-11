#ifndef BAMALIGNMENTTRACK_H
#define BAMALIGNMENTTRACK_H

// #include "BamReader.h"
#include "BamTrackData.h"
#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "RowPacker.h"

#include <QSharedPointer>

// struct for storing a pair of reads
struct CPPVISUALSHARED_EXPORT ReadPair
{
	// after initialization, first will never be -1; however, second may
	// be -1 if no pair is found
	int first =-1; // index of the first alignment
	int second =-1; // index of the second alignment
	int start = INT_MAX; // min of the starts of the two alignments
	int end = INT_MIN; // max of the ends of the two alignments
};

//Track that displays alignmnets in a BAM/CRAM file
class CPPVISUALSHARED_EXPORT BamAlignmentTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	BamAlignmentTrack(QWidget* parent, QString file_path, QString name);
	~BamAlignmentTrack();
	static QByteArray type() { return "BamAlignmentTrack"; }

	void setTrackData(QSharedPointer<BamTrackData> track_data);

	QSize sizeHint() const override;


	virtual void reloadTrack() override;

	static BamAlignmentTrack* createTrack(QWidget* parent, QString file_path, QString name);

protected:
	QList<Parameter> getParameters() const override;
	QHash<QByteArray, QVariant> getParameterDefaults() const override;
	void paintEvent(QPaintEvent*) override;
	void populateContextMenu(QMenu&, const QPoint&) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent* event) override;

private:
	// Read once before processing or painting alignments.
	struct DisplaySettings
	{
		bool view_as_pairs;
		bool show_all_bases;
		bool show_soft_clip_bases;
		bool show_secondary_alignments;
		QByteArray group_by;
		QByteArray coloring_scheme;
	};
	DisplaySettings displaySettings() const;
	QSharedPointer<BamTrackData> track_data_;

	void drawZoomInText(QPainter&);

	// assigns rows to alignments and calculates number of rows
	// calls the right method depending on view mode
	void calculateRows(const DisplaySettings& options);
	// assigns rows for normal mode
	void calculateRowsNormalMode(const DisplaySettings& options);
	// assigns rows to pairs for pair mode
	void calculateRowsPairMode();
	// Packs haplotypes 1, 2, and unassigned into separate groups.
	void calculateRowsByPhase(const DisplaySettings& options);
	// computes the stats for insertsize
	void computeInsertSizeStats();
	// iterates through the alignments and stores pairs as ReadPair in read_pairs_
	void makePairs(const DisplaySettings& options);

	void drawNormalMode(QPainter& painter, const DisplaySettings& options);
	void drawPairMode(QPainter& painter, const BedLine& region, const DisplaySettings& options);

	void drawAlignmentAndMismatches(QPainter&, const BamAlignmentWrapper& al, int row_y, const DisplaySettings& options);

	void drawAlignment(QPainter&, const BamAlignmentWrapper& al, int row_y, const DisplaySettings& options);
	void drawBase(QPainter&, const Viewport& view_port, int genome_pos,
				  char base, int qual, int row_y);
	// draws bases in BamAlignment that do not match the reference base
	// which are pre calculated in the AlignmentWrapper
	void drawMismatches(QPainter&, const BamAlignmentWrapper& al, int row_y);
	// draws all bases if enabled in settings_, including soft clips if enabled
	// if the corresponding flag is set
	void drawAllBases(QPainter&, const BamAlignmentWrapper& al, int row_y, const DisplaySettings& options);
	// draws a highlight on the given alignment
	void drawHighlight(QPainter&, const BamAlignmentWrapper& al, int row_y, bool show_soft_clip_bases);

	QString getBamAlignmentText(const BamAlignmentWrapper& al, int genome_pos, bool show_soft_clip_bases);

	void updateFontCache();

	static QColor baseColor(QChar base);
	static QColor strandColor(bool is_reverse);
	QColor insertSizeColor(const BamAlignmentWrapper&); //

	static QSize characterSize(QFont font);
	// display info of Alignment that was at local pos
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);
	// gives the index of alignment that is being display at localpos
	// for pair_mode this gives the index in the read_pairs_ list, from which
	// pair.first, pair.second can be used for getting the exact index
	int getAlnIndexFromLocalPos(QPoint local_pos);
	// returns true if size of region < max_region_len
	bool isCurrentRegionValid();
	// gives the start and end point of the alignments
	// if current mode is show_clip_bases_, this gives start and end with that
	static int getAlignmentStart(const BamAlignmentWrapper&, bool show_soft_clip_bases);
	static int getAlignmentEnd(const BamAlignmentWrapper&, bool show_soft_clip_bases);
	static bool isAlignmentVisible(const BamAlignmentWrapper& alignment, bool show_secondary_alignments);
	// returns color of alignment based on its properties and current coloring scheme
	QColor getAlignmentColor(const BamAlignmentWrapper&, const QByteArray& coloring_scheme);
	// adds Go To Mate, select/delect options to context menu
	void addAlignmentOptionsToCtxtMenu(QMenu& menu, const QPoint& local_pos);

	QHash<AlignmentKey, int> row_idxes_; // BamAlignmentWrapperId -> row index
	QHash<AlignmentKey, int> pair_row_idxes_; // First alignment ID -> row index

	QHash<AlignmentKey, bool> row_stored_with_pair_; // First alignment ID -> whether a mate was stored

	QHash<int, QVector<int>> normal_row_store_; // row -> vector of alignment indices
	QHash<int, QVector<int>> pair_row_store_; // row -> vector of pair indices


	RowPacker row_packer_;
	QVector<ReadPair> read_pairs_; // vector of pairs

	int num_rows_ = 1;
	QPoint mouse_press_pos_;
	QSize cached_char_size_;
	QFont cached_font_;

	QString selected_name_ = ""; // name of selected alignment


private slots:
	void dataReady();
	void fullLoad();
};

#endif // BAMALIGNMENTTRACK_H
