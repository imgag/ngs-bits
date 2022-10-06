#include "VariantValidationWidget.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "DBEditor.h"
#include "Settings.h"
#include "ValidationDialog.h"
#include "LoginManager.h"
#include "GSvarHelper.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QAction>

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
	connect(ui_.table, SIGNAL(rowDoubleClicked(int)), this, SLOT(edit(int)));

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
	connect(ui_.cb_var_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTable()));
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
	cols << "vv.variant_type as 'variant type'";
	cols << QString() + "(CASE "
			+ "WHEN vv.variant_type = 'SNV_INDEL' THEN (SELECT CONCAT(v.chr, ':', v.start, '-', v.end, ' ', v.ref, '>', v.obs) FROM variant v WHERE vv.variant_id = v.id) "
			+ "WHEN vv.variant_type = 'CNV' THEN (SELECT CONCAT(c.chr, ':', c.start, '-', c.end, ' (CN: ', c.cn, ')') FROM cnv c WHERE vv.cnv_id = c.id) "
			+ "WHEN vv.variant_type = 'SV' THEN CASE "
												+ "WHEN vv.sv_deletion_id IS NOT NULL THEN (SELECT CONCAT('DEL at ', sv_del.chr, ':', sv_del.start_min, '-', sv_del.end_max) FROM sv_deletion sv_del WHERE vv.sv_deletion_id = sv_del.id) "
												+ "WHEN vv.sv_duplication_id IS NOT NULL THEN (SELECT CONCAT('DUP at ', sv_dup.chr, ':', sv_dup.start_min, '-', sv_dup.end_max) FROM sv_duplication sv_dup WHERE vv.sv_duplication_id = sv_dup.id) "
												+ "WHEN vv.sv_inversion_id IS NOT NULL THEN (SELECT CONCAT('INV at ', sv_inv.chr, ':', sv_inv.start_min, '-', sv_inv.end_max) FROM sv_inversion sv_inv WHERE vv.sv_inversion_id = sv_inv.id) "
												+ "WHEN vv.sv_insertion_id IS NOT NULL THEN (SELECT CONCAT('INS at ', sv_ins.chr, ':', (sv_ins.pos - sv_ins.ci_lower), '-', (sv_ins.pos + sv_ins.ci_upper)) FROM sv_insertion sv_ins WHERE vv.sv_insertion_id = sv_ins.id) "
												+ "WHEN vv.sv_translocation_id IS NOT NULL THEN (SELECT CONCAT('BND from ', sv_bnd.chr1, ':', sv_bnd.start1, '-', sv_bnd.end1, ' to ', sv_bnd.chr2, ':', sv_bnd.start2, '-', sv_bnd.end2) FROM sv_translocation sv_bnd WHERE vv.sv_translocation_id = sv_bnd.id) "
												+ "ELSE 'invalid SV' "
											+ "END "
			+ "ELSE 'invalid variant type' "
		+ "END) as variant";
	cols << "vv.genotype";
	cols << "vv.validation_method as 'validation method'";
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

	//add genes column
	QStringList genes;
	int idx_type = table.columnIndex("variant type");
	for(int r=0; r<table.rowCount(); ++r)
	{
		const DBRow& row = table.row(r);
		if (row.value(idx_type)=="SNV_INDEL")
		{
			QString variant_id = db.getValue("SELECT variant_id FROM variant_validation WHERE id="+row.id()).toString();
			Variant var = db.variant(variant_id);
			genes << db.genesOverlapping(var.chr(), var.start(), var.end(), 5000).join(", ");
		}
		else if (row.value(idx_type)=="CNV")
		{
			int cnv_id = db.getValue("SELECT cnv_id FROM variant_validation WHERE id="+row.id()).toInt();
			CopyNumberVariant var = db.cnv(cnv_id);
			genes << db.genesOverlapping(var.chr(), var.start(), var.end(), 5000).join(", ");
		}
		else
		{
			genes << "";
		}
	}
	table.insertColumn(4, genes, "gene(s)");

	//apply filters not possible during query
	table.filterRows(ui_.text->text());
	table.filterRowsByColumn(table.columnIndex("variant type"), ui_.cb_var_type->currentText());

	ui_.table->setData(table);
	GUIHelper::resizeTableCells(ui_.table, 400, false);

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

	edit(rows.toList().first());
}

void VariantValidationWidget::edit(int row)
{
	//edit
	int id = ui_.table->getId(row).toInt();
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

			Chromosome chr = variant.chr();
			int start = variant.start();
			int end = variant.end();
			if(GSvarHelper::build()==GenomeBuild::HG38) //PrimerDesign supports HG19 only
			{
				BedLine region = GSvarHelper::liftOver(chr, start, end, false);
				chr = region.chr();
				start = region.start();
				end = region.end();
			}

			QString url = Settings::string("PrimerDesign")+"/index.php?user="+LoginManager::userLogin()+"&sample="+sample+"&chr="+chr.str()+"&start="+QString::number(start)+"&end="+QString::number(end)+"";
			QDesktopServices::openUrl(QUrl(url));
		}
	}
	catch (Exception& e)
	{
		GUIHelper::showMessage("PrimerDesign error", e.message());
		return;
	}
}
