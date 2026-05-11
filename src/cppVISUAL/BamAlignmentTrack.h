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
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;

private:
	QSharedPointer<BamTrackData> track_data_;

	void drawZoomInText(QPainter&);
	void calculateRows();
	void calculateRowsNormalMode();
	void calculateRowsPairMode();

	void drawNormalMode(QPainter& painter, const BedLine& region);
	void drawPairMode(QPainter& painter, const BedLine& region);

	void drawAlignment(QPainter&, const BamAlignment& al, int row_y,
					   int x0, int total_width);
	void drawVariants(QPainter&, const BamAlignmentWrapper& al, int row_y,
					  int x0, int total_width);
	void makePairs();

	static QColor baseColor(QChar base);
	static QColor strandColor(bool is_reverse);
	static QSize characterSize(QFont font);
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);

	QHash<BamAlignmentWrapper, int> row_idxes_; // BamAlignmentWrapper -> row index
	QHash<QString, int> pair_row_idxes_; // Pair Name -> row index

	QHash<QString, bool> row_stored_with_pair_; // Pair Name -> bool, used for checking if alignment with name was stored as a pair or not

	QHash<int, QVector<int>> normal_row_store_; // row -> vector of alignment indices
	QHash<int, QVector<int>> pair_row_store_; // row -> vector of pair indices


	RowPacker row_packer_;
	QVector<ReadPair> read_pairs_; // vector of pairs

	int num_rows_ = 1;
	bool view_as_pairs_ = false;
	QAction* pairs_action_;
	QPoint mouse_press_pos_;

private slots:
	void dataReady();
};

#endif // BAMALIGNMENTTRACK_H
