#include "cfDNARemovedRegions.h"
#include "ui_cfDNARemovedRegions.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include <Exceptions.h>
#include <QComboBox>
#include <QMessageBox>

cfDNARemovedRegions::cfDNARemovedRegions(const QString& processed_sample_name, QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::cfDNARemovedRegions),
	processed_sample_name_(processed_sample_name)
{
	// abort if no connection to NGSD
	if (!LoginManager::active())
	{
		GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to modify cfDNA panels!");
		this->close();
	}

	// get created panel
	QString ps_id = NGSD().processedSampleId(processed_sample_name_);
	QList<CfdnaPanelInfo> cfdna_panels = NGSD().cfdnaPanelInfo(ps_id);
	if (cfdna_panels.size() < 1)
	{
		GUIHelper::showMessage("No cfDNA panel in NGSD!", "The NGSD does not contain a cfDNA panel for the processed sample '"
								+ processed_sample_name_ + "'!\nPlease create a cfDNA panel before adding removed regions.");
		this->close();
	}
	if (cfdna_panels.size() == 1)
	{
		cfdna_panel_info_ = cfdna_panels.at(0);
	}
	else
	{
		QStringList panel_text;
		foreach (const CfdnaPanelInfo& panel, cfdna_panels)
		{
			panel_text.append("cfDNA panel for " + panel.processing_system  + " (" + panel.created_date.toString("dd.MM.yyyy") + " by " + panel.created_by + ")");
		}

		QComboBox* cfdna_panel_selector = new QComboBox(this);
		cfdna_panel_selector->addItems(panel_text);

		// create dlg
		QString message_text = "<br/>Multiple cfDNA panels for the processed sample " + processed_sample_name_ + " exist.<br/>"
				+ "<br/>Please select the cfDNA panel which should be used:";
		auto dlg = GUIHelper::createDialog(cfdna_panel_selector, "Multiple personalized cfDNA panels for " + processed_sample_name_ + " found", message_text, true);
		int btn = dlg->exec();
		if (btn!=1)
		{
			this->close();
		}
		cfdna_panel_info_ = cfdna_panels.at(cfdna_panel_selector->currentIndex());
	}
	ui_->setupUi(this);
	initGui();
}

cfDNARemovedRegions::~cfDNARemovedRegions()
{
	delete ui_;
}

void cfDNARemovedRegions::initGui()
{
	ui_->l_processed_sample->setText(processed_sample_name_);
	ui_->l_processing_system->setText(cfdna_panel_info_.processing_system);
	// get removed regions
	ui_->te_removed_regions->setText(NGSD().cfdnaPanelRemovedRegions(cfdna_panel_info_.cfdna_id).toText());
}

BedFile cfDNARemovedRegions::parseBed()
{
	BedFile bed = BedFile::fromText(ui_->te_removed_regions->toPlainText().toUtf8());
	bed.clearAnnotations();
	return bed;
}

void cfDNARemovedRegions::apply()
{
	BedFile bed;
	try
	{
		 bed = BedFile::fromText(ui_->te_removed_regions->toPlainText().toUtf8());
	}
	catch (FileParseException e)
	{
		QMessageBox::warning(this, "Input parsing error", e.message());
		return;
	}

	if (bed.count() > 0)
	{
		// import to db
		SqlQuery query = NGSD().getQuery();
		query.prepare("UPDATE `cfdna_panels` SET `removed_regions`=:0 WHERE `id`=:1");

		// bind values
		query.bindValue(0, bed.toText());
		query.bindValue(1, cfdna_panel_info_.cfdna_id);
		query.exec();
	}
}
