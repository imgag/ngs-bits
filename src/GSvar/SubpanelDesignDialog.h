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

	///Sets the gene list
	void setGenes(QStringList genes);

	///Returns the last created subpane name (or an empty string if not subpanel was designed).
	QString lastCreatedSubPanel();

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
	QString last_created_subpanel;

};

#endif // SUBPANELDESIGNDIALOG_H
