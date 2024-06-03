#include "RepeatLocusList.h"
#include "VcfFile.h"

RepeatLocus::RepeatLocus()
{
}

bool RepeatLocus::isValid() const
{
	//check name
	if  (!region_.isValid() || unit_.isEmpty()) return false;

	//check alleles
	if  (allele1_.isEmpty()) return false;

	return true;
}

void RepeatLocus::setAllele1(const QByteArray& allele1)
{
	if (allele1==".") return;
	if (!Helper::isNumeric(allele1)) THROW(ArgumentException, "Cannot set non-numeric allele 1: " + allele1);
	allele1_ = allele1;
}

void RepeatLocus::setAllele2(const QByteArray& allele2)
{
	if (allele2==".") return;
	if (!Helper::isNumeric(allele2)) THROW(ArgumentException, "Cannot set non-numeric allele 2: " + allele2);
	allele2_ = allele2;
}

void RepeatLocus::setFilters(const QByteArrayList& filters)
{
	QByteArrayList tmp;
	foreach(QByteArray filter, filters)
	{
		filter = filter.trimmed();
		if (filter.isEmpty() || filter=="PASS") continue;
		tmp << filter;
	}
	filters_ = tmp;
}

void RepeatLocus::setCoverage(const QByteArray& coverage)
{
	if (!Helper::isNumeric(coverage)) THROW(ArgumentException, "Cannot set non-numeric coverage: " + coverage);
	coverage_ = coverage.trimmed();
}

bool RepeatLocus::sameRegionAndLocus(const RepeatLocus& rhs) const
{
	return region_==rhs.region_ && unit_==rhs.unit_;
}

QString RepeatLocus::toString(bool add_region_unit, bool add_genotypes) const
{
	QString output = name_;

	if (add_region_unit)
	{
		output += " - " + region_.toString(true) + "/" + unit_.trimmed();
	}

	if(add_genotypes)
	{
		output += " (allele1:" + allele1_;
		if (!allele2_.isEmpty()) output += " / allele2:" + allele2_;
		output += ")";
	}

	return output;
}

RepeatLocusList::RepeatLocusList()
	: variants_()
{
}

void RepeatLocusList::load(QString filename)
{
	clear();

	//load VCF file
	VcfFile repeat_expansions;
	repeat_expansions.load(filename);

	// check that there is exactly one sample
	const QByteArrayList& samples = repeat_expansions.sampleIDs();
	if (samples.count()!=1)
	{
		THROW(ArgumentException, "Repeat expansion VCF file '" + filename + "' does not contain exactly one sample!");
	}

	// fill table widget with variants/repeat expansions
	for(int row_idx=0; row_idx<repeat_expansions.count(); ++row_idx)
	{
		const VcfLine& re = repeat_expansions[row_idx];

		RepeatLocus rl;

		//repeat ID
		QByteArray repeat_id = re.info("REPID").trimmed();
		rl.setName(repeat_id);

		//region
		int end = Helper::toInt(re.info("END"), "END entry in INFO", "Repeat locus " + repeat_id);
		rl.setRegion(BedLine(re.chr(), re.start(), end));


		//repreat unit
		QByteArray repeat_unit = re.info("RU").trimmed();
		rl.setUnit(repeat_unit);

		//filters
		rl.setFilters(re.filters());

		//genotype
		QByteArrayList genotypes = re.formatValueFromSample("REPCN").trimmed().split('/');
		rl.setAllele1(genotypes[0]);
		if (genotypes.count()==2) rl.setAllele2(genotypes[1]);
		else if (genotypes.count()>2) THROW(ArgumentException, "Invalid number of genotypes in " + repeat_id +":" + re.formatValueFromSample("REPCN"));

		//genotype CI
		QByteArray genotype_ci = re.formatValueFromSample("REPCI").trimmed().replace(".", "-");
		rl.setConfidenceIntervals(genotype_ci);

		//local coverage
		QByteArray coverage = re.formatValueFromSample("LC").trimmed();
		rl.setCoverage(coverage);

		//reads flanking
		QByteArray reads_flanking = re.formatValueFromSample("ADFL").trimmed().replace(".", "-");
		rl.setReadsFlanking(reads_flanking);

		//reads in repeat
		QByteArray read_in_repeat = re.formatValueFromSample("ADIR").trimmed().replace(".", "-");
		rl.setReadsInRepeat(read_in_repeat);

		//reads flanking
		QByteArray reads_spanning = re.formatValueFromSample("ADSP").trimmed().replace(".", "-");
		rl.setReadsSpanning(reads_spanning);

		variants_.append(rl);
	}
}
