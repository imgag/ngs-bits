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
	void loadMidsFromCache();

private slots:
	void filter(QString text);
	void delayedInizialization();

private:
	Ui::MidList* ui;
};

#endif // MIDLIST_H
