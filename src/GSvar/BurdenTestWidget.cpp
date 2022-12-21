#include "BurdenTestWidget.h"
#include "ui_BurdenTestWidget.h"

#include <GUIHelper.h>
#include <LoginManager.h>
#include <NGSD.h>
#include <QDialog>
#include <QMessageBox>
#include <QTextEdit>

BurdenTestWidget::BurdenTestWidget(QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::BurdenTestWidget)
{
	if (!LoginManager::active())
	{
		INFO(DatabaseException, "Burden Test Widget requires logging in into NGSD!");
	}

	LoginManager::checkRoleNotIn(QStringList{"user_restricted"});

	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->b_load_samples_cases, SIGNAL(clicked(bool)), this, SLOT(loadCaseSamples()));
	connect(ui_->b_load_samples_controls, SIGNAL(clicked(bool)), this, SLOT(loadControlSamples()));
}

BurdenTestWidget::~BurdenTestWidget()
{
	delete ui_;
}

void BurdenTestWidget::loadCaseSamples()
{
	QSet<int> ps_ids = loadSampleList("cases", case_samples_);
	if(ps_ids.size() > 0) case_samples_ = ps_ids;

	qDebug() << "Case:" << case_samples_.size() << case_samples_;

	updateSampleCounts();
	validateSamples();
}

void BurdenTestWidget::loadControlSamples()
{
	QSet<int> ps_ids = loadSampleList("controls", control_samples_);
	if(ps_ids.size() > 0) control_samples_ = ps_ids;

	qDebug() << "Control:" << control_samples_.size() << control_samples_;

	updateSampleCounts();
	validateSamples();
}

void BurdenTestWidget::validateSamples()
{
	NGSD db;
	QStringList errors;

	// check processing system
	QSet<int> processing_system_ids;
	foreach(int ps_id, (case_samples_ + control_samples_))
	{
		processing_system_ids << db.processingSystemIdFromProcessedSample(db.processedSampleName(QString::number(ps_id)));
	}
	if(processing_system_ids.size() > 1)
	{
		QStringList processing_systems;
		foreach (int sys_id, processing_system_ids)
		{
			processing_systems << db.getProcessingSystemData(sys_id).name_short;
		}
		errors << "ERROR: The cohorts contain more than one processing system (" + processing_systems.join(", ") + ")! ";
	}

	// check overlap
	QSet<int> overlap = case_samples_ & control_samples_;
	if(overlap.size() > 0)
	{
		QStringList ps_names;
		foreach (int ps_id, overlap)
		{
			ps_names << db.processedSampleName(QString::number(ps_id));
		}
		errors << "ERROR: The samples " + ps_names.join(", ") + " are present in both case and control cohort! ";
	}

	// check not 0
	if(case_samples_.size() < 1) errors << "ERROR: The case cohort doesn't contain any samples!";
	if(control_samples_.size() < 1) errors << "ERROR: The control cohort doesn't contain any samples!";

	if(errors.size() > 0)
	{
		QMessageBox::warning(this, "Validation error", "During cohort validation the following errors occured:\n" +  errors.join("\n"));
		ui_->b_burden_test->setEnabled(false);
	}
	else
	{
		ui_->b_burden_test->setEnabled(true);
	}

	qDebug() << "Case:" << case_samples_.size() << case_samples_;
	qDebug() << "Control:" << control_samples_.size() << control_samples_;
}

void BurdenTestWidget::updateSampleCounts()
{
	ui_->l_cases->setText("(" + QString::number(case_samples_.size()) + " Samples)");
	ui_->l_controls->setText("(" + QString::number(control_samples_.size()) + " Samples)");
}

QSet<int> BurdenTestWidget::loadSampleList(const QString& type, const QSet<int>& selected_ps_ids)
{
	NGSD db;

	//create dialog
	QTextEdit* te_samples = new QTextEdit();
	QStringList preselection;
	if(selected_ps_ids.size() > 0)
	{
		foreach (int ps_id, selected_ps_ids)
		{
			preselection << db.processedSampleName(QString::number(ps_id));
		}
		std::sort(preselection.begin(), preselection.end());

		te_samples->setText(preselection.join("\n"));
	}
	QSharedPointer<QDialog> dialog = GUIHelper::createDialog(te_samples, "Sample selection (" +  type +")",
															 "Paste the processed sample list. Either as one sample per line or as comma seperated list.", true);

	//show dialog
	if(dialog->exec()!=QDialog::Accepted) return QSet<int>();

	//parse text field
	QStringList samples;
	QStringList text = te_samples->toPlainText().split('\n');
	if(text.size() == 1)
	{
		samples = QString(text.at(0)).replace(";", ",").split(',');
	}
	else
	{
		foreach (const QString& line, text)
		{
			if(line.startsWith("#") || line.trimmed().isEmpty()) continue;
			samples << line.split('\t').at(0).trimmed();

			qDebug() << "Sample: " << line;
		}
	}

	//convert ps names to ids and check existence in NGSD
	QSet<int> ps_ids;
	QStringList invalid_samples;
	foreach (const QString& ps_name, samples)
	{
		QString ps_id = db.processedSampleId(ps_name, false);
		if(ps_id.isEmpty())
		{
			invalid_samples.append(ps_name);

			qDebug() << ps_name << "not found";
		}
		else
		{
			ps_ids << ps_id.toInt();

			qDebug() << ps_id.toInt() << db.processedSampleName(ps_id);
		}
	}

	if(invalid_samples.size() > 0)
	{
		QMessageBox::warning(this, "Invalid samples", "The following samples were not found in the NGSD:\n" + invalid_samples.join('\n'));
	}

	return ps_ids;
}

void BurdenTestWidget::performBurdenTest()
{
	NGSD db;
	/////////// parse parameter ///////////

	// get variant constraints

	// impacts
	QStringList impacts;
	if (ui_->cb_high->isChecked()) impacts << "HIGH";
	if (ui_->cb_medium->isChecked()) impacts << "MODERATE";
	if (ui_->cb_low->isChecked()) impacts << "LOW";
	if (ui_->cb_modifier->isChecked()) impacts << "MODIFIER";

	// prepare query constraints
	QStringList constraints;
	constraints << "(germline_het>0 OR germline_hom>0)"; //skip somatic only variants
	int max_ngsd = ui_->sb_max_ngsd_count->value();
	if (max_ngsd>0)
	{
		constraints << "germline_het+germline_hom<=" + QString::number(max_ngsd);
	}
	double max_af = ui_->sb_max_gnomad_af->value()/100.0;
	if (max_af<1.0)
	{
		constraints << "(gnomad IS NULL OR gnomad<=" + QString::number(max_af) + ")";
	}

	// get genes
	QList<int> gene_ids = db.getValuesInt(QString() + "SELECT `id` FROM `gene`" + ((ui_->rb_protein_coding->isChecked())? " WHERE `type`='protein-coding gene'": ""));

	// perform search
	foreach (int gene_id, gene_ids)
	{
		QByteArray gene_name = db.geneSymbol(gene_id);

		// get gene region
		BedFile gene_regions = db.geneToRegions(gene_name, Transcript::ENSEMBL, "gene");
		gene_regions.sort();
		gene_regions.merge();


		// get all variants in gene region

		// for all matching variants: get counts of case and control cohort

		int n_cases = 0;
		int n_controls = 0;

		foreach(int ps_id, control_samples_)
		{
			//check for variant in gene
		}

		foreach(int ps_id, control_samples_)
		{

		}


	}




	// fill table
}
