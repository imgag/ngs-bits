#ifndef CANDIDATEGENEDIALOG_H
#define CANDIDATEGENEDIALOG_H

#include <QDialog>
#include "ui_CandidateGeneDialog.h"
#include "DelayedInitializationTimer.h"

class CandidateGeneDialog
	: public QDialog
{
	Q_OBJECT

public:
	CandidateGeneDialog(QWidget* parent = 0);
	void setGene(const QString& gene);

private slots:
	void updateVariants();
	void copyToClipboard();

private:
	Ui::CandidateGeneDialog ui_;
	DelayedInitializationTimer init_timer_;

};

#endif // CANDIDATEGENEDIALOG_H
