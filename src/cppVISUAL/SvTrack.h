#ifndef SVTRACK_H
#define SVTRACK_H

#include "TrackWidget.h"
#include "BedpeFile.h"
#include "ChromosomalIndex.h"
#include <memory>

struct CPPVISUALSHARED_EXPORT SvTrackSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double min_qual MEMBER min_qual)
	Q_PROPERTY(bool pass_only MEMBER pass_only)
public:
	double min_qual{};
	bool pass_only{};
};

//Displays paired BEDPE breakpoints, including mates outside the current view.
class CPPVISUALSHARED_EXPORT SvTrack : public TrackWidget
{
	Q_OBJECT
public:
	SvTrack(QWidget* parent, QString file_path, QString name);
	static QByteArray type() { return "SvTrack"; }
	static SvTrack* createTrack(QWidget* parent, QString file_path, QString name);
	void reloadTrack() override;
	QSize sizeHint() const override;

protected:
	QList<Parameter> getParameters() const override;
	QHash<QByteArray, QVariant> getParameterDefaults() const override;
	void paintEvent(QPaintEvent*) override;
	void resizeEvent(QResizeEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	bool event(QEvent*) override;

private:
	struct Placement
	{
		int index;
		int side;
		int level;
		int mate = -1;
		bool mate_visible;
		double x;
		double arrow_x;
		double arrow_y;
		QString mate_label;
		QRectF label_rect;
		QRectF mate_rect;
		QRectF bounds;
	};
	SvTrackSettings fast_settings_;
	BedpeFile variants_;
	BedFile breakpoints_;
	QVector<int> breakpoint_variants_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;
	QVector<Placement> placements_;
	int content_height_ = 30;
	double tick_top_ = 0;
	int qual_index_ = -1;
	int filter_index_ = -1;
	QPoint mouse_press_pos_;
	int pressed_variant_ = -1;
	void loadSource();
	void updateLayout();
	QString variantText(int index) const;
	int variantAt(const QPoint& pos, QRect* hit_rect = nullptr, bool labels_only = false) const;
};

#endif // SVTRACK_H
