#include "SampleDiseaseInfoWidget.h"
#include "ui_SampleDiseaseInfoWidget.h"
#include "GUIHelper.h"
#include "PhenotypeSelectionWidget.h"
#include "GenLabDB.h"
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>

SampleDiseaseInfoWidget::SampleDiseaseInfoWidget(QString sample_name, QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::SampleDiseaseInfoWidget)
	, sample_name_(sample_name)
{
	ui_->setupUi(this);
	connect(ui_->remove_btn, SIGNAL(clicked(bool)), this, SLOT(removeDiseaseInfo()));
	connect(ui_->genlab_btn, SIGNAL(clicked(bool)), this, SLOT(importDiseaseInfoFromGenLab()));

	//add button
	QMenu* menu = new QMenu();
	foreach(const QString& type, db_.getEnum("sample_disease_info", "type"))
	{
		menu->addAction(type, this, SLOT(addDiseaseInfo()));
	}
	ui_->add_btn->setMenu(menu);
}

SampleDiseaseInfoWidget::~SampleDiseaseInfoWidget()
{
	delete ui_;
}

void SampleDiseaseInfoWidget::setDiseaseInfo(const QList<SampleDiseaseInfo>& disease_info)
{
	disease_info_ = disease_info;
	updateDiseaseInfoTable();
}

const QList<SampleDiseaseInfo>& SampleDiseaseInfoWidget::diseaseInfo() const
{
	return disease_info_;
}

void SampleDiseaseInfoWidget::updateDiseaseInfoTable()
{
	//reset
	ui_->disease_info->clearContents();
	ui_->disease_info->setRowCount(0);

	//add new
	ui_->disease_info->setRowCount(disease_info_.count());
	for(int i=0; i<disease_info_.count(); ++i)
	{
		const SampleDiseaseInfo& entry = disease_info_[i];

		//special handling of HPO (show term name)
		QString disease_info = entry.disease_info;
		if (entry.type=="HPO term id")
		{
			Phenotype pheno = db_.phenotypeByAccession(disease_info.toLatin1(), false);
			disease_info +=  " (" + (pheno.name().isEmpty() ? "invalid" : pheno.name()) + ")";
		}
		ui_->disease_info->setItem(i, 0, new QTableWidgetItem(disease_info));
		ui_->disease_info->setItem(i, 1, new QTableWidgetItem(entry.type));
		ui_->disease_info->setItem(i, 2, new QTableWidgetItem(entry.user));
		ui_->disease_info->setItem(i, 3, new QTableWidgetItem(entry.date.toString(Qt::ISODate).replace("T", " ")));
	}

	GUIHelper::resizeTableCells(ui_->disease_info);
}

void SampleDiseaseInfoWidget::addDiseaseInfo()
{
	QString type = qobject_cast<QAction*>(sender())->text();

	//preprare entry
	SampleDiseaseInfo tmp;
	tmp.type = type;
	tmp.user = Helper::userName();
	tmp.date = QDateTime::currentDateTime();

	//get info from user
	if (type=="HPO term id")
	{
		PhenotypeSelectionWidget* selector = new PhenotypeSelectionWidget(this);
		auto dlg = GUIHelper::createDialog(selector, "Select HPO term(s)", "", true);
		if (dlg->exec()!=QDialog::Accepted) return;

		foreach(const Phenotype& pheno, selector->selectedPhenotypes())
		{
			tmp.disease_info = pheno.accession();
			disease_info_ <<  tmp;
		}
	}
	else
	{
		QString text = QInputDialog::getText(this, "Add disease info", type);
		if (text.isEmpty()) return;

		//check if valid
		if (type=="OMIM disease/phenotype identifier" && !QRegExp("#\\d*").exactMatch(text))
		{
			QMessageBox::critical(this, "Invalid OMIM identifier", "OMIM disease/phenotype identifier invaid!\nA valid identifier is for example '#164400'.");
			return;
		}
		if (type=="Orpha number" && !QRegExp("ORPHA:\\d*").exactMatch(text))
		{
			QMessageBox::critical(this, "Invalid Orpha number", "Orpha number invaid!\nA valid number is for example 'ORPHA:1172'.");
			return;
		}

		tmp.disease_info = text;
		disease_info_ <<  tmp;
	}

	//update GUI
	updateDiseaseInfoTable();
}

void SampleDiseaseInfoWidget::removeDiseaseInfo()
{
	QList<QTableWidgetSelectionRange> ranges = ui_->disease_info->selectedRanges();

	if (ranges.count()!=1) return;
	if (ranges[0].topRow()!=ranges[0].bottomRow()) return;

	int row = ranges[0].topRow();
	disease_info_.removeAt(row);

	//update GUI
	updateDiseaseInfoTable();
}

void SampleDiseaseInfoWidget::importDiseaseInfoFromGenLab()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	GenLabDB genlab_db;

	//find the correct sample name in GenLab
	QString name = sample_name_;

	bool entries_exist = genlab_db.entriesExistForSample(name);
	while (!entries_exist && name.contains("_"))
	{
		QStringList parts = name.split("_");
		parts = parts.mid(0, parts.count()-1);
		name = parts.join("_");
		entries_exist = genlab_db.entriesExistForSample(name);
	}
	if (!entries_exist)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "GenLab import error", "Could not find sample '" + sample_name_ + "' in GenLab database!");
		return;
	}

	//prepare disease info for import
	SampleDiseaseInfo tmp;
	tmp.user = "genlab_import";
	tmp.date = QDateTime::currentDateTime();

	//ICD10
	QStringList icd10s = genlab_db.diagnosis(name);
	foreach(const QString& icd10, icd10s)
	{
		tmp.disease_info = icd10;
		tmp.type = "ICD10 code";
		disease_info_ << tmp;
	}

	//HPO
	QList<Phenotype> phenos = genlab_db.phenotypes(name);
	foreach(const Phenotype& pheno, phenos)
	{
		tmp.disease_info = pheno.accession();
		tmp.type = "HPO term id";
		disease_info_ << tmp;
	}

	//Orphanet
	QStringList ids = genlab_db.orphanet(name);
	foreach(const QString& id, ids)
	{
		tmp.disease_info = id;
		tmp.type = "Orpha number";
		disease_info_ << tmp;
	}

	//tumor fraction (only for tumor samples)
	NGSD db;
	QString sample_id = db.sampleId(sample_name_, false);
	bool is_tumor = db.getSampleData(sample_id).is_tumor;
	if (is_tumor)
	{
		QStringList tumor_fractions = genlab_db.tumorFraction(name);
		foreach(const QString& fraction, tumor_fractions)
		{
			tmp.disease_info = fraction;
			tmp.type = "tumor fraction";
			disease_info_ << tmp;
		}
	}

	//update GUI
	updateDiseaseInfoTable();
	QApplication::restoreOverrideCursor();
}
