#ifndef LIFTOVERWIDGET_H
#define LIFTOVERWIDGET_H

#include <QWidget>

namespace Ui {
class LiftOverWidget;
}

class LiftOverWidget : public QWidget
{
	Q_OBJECT

public:
	explicit LiftOverWidget(QWidget *parent = 0);
	~LiftOverWidget();

protected slots:
	void performLiftover();

private:
	Ui::LiftOverWidget *ui;
};

#endif // LIFTOVERWIDGET_H
