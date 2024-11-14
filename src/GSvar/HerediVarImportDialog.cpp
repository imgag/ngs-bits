#include "HerediVarImportDialog.h"
#include "VcfFile.h"
#include "GUIHelper.h"
#include "HttpHandler.h"
#include "NGSD.h"

HerediVarImportDialog::HerediVarImportDialog(QWidget* parent)
	: QDialog(parent)
{
	ui_.setupUi(this);
	connect(ui_.import_btn, SIGNAL(clicked()), this, SLOT(import()));
}

void HerediVarImportDialog::import()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		ui_.output->clear();

		//init
		QString date = QDate::currentDate().toString(Qt::ISODate);
		FastaFileIndex genome_idx(Settings::string("reference_genome", false));
		NGSD db;
		QStringList valid_classes = db.getEnum("variant_classification", "class");

		//load VCF
		QString url = "https://heredivar.uni-koeln.de/download/vcf/classified?force=True";
		ui_.output->appendPlainText("Downloading VCF from " + url + " ...");
		HttpHandler http(false);
		QByteArray vcf_text = http.get(url);

		ui_.output->appendPlainText("parsing VCF ...");
		VcfFile vcf;
		vcf.fromText(vcf_text);
		ui_.output->appendPlainText(QString::number(vcf.count()) + " variants loaded");
		ui_.output->appendPlainText("");

		//import
		int c_not_in_ngsd = 0;
		int c_in_ngsd = 0;
		int c_skipped_sv = 0;
		int c_skipped_invalid = 0;
		int c_class_added = 0;
		int c_class_updated = 0;
		int c_class_kept = 0;
		ui_.output->appendPlainText("Importing variants ...");
		for (int i=0; i<vcf.count(); ++i)
		{
			const VcfLine& line = vcf[i];

			//skip structural variants
			QByteArray sv_type = line.info("SVTYPE", false).trimmed();
			if (sv_type!="")
			{
				++c_skipped_sv;
				continue;
			}

			//invalid variant
			if (!line.isValid(genome_idx))
			{
				++c_skipped_invalid;
				if (ui_.show_invalid->isChecked()) ui_.output->appendPlainText("  invalid variant:" + line.toString());
				continue;
			}

			//map classifications from HerediVar to ACMG classes
			QByteArray classification = line.info("classification", false).trimmed();
			QByteArray classification_comment = "";
			classification.replace("%2B", "+");
			if (classification=="3+")
			{
				classification_comment = "; was classificaiton '" + classification + "' in HerediVar";
				classification = "3";
			}
			if (classification=="3-")
			{
				classification_comment = "; was classificaiton '" + classification + "' in HerediVar";
				classification = "3";
			}
			if (classification=="4M")
			{
				classification_comment = "; was classificaiton '" + classification + "' in HerediVar";
				classification = "4";
			}
			if (classification=="5M")
			{
				classification_comment = "; was classificaiton '" + classification + "' in HerediVar";
				classification = "5";
			}
			if (!valid_classes.contains(classification))
			{
				++c_skipped_invalid;
				if (ui_.show_invalid->isChecked()) ui_.output->appendPlainText("  invalid classification '" + classification + "' of variant " + line.toString());
				continue;
			}

			//add variant to NGSD if necessary
			Variant v(line);
			QString v_id = db.variantId(v, false);
			if (v_id.isEmpty())
			{
				++c_not_in_ngsd;
				v_id = db.addVariant(v);
			}
			else ++c_in_ngsd;

			//add/update classifications
			QString class_ngsd = db.getValue("SELECT class FROM variant_classification WHERE variant_id=:0", true, v_id).toString();
			if (class_ngsd.isEmpty()) //add classification
			{
				++c_class_added;

				SqlQuery query = db.getQuery();
				query.prepare("INSERT INTO variant_classification (variant_id, class, comment) VALUES (" + v_id + ",:0,:1) ON DUPLICATE KEY UPDATE class=VALUES(class), comment=VALUES(comment)");
				query.bindValue(0, classification);
				query.bindValue(1, "[" + classification + "] Imported from HerediVar on " + date + classification_comment);
				query.exec();
			}
			else if (class_ngsd!=classification) //update
			{
				++c_class_updated;

				QString class_comment = db.getValue("SELECT comment FROM variant_classification WHERE variant_id=:0", true, v_id).toString();
				class_comment.prepend("[" + classification + "] Imported from HerediVar on " + date + classification_comment + "\n");

				SqlQuery query = db.getQuery();
				query.prepare("UPDATE variant_classification SET class=:0, comment=:1 WHERE variant_id=:2");
				query.bindValue(0, classification);
				query.bindValue(1, class_comment);
				query.bindValue(2, v_id);
				query.exec();
			}
			else //same classification > nothing to do
			{
				++c_class_kept;
			}
		}
		ui_.output->appendPlainText("Variants not found in NGSD: " + QString::number(c_not_in_ngsd));
		ui_.output->appendPlainText("Variants found in NGSD: " + QString::number(c_in_ngsd));
		ui_.output->appendPlainText("Skipped invalid variants: " + QString::number(c_skipped_invalid));
		ui_.output->appendPlainText("Skipped structural variants: " + QString::number(c_skipped_sv));
		ui_.output->appendPlainText("");

		ui_.output->appendPlainText("Added new classifications: " + QString::number(c_class_added));
		ui_.output->appendPlainText("Updated existing classifications: " + QString::number(c_class_updated));
		ui_.output->appendPlainText("Unchanged existing classifications: " + QString::number(c_class_kept));

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, windowTitle());
	}
}



