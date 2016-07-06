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

	bool addedSubpanel();

protected slots:
	void checkAndCreatePanel();
	void storePanel();
	void openSubpanelFolder();

private:
	void loadProcessingSystems();
	void createSubpanelCompleter();
	QStringList geneList();
	QString getBedFilename();
	void showMessage(QString message, bool error);

	Ui::SubpanelDesignDialog *ui;
	QCompleter* completer;
	QStringList genes;
	BedFile regions;
	QString roi_file;
	QString gene_file;
	bool added_subpanel;

};

#endif // SUBPANELDESIGNDIALOG_H
