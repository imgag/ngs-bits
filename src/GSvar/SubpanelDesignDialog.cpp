#include "SubpanelDesignDialog.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include <GUIHelper.h>
#include <QMessageBox>

SubpanelDesignDialog::SubpanelDesignDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, subpanel_names_(NGSD().subPanelList(false))
	, last_created_subpanel_("")
{
	ui_.setupUi(this);
	createSubpanelCompleter();

	connect(ui_.check, SIGNAL(clicked()), this, SLOT(checkAndCreatePanel()));
	connect(ui_.store, SIGNAL(clicked()), this, SLOT(storePanel()));
	connect(ui_.import_btn, SIGNAL(clicked()), this, SLOT(importFromExistingSubpanel()));

	connect(ui_.name, SIGNAL(textEdited(QString)), this, SLOT(disableStoreButton()));
	connect(ui_.genes, SIGNAL(textChanged()), this, SLOT(disableStoreButton()));
}

void SubpanelDesignDialog::setGenes(const GeneSet& genes)
{
	ui_.genes->clear();
	ui_.genes->setPlainText(genes.join("\n"));
}

QString SubpanelDesignDialog::lastCreatedSubPanel()
{
	return last_created_subpanel_;
}

void SubpanelDesignDialog::createSubpanelCompleter()
{
	completer_ = new QCompleter(subpanel_names_);
	completer_->setCaseSensitivity(Qt::CaseInsensitive);
	ui_.name->setCompleter(completer_);
}

void SubpanelDesignDialog::checkAndCreatePanel()
{
	NGSD db;

	//clear
	disableStoreButton();
	clearMessages();

	//name check name
	QString name = getName(true);
	if (name.isEmpty() || !QRegExp("[0-9a-zA-Z_\\.]+").exactMatch(name))
	{
		addMessage("Name '" + name + "' is empty or contains invalid characters!", true, true);
		return;
	}

	//check name is not already used
	if(!db.getValue("SELECT id FROM subpanels WHERE name=:0", true, name).isNull())
	{
		addMessage("Name '" + name + "' is already used!", true, true);
		return;
	}

	//check gene names
	genes_.clear();
	QByteArrayList lines = ui_.genes->toPlainText().toLatin1().split('\n');
	foreach(QByteArray line, lines)
	{
		int tab_idx = line.indexOf("\t");
		if (tab_idx==-1)
		{
			genes_.insert(line);
		}
		else
		{
			genes_.insert(line.left(tab_idx));
		}
	}
	if (genes_.count()==0)
	{
		addMessage("Genes are not set!", true, true);
		return;
	}
	bool ignore_gene_errors = ui_.ignore_gene_errors->isChecked();
	foreach(QString gene, genes_)
	{
		QPair<QString, QString> geneinfo = db.geneToApprovedWithMessage(gene);
		if (geneinfo.first!=gene || geneinfo.second.startsWith("ERROR"))
		{
			addMessage("Gene " + geneinfo.first + ": " + geneinfo.second, !ignore_gene_errors, false);
		}
	}

	//create target region
	QString mode = ui_.mode->currentText();
	QString messages;
	QTextStream stream(&messages);
	regions_ = db.genesToRegions(genes_, Transcript::ENSEMBL, mode, ui_.fallback->isChecked(), false, &stream);
	if (messages!="")
	{
		foreach(QString message, messages.split('\n'))
		{
			addMessage(message.trimmed(), !ignore_gene_errors, false);
		}
	}

	//add flanking regions
	int flanking  = ui_.flanking->currentText().toInt();
	if (flanking>0)
	{
		regions_.extend(flanking);
	}

	//add special regions (gene symbol, region1, region2, ...)
    auto special_regions = GSvarHelper::specialRegions();
    QStringList genes_special;
	foreach(QByteArray gene, genes_)
    {
        if (special_regions.contains(gene))
        {
            genes_special << gene;

            foreach(const BedLine& region, special_regions[gene])
            {
				regions_.append(region);
            }
        }
    }
	regions_.merge();

	//show message
	addMessage("Sub-panel with " + QString::number(genes_.count()) + " genes of size " + QString::number(regions_.baseCount()) + " bp (" + mode + " plus " + ui_.flanking->currentText() + " flanking bases) designed. You can store it now!", false, true);
	if (!genes_special.isEmpty())
	{
		addMessage("Added special regions for gene(s): " + genes_special.join(", "), false, true);
	}

	ui_.store->setEnabled(!errorMessagesPresent());
}

void SubpanelDesignDialog::storePanel()
{
	NGSD db;

	QString name = getName(true);

	SqlQuery query = db.getQuery();
	query.prepare("INSERT INTO `subpanels`(`name`, `created_by`, `created_date`, `mode`, `extend`, `genes`, `roi`, `archived`) VALUES (:0,:1,:2,:3,:4,:5,:6,0)");
	query.bindValue(0, name);
	query.bindValue(1, db.userId(Helper::userName()));
	query.bindValue(2, QDate::currentDate().toString(Qt::ISODate));
	query.bindValue(3, ui_.mode->currentText());
	query.bindValue(4, ui_.flanking->currentText());
	query.bindValue(5, genes_.toStringList().join("\n"));
	query.bindValue(6, regions_.toText());
	query.exec();

	clearMessages();
	addMessage("Sub-panel '" + name + "' stored to NGSD.", false, true);
	disableStoreButton();

	last_created_subpanel_ = name;
}

void SubpanelDesignDialog::disableStoreButton()
{
	ui_.store->setEnabled(false);
}

void SubpanelDesignDialog::importFromExistingSubpanel()
{
	//show selection dialog
	QLineEdit* panels = new QLineEdit(this);
	panels->setMinimumWidth(400);
	panels->setCompleter(completer_);
	auto dlg = GUIHelper::createDialog(panels, "Import data from existing sub-panel", "source sub-panel:", true);
	if(dlg->exec()!=QDialog::Accepted) return;

	QString selected = panels->text();
	if (!subpanel_names_.contains(selected))
	{
		QMessageBox::warning(this, "Invalid sub-panel", "Please select an existing sub-panel!\n'" + selected + "' is not valid");
		return;
	}

	//set base name (remove auto-suffix)
	QStringList parts = selected.split('_');
	QString basename = parts.mid(0, parts.count()-3).join("_");
	ui_.name->setText(basename);

	//set genes
	setGenes(NGSD().subpanelGenes(selected));
}

QString SubpanelDesignDialog::getName(bool with_suffix) const
{
	QString output = ui_.name->text().trimmed();

	if (with_suffix)
	{
		output += "_" + ui_.mode->currentText() + ui_.flanking->currentText() + "_" + Helper::userName() + "_" + QDate::currentDate().toString("yyyyMMdd");
	}

	return output;
}

void SubpanelDesignDialog::clearMessages()
{
	messages_.clear();
	ui_.messages->clear();
}

void SubpanelDesignDialog::addMessage(QString text, bool is_error, bool update_gui)
{
	messages_ << Message{text, is_error};

	if (update_gui)
	{
		QString text;
		foreach(const Message& m, messages_)
		{
			if (!text.isEmpty()) text += "<br>";
			text += m.is_error ? "<font color='red'>" + m.text + "</font>" : m.text;
		}
		ui_.messages->setHtml(text);
	}
}

bool SubpanelDesignDialog::errorMessagesPresent()
{
	foreach(const Message& m, messages_)
	{
		if (m.is_error) return true;
	}

	return false;
}
