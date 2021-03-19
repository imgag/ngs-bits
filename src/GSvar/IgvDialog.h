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

	enum InitAction
	{
		INIT,
		SKIP_ONCE,
		SKIP_SESSION
	};
	//Returns the initialiation action
	InitAction initializationAction() const;

	//Returns the files to load
	QStringList filesToLoad();

protected slots:
	void on_skip_once_clicked();
	void on_skip_session_clicked();
	void treeItemChanged(QTreeWidgetItem* item);

private:
	Ui::IgvDialog ui_;
	InitAction init_action_;
};

#endif // IGVDIALOG_H
