#ifndef TRACKPANEL_H
#define TRACKPANEL_H


#include "cppVISUAL_global.h"
#include "Track.h"
#include "ChromosomalIndex.h"
#include <QWidget>


class CPPVISUALSHARED_EXPORT TrackPanel
	: public QWidget
{
	Q_OBJECT

public:
	TrackPanel(QWidget* parent, Track tack);

public slots:
	void regionChanged();

private:
	Track track;
	void paintEvent(QPaintEvent* event) override;
	ChromosomalIndex<BedFile> chrIdx;
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
};

#endif // TRACKPANEL_H
