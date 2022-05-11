#include "ValidationDialog.h"
#include "NGSD.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QStringList>
#include "LoginManager.h"
#include "GSvarHelper.h"

ValidationDialog::ValidationDialog(QWidget* parent, int id)
	: QDialog(parent)
	, ui_()
	, db_()
	, val_id_(QString::number(id))
{
	ui_.setupUi(this);

	//set up UI
	QStringList methods = db_.getEnum("variant_validation", "validation_method");
	foreach(QString m, methods)
	{
		ui_.method->addItem(m);
	}
	connect(ui_.status, SIGNAL(currentTextChanged(QString)), this, SLOT(statusChanged()));
	QStringList status = db_.getEnum("variant_validation", "status");
	foreach(QString s, status)
	{
		ui_.status->addItem(s);
	}

	//fill with data
	SqlQuery query = db_.getQuery();
	query.exec("SELECT * FROM variant_validation WHERE id=" + val_id_);
	query.next();

	// determine variant type
	variant_type_ = query.value("variant_type").toString();
	if (variant_type_ == "SNV_INDEL")
	{
		ui_.variant_type->setText("SNV/Indel");
		QString variant_id = query.value("variant_id").toString();
		Variant variant = db_.variant(variant_id);
		QString text = variant.toString() + " (" + query.value("genotype").toString() + ")";
		if (GSvarHelper::build()==GenomeBuild::HG38)
		{
			QString tmp;
			try
			{
				Variant variant_hg19 = GSvarHelper::liftOverVariant(variant, false);
				tmp = variant_hg19.toString();
			}
			catch(Exception& e)
			{
				tmp = e.message();
			}
			text += " // HG19: " + tmp;
		}
		ui_.variant->setText(text);

		QString transcript_info = db_.getValue("SELECT coding FROM variant WHERE id=" + variant_id).toString().replace(",", "<br>");
		ui_.transcript_info->setText(transcript_info);
	}
	else if (variant_type_ == "CNV")
	{
		ui_.variant_type->setText("CNV");
		int cnv_id = query.value("cnv_id").toInt();
		CopyNumberVariant cnv = db_.cnv(cnv_id);
		ui_.variant->setText(cnv.toString());
	}
	else // SV
	{
		ui_.variant_type->setText("SV");
		int sv_id = -1;
		if (!query.value("sv_deletion_id").isNull())
		{
			sv_id = query.value("sv_deletion_id").toInt();
			sv_type_ = StructuralVariantType::DEL;
		}
		else if (!query.value("sv_duplication_id").isNull())
		{
			sv_id = query.value("sv_duplication_id").toInt();
			sv_type_ = StructuralVariantType::DUP;
		}
		else if (!query.value("sv_inversion_id").isNull())
		{
			sv_id = query.value("sv_inversion_id").toInt();
			sv_type_ = StructuralVariantType::INV;
		}
		else if (!query.value("sv_insertion_id").isNull())
		{
			sv_id = query.value("sv_insertion_id").toInt();
			sv_type_ = StructuralVariantType::INS;
		}
		else if (!query.value("sv_translocation_id").isNull())
		{
			sv_id = query.value("sv_translocation_id").toInt();
			sv_type_ = StructuralVariantType::BND;
		}
		else
		{
			THROW(DatabaseException, "No valid sv id for variant validation found!");
		}

		BedpeFile bedpe_structure;
		bedpe_structure.setAnnotationHeaders(QList<QByteArray>() << "FORMAT" << "");
		BedpeLine sv = db_.structuralVariant(sv_id, sv_type_, bedpe_structure, true);
		ui_.variant->setText(sv.toString());
	}



	ui_.sample->setText(db_.getValue("SELECT name FROM sample WHERE id=" + query.value("sample_id").toString()).toString());

	ui_.requested_by->setText(db_.getValue("SELECT name FROM user WHERE id=" + query.value("user_id").toString()).toString());

	ui_.method->setCurrentText(query.value("validation_method").toString());
	ui_.status->setCurrentText(query.value("status").toString());

	ui_.comment->setPlainText(query.value("comment").toString());
}

void ValidationDialog::store()
{
	SqlQuery query = db_.getQuery(); //use binding (user input)
	query.prepare("UPDATE variant_validation SET validation_method=:0, status=:1, comment=:2 WHERE id='" + val_id_ + "'");
	query.bindValue(0, ui_.method->currentText());
	query.bindValue(1, ui_.status->currentText());
	query.bindValue(2, ui_.comment->toPlainText().trimmed());
	query.exec();
}

QString ValidationDialog::status()
{
	return ui_.status->currentText();
}

void ValidationDialog::statusChanged()
{
	QString text = ui_.comment->toPlainText().trimmed();
	if (text!="") text += "\n";
	text += "[" + ui_.status->currentText() + "] " + LoginManager::userLogin() + " " + QDate::currentDate().toString("dd.MM.yyyy");
	ui_.comment->setPlainText(text);
}
