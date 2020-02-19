#include "VariantValidationWidget.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "DBEditor.h"
#include "Settings.h"
#include "ValidationDialog.h"
#include <QMessageBox>
#include <QDesktopServices>

VariantValidationWidget::VariantValidationWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	//context menu
	QAction* action = new QAction(QIcon(":/Icons/Edit.png"), "Edit", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(edit()));

	action = new QAction(QIcon(":/Icons/Remove.png"), "Delete", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(remove()));

	action = new QAction(QIcon("://Icons/WebService.png"), "PrimerDesign", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openPrimerDesign()));
	action->setEnabled(Settings::string("PrimerDesign")!="");

	//fill status combobox
	NGSD db;
	QStringList tmp;
	tmp << "";
	tmp << db.getEnum("variant_validation", "status");
	ui_.status->addItems(tmp);
	ui_.status->setCurrentText("to validate");

	//signals&slots
	connect(ui_.status, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTable()));
	connect(ui_.text, SIGNAL(editingFinished()), this, SLOT(updateTable()));
}

void VariantValidationWidget::delayedInitialization()
{
	updateTable();
}

void VariantValidationWidget::updateTable()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//create query (with all possible filters)
	QStringList cols;
	cols << "vv.id";
	cols << "(SELECT user.name FROM user WHERE user.id=vv.user_id) as requested_by";
	cols << "(SELECT CONCAT(sample.name, ' (', sample.name_external, ')') FROM sample WHERE sample.id=vv.sample_id) as sample";
	//cols << "(SELECT u.name FROM user u, processed_sample ps, sample s WHERE u.id=ps.operator_id AND ps.sample_id=s.id AND s.id=vv.sample_id ORDER BY ps.id DESC LIMIT 1) as operator";
	cols << "(SELECT CONCAT(v.chr, ':', v.start, '-', v.end, ' ', v.ref, '>', v.obs, ' (', v.gene, ')') FROM variant v WHERE v.id=vv.variant_id) as variant";
	cols << "vv.genotype";
	cols << "vv.status";
	cols << "vv.comment";
	QString query_str = "SELECT " + cols.join(", ") + " FROM variant_validation vv WHERE 1";
	if (ui_.status->currentText()!="")
	{
		query_str += " AND status='" + ui_.status->currentText() + "'";
	}
	query_str += " ORDER BY vv.id DESC";

	//create table
	NGSD db;
	DBTable table = db.createTable("variant_validation", query_str);

	//apply filters not possible during query
	table.filterRows(ui_.text->text());

	ui_.table->setData(table);
	GUIHelper::resizeTableCells(ui_.table, -1, false);

	QApplication::restoreOverrideCursor();
}

void VariantValidationWidget::edit()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}

	//edit
	int id = ui_.table->getId(rows.toList().first()).toInt();
	ValidationDialog dlg(this, id);
	if (!dlg.exec()) return;

	dlg.store();
	updateTable();
}

void VariantValidationWidget::remove()
{
	//check
	QSet<int> rows = ui_.table->selectedRows();
	if (rows.count()!=1)
	{
		QMessageBox::information(this, "Selection error", "Please select exactly one item!");
		return;
	}

	//confirm
	int btn = QMessageBox::information(this, "Confirm deleting", "Do you really want to delete it?", QMessageBox::Yes, QMessageBox::Cancel);
	if (btn!=QMessageBox::Yes) return;

	//delete
	NGSD db;
	SqlQuery query = db.getQuery();
	try
	{
		foreach(int row, rows)
		{
			 query.exec("DELETE FROM variant_validation WHERE id=" + ui_.table->getId(row));
		}
	}
	catch (DatabaseException e)
	{
		QMessageBox::warning(this, "Error deleting item", "Could not delete an item!"
														  "\nThis is probably caused by the item being referenced from another table."
														  "\n\nDatabase error:\n" + e.message());
	}

	updateTable();
}

void VariantValidationWidget::openPrimerDesign()
{
	try
	{
		NGSD db;
		QSet<int> rows = ui_.table->selectedRows();
		foreach(int row, rows)
		{
			QString sample = db.getValue("SELECT s.name FROM sample s, variant_validation vv WHERE s.id=vv.sample_id AND vv.id=" + ui_.table->getId(row)).toString();

			QString variant_id = db.getValue("SELECT variant_id FROM variant_validation WHERE id=" + ui_.table->getId(row)).toString();
			Variant variant = db.variant(variant_id);

			QString url = Settings::string("PrimerDesign")+"/index.php?user="+Helper::userName()+"&sample="+sample+"&chr="+variant.chr().str()+"&start="+QString::number(variant.start())+"&end="+QString::number(variant.end())+"";
			QDesktopServices::openUrl(QUrl(url));
		}
	}
	catch (Exception& e)
	{
		GUIHelper::showMessage("PrimerDesign error", e.message());
		return;
	}
}
