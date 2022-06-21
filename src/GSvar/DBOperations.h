#ifndef DBOPERATIONS_H
#define DBOPERATIONS_H

#include <QWidget>
#include "DelayedInitializationTimer.h"
#include "ui_DBOperations.h"
#include "NGSD.h"

class DBOperations : public QWidget
{
	Q_OBJECT

public:
	DBOperations(QWidget *parent = 0);

protected slots:
	void delayedInitialization();
	void exportTable();

private:
	Ui::DbOperations ui_;
	DelayedInitializationTimer init_timer_;
	NGSD db_;
};

#endif // DBOPERATIONS_H
