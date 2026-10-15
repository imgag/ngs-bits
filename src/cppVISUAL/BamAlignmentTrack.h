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

//Fast read-only parameters access; populated and maintained by ParameterList::link().
struct CPPVISUALSHARED_EXPORT BamAlignmentTrackSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool view_as_pairs MEMBER view_as_pairs)
	Q_PROPERTY(bool show_all_bases MEMBER show_all_bases)
	Q_PROPERTY(bool show_soft_clipped_bases MEMBER show_soft_clipped_bases)
	Q_PROPERTY(bool show_secondary_alignments MEMBER show_secondary_alignments)
	Q_PROPERTY(int min_mq MEMBER min_mq)
	Q_PROPERTY(QByteArray group_by MEMBER group_by)
	Q_PROPERTY(QByteArray color_by MEMBER color_by)
	Q_PROPERTY(int visibility_range MEMBER visibility_range)

public:
	bool view_as_pairs{};
	bool show_all_bases{};
	bool show_soft_clipped_bases{};
	bool show_secondary_alignments{};
	int min_mq{};
	QByteArray group_by{};
	QByteArray color_by{};
	int visibility_range{};
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
	BamAlignmentTrackSettings fast_settings_;
	bool isGroupingEnabled() const;
	QSharedPointer<BamTrackData> track_data_;

	void drawZoomInText(QPainter&);

	// assigns rows to alignments and calculates number of rows
	// calls the right method depending on view mode
	void calculateRows();
	// assigns rows for normal mode
	void calculateRowsNormalMode();
	// assigns rows to pairs for pair mode
	void calculateRowsPairMode();
	// Packs reads into groups according to the selected grouping mode.
	void calculateGroupedRows();
	// iterates through the alignments and stores pairs as ReadPair in read_pairs_
	void makePairs();

	void drawNormalMode(QPainter& painter);
	void drawPairMode(QPainter& painter, const BedLine& region);
	void drawGroups(QPainter& painter);

	void drawAlignmentAndMismatches(QPainter&, const BamAlignmentWrapper& al, int row_y);

	void drawAlignment(QPainter&, const BamAlignmentWrapper& al, int row_y);
	void drawBase(QPainter&, const Viewport& view_port, int genome_pos,
				  char base, int qual, int row_y);
	// draws bases in BamAlignment that do not match the reference base
	// which are pre calculated in the AlignmentWrapper
	void drawMismatches(QPainter&, const BamAlignmentWrapper& al, int row_y);
	// draws all bases if enabled in settings_, including soft clips if enabled
	// if the corresponding flag is set
	void drawAllBases(QPainter&, const BamAlignmentWrapper& al, int row_y);

	QString getBamAlignmentText(const BamAlignmentWrapper& al, int genome_pos);

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
	int getAlignmentStart(const BamAlignmentWrapper&) const;
	int getAlignmentEnd(const BamAlignmentWrapper&) const;
	bool isAlignmentVisible(const BamAlignmentWrapper& alignment) const;
	// returns color of alignment based on its properties and current coloring scheme
	QColor getAlignmentColor(const BamAlignmentWrapper&);
	// adds Go To Mate, select/delect options to context menu
	void addAlignmentOptionsToCtxtMenu(QMenu& menu, const QPoint& local_pos);

	QHash<AlignmentKey, int> row_idxes_; // BamAlignmentWrapperId -> row index
	QHash<AlignmentKey, int> pair_row_idxes_; // First alignment ID -> row index

	QHash<AlignmentKey, bool> row_stored_with_pair_; // First alignment ID -> whether a mate was stored


	RowPacker row_packer_;
	QVector<ReadPair> read_pairs_; // vector of pairs
	struct ReadGroup
	{
		QString label;
		int first_row;
	};
	QVector<ReadGroup> read_groups_;

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
