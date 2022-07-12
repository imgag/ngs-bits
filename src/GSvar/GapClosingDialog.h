#ifndef GAPCLOSINGDIALOG_H
#define GAPCLOSINGDIALOG_H

#include <QDialog>
#include "ui_GapClosingDialog.h"
#include "NGSD.h"
#include "DelayedInitializationTimer.h"

class GapClosingDialog
	: public QDialog
{
	Q_OBJECT

public:
	GapClosingDialog(QWidget* parent = 0);

protected slots:
	void delayedInitialization();
	void updateTable();
	void edit();
	void edit(int row);
	void addComment();
	void openPrimerDesign();
	void copyForPrimerGap();

private:
	Ui::GapClosingDialog ui_;
	DelayedInitializationTimer init_timer_;
	NGSD db_;
	DBTable table_;

	void gapCoordinates(int row, Chromosome& chr, int& start, int& end);
	QString exonNumber(const QByteArray& gene, int start, int end);
};

#endif // GAPCLOSINGDIALOG_H
