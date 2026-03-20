#ifndef FILTERWIDGETHELPER_H
#define FILTERWIDGETHELPER_H

#include <QObject>
#include <QComboBox>

#include "GeneSet.h"
#include "NGSHelper.h"
#include "FilterState.h"

class FilterWidgetHelper : public QWidget
{
    Q_OBJECT
public:
    ///Create sub-panel from phenotype
    static void createSubPanelFromPhenotypeFilter(const PhenotypeList& phenotypes, QComboBox* box, FilterState& state);
    ///Subpanel design dialog
    static void openSubpanelDesignDialog(const GeneSet& genes, QComboBox* box, FilterState& state);


    static bool setTargetRegionByName(QString name, QComboBox* roi);
    /// Helper for loading target regions
    static void loadTargetRegions(QComboBox* box);
    /// Helper for loading target region data. Throws an exception of the target region file is missing!
    static void loadTargetRegionData(TargetRegionInfo& roi, QString display_name);
    /// Helper for checking that gene names are approved symbols (also in CNV/SV widget)
    static void checkGeneNames(const GeneSet& genes, QLineEdit* widget);

    ///Updates phenotype history with new phenotype list
    static void updatePhenotypeHistory(const PhenotypeList& phenos);
    ///Returns phenotype history
    static const QList<PhenotypeList>& phenotypeHistory();

    ///Updates ROI history
    static void updateRoiHistory(QString name);
    ///Returns ROI history
    static const QStringList& roiHistory();

protected:
    static FilterWidgetHelper& instance();
    FilterWidgetHelper();

private:
    QList<PhenotypeList> history_pheno;
    QStringList history_roi;
};

#endif // FILTERWIDGETHELPER_H
