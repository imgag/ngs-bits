#include "SubpanelDesignDialog.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include <GUIHelper.h>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStringListModel>

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
    QStringListModel *model = new QStringListModel(subpanel_names_);
    QSortFilterProxyModel *proxy_model = new QSortFilterProxyModel();
    proxy_model->setSourceModel(model);
    proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    completer_ = new QCompleter(proxy_model);
	completer_->setCaseSensitivity(Qt::CaseInsensitive);
}

void SubpanelDesignDialog::checkAndCreatePanel()
{
	NGSD db;

	//clear
	disableStoreButton();
	clearMessages();

	//name check name
	QString name = getName(true);
    if (name.isEmpty() || !QRegularExpression("[0-9a-zA-Z_\\.]+").match(name).hasMatch())
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

	//parse genes from GUI
	genes_.clear();
	QByteArrayList lines = ui_.genes->toPlainText().toUtf8().split('\n');
	foreach(QByteArray line, lines)
	{
		QByteArray gene = line.split('\t')[0].trimmed();
		if (gene.isEmpty()) continue;

		genes_ << gene;
	}
	if (genes_.count()==0)
	{
		addMessage("Genes are not set!", true, true);
		return;
	}
	addMessage("Number of genes given: " + QString::number(genes_.count()), false, true);

	//check gene names
	bool ignore_gene_errors = ui_.ignore_gene_errors->isChecked();
	GeneSet valid_genes;
	foreach(QString gene, genes_)
	{
		QPair<QString, QString> geneinfo = db.geneToApprovedWithMessage(gene);
		QByteArray gene_new = geneinfo.first.toLatin1();
		if (geneinfo.second.startsWith("ERROR"))
		{
			addMessage("Gene name " + gene + " is not valid > skipped", !ignore_gene_errors, true);
		}
		else if (gene_new!=gene)
		{
			valid_genes << gene_new;
			addMessage("Gene name " + gene + " is a previous gene name - replaced by " + gene_new, false, true);
		}
		else
		{
			valid_genes << gene_new;
		}
	}
	genes_ = valid_genes;

	//indikationsspezifische Abrechnung
	QString warning = GSvarHelper::specialGenes(genes_);
	if (!warning.isEmpty())
	{
		addMessage(warning, false, false);
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
