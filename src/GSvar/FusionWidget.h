#ifndef FUSIONWIDGET_H
#define FUSIONWIDGET_H

#include <QWidget>

namespace Ui {
class FusionWidget;
}

class FusionWidget : public QWidget
{
	Q_OBJECT

public:
	explicit FusionWidget(QWidget *parent = 0);
	~FusionWidget();

private:
	Ui::FusionWidget *ui;
};

#endif // FUSIONWIDGET_H
