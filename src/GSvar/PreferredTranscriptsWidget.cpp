#include "PreferredTranscriptsWidget.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QAction>
#include "NGSD.h"

PreferredTranscriptsWidget::PreferredTranscriptsWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);
	connect(ui_.add_btn, SIGNAL(clicked(bool)), this, SLOT(addPreferredTranscript()));
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));
	connect(ui_.f_gene, SIGNAL(returnPressed()), this, SLOT(updateTable()));

	QAction* action = new QAction(QIcon(":/Icons/Remove.png"), "Delete", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(remove()));
}

void PreferredTranscriptsWidget::delayedInitialization()
{
	updateTable();
}

void PreferredTranscriptsWidget::updateTable()
{
	//get table from NGSD
	NGSD db;

	QStringList conditions;
	conditions << "g.id=gt.gene_id";
	conditions << "gt.name=pt.name";
	if (ui_.f_gene->text().trimmed()!="")
	{
		QString gene = ui_.f_gene->text().trimmed();
		conditions << "g.symbol LIKE " + db.escapeText("%" + gene + "%");
	}

	DBTable table = db.createTable("preferred_transcripts", "SELECT pt.id, g.symbol as gene, pt.name, pt.added_by, pt.added_date FROM preferred_transcripts pt, gene g, gene_transcript gt WHERE " + conditions.join(" AND ") + " ORDER BY g.symbol ASC, pt.name ASC");
	db.replaceForeignKeyColumn(table, table.columnIndex("added_by"), "user", "name");

	//replace headers
	QStringList headers = table.headers();
	headers.replace(headers.indexOf("name"), "transcript");
	headers.replace(headers.indexOf("added_by"), "added by");
	headers.replace(headers.indexOf("added_date"), "added");
	table.setHeaders(headers);

	//set data
	ui_.table->setData(table);
}

void PreferredTranscriptsWidget::addPreferredTranscript()
{
	QString title = "Add preferred transcript";

	//get transcript name
	QByteArray transcript_name = QInputDialog::getText(this, "Add preferred transcript", "Ensembl transcript identifier").toLatin1();
	if (transcript_name.contains(".")) //remove version number
	{
		transcript_name = transcript_name.left(transcript_name.indexOf('.'));
	}
	transcript_name = transcript_name.trimmed();
	if (transcript_name.isEmpty()) return;

	//add
	try
	{
		NGSD db;
		bool added = db.addPreferredTranscript(transcript_name);
		QString gene = db.getValue("SELECT g.symbol FROM gene g, gene_transcript gt WHERE gt.gene_id=g.id AND gt.name='" + transcript_name + "'", true).toString();
		if (added)
		{
			QMessageBox::information(this, title, "Added preferred transcript '" + transcript_name + "' for gene '" + gene + "'");
		}
		else
		{
			QMessageBox::information(this, title, "Transcript '" + transcript_name + "' already is a preferred transcript for gene '" + gene + "'");
		}

		updateTable();
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, title, "Transcript '" + transcript_name + "' could not be added:\n" + e.message());
	}
}

void PreferredTranscriptsWidget::remove()
{
	QString title = "Delete preferred transcript(s)";
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()==0)
	{
		QMessageBox::information(this, title, "Please select at least one preferred transcript!");
		return;
	}

	//confirm
	int btn = QMessageBox::information(this, title, "You have selected " + QString::number(rows.count()) + " preferred transcripts.\nDo you really want to delete them?", QMessageBox::Yes, QMessageBox::Cancel);
	if (btn!=QMessageBox::Yes) return;

	//delete
	NGSD db;
	SqlQuery query = db.getQuery();
	try
	{
		foreach(int row, rows)
		{
			 query.exec("DELETE FROM preferred_transcripts WHERE id=" + ui_.table->getId(row));
		}
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, title, "Could not delete an item!\nDatabase error:\n" + e.message());
	}

	updateTable();
}
