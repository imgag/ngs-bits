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

protected slots:
	void treeItemChanged(QTreeWidgetItem* item);

private:
	Ui::IgvDialog ui_;
};

#endif // IGVDIALOG_H
