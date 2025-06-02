#ifndef PATHOGENICWTDIALOG_H
#define PATHOGENICWTDIALOG_H

#include "ui_PathogenicWtDialog.h"
#include "DelayedInitializationTimer.h"


class PathogenicWtDialog
	: public QDialog
{
	Q_OBJECT

public:
	PathogenicWtDialog(QWidget* parent, QString bam, bool is_long_read);

private slots:
	void delayedInitialization();
	void variantsContextMenu(QPoint pos);

private:
	Ui::PathogenicWtDialog ui_;
	QString bam_;
	bool is_long_read_;
	DelayedInitializationTimer delayed_init_timer_;
};

#endif // PATHOGENICWTDIALOG_H
