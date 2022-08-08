#ifndef SPLICINGWIDGET_H
#define SPLICINGWIDGET_H

#include <QWidget>

namespace Ui {
class SplicingWidget;
}

class SplicingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SplicingWidget(QWidget *parent = 0);
	~SplicingWidget();

private:
	Ui::SplicingWidget *ui;
};

#endif // SPLICINGWIDGET_H
