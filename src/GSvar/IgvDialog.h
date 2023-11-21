#ifndef IGVDIALOG_H
#define IGVDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "ui_IgvDialog.h"
#include "FileLocationProvider.h"

class IgvDialog
		: public QDialog
{
	Q_OBJECT

public:
	//Constructor
	IgvDialog(QWidget* parent = 0);

	//Add a file
	void addFile(const FileLocation& file, bool checked);

	//Returns the files to load
	QStringList filesToLoad();

	//Returns if initialization should be skipped for the session - dialog is rejected in this case!
	bool skipInitialization() const;

protected slots:
	void treeItemChanged(QTreeWidgetItem* item);
	void skipInitializationClicked();

private:
	Ui::IgvDialog ui_;
	bool skip_;
};

#endif // IGVDIALOG_H
