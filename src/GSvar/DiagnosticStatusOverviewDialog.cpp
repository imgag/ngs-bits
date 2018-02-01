#include "DiagnosticStatusOverviewDialog.h"
#include "GUIHelper.h"
#include "GenLabDB.h"
#include <QCompleter>
#include <QClipboard>
#include <QMenu>

DiagnosticStatusOverviewDialog::DiagnosticStatusOverviewDialog(QWidget *parent)
	: QDialog(parent)
	, ui()
{
	ui.setupUi(this);
	connect(ui.clipboard, SIGNAL(clicked(bool)), this, SLOT(copyTableToClipboard()));
	connect(ui.sample_infos, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(sampleContextMenu(QPoint)));

	connect(ui.project, SIGNAL(currentTextChanged(QString)), this, SLOT(updateOverviewTable()));
	connect(ui.user, SIGNAL(currentTextChanged(QString)), this, SLOT(updateOverviewTable()));
	connect(ui.genlab, SIGNAL(stateChanged(int)), this, SLOT(updateOverviewTable()));
	connect(ui.hide_done, SIGNAL(stateChanged(int)), this, SLOT(updateOverviewTable()));

	//init projects combo box
	NGSD db;
	QStringList projects = db.getValues("SELECT name FROM project ORDER BY name ASC");
	ui.project->addItem("[select]");
	ui.project->addItems(projects);
	auto completer = new QCompleter(projects);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	ui.project->setCompleter(completer);

	//init user combo box
	QStringList users = db.getValues("SELECT u.name FROM user u WHERE EXISTS(SELECT * FROM diag_status ds WHERE ds.user_id=u.id) ORDER BY u.name ASC");
	ui.user->addItem("[select]");
	ui.user->addItems(users);
	completer = new QCompleter(users);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	ui.user->setCompleter(completer);

	updateOverviewTable();
}

void DiagnosticStatusOverviewDialog::updateOverviewTable()
{
	//init
	NGSD db;
	GenLabDB db_genlab;
	bool genlab = ui.genlab->isChecked();
	bool hide_done = ui.hide_done->isChecked();

	//abort if no project selected
	QString project_name = ui.project->currentText();
	if (project_name=="[select]")
	{
		ui.sample_infos->clearContents();
		ui.sample_infos->setRowCount(0);
		ui.status->setText("");
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	//determine project id
	QString project_id = db.getValue("SELECT id FROM project WHERE name='" + project_name + "'").toByteArray();

	//add one row per processed sample to table
	SqlQuery query = db.getQuery();
	query.exec("SELECT id, sample_id FROM processed_sample WHERE project_id=" + project_id);
	ui.sample_infos->setRowCount(query.size());
	ui.sample_infos->clearContents();
	int r = 0;
	while(query.next())
	{
		//get data from NGSD
		QString processed_sample_id = query.value(0).toString();
		DiagnosticStatusData diag_data = db.getDiagnosticStatus(processed_sample_id);
		ProcessedSampleData processed_sample_data = db.getProcessedSampleData(processed_sample_id);
		QString sample_id = query.value(1).toString();
		SampleData sample_data = db.getSampleData(sample_id);

		//filter done
		if (hide_done && !(diag_data.dagnostic_status=="pending" || diag_data.dagnostic_status=="in progress" || diag_data.dagnostic_status=="")) continue;

		//filter user
		QString user_filter = ui.user->currentText();
		if (user_filter!="[select]" && diag_data.user!=user_filter) continue;

		//set row content
		addItem(r, 0, sample_data.quality + " | " + processed_sample_data.quality);
		addItem(r, 1, processed_sample_data.name);
		addItem(r, 2, sample_data.name_external);
		addItem(r, 3, QString(sample_data.is_tumor ? "yes" : "no") + " | " + (sample_data.is_ffpe ? "yes" : "no"));
		addItem(r, 4, sample_data.disease_group + " | " + sample_data.disease_status);
		if (genlab)
		{
			addItem(r, 5, db_genlab.diagnosis(sample_data.name));
			QList<Phenotype> phenos = db_genlab.phenotypes(sample_data.name);
			QStringList tmp;
			foreach(const Phenotype& pheno, phenos)
			{
				tmp << pheno.name();
			}
			addItem(r, 6, tmp.join(", "));
		}
		else
		{
			addItem(r, 5, "");
			addItem(r, 6, "");
		}
		addItem(r, 7, processed_sample_data.run_name);
		addItem(r, 8, processed_sample_data.processing_system);
		addItem(r, 9, processed_sample_data.project_name);
		addItem(r, 10, processed_sample_data.normal_sample_name);
		addItem(r, 11, diag_data.dagnostic_status);
		addItem(r, 12, diag_data.outcome);
		addItem(r, 13, diag_data.genes_causal);
		addItem(r, 14, diag_data.inheritance_mode);
		addItem(r, 15, diag_data.comments, true);
		addItem(r, 16, sample_data.comments, true);
		addItem(r, 17, processed_sample_data.comments, true);

		++r;
	}
	ui.sample_infos->setRowCount(r);
	GUIHelper::resizeTableCells(ui.sample_infos, 200);

	//update status
	ui.status->setText(QString::number(r) + " samples after filtering");

	QApplication::restoreOverrideCursor();
}

void DiagnosticStatusOverviewDialog::copyTableToClipboard()
{
	//header
	QString output = "#";
	for (int col=0; col<ui.sample_infos->columnCount(); ++col)
	{
		if (col!=0) output += "\t";
		output += ui.sample_infos->horizontalHeaderItem(col)->text();
	}
	output += "\n";

	//rows
	for (int row=0; row<ui.sample_infos->rowCount(); ++row)
	{
		for (int col=0; col<ui.sample_infos->columnCount(); ++col)
		{
			if (col!=0) output += "\t";
			output += ui.sample_infos->item(row, col)->text();
		}
		output += "\n";
	}

	QApplication::clipboard()->setText(output);
}

void DiagnosticStatusOverviewDialog::addItem(int r, int c, QString text, bool text_as_tooltip)
{
	text = text.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ').trimmed();
	QTableWidgetItem* item = new QTableWidgetItem(text);
	ui.sample_infos->setItem(r, c, item);
	if (text_as_tooltip)
	{
		item->setToolTip(text);
	}
}

void DiagnosticStatusOverviewDialog::sampleContextMenu(QPoint pos)
{
	//get item
	QTableWidgetItem* item = ui.sample_infos->itemAt(pos);
	if (!item) return;

	//create contect menu
	QMenu menu(ui.sample_infos);
	menu.addAction("Open sample");

	//execute contect menu
	QAction* action = menu.exec(ui.sample_infos->viewport()->mapToGlobal(pos));
	if (!action) return;

	if (action->text()=="Open sample")
	{
		QString processed_sample_name = ui.sample_infos->item(item->row(), 1)->text();
		emit openProcessedSample(processed_sample_name);
	}
}
