#ifndef NSXSETTINGSDIALOG_H
#define NSXSETTINGSDIALOG_H

#include <QDialog>
#include "NGSD.h"
#include "ui_NsxSettingsDialog.h"

class NsxSettingsDialog
	: public QDialog
{
	Q_OBJECT

public:
	NsxSettingsDialog(QWidget* parent);
	NsxAnalysisSettings getSettings() const;

private:
	Ui::NsxSettingsDialog ui_;
};

#endif // NSXSETTINGSDIALOG_H
