#ifndef TRACKPANEL_H
#define TRACKPANEL_H


#include "cppVISUAL_global.h"
#include "Track.h"
#include <QWidget>


class CPPVISUALSHARED_EXPORT TrackPanel
	: public QWidget
{
	Q_OBJECT

public:
	TrackPanel(QWidget* parent, Track tack);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override {return sizeHint();};

public slots:
	void regionChanged();
	void contextMenu(QPoint pos);

private:
	Track track;
	void paintEvent(QPaintEvent* event) override;
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
	int num_rows_;
	int calculateNumRows();

	static const int BLOCK_HEIGHT = 10;
	static const int BLOCK_PADDING = 5;
	static const int SPACING_BELOW = 20;
};

#endif // TRACKPANEL_H
