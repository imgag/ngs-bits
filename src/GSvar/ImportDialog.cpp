#include "ImportDialog.h"
#include "Exceptions.h"
#include <QMessageBox>
#include <QClipboard>
#include <QAction>
#include "VariantOpenDialog.h"
#include "GUIHelper.h"
#include "NGSD.h"
#include "GlobalServiceProvider.h"
#include "Background/VariantAnnotator.h"
#include "LoginManager.h"

ImportDialog::ImportDialog(QWidget* parent, Type type)
	: QDialog(parent)
	, ui_()
	, type_(type)
	, db_()
	, ref_genome_idx_(Settings::string("reference_genome"))
	, special_fields_(0)
{
	ui_.setupUi(this);
	setWindowFlags(Qt::Window);
	connect(ui_.import_btn, SIGNAL(clicked()), this, SLOT(import()));

	//add context menu item to paste data
	QAction* action = new QAction("paste");
	connect(action, SIGNAL(triggered(bool)), this, SLOT(pasteTable()));
	ui_.table->addAction(action);

	//type-specific UI setup
	setupGUI();
}

void ImportDialog::setupGUI()
{
	QStringList labels;
	if (type_==VARIANTS)
	{
		setWindowTitle("Import variants");
		ui_.label->setText("Select format and paste variants into the table (one per line).");
		labels << "input" << "normalized variant" << "in NGSD" << "NGSD class";
		ui_.import_btn->setText("open as variant list");
	}
	else if (type_==SAMPLES)
	{
		setWindowTitle("Import samples");
		ui_.label->setText("Batch import of samples (paste tab-separated data to table)");
		labels << 	"name" << "name external" << "sender" << "received" << "received by" << "sample type" << "tumor" << "ffpe" << "species" << "concentration [ng/ul]" << "volume" << "260/280" << "260/230" << "RIN/DIN" << "gender" << "quality" << "comment" << "disease group" << "disease status" << "tissue" << "(cfDNA tumor sample)";
		db_table_ = "sample";
		db_fields_ << "name" << "name_external" << "sender_id" << "received" << "receiver_id" << "sample_type" << "tumor" << "ffpe" << "species_id" << "concentration" << "volume" << "od_260_280" << "od_260_230" << "integrity_number" << "gender" << "quality" << "comment" << "disease_group" << "disease_status" << "tissue";
		special_fields_ = 1;
	}
	else if (type_==RUNS)
	{
		setWindowTitle("Import sequencing runs");
		ui_.label->setText("Batch import of sequencing runs (paste tab-separated data to table)");
		labels << "name" << "flowcell ID" << "flowcell type" << "start date" << "end date" << "device" << "recipe" << "pool_molarity" << "pool quantification method" << "comment";
		db_table_ = "sequencing_run";
		db_fields_ << "name" << "fcid" << "flowcell_type" << "start_date" << "end_date" << "device_id" << "recipe" << "pool_molarity" << "pool_quantification_method" << "comment";
	}
	else if (type_==PROCESSED_SAMPLES)
	{
		setWindowTitle("Import processed samples");
		ui_.label->setText("Batch import of processed samples (paste tab-separated data to table)");
		labels << "sample" << "project" << "run name" << "lane" << "mid1 name" << "mid2 name" << "operator" << "processing system" << "processing input [ng]" << "molarity [nM]" << "comment" << "normal processed sample" << "processing modus" << "batch number";
		db_table_ = "processed_sample";
		db_fields_ << "sample_id" << "project_id" << "sequencing_run_id" << "lane" << "mid1_i7" << "mid2_i5" << "operator_id" << "processing_system_id" << "processing_input" << "molarity" << "comment" << "normal_id" << "processing_modus" << "batch_number";
		db_extra_fields_ << "process_id";
	}
	else if (type_==MIDS)
	{
		setWindowTitle("Import MIDs (paste tab-separated data to table)");
		ui_.label->setText("Batch import of MIDs.");
		labels << "name" << "sequence";
		db_table_ = "mid";
		db_fields_ << "name" << "sequence";
	}
	else if (type_==STUDY_SAMPLE)
	{
		setWindowTitle("Import study samples");
		ui_.label->setText("Batch import of study samples (paste tab-separated data to table)");
		labels << "study" << "processed sample" << "study-specific name of sample";
		db_table_ = "study_sample";
		db_fields_ << "study_id" << "processed_sample_id" << "study_sample_idendifier";
	}
	else if (type_==SAMPLE_RELATIONS)
	{
		setWindowTitle("Import sample relations");
		ui_.label->setText("Batch import of sample relations (paste tab-separated data to table)");
		labels << "sample" << "relation" << "sample";
		db_table_ = "sample_relations";
		db_fields_ << "sample1_id" << "relation" << "sample2_id";
		db_extra_fields_ << "user_id";
	}
	else if (type_==SAMPLE_HPOS)
	{
		setWindowTitle("Import sample HPO terms");
		ui_.label->setText("Batch import of sample HPO termss (paste tab-separated data to table)");
		labels << "sample" << "HPO term id e.g. 'HP:0003002'";
		db_table_ = "sample_disease_info";
		db_fields_ << "sample_id" << "disease_info";
		db_extra_fields_ << "type" << "user_id";
	}
	else
	{
		THROW(ProgrammingException, "Unhandled type in ImportDialog::setupGUI");
	}

	//general stuff
	ui_.variant_options->setVisible(type_==VARIANTS);
	ui_.table->setColumnCount(labels.count());
	ui_.table->setHorizontalHeaderLabels(labels);
}

