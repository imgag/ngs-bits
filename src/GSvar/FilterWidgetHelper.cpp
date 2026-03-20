#include "FilterWidgetHelper.h"
#include "NGSD.h"
#include "LoginManager.h"
#include "Settings.h"
#include "GSvarHelper.h"
#include "GlobalServiceProvider.h"
#include "SubpanelDesignDialog.h"
#include <QMessageBox>

void FilterWidgetHelper::createSubPanelFromPhenotypeFilter(const PhenotypeList& phenotypes, QComboBox* box, FilterState& state)
{
    //convert phenotypes to genes
    QApplication::setOverrideCursor(Qt::BusyCursor);
    NGSD db;
    GeneSet genes;
    for (const Phenotype& pheno : phenotypes)
    {
        genes << db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);
    }
    QApplication::restoreOverrideCursor();

    //open dialog
    openSubpanelDesignDialog(genes, box, state);
}

void FilterWidgetHelper::openSubpanelDesignDialog(const GeneSet& genes, QComboBox* box, FilterState& state)
{
    SubpanelDesignDialog dlg(&instance());
    dlg.setGenes(genes);

    dlg.exec();

    if (dlg.lastCreatedSubPanel()!="")
    {
        loadTargetRegions(box);

        //optionally use sub-panel as target regions
        if (QMessageBox::question(&instance(), "Use sub-panel?", "Do you want to set the sub-panel as target region?")==QMessageBox::Yes)
        {

            QString name = dlg.lastCreatedSubPanel();
            setTargetRegionByName(name, box);
            state.setTargetRegionInfoByName(name);
        }
    }
}


bool FilterWidgetHelper::setTargetRegionByName(QString name, QComboBox* roi)
{
    if (name.endsWith(".bed")) name = name.left(name.size()-4);

    QString system = "Processing system: " + name;
    QString subpanel ="Sub-panel: " + name;

    for (int i=0; i<roi->count(); ++i)
    {
        if (roi->itemText(i)==system || roi->itemText(i)==subpanel)
        {
            roi->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}


void FilterWidgetHelper::loadTargetRegions(QComboBox* box)
{
    box->blockSignals(true);

    //store old selection
    QString current = box->currentText();

    box->clear();
    box->addItem("", "");
    box->addItem("none", "");
    box->insertSeparator(box->count());

    if (LoginManager::active())
    {
        NGSD db;
        //load ROIs of NGSD processing systems
        SqlQuery query = db.getQuery();
        query.exec("SELECT name_manufacturer, target_file FROM processing_system ORDER by name_manufacturer ASC");
        while(query.next())
        {
            QString name = query.value(0).toString();
            QString roi = query.value(1).toString().trimmed();
            if (roi.isEmpty()) continue;

            box->addItem("Processing system: " + name, "Processing system: " + name);
        }
        box->insertSeparator(box->count());

        //load ROIs of NGSD sub-panels
        foreach(const QString& subpanel, db.subPanelList(false))
        {
            box->addItem("Sub-panel: " + subpanel, "Sub-panel: " + subpanel);
        }
        box->insertSeparator(box->count());
    }

    //load additional ROIs from settings
    QStringList rois = Settings::stringList("target_regions", true);
    std::sort(rois.begin(), rois.end(), [](const QString& a, const QString& b){return QFileInfo(a).fileName().toUpper() < QFileInfo(b).fileName().toUpper();});
    foreach(const QString& roi_file, rois)
    {
        QFileInfo info(roi_file);
        box->addItem(info.fileName(), roi_file);
    }

    //restore old selection
    int current_index = box->findText(current);
    if (current_index==-1) current_index = 1;
    box->setCurrentIndex(current_index);

    box->blockSignals(false);
}

void FilterWidgetHelper::loadTargetRegionData(TargetRegionInfo& roi, QString display_name)
{
    roi.clear();

    if (display_name.trimmed()=="") return;

    //ROI history
    FilterWidgetHelper::updateRoiHistory(display_name);

    if (display_name.startsWith("Sub-panel: "))
    {
        roi.name = display_name.split(":")[1].trimmed();

        NGSD db;
        roi.regions = db.subpanelRegions(roi.name);
        roi.regions.merge();

        roi.genes = db.subpanelGenes(roi.name);
    }
    else if (display_name.startsWith("Processing system: "))
    {
        roi.name = display_name.split(":")[1].trimmed();

        NGSD db;
        int sys_id = db.processingSystemId(roi.name);
        roi.regions = GlobalServiceProvider::database().processingSystemRegions(sys_id, false);
        roi.regions.merge();
        roi.genes = GlobalServiceProvider::database().processingSystemGenes(sys_id, true);
    }
    else //local target regions
    {
        roi.name = QFileInfo(display_name).baseName();

        roi.regions.load(display_name);
        roi.regions.merge();

        QString genes_file = display_name.left(display_name.size()-4) + "_genes.txt";
        if (QFile::exists(genes_file))
        {
            roi.genes = GeneSet::createFromFile(genes_file);
        }
    }
}

void FilterWidgetHelper::checkGeneNames(const GeneSet& genes, QLineEdit* widget)
{
    if (!LoginManager::active()) return;

    QStringList errors;
    NGSD db;
    for (const QByteArray& gene : genes)
    {
        if (!db.approvedGeneNames().contains(gene))
        {
            QByteArray approved = db.geneToApproved(gene, false);
            if (approved!="")
            {
                errors << "Gene symbol " + gene + " is not an approved HGNC symbol! Please use " + approved + "!";
            }
        }
    }
    if (errors.isEmpty())
    {
        widget->setToolTip("");
        widget->setStyleSheet("");
    }
    else
    {
        widget->setToolTip(errors.join("\n"));
        widget->setStyleSheet("QLineEdit {border: 2px solid red;}");
    }
}

void FilterWidgetHelper::updatePhenotypeHistory(const PhenotypeList& phenos)
{
    if (phenos.isEmpty()) return;

    QList<PhenotypeList>& history = instance().history_pheno;

    //already  contained > shift to top
    if (history.contains(phenos))
    {
        history.removeAll(phenos);
        history.prepend(phenos);
        return;
    }

    //new > prepend
    history.prepend(phenos);
    while(history.count()>10)
    {
        history.pop_back();
    }
}

const QList<PhenotypeList>& FilterWidgetHelper::phenotypeHistory()
{
    return instance().history_pheno;
}

void FilterWidgetHelper::updateRoiHistory(QString name)
{
    name.replace("Sub-panel:", "");
    name.replace("Processing system:", "");
    name = name.trimmed();
    if (name=="") return;

    QStringList& history = instance().history_roi;

    //already  contained > shift to top
    if (history.contains(name))
    {
        history.removeAll(name);
        history.prepend(name);
        return;
    }

    //new > prepend
    history.prepend(name);
    while(history.count()>10)
    {
        history.pop_back();
    }
}

const QStringList& FilterWidgetHelper::roiHistory()
{
    return instance().history_roi;
}

FilterWidgetHelper::FilterWidgetHelper()
    : QWidget{}
{
}


FilterWidgetHelper& FilterWidgetHelper::instance()
{
    static FilterWidgetHelper inst;
    return inst;
}

