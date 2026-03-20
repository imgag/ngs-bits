#ifndef BEDTRACK_H
#define BEDTRACK_H


#include "cppVISUAL_global.h"
#include "ChromosomalIndex.h"
#include "TrackData.h"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>


class CPPVISUALSHARED_EXPORT BedTrack
	: public QWidget
{
	Q_OBJECT

public:
	BedTrack(QWidget* parent, QSharedPointer<TrackData> tack);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();};

signals:
	void trackDeleted();
	void trackMoved();

public slots:
	void regionChanged();
	void contextMenu(QPoint pos);

private:
	QSharedPointer<TrackData> track_;
	ChromosomalIndex<BedFile> chr_index_;

	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;


	/*
	 * utility function for mapping
	 * a \in [min_, max_] \to [c, d]
	 */
	template <typename T1, typename T2>
	T2 map(T1 a, T1 min_, T1 max_, T2 c, T2 d)
	{
		float p = (float)(a - min_)/(float)(max_ - min_);
		p = std::clamp(p, 0.f, 1.f);
		return c + (d - c) * p;
	}

	enum DrawMode
	{
		COLLAPSED,
		EXPANDED
	};

	DrawMode draw_mode_ = COLLAPSED;
	// pre count of num rows required per chromosome
	QHash<Chromosome, int> num_rows_;
	QVector<int> row_idxes_;

	QPoint drag_start_pos_;
	bool is_dragging_;

	//utility function for calculating the nubmer of rows required/chr
	void calculateNumRows();

	static const int BLOCK_HEIGHT = 10;
	static const int BLOCK_PADDING = 5;
	static const int SPACING_BELOW = 20;
};

#endif // BEDTRACK_H
