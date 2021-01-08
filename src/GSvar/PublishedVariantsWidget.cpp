#include <QDesktopServices>
#include <QMessageBox>
#include "PublishedVariantsWidget.h"
#include "ui_PublishedVariantsWidget.h"
#include "NGSD.h"
#include "NGSHelper.h"
#include <QAction>

PublishedVariantsWidget::PublishedVariantsWidget(QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::PublishedVariantsWidget)
{
	ui_->setupUi(this);
	connect(ui_->search_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));

	//fill filter boxes
	NGSD db;
	ui_->f_sample->fill(db.createTable("sample", "SELECT id, name FROM sample"));
	ui_->f_published->fill(db.createTable("user", "SELECT id, name FROM user ORDER BY name ASC"));
	ui_->f_db->addItem("n/a");
	ui_->f_db->addItems(db.getEnum("variant_publication", "db"));

	//link to LOVD
	QAction* action = new QAction(QIcon(":/Icons/LOVD.png"), "Find in LOVD", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(searchForVariantInLOVD()));

	//link to ClinVar
	action = new QAction(QIcon(":/Icons/ClinGen.png"), "Find in ClinVar", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(searchForVariantInClinVar()));
}

PublishedVariantsWidget::~PublishedVariantsWidget()
{
	delete ui_;
}

void PublishedVariantsWidget::updateTable()
{
	//init
	NGSD db;
	QStringList constraints;

	//filter "published by"
	if (ui_->f_published->isValidSelection())
	{
		constraints << "user_id='" + ui_->f_published->getId() + "'";
	}
	ui_->f_published->showVisuallyIfValid(true);

	//filter "region"
	try
	{
		if (!ui_->f_region->text().trimmed().isEmpty())
		{
			Chromosome chr;
			int start, end;
			NGSHelper::parseRegion(ui_->f_region->text(), chr, start, end);
			constraints << ("variant_id IN (SELECT id FROM variant where chr='" + chr.strNormalized(true) + "' AND start>=" + QString::number(start) + " AND end<=" + QString::number(end) + ")");

			ui_->f_region->setStyleSheet("");
		}
	}
	catch (...)
	{
		ui_->f_region->setStyleSheet("QLineEdit {border: 2px solid red;}");
	}

	//filter "DB"
	if (ui_->f_db->currentText()!="n/a")
	{
		constraints << ("db='" + ui_->f_db->currentText() + "'");
	}

	//filter "sample"
	if (ui_->f_sample->isValidSelection())
	{
		constraints << "sample_id='" + ui_->f_sample->getId() + "'";
	}
	ui_->f_sample->showVisuallyIfValid(true);


	//create table
	QApplication::setOverrideCursor(Qt::BusyCursor);
	QString constraints_str;
	if (!constraints.isEmpty())
	{
		constraints_str = " WHERE (" + constraints.join(") AND (") + ")";
	}
	DBTable table = db.createTable("variant_publication", "SELECT * FROM variant_publication" + constraints_str + " ORDER BY id ASC");

	//replace foreign keys
	db.replaceForeignKeyColumn(table, table.columnIndex("sample_id"), "sample", "name");
	db.replaceForeignKeyColumn(table, table.columnIndex("user_id"), "user", "name");
	db.replaceForeignKeyColumn(table, table.columnIndex("variant_id"), "variant", "CONCAT(chr, ':', start, '-', end, ' ', ref, '>', obs)");

	//rename columns (after keys)
	QStringList headers = table.headers();
	headers.replace(headers.indexOf("sample_id"), "sample");
	headers.replace(headers.indexOf("user_id"), "published by");
	headers.replace(headers.indexOf("variant_id"), "variant");
	table.setHeaders(headers);

	//show data
	ui_->table->setData(table);
	QApplication::restoreOverrideCursor();
}

void PublishedVariantsWidget::searchForVariantInLOVD()
{
	try
	{
		int col = ui_->table->columnIndex("variant");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_text = ui_->table->item(row, col)->text();
			Variant variant = Variant::fromString(variant_text);

			int pos = variant.start();
			if (variant.ref()=="-") pos += 1;
			QDesktopServices::openUrl(QUrl("https://databases.lovd.nl/shared/variants#search_chromosome=" + variant.chr().strNormalized(false) + "&search_VariantOnGenome/DNA=g." + QString::number(pos)));
		}
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "LOVD search error", e.message());
	}
}

void PublishedVariantsWidget::searchForVariantInClinVar()
{
	try
	{
		int col = ui_->table->columnIndex("variant");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_text = ui_->table->item(row, col)->text();
			Variant variant = Variant::fromString(variant_text);

			int start = variant.start();
			int end = variant.end();

			QDesktopServices::openUrl(QUrl("http://www.ncbi.nlm.nih.gov/clinvar/?term=" + variant.chr().strNormalized(false) + "[chr]+AND+" + QString::number(start) + ":" + QString::number(end) + "[chrpos37]"));
		}
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "LOVD search error", e.message());
	}
}
