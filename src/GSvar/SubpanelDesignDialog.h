#ifndef SUBPANELDESIGNDIALOG_H
#define SUBPANELDESIGNDIALOG_H

#include <QDialog>
#include <QCompleter>
#include "BedFile.h"

namespace Ui {
class SubpanelDesignDialog;
}

class SubpanelDesignDialog
	: public QDialog
{
	Q_OBJECT

public:
	explicit SubpanelDesignDialog(QWidget *parent = 0);
	~SubpanelDesignDialog();

	///Indicates if one or more sub-panels were added
	bool changedSubpanels();

protected slots:
	void checkAndCreatePanel();
	void storePanel();
	void disableStoreButton();

private:
	void loadProcessingSystems();
	void createSubpanelCompleter();
	QString getBedFilename();
	QString getBedFilenameArchive();
	void showMessage(QString message, bool error);

	Ui::SubpanelDesignDialog *ui;
	QCompleter* completer;
	QStringList genes;
	BedFile regions;
	QString roi_file;
	QString gene_file;
	bool changed;

};

#endif // SUBPANELDESIGNDIALOG_H
