#ifndef CANDIDATEGENEDIALOG_H
#define CANDIDATEGENEDIALOG_H

#include <QDialog>
#include "ui_CandidateGeneDialog.h"

class CandidateGeneDialog
	: public QDialog
{
	Q_OBJECT

public:
	CandidateGeneDialog(QWidget *parent = 0);

	void keyPressEvent(QKeyEvent* e);

private slots:
	void updateVariants();
	void copyToClipboard();

private:
	Ui::CandidateGeneDialog ui_;
};

#endif // CANDIDATEGENEDIALOG_H
