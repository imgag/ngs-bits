#ifndef GENESTOREGIONSDIALOG_H
#define GENESTOREGIONSDIALOG_H

#include <QDialog>
#include "BedFile.h"

namespace Ui {
class GenesToRegionsDialog;
}

class GenesToRegionsDialog
	: public QDialog
{
	Q_OBJECT

public:
	explicit GenesToRegionsDialog(QWidget *parent = 0);
	~GenesToRegionsDialog();

private slots:
	void convertGenesToRegions();
	void copyRegionsToClipboard();
	void storeRegionsAsBED();

private:
	Ui::GenesToRegionsDialog *ui;
	BedFile regions;
};

#endif // GENESTOREGIONSDIALOG_H
