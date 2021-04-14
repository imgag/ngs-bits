#ifndef SUBPANELARCHIVEDIALOG_H
#define SUBPANELARCHIVEDIALOG_H

#include "ui_SubpanelArchiveDialog.h"
#include <QListWidgetItem>
#include "GeneSet.h"

class SubpanelArchiveDialog
	: public QDialog
{
	Q_OBJECT

public:
	SubpanelArchiveDialog(QWidget *parent = 0);

	///Indicates if one or more sub-panels were moved
	bool changedSubpanels();

protected slots:
	void updateSubpanelLists();
	void archive(QListWidgetItem* item);
	void restore(QListWidgetItem* item);

private:
	void updateSubpanelList(QListWidget* list, bool archived, const GeneSet& f_genes, QString f_filename);

	Ui::SubpanelArchiveDialog ui_;
	bool changed_;
};

#endif // SUBPANELARCHIVEDIALOG_H
