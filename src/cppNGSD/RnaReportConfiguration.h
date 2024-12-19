#ifndef RNAREPORTCONFIGURATION_H
#define RNAREPORTCONFIGURATION_H
#include "cppNGSD_global.h"
#include "VariantType.h"
#include "Helper.h"
#include "ReportConfiguration.h"
#include "FilterCascade.h"
#include <Chromosome.h>
#include <QString>
#include <QDateTime>

struct CPPNGSDSHARED_EXPORT RnaReportFusionConfiguration
{
	RnaReportFusionConfiguration();

	bool showInReport() const;

	int variant_index = -1;
	//exclusions
	bool exclude_artefact;
	bool exclude_low_tumor_content;
	bool exclude_low_evidence;
	bool exclude_other_reason;

	QString comment;
};

class CPPNGSDSHARED_EXPORT RnaReportConfiguration
{
public:
	RnaReportConfiguration();

	///Returns list containing all report variant configurations
	const QList<RnaReportFusionConfiguration>& fusionConfig() const;
	///Returns variant configuration for variant_index (index referes to index of main variant list!)
	const RnaReportFusionConfiguration& fusionConfig(int fusion_index) const;

	///all variant indices in configuration
	QList<int> fusionIndices(bool only_selected) const;

	///Check whether variant configuration exists for index
	bool exists(int index) const;

	/// clears the current list of fusion configurations.
	void clearRnaFusionConfigurations();
	/// adds rna fusion variant configuration to list if variant is already contained it updates the config.
	void addRnaFusionConfiguration(const RnaReportFusionConfiguration& config);

	///rna fusion report configuration index in file
	const RnaReportFusionConfiguration& get(int var_index) const;

	bool remove(int index);

	///returns variant count
	int count() const;

	///GETTER/SETTER
	QString createdBy() const;
	void setCreatedBy(QString user);

	QDateTime createdAt() const;
	void setCreatedAt(QDateTime time);

	QString processedSampleId() const;
	void setprocessedSampleId(QString id);

private:
	void sortByIndex();

	QList<RnaReportFusionConfiguration> fusion_config_;

	QString created_by_;
	QDateTime created_at_;
};

#endif // RNAREPORTCONFIGURATION_H
