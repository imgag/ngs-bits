#ifndef BAMALIGNMENTTRACK_H
#define BAMALIGNMENTTRACK_H

// #include "BamReader.h"
#include "TrackData.h"
#include "cppVISUAL_global.h"
#include "TrackWidget.h"
#include "RowPacker.h"

#include <QSharedPointer>

struct ReadPair
{
	int first =-1;
	int second =-1;
	int start = INT_MAX;
	int end = INT_MIN;
};

class CPPVISUALSHARED_EXPORT BamAlignmentTrack
	: public TrackWidget
{
	Q_OBJECT
public:
	explicit BamAlignmentTrack(QWidget* parent, QString file_path, QString name);
	void setTrackData(QSharedPointer<BamTrackData> track_data);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();}

	QString getType() override {return "BAM/CRAM";}

protected:
	void paintEvent(QPaintEvent*) override;
	void populateContextMenu(QMenu&) override;
	void handleContextMenuAction(QAction*) override;

private:
	//constants
	static constexpr int ROW_HEIGHT = 10;
	static constexpr int ROW_PADDING = 2;
	static constexpr int SPACING_BELOW = 4;

	static constexpr float CIGAR_DETAIL_SCALE = 20.0f;
	static constexpr float BASE_DETAIL_SCALE  = 1.0f;

	QSharedPointer<BamTrackData> track_data_;

	void drawZoomInText(QPainter&);
	void calculateRows();
	void calculateRowsNormalMode();
	void calculateRowsPairMode();

	void drawNormalMode(QPainter& painter, const BedLine& region);
	void drawPairMode(QPainter& painter, const BedLine& region);

	void drawAlignment(QPainter&, const BamAlignment& al, int row_y,
					   int x0, int total_width);
	void drawVariants(QPainter&, const BamAlignment& al, int row_y,
					  int x0, int total_width);
	void makePairs();

	static QColor baseColor(QChar base);
	static QColor strandColor(bool is_reverse);
	static QSize characterSize(QFont font);

	QHash<BamAlignmentWrapper, int> row_idxes_;
	QHash<QString, int> pair_row_idxes_;
	QHash<QString, bool> row_stored_with_pair_;
	RowPacker row_packer_;
	using PairMap = QHash<QString, QPair<int, int>>;
	PairMap pair_map_;
	QVector<ReadPair> read_pairs_;

	int num_rows_ = 1;
	bool view_as_pairs_ = false;
	QAction* pairs_action_;

private slots:
	void dataReady();
};

#endif // BAMALIGNMENTTRACK_H
