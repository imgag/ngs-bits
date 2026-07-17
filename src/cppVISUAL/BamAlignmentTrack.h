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
	explicit BamAlignmentTrack(QWidget* parent, QString file_path, QString name);
	void setTrackData(QSharedPointer<BamTrackData> track_data);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

	static QString staticType() {return "BAM/CRAM";}
	QString getType() override {return staticType();}

	virtual QMap<QString, QVariant> getSettings() override;
	virtual void loadKeyValueFromXml(QString key, QString value) override;

	virtual void reloadTrack() override;

	static BamAlignmentTrack* createTrack(QWidget* parent, QString file_path, QString name);

protected:
	void paintEvent(QPaintEvent*) override;
	void populateContextMenu(QMenu&, const QPoint&) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;



private:
	QSharedPointer<BamTrackData> track_data_;

	void drawZoomInText(QPainter&);

	// assigns rows to alignments and calculates number of rows
	// calls the right method depending on view mode
	void calculateRows();
	// assigns rows for normal mode
	void calculateRowsNormalMode();
	// assigns rows to pairs for pair mode
	void calculateRowsPairMode();

	void drawNormalMode(QPainter& painter);
	void drawPairMode(QPainter& painter, const BedLine& region);

	void drawAlignmentAndMismatches(QPainter&, const BamAlignmentWrapper& al, int row_y);

	void drawAlignment(QPainter&, const BamAlignmentWrapper& al, int row_y);
	void drawBase(QPainter&, const Viewport& view_port, int genome_pos,
				  char base, int qual, int row_y);
	// draws bases in BamAlignment that do not match the reference base
	// which are pre calculated in the AlignmentWrapper
	void drawMismatches(QPainter&, const BamAlignmentWrapper& al, int row_y);
	// draws all bases (only if show_all_bases_ flag is true), draws soft clip bases too
	// if the corresponding flag is set
	void drawAllBases(QPainter&, const BamAlignmentWrapper& al, int row_y);
	// draws a highlight on the given alignment
	void drawHighlight(QPainter&, const BamAlignmentWrapper& al, int row_y);

	QString getBamAlignmentText(const BamAlignmentWrapper& al, int genome_pos);
	// iterates through the alignments and stores pairs as ReadPair in read_pairs_
	void makePairs();

	void updateFontCache();

	static QColor baseColor(QChar base);
	static QColor strandColor(bool is_reverse);
	static QSize characterSize(QFont font);
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);
	int getAlnIndexFromLocalPos(QPoint local_pos);


	/*TODO: all of these need to be stored as LRUCache*/
	QHash<AlignmentKey, int> row_idxes_; // BamAlignmentWrapperId -> row index
	QHash<QString, int> pair_row_idxes_; // Pair Name -> row index

	QHash<QString, bool> row_stored_with_pair_; // Pair Name -> bool, used for checking if alignment with name was stored as a pair or not

	QHash<int, QVector<int>> normal_row_store_; // row -> vector of alignment indices
	QHash<int, QVector<int>> pair_row_store_; // row -> vector of pair indices


	RowPacker row_packer_;
	QVector<ReadPair> read_pairs_; // vector of pairs

	int num_rows_ = 1;
	QPoint mouse_press_pos_;
	QSize cached_char_size_;
	QFont cached_font_;

	//settings
	bool view_as_pairs_ = false;
	bool show_all_bases_ = false;
	bool show_soft_clip_bases_ = false;
	QString selected_name_ = ""; // name of selected alignment


private slots:
	void dataReady();
	void fullLoad();
};

#endif // BAMALIGNMENTTRACK_H
