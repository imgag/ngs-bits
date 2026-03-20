#ifndef FILTERSTATE_H
#define FILTERSTATE_H

#include <QObject>
#include "GeneSet.h"
#include "BedFile.h"
#include "Phenotype.h"
#include "PhenotypeList.h"
#include "NGSHelper.h"
#include "FilterCascade.h"

//Filter settings for report configuration
enum class ReportConfigFilter
{
    NONE,
    NO_RC,
    HAS_RC
};


class FilterState : public QObject
{
    Q_OBJECT
public:
    FilterState();

    GeneSet getRelevantGenes() const;

    //Getters:
    const QString& getFilterName() const;
    const FilterCascade& getFilterCascade() const;
    QStringList getFilterCascadeText() const;
    const TargetRegionInfo& getTargetRegionInfo() const;
    const GeneSet& getGenes() const;
    const QString& getTextFilter() const;
    const BedLine& getRegionFilter() const;
    const PhenotypeList& getPhenotypes() const;
    const PhenotypeSettings& getPhenotypeSettings() const;
    const BedFile& getPhenotypeRoi() const;
    const ReportConfigFilter& getReportConfigFilter() const;

signals:
    //general signal that filtering state has changed
    void filterStateChanged();

    //specific signals for the different filter types
    void filterNameChanged(const QString& name);
    void filterCascadeChanged(const FilterCascade& cascade);
    void targetRegionChanged(const TargetRegionInfo& target_region);
    void genesChanged(const GeneSet& genes);
    void textFilterChanged(const QString& text_filter);
    void regionFilterChanged(const BedLine& region);
    void phenotypesChanged(const PhenotypeList& phenotypes);
    void phenotypeSettingsChanged(const PhenotypeSettings& settings);
    void reportConfigFilterChanged(const ReportConfigFilter& rc_filter);

public slots:
    void clearPhenotypeFilter();
    void clearFilters(bool clear_roi);
    void clearTargetRegionFilter();

    //Setters:
    void setFilterName(const QString& filter_name, bool emit_gui_signal=true);
    void setFilterCascade(const FilterCascade& filter_cascade, bool emit_gui_signal=true);
    void setTargetRegionInfoByName(const QString& name, bool emit_gui_signal=true);
    void setTargetReionInfo(const TargetRegionInfo& target_region, bool emit_gui_signal=true);
    void setGenes(const GeneSet& genes, bool emit_gui_signal=true);
    void setTextFilter(const QString& text_filter, bool emit_gui_signal=true);
    void setRegionFilter(const BedLine& region_filter, bool emit_gui_signal=true);
    void setPhenotypes(const PhenotypeList& phenotypes, bool emit_gui_signal=true);
    void setPhenotypeSettings(const PhenotypeSettings& phenotype_settings, bool emit_gui_signal=true);
    void setReportConfigFilter(const ReportConfigFilter& rc_filter, bool emit_gui_signal=true);

private:
    void updatePhenotypeRoi();

    QString filter_name_;
    FilterCascade filter_cascade_;
    TargetRegionInfo target_region_;
    GeneSet genes_;
    QString text_filter_;
    BedLine region_filter_;
    PhenotypeList phenotypes_;
    PhenotypeSettings phenotype_settings_;
    BedFile phenotype_roi_;
    ReportConfigFilter rc_filter_;

};

#endif // FILTERSTATE_H
