#ifndef MIDLIST_H
#define MIDLIST_H

#include <QWidget>

namespace Ui {
class MidList;
}

class MidList
	: public QWidget
{
	Q_OBJECT

public:
	MidList(QWidget *parent = 0);
	~MidList();

private slots:
	void filter(QString text);

private:
	Ui::MidList* ui;
	void loadMidsFromDB();
};

#endif // MIDLIST_H
