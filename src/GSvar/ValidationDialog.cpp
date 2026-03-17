#include "ValidationDialog.h"
#include "LoginManager.h"
#include "GSvarHelper.h"
#include "VariantHgvsAnnotator.h"
#include "Settings.h"

ValidationDialog::ValidationDialog(QWidget* parent, VariantValidation var_val)
	: QDialog(parent)
	, ui_()
	, db_()
    , var_val_(var_val)
{
	ui_.setupUi(this);

	//set up UI
	QStringList methods = db_.getEnum("variant_validation", "validation_method");
	foreach(QString m, methods)
	{
		ui_.method->addItem(m);
	}

    connect(ui_.method, SIGNAL(currentTextChanged(QString)), this, SLOT(changed()));
    connect(ui_.comment, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(changed()));
    //status doesnt need to directly update the object with changed() as statusChanged() modifys the comment and that change triggers the changed() function.
    connect(ui_.status, SIGNAL(currentTextChanged(QString)), this, SLOT(statusChanged()));

	QStringList status = db_.getEnum("variant_validation", "status");
	foreach(QString s, status)
	{
		ui_.status->addItem(s);
	}

    if (var_val_.variant_type == "SNV_INDEL")
	{
		ui_.variant_type->setText("SNV/Indel");
        QString variant_id = QString::number(var_val_.variant_id);
		Variant variant = db_.variant(variant_id);
        QString text = variant.toString() + " (" + var_val_.genotype + ") // HG19: ";
		try
		{
			Variant variant_hg19 = GSvarHelper::liftOverVariant(variant, false);
			text += variant_hg19.toString();
		}
		catch(Exception& e)
		{
			text += e.message();
		}
		ui_.variant->setText(text);

		QStringList transcript_infos;

		//get all transcripts containing the variant
		TranscriptList transcripts  = db_.transcriptsOverlapping(variant.chr(), variant.start(), variant.end(), 5000);
		transcripts.sortByRelevance();

		//annotate consequence for each transcript
		FastaFileIndex genome_idx(Settings::string("reference_genome"));
		VariantHgvsAnnotator hgvs_annotator(genome_idx);
		foreach(const Transcript& trans, transcripts)
		{
			VariantConsequence consequence = hgvs_annotator.annotate(trans, variant);

			QString line = trans.gene() + " " + trans.nameWithVersion() + " " + consequence.exonOrIntron(trans) + " " + variant.chr().str() + ":" + QString::number(variant.start()) + "-" + QString::number(variant.end()) + " " + consequence.hgvs_c + " " + consequence.hgvs_p;

			//flags for important transcripts
			QStringList flags = trans.flags(true);
			if (!flags.isEmpty()) line.append(" " + flags.join(" "));

			transcript_infos << line;
		}

		ui_.transcript_info->setText(transcript_infos.join("<br>"));
	}
    else if (var_val_.variant_type == "CNV")
	{
		ui_.variant_type->setText("CNV");
        int cnv_id = var_val_.variant_id;
		CopyNumberVariant cnv = db_.cnv(cnv_id);
		ui_.variant->setText(cnv.toString());
	}
    else if (var_val_.variant_type == "SV")
	{
		ui_.variant_type->setText("SV");
        int sv_id = var_val_.variant_id;
        if (var_val_.sv_deletion_id != -1)
		{
			sv_type_ = StructuralVariantType::DEL;
		}
        else if (var_val_.sv_duplication_id != -1)
		{
            sv_type_ = StructuralVariantType::DUP;
		}
        else if (var_val_.sv_inversion_id != -1)
		{
            sv_type_ = StructuralVariantType::INV;
		}
        else if (var_val_.sv_insertion_id != -1)
		{
            sv_type_ = StructuralVariantType::INS;
		}
        else if (var_val_.sv_translocation_id != -1)
		{
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
    else
    {
        THROW(ProgrammingException, "Unhandled variant type in ValidationDialog!");
    }

    ui_.sample->setText(db_.sampleName(QString::number(var_val_.sample_id)));
    ui_.requested_by->setText(db_.getValue("SELECT name FROM user WHERE id=" + QString::number(var_val_.user_id)).toString());

    ui_.method->setCurrentText(var_val_.validation_method);
    ui_.status->setCurrentText(var_val_.status);
    ui_.comment->setPlainText(var_val_.comment);
}

void ValidationDialog::changed()
{
    var_val_.validation_method = ui_.method->currentText().toUtf8();
    var_val_.status = ui_.status->currentText().toUtf8();
    var_val_.comment = ui_.comment->toPlainText().toUtf8();
}

VariantValidation ValidationDialog::getValidation()
{
    return var_val_;
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
