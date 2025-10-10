#ifndef GENEINTERPRETABILITYDIALOG_H
#define GENEINTERPRETABILITYDIALOG_H

#include <QDialog>
#include "ui_GeneInterpretabilityDialog.h"

struct GeneInterpretabilityRegion
{
	QString name; //name shown in GUI
	QString filename; //BED file path
};

class GeneInterpretabilityDialog
	: public QDialog
{
	Q_OBJECT

public:
	GeneInterpretabilityDialog(QWidget* parent, QList<GeneInterpretabilityRegion> regoins);

private slots:
	void initTable();
	void calculate();

private:
	Ui::GeneInterpretabilityDialog ui_;
	QList<GeneInterpretabilityRegion> regions_;

};

#endif // GENEINTERPRETABILITYDIALOG_H
