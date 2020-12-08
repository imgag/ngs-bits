#ifndef PRSVIEW_H
#define PRSVIEW_H

#include <QWidget>

namespace Ui {
class PRSView;
}

class PRSView : public QWidget
{
	Q_OBJECT

public:
	explicit PRSView(QWidget *parent = 0);
	~PRSView();

private:
	Ui::PRSView *ui;
};

#endif // PRSVIEW_H
