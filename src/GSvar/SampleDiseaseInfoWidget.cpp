#include "SampleDiseaseInfoWidget.h"
#include "ui_SampleDiseaseInfoWidget.h"
#include "GUIHelper.h"
#include "PhenotypeSelectionWidget.h"
#include "GenLabDB.h"
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include "LoginManager.h"

SampleDiseaseInfoWidget::SampleDiseaseInfoWidget(QString ps_name, QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::SampleDiseaseInfoWidget)
	, ps_name_(ps_name)
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
			int id = db_.phenotypeIdByAccession(disease_info.toLatin1(), false);
			disease_info +=  " (" + (id==-1 ? "invalid" : db_.phenotype(id).name()) + ")";
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
	tmp.user = LoginManager::user();
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

	//import disease details to NGSD
	GenLabDB genlab_db;
	genlab_db.addMissingMetaDataToNGSD(ps_name_, false, false, true);

	//load them from NGSD into the local datastructure
	QString sample_id = db_.sampleId(ps_name_);
	disease_info_ = db_.getSampleDiseaseInfo(sample_id);

	//update GUI
	updateDiseaseInfoTable();

	QApplication::restoreOverrideCursor();
}
