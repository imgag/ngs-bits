#include "GapClosingDialog.h"
#include "Settings.h"
#include "GUIHelper.h"
#include "LoginManager.h"
#include "GSvarHelper.h"
#include "GapClosingEditDialog.h"
#include <QDesktopServices>
#include <QAction>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>

GapClosingDialog::GapClosingDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);
	ui_.f_ps->fill(db_.createTable("processed_sample", "SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.id IN (SELECT DISTINCT processed_sample_id FROM gaps)"));
	ui_.f_status->addItem("");
	ui_.f_status->addItems(db_.getEnum("gaps", "status"));
	ui_.f_status->setCurrentText("to close");
	ui_.f_user->fill(db_.createTable("user", "SELECT id, name FROM user ORDER BY name ASC"), true);

	connect(ui_.f_ps, SIGNAL(editingFinished()), this, SLOT(updateTable()));
	connect(ui_.f_status, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateTable()));
	connect(ui_.f_user, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateTable()));

	//context menu
	QAction* action = new QAction(QIcon(":/Icons/CopyClipboard.png"), "Copy for PrimerGap", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(copyForPrimerGap()));

	action = new QAction(QIcon("://Icons/WebService.png"), "PrimerDesign", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openPrimerDesign()));
	action->setEnabled(Settings::string("PrimerDesign")!="");

	action = new QAction(QIcon(":/Icons/Edit.png"), "Edit", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(edit()));
	connect(ui_.table, SIGNAL(rowDoubleClicked(int)), this, SLOT(edit(int)));

	action = new QAction(QIcon(":/Icons/Comment.png"), "Add comment", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(addComment()));
}

void GapClosingDialog::delayedInitialization()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	updateTable();

	QApplication::restoreOverrideCursor();
}


void GapClosingDialog::gapCoordinates(int row, Chromosome& chr, int& start, int& end)
{
	QString gap = table_.row(row).value(1);

	QStringList parts = gap.replace("-", ":").split(":");
	if (parts.count()!=3) THROW(ProgrammingException, "gap does not consist of 3 parts!");

	chr = Chromosome(parts[0]);
	start = Helper::toInt(parts[1], "Gap start position", gap);
	end = Helper::toInt(parts[2], "Gap end position", gap);
}

QString GapClosingDialog::exonNumber(const QByteArray& gene, int start, int end)
{
	QString output;

	Transcript trans = db_.bestTranscript(db_.geneId(gene));
	if (trans.isValid())
	{
		int exon_nr = trans.exonNumber(start-20, end+20);
		if (exon_nr!=-1)
		{
			output = QByteArray::number(exon_nr) + " (" + trans.nameWithVersion() + ")";
		}
	}

	return output;
}

void GapClosingDialog::updateTable()
{

	//clear
	ui_.table->setRowCount(0);
	ui_.errors->setVisible(false);

	//create conditions
	QStringList conditions;
	if (ui_.f_ps->isValidSelection())
	{
		conditions << "processed_sample_id='" + ui_.f_ps->getId() + "'";
	}
	QString status = ui_.f_status->currentText();
	if (status!="")
	{
		conditions << "status='" + status + "'";
	}
	QString user = ui_.f_user->currentText();
	if (user!="")
	{
		conditions << "history LIKE '%" + user + "%'";
	}

	//check at least one filter is present
	if (conditions.isEmpty())
	{
		ui_.errors->setText("<font style='color:red;'>Select at least one filter option!</font>");
		ui_.errors->setVisible(true);
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	//create table
	table_ = db_.createTable("gaps", "SELECT g.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as processed_sample, CONCAT(g.chr, ':', g.start, '-', g.end) as gap, g.status, g.history FROM gaps g, processed_sample ps, sample s WHERE g.processed_sample_id=ps.id AND ps.sample_id=s.id AND " + conditions.join(" AND "));

	//add gene/exon information
	QStringList genes;
	QStringList exons;
	Chromosome chr;
	int start;
	int end;
	for (int row=0; row<table_.rowCount(); ++row)
	{
		gapCoordinates(row, chr, start, end);
		GeneSet genes_overlapping = db_.genesOverlappingByExon(chr, start, end, 20);
		genes << genes_overlapping.join("<br>");
		QStringList tmp;
        for (const QByteArray& gene : genes_overlapping)
		{
			QString exon = exonNumber(gene, start, end);
			if (exon!="") tmp << exon;
		}
		exons << tmp.join("<br>");

	}
	table_.insertColumn(2, genes, "gene");
	table_.insertColumn(3, exons, "exons");


	//show table in GUI
	ui_.table->setData(table_, 300);

	QApplication::restoreOverrideCursor();
}

void GapClosingDialog::edit()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}

    edit(rows.values().first());
}

void GapClosingDialog::edit(int row)
{
	//edit
	int id = ui_.table->getId(row).toInt();
	GapClosingEditDialog dlg(this, id);
	if (!dlg.exec()) return;

	dlg.store();
	updateTable();
}

void GapClosingDialog::addComment()
{

	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}

	//get text
	QString comment = QInputDialog::getText(this, "Comment", "Comment text:").trimmed();
	if (comment.isEmpty()) return;

    int row = rows.values().first();
	int id = ui_.table->getId(row).toInt();
	db_.addGapComment(id, comment);

	updateTable();
}

void GapClosingDialog::openPrimerDesign()
{
	try
	{
		QSet<int> rows = ui_.table->selectedRows();
		foreach(int row, rows)
		{
			QString ps = ui_.table->item(row, 0)->text();

			Chromosome chr;
			int start;
			int end;
			gapCoordinates(row, chr, start, end);

			if(GSvarHelper::build()==GenomeBuild::HG19) //PrimerDesign supports HG38 only
			{
				BedLine region = GSvarHelper::liftOver(chr, start, end, true);
				chr = region.chr();
				start = region.start();
				end = region.end();
			}

			QString url = Settings::string("PrimerDesign")+"/primer3/query?region="+chr.strNormalized(true)+":"+QString::number(start)+"-"+QString::number(end)+"";
			QDesktopServices::openUrl(QUrl(url));
		}
	}
	catch (Exception& e)
	{
		GUIHelper::showMessage("PrimerDesign error", e.message());
		return;
	}
}

void GapClosingDialog::copyForPrimerGap()
{
	try
	{
		QStringList output;

		QSet<int> rows = ui_.table->selectedRows();
		foreach(int row, rows)
		{
			Chromosome chr;
			int start;
			int end;
			gapCoordinates(row, chr, start, end);
			QString genes = ui_.table->item(row, 2)->text();

			output << chr.strNormalized(true) + "\t" + QString::number(start) + "\t" + QString::number(end) + "\t" + genes;
		}
		QApplication::clipboard()->setText(output.join("\n"));
	}
	catch (Exception& e)
	{
		GUIHelper::showMessage("PrimerGap error", e.message());
		return;
	}
}
