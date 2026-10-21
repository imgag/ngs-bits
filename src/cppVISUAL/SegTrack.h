#ifndef SEGTRACK_H
#define SEGTRACK_H

#include "TrackWidget.h"
#include "SegFile.h"
#include "ChromosomalIndex.h"

struct CPPVISUALSHARED_EXPORT SegTrackSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QByteArray draw_mode MEMBER draw_mode)
	Q_PROPERTY(QByteArray color MEMBER color)
	Q_PROPERTY(QByteArray alt_color MEMBER alt_color)
	Q_PROPERTY(double min MEMBER min)
	Q_PROPERTY(double max MEMBER max)
	Q_PROPERTY(double reference MEMBER reference)
	Q_PROPERTY(bool show_reference MEMBER show_reference)
	Q_PROPERTY(int track_height MEMBER track_height)

public:
	QByteArray draw_mode{}, color{}, alt_color{};
	double min{}, max{}, reference{};
	bool show_reference{};
	int track_height{};
};

//Numeric SEG track with the fifth column retained as a popup annotation.
class CPPVISUALSHARED_EXPORT SegTrack : public TrackWidget
{
	Q_OBJECT
public:
	SegTrack(QWidget* parent, QString file_path, QString name);
	static QByteArray type() { return "SegTrack"; }
	static SegTrack* createTrack(QWidget* parent, QString file_path, QString name = "");
	void setSegFile(QSharedPointer<SegFile> file);
	void reloadTrack() override;
	QSize sizeHint() const override;

protected:
	QList<Parameter> getParameters() const override;
	QHash<QByteArray, QVariant> getParameterDefaults() const override;
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;

private:
	SegTrack(QWidget* parent, QString file_path, QString name, QSharedPointer<SegFile> file);
	SegTrackSettings fast_settings_;
	QSharedPointer<SegFile> file_;
	std::unique_ptr<ChromosomalIndex<SegFile>> chr_index_;
	QPoint mouse_press_pos_;
	double valueToY(double value) const;
	double displayedValue(double value) const;
	QColor segmentColor(double value) const;
	void drawPlot(QPainter& painter, const BedLine& region, const Viewport& viewport);
	void handlePopupRequest(QPoint local_pos, QPointF global_pos);
};

#endif // SEGTRACK_H
