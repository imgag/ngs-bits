#ifndef NGSDREANNOTATIONDIALOG_H
#define NGSDREANNOTATIONDIALOG_H

#include <QDialog>
#include "ui_NGSDReannotationDialog.h"

class NGSDReannotationDialog
	: public QDialog
{
	Q_OBJECT

public:
	NGSDReannotationDialog(QString roi_file, QWidget *parent = 0);

	///Returns the AF filter value if it is applied, or 0.0 otherwise.
	double maxAlleleFrequency() const;
	///Returns the ROI filter filename if applied, or "" otherwise.
	QString roiFile() const;

private:
	Ui::NGSDReannotationDialog ui_;
	QString roi_file_;
};

#endif // NGSDREANNOTATIONDIALOG_H