void ImportDialog::pasteTable()
{
	//init
	ui_.table->setRowCount(0);
	ui_.warnings->clear();
	int row_index = 0;

	//process lines
	foreach(QString line, QApplication::clipboard()->text().split("\n"))
	{
		if (line.trimmed().isEmpty()) continue;

		//remove newline characters
		while(line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		ui_.table->setRowCount(row_index + 1);

		try
		{
			pasteRow(row_index, line);
		}
		catch(Exception& e)
		{
			ui_.warnings->appendPlainText("row " + QString::number(row_index+1) + ": " +e.message().trimmed());
		}

		++row_index;
	}
	GUIHelper::resizeTableCellWidths(ui_.table, 500);
	ui_.table->resizeRowsToContents();

	//enable import button if there are no errors
	ui_.import_btn->setEnabled(ui_.warnings->toPlainText().trimmed().isEmpty());
}

void ImportDialog::pasteRow(int row_index, QString line)
{
	bool all_valid = true;

	if (type_==VARIANTS)
	{
		QString format = VariantOpenDialog::selectedFormat(ui_.variant_options->layout());
		Variant variant;
		QString error;
		VariantOpenDialog::parseVariant(format, line, ref_genome_idx_, variant, error);
		ui_.table->setItem(row_index, 0, GUIHelper::createTableItem(line));
		ui_.table->setItem(row_index, 1, GUIHelper::createTableItem(variant.isValid() ? variant.toString(QChar(), -1, true) : "error: " + error));
		QString var_id = db_.variantId(variant, false);
		if (var_id.isEmpty())
		{
			ui_.table->setItem(row_index, 2, GUIHelper::createTableItem("no"));
		}
		else
		{
			ui_.table->setItem(row_index, 2, GUIHelper::createTableItem("yes"));
			ClassificationInfo class_info = db_.getClassification(variant);
			ui_.table->setItem(row_index, 3, GUIHelper::createTableItem(class_info.classification));
		}
	}
	else if (type_==SAMPLES || type_==RUNS || type_==PROCESSED_SAMPLES || type_==MIDS || type_==STUDY_SAMPLE || type_==SAMPLE_RELATIONS || type_==SAMPLE_HPOS)
	{
		QStringList parts = line.split("\t");
		checkNumberOfParts(parts);
		for (int c=0; c<parts.count(); ++c)
		{
			QString value = parts[c];
			QString actual = value;
			QString validation_error;
			QString notice;

			//get field info and perform checks/conversions
			if (c<db_fields_.count())
			{
				const TableFieldInfo& field_info =  db_.tableInfo(db_table_).fieldInfo(db_fields_[c]);
				fixValue(value, field_info, actual, validation_error);

				//additional check to make sure a corresponding tumor sample exists for each cfDNA sample
				if (type_==PROCESSED_SAMPLES && field_info.name=="processing_system_id" && db_.getValue("SELECT type FROM processing_system WHERE name_manufacturer=:0", false, value).toString()=="cfDNA (patient-specific)")
				{
					int sample_id = ui_.table->item(row_index, 0)->data(Qt::UserRole).toInt();
					if (db_.relatedSamples(sample_id, "tumor-cfDNA").isEmpty())
					{
						THROW(Exception, "No tumor sample specified for cfDNA (patient-specific) sample!");
					}
				}

				//additional check to make sure sample names are unique
				if (db_table_=="sample" && field_info.name=="name" && db_.getValue("SELECT id FROM sample WHERE name=:0", true, actual).toString()!="")
				{
					notice = "Sample with name '" + actual + " already exists in NGSD. Import will be skipped!";
				}
			}

			//create item
			all_valid &= addItem(row_index, c, value, actual, validation_error, notice);
		}
	}
	else
	{
		THROW(ProgrammingException, "Unhandled type in ImportDialog::handlePaste");
	}

	if (!all_valid) THROW(ArgumentException, "Not all fields are valid");
}

void ImportDialog::keyPressEvent(QKeyEvent* e)
{
	if (e == QKeySequence::Paste)
	{
		pasteTable();
		e->accept();
		return;
	}

	QDialog::keyPressEvent(e);
}

void ImportDialog::fixValue(QString value, const TableFieldInfo& field_info, QString& actual, QString& validation_error)
{
	//accept German dates as well
	if (field_info.type==TableFieldInfo::DATE && !value.isEmpty())
	{
		QDate date_german = QDate::fromString(value, "dd.MM.yyyy");
		if (date_german.isValid())
		{
			actual = date_german.toString(Qt::ISODate);
		}
	}

	//handle german numbers
	if (field_info.type==TableFieldInfo::FLOAT && value.contains(','))
	{
		actual = value.replace(',', '.');
	}

	//handle FKs
	if (field_info.type==TableFieldInfo::FK && !value.isEmpty())
	{
		QString name_field = field_info.fk_name_sql;
		if (name_field.startsWith("CONCAT(name")) //some FK-fields show additional information after the name > use only the name
		{
			name_field = name_field.left(name_field.indexOf(','));
			name_field = name_field.mid(7);
		}
		actual = db_.getValue("SELECT " + field_info.fk_field + " FROM " + field_info.fk_table + " WHERE " + name_field + "=:0", true, value).toString();
	}

	//check if valid type
	if (!field_info.isValid(actual))
	{
		validation_error = "'" + actual + "' is not valid for " + field_info.toString();
	}

	//check if valid constraints
	QStringList constraint_errors = db_.checkValue(db_table_, field_info.name, actual, false);
	if (!constraint_errors.isEmpty())
	{
		validation_error = "'" + actual + "' does not match constraints for " + db_table_ + "/" + field_info.name + ": " + constraint_errors.join(", ");
	}

	//additional check to make sure a valid HPO term id was given
	if (db_table_=="sample_disease_info" && field_info.name=="disease_info")
	{
		QVariant hpo_id = db_.getValue("SELECT id FROM hpo_term WHERE hpo_id=:0", true, value);
		if (!hpo_id.isValid()) THROW(ArgumentException, "Invalid HPO term id '" + value + "' given!");
	}
}

bool ImportDialog::addItem(int r, int c, const QString& value, const QString& actual, const QString& validation_error, const QString& notice)
{
	QTableWidgetItem* item = GUIHelper::createTableItem(value);
	ui_.table->setItem(r, c, item);
	item->setData(Qt::UserRole, actual);

	//color item if not valid
	if(validation_error!="")
	{
		item->setBackgroundColor("#FF9600");
		item->setToolTip(validation_error);
		return false;
	}

	//color item if not valid
	if(!notice.trimmed().isEmpty())
	{
		item->setBackgroundColor("#BCBCBC");
		item->setToolTip(notice);
	}

	return true;
}

void ImportDialog::checkNumberOfParts(const QStringList& parts)
{
	int max = db_fields_.count() + special_fields_;
	if (parts.count()>max)
	{
		THROW(ArgumentException, "Line has " + QString::number(parts.count()) + " elements, but expected a maximum of " + QString::number(max) + " elements!");
	}
}

QString ImportDialog::insertQuery()
{
	QStringList fields = db_fields_;
	fields.append(db_extra_fields_);

	QString query_str = "INSERT INTO " + db_table_ + " (" + fields.join(", ") + ") VALUES (";
	for(int i=0; i<fields.count(); ++i)
	{
		if (i!=0) query_str += ", ";
		query_str += ":" + QString::number(i);
	}
	query_str += ")";

	return query_str;
}

void ImportDialog::addRow(SqlQuery& query, int r)
{
	int c=0;
	foreach (const QString& field, db_fields_)
	{
		QTableWidgetItem* item = ui_.table->item(r,c);
		QString value = item==nullptr ? "" : item->data(Qt::UserRole).toString();
		const TableFieldInfo& field_info = db_.tableInfo(db_table_).fieldInfo(field);
		query.bindValue(c, value.isEmpty() && field_info.is_nullable ? QVariant() : value);
		++c;
	}

	if (!db_extra_fields_.isEmpty())
	{
		//get extra values
		QStringList extra_values = extraValues(r);
		if (db_extra_fields_.count()!=extra_values.count()) THROW(ProgrammingException, "Number of extra fields and values differ!");
		for (int i=0; i<db_extra_fields_.count(); ++i)
		{
			const TableFieldInfo& field_info = db_.tableInfo(db_table_).fieldInfo(db_extra_fields_[i]);
			query.bindValue(c, extra_values[i].isEmpty() && field_info.is_nullable ? QVariant() : extra_values[i]);
			++c;
		}
	}
	query.exec();
}

void ImportDialog::import()
{
	ui_.warnings->clear();
	db_.transaction();
	int row_num = 0;
	int skipped = 0;

	try
	{
		if (type_==VARIANTS)
		{
			VariantList variants;
			for(int i=0; i<ui_.table->rowCount(); ++i)
			{
				++row_num;
				QString text = ui_.table->item(i, 1)->text();
				if (!text.startsWith("error:"))
				{
					variants.append(Variant::fromString(text));
				}
			}

			VariantAnnotator* worker = new VariantAnnotator(variants);
			connect(worker, SIGNAL(failed()), this, SLOT(variantImportFailed()));
			GlobalServiceProvider::startJob(worker, true);
		}
		else if (type_==MIDS || type_==STUDY_SAMPLE || type_==RUNS || type_==PROCESSED_SAMPLES || type_==SAMPLE_RELATIONS || type_==SAMPLE_HPOS)
		{
			//prepare query
			SqlQuery query = db_.getQuery();
			query.prepare(insertQuery());

			//add entries
			for (int r=0; r<ui_.table->rowCount(); ++r)
			{
				++row_num;
				addRow(query, r);
			}

			db_.commit();
			ui_.warnings->appendPlainText("Import successful!");
			ui_.import_btn->setEnabled(false);
		}
		else if (type_==SAMPLES)
		{
			//prepare query
			SqlQuery query = db_.getQuery();
			query.prepare(insertQuery());

			//add entries
			for (int r=0; r<ui_.table->rowCount(); ++r)
			{
				++row_num;

				//skip already imported samples
				QString sample_name = ui_.table->item(r,0)->data(Qt::UserRole).toString();
				if (db_.getValue("SELECT id FROM sample WHERE name=:0", true, sample_name).toString()!="")
				{
					++skipped;
					continue;
				}

				addRow(query, r);

				//link corresponding tumor and cfDNA sample
				QByteArray cfdna_sample = ui_.table->item(r, 0)->text().trimmed().toUtf8();
				QTableWidgetItem* tumor_item = ui_.table->item(r, db_fields_.count());
				if (tumor_item==nullptr) continue;
				QByteArray tumor_sample = tumor_item->text().trimmed().toUtf8();
				if (tumor_sample.isEmpty()) continue;
				if (!db_.getSampleData(db_.sampleId(tumor_sample)).is_tumor)
				{
					THROW(DatabaseException, "Sample " + tumor_sample + " is not a tumor! Can't import relation.");
				}
				db_.addSampleRelation(SampleRelation{tumor_sample, "tumor-cfDNA", cfdna_sample});
			}

			db_.commit();
			ui_.warnings->appendPlainText("Import successful!");
			if (skipped>0)
			{
				ui_.warnings->appendPlainText("Skipped " + QString::number(skipped) + " rows!");
			}
			ui_.import_btn->setEnabled(false);
		}
		else
		{
			THROW(ProgrammingException, "Unhandled type in ImportDialog::process");
		}
	}
	catch (Exception& e)
	{
		db_.rollback();
		ui_.warnings->appendPlainText("Import failed:");
		ui_.warnings->appendPlainText("row " + QString::number(row_num) + ": " + e.message().trimmed());
	}
}


void ImportDialog::variantImportFailed()
{
	QMessageBox::warning(this, "Variant import", "Variant import failed!\nCheck background job history in the lower right corner of the GSvar main window for details!");
}


QStringList ImportDialog::extraValues(int r)
{
	QStringList extra_values;

	if (type_==SAMPLE_RELATIONS)
	{
		//add user
		extra_values << QString::number(LoginManager::userId());
	}
	else if (type_==SAMPLE_HPOS)
	{
		//add type and user
		extra_values << "HPO term id";
		extra_values << QString::number(LoginManager::userId());
	}
	else if (type_==PROCESSED_SAMPLES)
	{
		//add next processing id of sample
		QString sample_name = ui_.table->item(r,0)->text();
		QString sample_id = db_.sampleId(sample_name);
		QString next_id = db_.nextProcessingId(sample_id);
		if (next_id.toInt() > 99)
		{
			THROW(ArgumentException, "Error: For sample " + sample_name + " already 99 processed samples exist.\nCannot create more processed samples for " + sample_name + ".\n\nPlease import a new sample e.g. NA12878 -> NA12878x2");
		}
		extra_values << next_id;
	}
	else
	{
		THROW(ProgrammingException, "Unhandled type in ImportDialog::process");
	}

	return extra_values;
}
