#ifndef SUBPANELARCHIVEDIALOG_H
#define SUBPANELARCHIVEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "GeneSet.h"

namespace Ui {
class SubpanelArchiveDialog;
}

class SubpanelArchiveDialog
	: public QDialog
{
	Q_OBJECT

public:
	explicit SubpanelArchiveDialog(QWidget *parent = 0);
	~SubpanelArchiveDialog();

	///Indicates if one or more sub-panels were moved
	bool changedSubpanels();

protected slots:
	void openSubpanelFolder();
	void updateSubpanelLists();
	void archive(QListWidgetItem* item);
	void restore(QListWidgetItem* item);

private:
	void updateSubpanelList(QListWidget* list, QString path, const GeneSet& f_genes, QString f_filename);
	void move(QString name, QString from, QString to);

	Ui::SubpanelArchiveDialog *ui;
	bool changed;
	QString path_subpanel;
	QString path_archive;
};

#endif // SUBPANELARCHIVEDIALOG_H
