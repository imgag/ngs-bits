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
		close();
	}

	// get created panel
	QString ps_id = NGSD().processedSampleId(processed_sample_name_);
	QList<CfdnaPanelInfo> cfdna_panels = NGSD().cfdnaPanelInfo(ps_id);
	if (cfdna_panels.size() < 1)
	{
		GUIHelper::showMessage("No cfDNA panel in NGSD!", "The NGSD does not contain a cfDNA panel for the processed sample '"
								+ processed_sample_name_ + "'!\nPlease create a cfDNA panel before adding removed regions.");
		close();
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
			panel_text.append("cfDNA panel for " + NGSD().getProcessingSystemData(panel.processing_system_id).name  + " (" + panel.created_date.toString("dd.MM.yyyy") + " by "
							  + NGSD().userName(panel.created_by) + ")");
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
			close();
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
	ui_->l_processing_system->setText(NGSD().getProcessingSystemData(cfdna_panel_info_.processing_system_id).name);
	// get removed regions
	BedFile previous_regions = NGSD().cfdnaPanelRemovedRegions(cfdna_panel_info_.id);
	ui_->te_removed_regions->setText(previous_regions.toText());
	foreach (const QByteArray& header, previous_regions.headers())
	{
		if (header.startsWith("##modified"))
		{
			ui_->l_last_modified->setText("(last " + header.mid(2) + ")");
			break;
		}
	}


	// connect signal and slots
	connect(ui_->buttonBox, SIGNAL(accepted()), this, SLOT(importInNGSD()));
	connect(ui_->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

void cfDNARemovedRegions::importInNGSD()
{
	try
	{
		BedFile removed_regions = BedFile::fromText(ui_->te_removed_regions->toPlainText().toUtf8());
		NGSD().setCfdnaRemovedRegions(cfdna_panel_info_.id, removed_regions);
		close();
	}
	catch (Exception e)
	{
		QMessageBox::warning(this, "Input parsing error", e.message());
	}
}
