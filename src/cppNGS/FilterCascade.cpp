#include "FilterCascade.h"
#include "Exceptions.h"
#include "GeneSet.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "Log.h"
#include "cmath"

/*************************************************** FilterParameter ***************************************************/

FilterParameter::FilterParameter(QString n, FilterParameterType t, QVariant v, QString d)
	: name(n)
	, type(t)
	, value(v)
	, description(d)
{
}

QString FilterParameter::valueAsString() const
{
	if (type==FilterParameterType::INT || type==FilterParameterType::DOUBLE || type==FilterParameterType::STRING)
	{
		return value.toString();
	}
	else if (type==FilterParameterType::BOOL)
	{
		return value.toBool() ? "yes" : "no";
	}
	else if (type==FilterParameterType::STRINGLIST)
	{
		return value.toStringList().join(",");
	}
	else
	{
		THROW(ProgrammingException, "Missing type in FilterParameter::typeAsString!");
	}
}

QString FilterParameter::typeAsString(FilterParameterType type)
{
	if (type==FilterParameterType::INT)
	{
		return "INT";
	}
	else if (type==FilterParameterType::DOUBLE)
	{
		return "DOUBLE";
	}
	else if (type==FilterParameterType::BOOL)
	{
		return "BOOL";
	}
	else if (type==FilterParameterType::STRING)
	{
		return "STRING";
	}
	else if (type==FilterParameterType::STRINGLIST)
	{
		return "STRINGLIST";
	}
	else
	{
		THROW(ProgrammingException, "Missing type in FilterParameter::typeAsString!");
	}
}

bool FilterParameter::operator==(const FilterParameter& rhs) const
{
	if (name!=rhs.name) return false;
	if (type!=rhs.type) return false;
	if (value.type()!=rhs.value.type()) return false;
	if (valueAsString()!=rhs.valueAsString()) return false;

	return true;
}


/*************************************************** FilterResult ***************************************************/

FilterResult::FilterResult()
{
}

FilterResult::FilterResult(int variant_count, bool value)
{
	pass = QBitArray(variant_count, value);
}

void FilterResult::removeFlagged(VariantList& variants)
{
	//skip if all variants pass
	if (countPassing()==variants.count()) return;

	//move passing variant to the front of the variant list
	int to_index = 0;
	for (int i=0; i<variants.count(); ++i)
	{
		if (pass[i])
		{
			if (to_index!=i)
			{
				variants[to_index] = variants[i];
			}
			++to_index;
		}
	}

	//resize to new size
	variants.resize(to_index);

	//update flags
	pass = QBitArray(variants.count(), true);
}

void FilterResult::removeFlagged(VcfFile& variants)
{
	//skip if all variants pass
	if (countPassing()==variants.count()) return;

	//move passing variant to the front of the variant list
	int to_index = 0;
	for (int i=0; i<variants.count(); ++i)
	{
		if (pass[i])
		{
			if (to_index!=i)
			{
				variants.vcfLine(to_index) = variants.vcfLine(i);
			}
			++to_index;
		}
	}

	//resize to new size
	variants.resize(to_index);

	//update flags
	pass = QBitArray(variants.count(), true);
}

void FilterResult::removeFlagged(CnvList& cnvs)
{
    //skip if all variants pass
    if (countPassing()==cnvs.count()) return;

    // create new empty CnvList
    CnvList passed_cnvs;
    passed_cnvs.copyMetaData(cnvs);

    //copy passing variants to the new variant list
    for (int i=0; i<cnvs.count(); ++i)
    {
        if (pass[i]) passed_cnvs.append(cnvs[i]);
    }

    // overwrite original CnvList
    cnvs = passed_cnvs;

    //update flags
    pass = QBitArray(cnvs.count(), true);
}

void FilterResult::removeFlagged(BedpeFile& svs)
{
    //skip if all variants pass
    if (countPassing()==svs.count()) return;

    //remove non-passing structural variants from list
    int removed_svs = 0; // index offset for already removed list entries
    for (int i=0; i<pass.size(); ++i)
    {
        if (!pass[i])
        {
            svs.removeAt(i - removed_svs);
            removed_svs++;
        }
    }

    //update flags
    pass = QBitArray(svs.count(), true);
}

void FilterResult::tagNonPassing(VariantList& variants, QByteArray tag, QByteArray description)
{
	//create 'filter' column (if missing)
	int index = variants.addAnnotationIfMissing("filter", "Filter column.");

	//add tag description (if missing)
	if (!variants.filters().contains(tag))
	{
		variants.filters().insert(tag, description);
	}

	//tag variants that did not pass
	for (int i=0; i<variants.count(); ++i)
	{
		if (!pass[i])
		{
			variants[i].addFilter(tag, index);
		}
	}
}

void FilterResult::tagNonPassing(VcfFile& variants, QByteArray tag, QString description)
{

	//add tag description (if missing)
	if (!variants.filterIDs().contains(tag))
	{
		variants.vcfHeader().addFilter(tag, description);
	}

	//tag variants that did not pass
	for (int i=0; i<variants.count(); ++i)
	{
		if (!pass[i])
		{
			variants[i].addFilter(tag);
		}
	}
}

/*************************************************** FilterBase ***************************************************/

FilterBase::FilterBase()
	: name_()
	, type_(VariantType::SNVS_INDELS)
	, description_()
	, params_()
	, enabled_(true)
{
}

FilterBase::~FilterBase()
{
}

QStringList FilterBase::description(bool add_parameter_description) const
{
	QStringList output = description_;
	if (add_parameter_description && params_.count()>0)
	{
		output << "Parameters:";

		foreach(const FilterParameter& p, params_)
		{
			QString text = p.name + " - " + p.description;
			QString default_value = p.type==FilterParameterType::STRINGLIST ? p.value.toStringList().join(",").trimmed() : p.value.toString().trimmed();
			if (default_value!="")
			{
				text += " [default=" + default_value + "]";
			}
			if (p.type==FilterParameterType::INT || p.type==FilterParameterType::DOUBLE)
			{
				if (p.constraints.contains("min"))
				{
					text += " [min=" + p.constraints["min"] + "]";
				}
				if (p.constraints.contains("max"))
				{
					text += " [max=" + p.constraints["max"] + "]";
				}
			}
			else if (p.type==FilterParameterType::STRING || p.type==FilterParameterType::STRINGLIST)
			{
				if (p.constraints.contains("valid"))
				{
					text += " [valid=" + p.constraints["valid"] + "]";
				}
				if (p.constraints.contains("not_empty"))
				{
					text += " [non-empty]";
				}
			}
			output << "  " + text;
		}
	}
	return output;
}

void FilterBase::setGeneric(const QString& name, const QString& value)
{
	FilterParameterType type = parameter(name).type;
	if (type==FilterParameterType::DOUBLE)
	{
		bool ok = false;
		double value_conv = value.toDouble(&ok);
		if (!ok) THROW(ArgumentException, "Could not convert '" + value + "' to double (parameter '" + name + "' of filter '" + this->name() + "')!");

		setDouble(name, value_conv);
	}
	else if (type==FilterParameterType::INT)
	{
		bool ok = false;
		int value_conv = value.toInt(&ok);
		if (!ok) THROW(ArgumentException, "Could not convert '" + value + "' to integer (parameter '" + name + "' of filter '" + this->name() + "')!");

		setInteger(name, value_conv);
	}
	else if (type==FilterParameterType::BOOL)
	{
		bool value_conv;
		if (value.toLower()=="yes" || value.toLower()=="true") value_conv = true;
		else if (value.toLower()=="no" || value.toLower()=="false") value_conv = false;
		else THROW(ArgumentException, "Could not convert '" + value + "' to boolean (parameter '" + name + "' of filter '" + this->name() + "')!");

		setBool(name, value_conv);
	}
	else if (type==FilterParameterType::STRING)
	{
		setString(name, value);
	}
	else if (type==FilterParameterType::STRINGLIST)
	{
		setStringList(name, value.split(','));
	}
	else
	{
		THROW(ProgrammingException, "Filter parameter type '" + FilterParameter::typeAsString(type) + "' not supported in setGenericParameter (parameter '" + name + "' of filter '" + this->name() + "')!");
	}
}

void FilterBase::setDouble(const QString& name, double value)
{
	checkParameterType(name, FilterParameterType::DOUBLE);

	parameter(name).value = value;
}

void FilterBase::setString(const QString& name, const QString& value)
{
	checkParameterType(name, FilterParameterType::STRING);

	parameter(name).value = value;
}

void FilterBase::setStringList(const QString& name, const QStringList& value)
{
	checkParameterType(name, FilterParameterType::STRINGLIST);

	parameter(name).value = value;
}

void FilterBase::overrideConstraint(const QString& parameter_name, const QString& constraint_name, const QString& constraint_value)
{
	parameter(parameter_name).constraints[constraint_name] = constraint_value;
}

void FilterBase::apply(const VariantList& /*variant_list*/, FilterResult& /*result*/) const
{
	THROW(NotImplementedException, "Method apply on VariantList not implemented for filter '" + name() + "'!");
}

void FilterBase::apply(const VcfFile& /*variants*/, FilterResult& /*result*/) const
{
	THROW(NotImplementedException, "Method apply on VcfFileHandler not implemented for filter '" + name() + "'!");
}


void FilterBase::apply(const CnvList& /*variant_list*/, FilterResult& /*result*/) const
{
	THROW(NotImplementedException, "Method apply on CnvList not implemented for filter '" + name() + "'!");
}

void FilterBase::apply(const BedpeFile& /*sv_list*/, FilterResult& /*result*/) const
{
	THROW(NotImplementedException, "Method apply on BedpeFile not implemented for filter '" + name() + "'!");
}

void FilterBase::setInteger(const QString& name, int value)
{
	checkParameterType(name, FilterParameterType::INT);

	parameter(name).value = value;
}

void FilterBase::setBool(const QString& name, bool value)
{
	checkParameterType(name, FilterParameterType::BOOL);

	parameter(name).value = value;
}

FilterParameter& FilterBase::parameter(const QString& name)
{
	for (int i=0; i<params_.count(); ++i)
	{
		if (params_[i].name==name) return params_[i];
	}

	THROW(ArgumentException, "Filter '" + this->name() + "' has no parameter '" + name + "'");
}

const FilterParameter& FilterBase::parameter(const QString& name) const
{
	for (int i=0; i<params_.count(); ++i)
	{
		if (params_[i].name==name) return params_[i];
	}

	THROW(ArgumentException, "Filter '" + this->name() + "' has no parameter '" + name + "'");
}

void FilterBase::checkParameterType(const QString& name, FilterParameterType type) const
{
	const FilterParameter& p = parameter(name);
	if (p.type!=type)
	{
		THROW(ProgrammingException, "Parameter '" + name + "' of filter '" + this->name() + "' used as '" + FilterParameter::typeAsString(type) + "', but has type '" + FilterParameter::typeAsString(p.type) + "!");
	}
}

double FilterBase::getDouble(const QString& name, bool check_constraints) const
{
	checkParameterType(name, FilterParameterType::DOUBLE);

	const FilterParameter& p = parameter(name);

	//value
	bool ok;
	double value = p.value.toDouble(&ok);
	if (!ok) THROW(ArgumentException, "Could not convert '" + p.value.toString() + "' to double (parameter '" + name + "' of filter '" + this->name() + "')!");

	if (check_constraints)
	{
		if (p.constraints.contains("min") && value < p.constraints["min"].toDouble())
		{
			THROW(ArgumentException, "Double value '" + QString::number(value) + "' smaller than minimum '" + p.constraints["min"] + "' (parameter '" + name + "' of filter '" + this->name() + "')!");
		}
		if (p.constraints.contains("max") && value > p.constraints["max"].toDouble())
		{
			THROW(ArgumentException, "Double value '" + QString::number(value) + "' bigger than maximum '" + p.constraints["max"] + "' (parameter '" + name + "' of filter '" + this->name() + "')!");
		}
	}

	return value;
}

int FilterBase::getInt(const QString& name, bool check_constraints) const
{
	checkParameterType(name, FilterParameterType::INT);

	const FilterParameter& p = parameter(name);

	//value
	bool ok;
	int value = p.value.toInt(&ok);
	if (!ok) THROW(ArgumentException, "Could not convert '" + p.value.toString() + "' to integer (parameter '" + name + "' of filter '" + this->name() + "')!");

	if (check_constraints)
	{
		if (p.constraints.contains("min") && value < p.constraints["min"].toInt())
		{
			THROW(ArgumentException, "Integer value '" + QString::number(value) + "' smaller than minimum '" + p.constraints["min"] + "' (parameter '" + name + "' of filter '" + this->name() + "')!");
		}
		if (p.constraints.contains("max") && value > p.constraints["max"].toInt())
		{
			THROW(ArgumentException, "Integer value '" + QString::number(value) + "' bigger than maximum '" + p.constraints["max"] + "' (parameter '" + name + "' of filter '" + this->name() + "')!");
		}
	}

	return value;
}

double FilterBase::getBool(const QString& name) const
{
	checkParameterType(name, FilterParameterType::BOOL);

	const FilterParameter& p = parameter(name);

	return p.value.toBool();
}

QString FilterBase::getString(const QString& name, bool check_constraints) const
{
	checkParameterType(name, FilterParameterType::STRING);

	const FilterParameter& p = parameter(name);

	QString value = p.value.toString().trimmed();

	if (check_constraints)
	{
		if (p.constraints.contains("valid"))
		{
			QStringList valid = p.constraints["valid"].split(',');
			if (!valid.contains(value))
			{
				THROW(ArgumentException, "String value '" + value + "' not valid. Valid are: '" + valid.join("', '") + "' (parameter '" + name + "' of filter '" + this->name() + "')!");
			}
		}
		if (p.constraints.contains("not_empty"))
		{
			if (value.isEmpty())
			{
				THROW(ArgumentException, "String value '" + value + "' must not be empty! (parameter '" + name + "' of filter '" + this->name() + "')!");
			}
		}
	}

	return value;
}

QStringList FilterBase::getStringList(const QString& name, bool check_constraints) const
{
	checkParameterType(name, FilterParameterType::STRINGLIST);

	const FilterParameter& p = parameter(name);

	QStringList list = p.value.toStringList();

	if (check_constraints)
	{
		if (p.constraints.contains("valid"))
		{
			QStringList valid = p.constraints["valid"].split(',');
			foreach(QString value, list)
			{
				if (!valid.contains(value))
				{
					THROW(ArgumentException, "String list value '" + value + "' not valid. Valid are: '" + valid.join("', '") + "' (parameter '" + name + "' of filter '" + this->name() + "')!");
				}
			}
		}
		if (p.constraints.contains("not_empty"))
		{
			if (list.join("").isEmpty())
			{
				THROW(ArgumentException, "String list must not be empty! (parameter '" + name + "' of filter '" + this->name() + "')!");
			}
		}
	}

	return list;
}

int FilterBase::annotationColumn(const VariantList& variant_list, const QString& column, bool throw_if_missing) const
{
	int index = variant_list.annotationIndexByName(column, true, false);
	if (throw_if_missing && index==-1)
	{
		THROW(ArgumentException, "Could not determine index of column '" + column + "' for filter '" + name() + "'!");
	}
	return index;
}

void FilterBase::checkIsRegistered() const
{
	if (!FilterFactory::filterNames().contains(name_))
	{
		THROW(ProgrammingException, "Filter '" + name() + "' is not registered!");
	}
}

/*************************************************** FilterCascade ***************************************************/

void FilterCascade::moveUp(int index)
{
	filters_.move(index, index-1);
	errors_.clear();
}

void FilterCascade::moveDown(int index)
{
	filters_.move(index, index+1);
	errors_.clear();
}

FilterResult FilterCascade::apply(const VariantList& variants, bool throw_errors, bool debug_time) const
{
	QTime timer;
	timer.start();

	FilterResult result(variants.count());

	//reset errors
	errors_.fill(QStringList(), filters_.count());

	if (debug_time)
	{
		Log::perf("FilterCascade: Initializing took ", timer);
		timer.start();
	}

	for(int i=0; i<filters_.count(); ++i)
	{
		QSharedPointer<FilterBase> filter = filters_[i];
		try
		{
			//check type
			if (filter->type()!=VariantType::SNVS_INDELS) THROW(ArgumentException, "Filter '" + filter->name() + "' cannot be applied to small variants!");

			//apply
			filter->apply(variants, result);

			if (debug_time)
			{
				Log::perf("FilterCascade: Filter " + filter->name() + " took ", timer);
				timer.start();
			}
		}
		catch(const Exception& e)
		{
			errors_[i].append(e.message());
			if (throw_errors)
			{
				throw e;
			}
		}
	}

	return result;
}

FilterResult FilterCascade::apply(const CnvList& cnvs, bool throw_errors, bool debug_time) const
{
	QTime timer;
	timer.start();

	FilterResult result(cnvs.count());

	//reset errors
	errors_.fill(QStringList(), filters_.count());

	if (debug_time)
	{
		Log::perf("FilterCascade: Initializing took ", timer);
		timer.start();
	}

	for(int i=0; i<filters_.count(); ++i)
	{
		QSharedPointer<FilterBase> filter = filters_[i];
		try
		{
			//check type
			if (filter->type()!=VariantType::CNVS) THROW(ArgumentException, "Filter '" + filter->name() + "' cannot be applied to CNVs!");

			//apply
			filter->apply(cnvs, result);

			if (debug_time)
			{
				Log::perf("FilterCascade: Filter " + filter->name() + " took ", timer);
				timer.start();
			}
		}
		catch(const Exception& e)
		{
			errors_[i].append(e.message());
			if (throw_errors)
			{
				throw e;
			}
		}
	}

	return result;
}

FilterResult FilterCascade::apply(const BedpeFile& svs, bool throw_errors, bool debug_time) const
{
	QTime timer;
	timer.start();

	FilterResult result(svs.count());

	//reset errors
	errors_.fill(QStringList(), filters_.count());

	if (debug_time)
	{
		Log::perf("FilterCascade: Initializing took ", timer);
		timer.start();
	}

	for(int i=0; i<filters_.count(); ++i)
	{
		QSharedPointer<FilterBase> filter = filters_[i];
		try
		{
			//check type
			if (filter->type()!=VariantType::SVS) THROW(ArgumentException, "Filter '" + filter->name() + "' cannot be applied to SVs!");

			//apply
			filter->apply(svs, result);

			if (debug_time)
			{
				Log::perf("FilterCascade: Filter " + filter->name() + " took ", timer);
				timer.start();
			}
		}
		catch(const Exception& e)
		{
			errors_[i].append(e.message());
			if (throw_errors)
			{
				throw e;
			}
		}
	}

	return result;
}


QStringList FilterCascade::errors(int index) const
{
	if (errors_.isEmpty())
	{
		return QStringList();
	}

	return errors_[index];
}

void FilterCascade::load(QString filename)
{
	//clear contents
	clear();

	//load filters from file
	QStringList lines = Helper::loadTextFile(filename, true, QChar::Null, true);
	this->operator=(fromText(lines));
}

void FilterCascade::store(QString filename)
{
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	foreach(QSharedPointer<FilterBase> filter, filters_)
	{
		QStringList params;
		foreach(const FilterParameter& param, filter->parameters())
		{
			params << param.name + "=" + param.valueAsString();
		}
		if (!filter->enabled()) params << "disabled";

		QString line = filter->name() + "\t" + params.join("\t") + "\n";

		file->write(line.toLatin1());
	}
	file->close();
}

FilterCascade FilterCascade::fromText(const QStringList& lines)
{
	FilterCascade output;

	foreach(QString line, lines)
	{
		line = line.trimmed();
		if (line.isEmpty()) continue;

		QStringList parts = line.split("\t");
		QString name = parts[0];
		output.add(FilterFactory::create(name, parts.mid(1)));
	}

	return output;
}

bool FilterCascade::operator==(const FilterCascade& rhs) const
{
	if (filters_.count()!=rhs.filters_.count()) return false;
	for (int i=0; i<filters_.count(); ++i)
	{
		QSharedPointer<FilterBase> f1 = filters_[i];
		QSharedPointer<FilterBase> f2 = rhs.filters_[i];

		//comare name/type
		if (f1->name()!=f2->name()) return false;
		if (f1->type()!=f2->type()) return false;

		//compare parameters
		if (f1->parameters().count()!=f2->parameters().count()) return false;
		for (int j=0; j<f1->parameters().count(); ++j)
		{
			if (f1->parameters()[j]!=f2->parameters()[j]) return false;
		}
	}

	return true;
}

/*************************************************** FilterCascadeFile ***************************************************/

QStringList FilterCascadeFile::names(QString filename)
{
	QStringList output;

	foreach(QString line, Helper::loadTextFile(filename, true, QChar::Null, true))
	{
		if (line.startsWith("#"))
		{
			output << line.mid(1);
		}
	}

	return output;
}

FilterCascade FilterCascadeFile::load(QString filename, QString filter)
{
	QStringList filter_file = Helper::loadTextFile(filename, true, QChar::Null, true);

	//extract text of filter
	QStringList filter_text;
	bool in_filter = false;
	foreach(const QString& line, filter_file)
	{
		if (line.startsWith("#"))
		{
			in_filter = (line == "#"+filter);
		}
		else if (in_filter)
		{
			filter_text << line;
		}
	}

	return FilterCascade::fromText(filter_text);
}


/*************************************************** FilterFactory ***************************************************/

QSharedPointer<FilterBase> FilterFactory::create(const QString& name, const QStringList& parameters)
{
	const auto& registry = getRegistry();

	//check that filter is known
	if (!registry.contains(name))
	{
		THROW(ArgumentException, "Filter name '" + name + "' is unknown! Valid filter names are: " + filterNames().join(", "));
	}

	//create filter
	QSharedPointer<FilterBase> filter = QSharedPointer<FilterBase>(registry[name]());

	//set parameters
	foreach(QString param, parameters)
	{
		if (param=="disabled")
		{
			filter->toggleEnabled();
		}
		else
		{
			int index = param.indexOf('=');
			filter->setGeneric(param.left(index), param.mid(index+1));
		}
	}

	return filter;
}

QStringList FilterFactory::filterNames()
{
	return getRegistry().keys();
}

QStringList FilterFactory::filterNames(VariantType subject)
{
	const auto& registry = getRegistry();
	QStringList names = registry.keys();

	foreach(const QString& name, names)
	{
		QSharedPointer<FilterBase> filter = QSharedPointer<FilterBase>(registry[name]());
		if (filter->type()!=subject)
		{
			names.removeAll(name);
		}
	}

	return names;
}

template<typename T> FilterBase* createInstance() { return new T; }

const QMap<QString, FilterBase*(*)()>& FilterFactory::getRegistry()
{
	static QMap<QString, FilterBase*(*)()> output;

	if (output.isEmpty())
	{
		output["Allele frequency"] = &createInstance<FilterAlleleFrequency>;
		output["Allele frequency (sub-populations)"] = &createInstance<FilterSubpopulationAlleleFrequency>;
		output["Genes"] = &createInstance<FilterGenes>;
		output["Filter column empty"] = &createInstance<FilterFilterColumnEmpty>;
		output["Filter columns"] = &createInstance<FilterFilterColumn>;
		output["SNVs only"] = &createInstance<FilterVariantIsSNV>;
		output["Impact"] = &createInstance<FilterVariantImpact>;
		output["Count NGSD"] = &createInstance<FilterVariantCountNGSD>;
		output["Classification NGSD"] = &createInstance<FilterClassificationNGSD>;
		output["Gene inheritance"] = &createInstance<FilterGeneInheritance>;
		output["Gene constraint"] = &createInstance<FilterGeneConstraint>;
		output["Genotype control"] = &createInstance<FilterGenotypeControl>;
		output["Genotype affected"] = &createInstance<FilterGenotypeAffected>;
		output["Column match"] = &createInstance<FilterColumnMatchRegexp>;
		output["Annotated pathogenic"] = &createInstance<FilterAnnotationPathogenic>;
		output["Predicted pathogenic"] = &createInstance<FilterPredictionPathogenic>;
		output["Text search"] = &createInstance<FilterAnnotationText>;
		output["Variant type"] = &createInstance<FilterVariantType>;
		output["Variant quality"] = &createInstance<FilterVariantQC>;
		output["Trio"] = &createInstance<FilterTrio>;
		output["OMIM genes"] = &createInstance<FilterOMIM>;
		output["Conservedness"] = &createInstance<FilterConservedness>;
		output["Regulatory"] = &createInstance<FilterRegulatory>;
		output["Somatic allele frequency"] = &createInstance<FilterSomaticAlleleFrequency>;
		output["Tumor zygosity"] = &createInstance<FilterTumorOnlyHomHet>;
		output["GSvar score/rank"] = &createInstance<FilterGSvarScoreAndRank>;
		output["CNV size"] = &createInstance<FilterCnvSize>;
		output["CNV regions"] = &createInstance<FilterCnvRegions>;
		output["CNV copy-number"] = &createInstance<FilterCnvCopyNumber>;
		output["CNV allele frequency"] = &createInstance<FilterCnvAlleleFrequency>;
		output["CNV z-score"] = &createInstance<FilterCnvZscore>;
		output["CNV log-likelihood"] = &createInstance<FilterCnvLoglikelihood>;
		output["CNV q-value"] = &createInstance<FilterCnvQvalue>;
		output["CNV compound-heterozygous"] = &createInstance<FilterCnvCompHet>;
		output["CNV OMIM genes"] = &createInstance<FilterCnvOMIM>;
		output["CNV polymorphism region"] = &createInstance<FilterCnvCnpOverlap>;
		output["CNV gene constraint"] = &createInstance<FilterCnvGeneConstraint>;
		output["CNV gene overlap"] = &createInstance<FilterCnvGeneOverlap>;
		output["SV type"] = &createInstance<FilterSvType>;
		output["SV remove chr type"] = &createInstance<FilterSvRemoveChromosomeType>;
		output["SV genotype control"] = &createInstance<FilterSvGenotypeControl>;
		output["SV genotype affected"] = &createInstance<FilterSvGenotypeAffected>;
		output["SV quality"] = &createInstance<FilterSvQuality>;
		output["SV filter columns"] = &createInstance<FilterSvFilterColumn>;
		output["SV paired read AF"] = &createInstance<FilterSvPairedReadAF>;
		output["SV split read AF"] = &createInstance<FilterSvSplitReadAF>;
		output["SV PE read depth"] = &createInstance<FilterSvPeReadDepth>;
		output["SV SomaticScore"] = &createInstance<FilterSvSomaticscore>;
		output["SV gene constraint"] = &createInstance<FilterSvGeneConstraint>;
		output["SV gene overlap"] = &createInstance<FilterSvGeneOverlap>;
		output["SV size"] = &createInstance<FilterSvSize>;
		output["SV OMIM genes"] = &createInstance<FilterSvOMIM>;
		output["SV compound-heterozygous"] = &createInstance<FilterSvCompHet>;
		output["CNV pathogenic CNV overlap"] = &createInstance<FilterCnvPathogenicCnvOverlap>;
		output["SV count NGSD"] = &createInstance<FilterSvCountNGSD>;
		output["SV allele frequency NGSD"] = &createInstance<FilterSvAfNGSD>;
        output["SV trio"] = &createInstance<FilterSvTrio>;
	}

	return output;
}

/*************************************************** concrete filters for small variants ***************************************************/

FilterAlleleFrequency::FilterAlleleFrequency()
{
	name_ = "Allele frequency";
	description_ = QStringList() << "Filter based on overall allele frequency given by 1000 Genomes and gnomAD.";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 1.0, "Maximum allele frequency in %");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "100.0";

	checkIsRegistered();
}

QString FilterAlleleFrequency::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_af", false), 'f', 2) + '%';
}

void FilterAlleleFrequency::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	//check parameters
	double max_af = getDouble("max_af")/100.0;

	//get column indices
	int i_1000g = annotationColumn(variants, "1000g");
	int i_gnomad = annotationColumn(variants, "gnomAD");

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		result.flags()[i] = result.flags()[i]
			&& variants[i].annotations()[i_1000g].toDouble()<=max_af
			&& variants[i].annotations()[i_gnomad].toDouble()<=max_af;
	}
}

FilterGenes::FilterGenes()
{
	name_ = "Genes";
	description_ = QStringList() << "Filter for that preserves a gene set.";
	params_ << FilterParameter("genes", FilterParameterType::STRINGLIST, QStringList(), "Gene set");
	params_.last().constraints["not_empty"] = "";

	checkIsRegistered();
}

QString FilterGenes::toText() const
{
	return name() + " " + getStringList("genes", false).join(",");
}

void FilterGenes::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	GeneSet genes = GeneSet::createFromStringList(getStringList("genes"));

	//get column indices
	int i_gene = annotationColumn(variants, "gene");

	//filter (text-based)
	if (!genes.join('|').contains("*"))
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = genes.intersectsWith(GeneSet::createFromText(variants[i].annotations()[i_gene], ','));
		}
	}
	else //filter (regexp)
	{
		QRegExp reg(genes.join('|').replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			GeneSet var_genes = GeneSet::createFromText(variants[i].annotations()[i_gene], ',');
			bool match_found = false;
			foreach(const QByteArray& var_gene, var_genes)
			{
				if (reg.exactMatch(var_gene))
				{
					match_found = true;
					break;
				}
			}
			result.flags()[i] = match_found;
		}
	}
}


void FilterRegions::apply(const VariantList& variants, const BedFile& regions, FilterResult& result)
{
	//check regions
	if(!regions.isMergedAndSorted())
	{
		THROW(ArgumentException, "Cannot filter variant list by regions that are not merged/sorted!");
	}

	//special case when only one region is contained
	if (regions.count()==1)
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = variants[i].overlapsWith(regions[0]);
		}
		return;
	}

	//general case with many regions
	ChromosomalIndex<BedFile> regions_idx(regions);
	for (int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		const Variant& v = variants[i];
		int index = regions_idx.matchingIndex(v.chr(), v.start(), v.end());
		result.flags()[i] = (index!=-1);
	}
}

void FilterRegions::apply(const VcfFile& variants, const BedFile& regions, FilterResult& result)
{
	//check regions
	if(!regions.isMergedAndSorted())
	{
		THROW(ArgumentException, "Cannot filter variant list by regions that are not merged/sorted!");
	}

	//special case when only one region is contained
	if (regions.count()==1)
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = variants[i].overlapsWith(regions[0]);
		}
		return;
	}

	//general case with many regions
	ChromosomalIndex<BedFile> regions_idx(regions);
	for (int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		const  VcfLine& v = variants[i];
		int index = regions_idx.matchingIndex(v.chr(), v.start(), v.end());
		result.flags()[i] = (index!=-1);
	}
}


FilterFilterColumnEmpty::FilterFilterColumnEmpty()
{
	name_ = "Filter column empty";
	description_ = QStringList() << "Filter that perserves variants which have no entry in the 'filter' column.";

	checkIsRegistered();
}

QString FilterFilterColumnEmpty::toText() const
{
	return name();
}

void FilterFilterColumnEmpty::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = variants[i].filters().isEmpty();
	}
}

void FilterFilterColumnEmpty::apply(const VcfFile& variants, FilterResult& result) const
{
	if (!enabled_) return;

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = variants.vcfLine(i).failedFilters().isEmpty();
	}
}

FilterVariantIsSNV::FilterVariantIsSNV()
{
	name_ = "SNVs only";
	description_ = QStringList() << "Filter that preserves SNVs and removes all other variant types.";
	params_ << FilterParameter("invert", FilterParameterType::BOOL, false, "If set, removes all SNVs and keeps all other variants.");

	checkIsRegistered();
}

QString FilterVariantIsSNV::toText() const
{
	return name() + (getBool("invert") ? " (invert)" : "");
}

void FilterVariantIsSNV::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	bool invert = getBool("invert");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (invert)
		{
			result.flags()[i] = !variants[i].isSNV();
		}
		else
		{
			result.flags()[i] = variants[i].isSNV();
		}
	}
}
void FilterVariantIsSNV::apply(const VcfFile& variants, FilterResult& result) const
{
	if (!enabled_) return;

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = variants[i].isSNV();
	}
}

FilterSubpopulationAlleleFrequency::FilterSubpopulationAlleleFrequency()
{
	name_ = "Allele frequency (sub-populations)";
	description_ = QStringList() << "Filter based on sub-population allele frequency given by gnomAD.";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 1.0, "Maximum allele frequency in %");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "100.0";

	checkIsRegistered();
}

QString FilterSubpopulationAlleleFrequency::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_af", false), 'f', 2) + '%';
}

void FilterSubpopulationAlleleFrequency::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	//check parameters
	double max_af = getDouble("max_af")/100.0;

	//filter
	int i_gnomad = annotationColumn(variants, "gnomAD_sub");
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QByteArrayList parts = variants[i].annotations()[i_gnomad].split(',');
		foreach(const QByteArray& part, parts)
		{
			if (part.toDouble()>max_af)
			{
				result.flags()[i] = false;
				break;
			}
		}
	}
}

FilterVariantImpact::FilterVariantImpact()
{
	name_ = "Impact";
	description_ = QStringList() << "Filter based on the variant impact given by VEP." << "For more details see: https://www.ensembl.org/info/genome/variation/prediction/predicted_data.html";

	params_ << FilterParameter("impact", FilterParameterType::STRINGLIST, QStringList() << "HIGH" << "MODERATE" << "LOW", "Valid impacts");
	params_.last().constraints["valid"] = "HIGH,MODERATE,LOW,MODIFIER";
	params_.last().constraints["not_empty"] = "";

	checkIsRegistered();
}

QString FilterVariantImpact::toText() const
{
	return name() + " " + getStringList("impact", false).join(",");
}

void FilterVariantImpact::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	//get column indices
	int i_co_sp = annotationColumn(variants, "coding_and_splicing");

	//prepare impacts list (convert to QByteArray and pad with ":")
	QByteArrayList impacts = getStringList("impact").join(":,:").prepend(":").append(":").toLatin1().split(',');

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		bool pass_impact = false;
		foreach(const QByteArray& impact, impacts)
		{
			if (variants[i].annotations()[i_co_sp].contains(impact))
			{
				pass_impact = true;
				break;
			}
		}
		result.flags()[i] = pass_impact;
	}
}


FilterVariantCountNGSD::FilterVariantCountNGSD()
{
	name_ = "Count NGSD";
	description_ = QStringList() << "Filter based on the hom/het occurances of a variant in the NGSD.";
	params_ << FilterParameter("max_count", FilterParameterType::INT, 20, "Maximum NGSD count");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("ignore_genotype", FilterParameterType::BOOL, false, "If set, all NGSD entries are counted independent of the variant genotype. Otherwise, for homozygous variants only homozygous NGSD entries are counted and for heterozygous variants all NGSD entries are counted.");

	checkIsRegistered();
}

QString FilterVariantCountNGSD::toText() const
{
	return name() + " &le; " + QString::number(getInt("max_count", false)) + (getBool("ignore_genotype") ? " (ignore genotype)" : "");
}

void FilterVariantCountNGSD::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int max_count = getInt("max_count");

	int i_ihdb_hom = annotationColumn(variants, "NGSD_hom");
	int i_ihdb_het = annotationColumn(variants, "NGSD_het");

	if (getBool("ignore_genotype"))
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = (variants[i].annotations()[i_ihdb_hom].toInt() + variants[i].annotations()[i_ihdb_het].toInt()) <= max_count;
		}
	}
	else
	{
		//get affected column indices
		QList<int> geno_indices = variants.getSampleHeader().sampleColumns(true);
		if (geno_indices.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without affected samples!");

		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			bool var_is_hom = false;
			foreach(int index, geno_indices)
			{
				const QByteArray& var_geno = variants[i].annotations()[index];
				if (var_geno=="hom")
				{
					var_is_hom = true;
					break;
				}
				else if (var_geno!="het" && var_geno!="wt" && var_geno!="n/a")
				{
					THROW(ProgrammingException, "Unknown genotype '" + var_geno + "'!");
				}
			}

			result.flags()[i] = (variants[i].annotations()[i_ihdb_hom].toInt() + (var_is_hom ? 0 : variants[i].annotations()[i_ihdb_het].toInt())) <= max_count;
		}
	}
}

FilterFilterColumn::FilterFilterColumn()
{
	name_ = "Filter columns";
	description_ = QStringList() << "Filter based on the entries of the 'filter' column.";

	params_ << FilterParameter("entries", FilterParameterType::STRINGLIST, QStringList(), "Filter column entries");
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("action", FilterParameterType::STRING, "REMOVE", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,REMOVE,FILTER";

	checkIsRegistered();
}

QString FilterFilterColumn::toText() const
{
	return name() + " " + getString("action", false) + ": " + getStringList("entries", false).join(",");
}

void FilterFilterColumn::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	entries.clear();
	foreach(const QString& entry, getStringList("entries"))
	{
		entries.append(entry.toLatin1());
	}

	QString action = getString("action");
	if (action=="REMOVE")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = !match(variants[i]);
		}
	}
	else if (action=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
}

bool FilterFilterColumn::match(const Variant& v) const
{
	foreach(const QByteArray& f,  v.filters())
	{
		if (entries.contains(f)) return true;
	}

	return false;
}


FilterClassificationNGSD::FilterClassificationNGSD()
{
	name_ = "Classification NGSD";
	description_ = QStringList() << "Filter for variant classification from NGSD.";

	params_ << FilterParameter("classes", FilterParameterType::STRINGLIST, QStringList() << "4" << "5", "NGSD classes");
	params_.last().constraints["valid"] = "1,2,3,4,5,M";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("action", FilterParameterType::STRING, "KEEP", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,FILTER,REMOVE";

	checkIsRegistered();
}

QString FilterClassificationNGSD::toText() const
{
	return name() + " " + getString("action", false) + ": " + getStringList("classes", false).join(",");
}

void FilterClassificationNGSD::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	classes = getStringList("classes");
	i_class = annotationColumn(variants, "classification");

	QString action = getString("action");
	if (action=="REMOVE")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = !match(variants[i]);
		}
	}
	else if (action=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
}

bool FilterClassificationNGSD::match(const Variant& v) const
{
	QString classification = v.annotations()[i_class].trimmed();
	if (classification.isEmpty()) return false;

	return classes.contains(classification);
}


FilterGeneInheritance::FilterGeneInheritance()
{
	name_ = "Gene inheritance";
	description_ = QStringList() << "Filter based on gene inheritance.";

	params_ << FilterParameter("modes", FilterParameterType::STRINGLIST, QStringList(), "Inheritance mode(s)");
	params_.last().constraints["valid"] = "AR,AD,XLR,XLD,MT";
	params_.last().constraints["not_empty"] = "";

	checkIsRegistered();
}

QString FilterGeneInheritance::toText() const
{
	return name() + " " + getStringList("modes", false).join(",");
}

void FilterGeneInheritance::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	//get column indices
	int i_geneinfo = annotationColumn(variants, "gene_info");
	QStringList modes_passing = getStringList("modes");

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//parse gene_info entry - example: AL627309.1 (inh=n/a pLI=n/a), PRPF31 (inh=AD pLI=0.97), 34P13.14 (inh=n/a pLI=n/a)
		QByteArrayList genes = variants[i].annotations()[i_geneinfo].split(',');
		bool any_gene_passed = false;
		foreach(const QByteArray& gene, genes)
		{
			int start = gene.indexOf('(');
			QByteArrayList entries = gene.mid(start+1, gene.length()-start-2).split(' ');
			foreach(const QByteArray& entry, entries)
			{
				if (entry.startsWith("inh="))
				{
					QStringList modes = QString(entry.mid(4)).split('+');
					foreach(const QString& mode, modes)
					{
						if (modes_passing.contains(mode))
						{
							any_gene_passed = true;
						}
					}
				}
			}
		}
		result.flags()[i] = any_gene_passed;
	}
}

FilterGeneConstraint::FilterGeneConstraint()
{
	name_ = "Gene constraint";
	description_ = QStringList() << "Filter based on gene constraint (gnomAD o/e score for LOF variants)." << "Note that gene constraint is most helpful for early-onset severe diseases." << "For details on gnomAD o/e, see https://macarthurlab.org/2018/10/17/gnomad-v2-1/" << "Note: ExAC pLI is deprected and support for backward compatibility with old GSvar files.";

	params_ << FilterParameter("max_oe_lof", FilterParameterType::DOUBLE, 0.35, "Maximum gnomAD o/e score for LoF variants");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	params_ << FilterParameter("min_pli",FilterParameterType:: DOUBLE, 0.9, "Minumum ExAC pLI score");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterGeneConstraint::toText() const
{
	return name() + " o/e&le;" + QString::number(getDouble("max_oe_lof", false), 'f', 2) + " (pLI&ge;" + QString::number(getDouble("min_pli", false), 'f', 2) + ")";
}

void FilterGeneConstraint::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	//get column indices
	int i_geneinfo = annotationColumn(variants, "gene_info");
	double min_pli = getDouble("min_pli");
	double max_oe_lof = getDouble("max_oe_lof");

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//parse gene_info entry - example: AL627309.1 (inh=n/a pLI=n/a), PRPF31 (inh=AD pLI=0.97), 34P13.14 (inh=n/a pLI=n/a oe_lof=)
		QByteArrayList genes = variants[i].annotations()[i_geneinfo].split(',');
		bool any_gene_passed = false;
		foreach(const QByteArray& gene, genes)
		{
			int start = gene.indexOf('(');
			QByteArrayList entries = gene.mid(start+1, gene.length()-start-2).split(' ');
			foreach(const QByteArray& entry, entries)
			{
				if (entry.startsWith("pLI="))
				{
					bool ok;
					double pli = entry.mid(4).toDouble(&ok);
					if (!ok) pli = 0.0; // value 'n/a' > pass
					if (pli>=min_pli)
					{
						any_gene_passed = true;
					}
				}
				if (entry.startsWith("oe_lof="))
				{
					bool ok;
					double oe = entry.mid(7).toDouble(&ok);
					if (!ok) oe = 1.0; // value 'n/a' > pass
					if (oe<=max_oe_lof)
					{
						any_gene_passed = true;
					}
				}
			}
		}
		result.flags()[i] = any_gene_passed;
	}
}


FilterGenotypeControl::FilterGenotypeControl()
{
	name_ = "Genotype control";
	description_ = QStringList() << "Filter for genotype of the 'control' sample(s).";

	params_ << FilterParameter("genotypes", FilterParameterType::STRINGLIST, QStringList(), "Genotype(s)");
	params_.last().constraints["valid"] = "wt,het,hom,n/a";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("same_genotype", FilterParameterType::BOOL, false, "Also check that all 'control' samples have the same genotype.");

	checkIsRegistered();
}

QString FilterGenotypeControl::toText() const
{
	return name() + " " + getStringList("genotypes", false).join(",");
}

void FilterGenotypeControl::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	QStringList genotypes = getStringList("genotypes");
	bool same_genotype = getBool("same_genotype");

	//get control column indices
	QList<int> geno_indices = variants.getSampleHeader().sampleColumns(false);
	if (geno_indices.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without control samples!");

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (same_genotype)
		{
			QByteArray geno_all = checkSameGenotype(geno_indices, variants[i]);
			if (geno_all.isEmpty() || !genotypes.contains(geno_all))
			{
				result.flags()[i] = false;
			}
		}
		else
		{
			foreach(int index, geno_indices)
			{
				QString geno = variants[i].annotations()[index];
				if (!genotypes.contains(geno))
				{
					result.flags()[i] = false;
					break;
				}
			}
		}
	}
}

QByteArray FilterGenotypeControl::checkSameGenotype(const QList<int>& geno_indices, const Variant& v) const
{
	QByteArray geno = v.annotations()[geno_indices[0]];
	for (int i=1; i<geno_indices.count(); ++i)
	{
		if (v.annotations()[geno_indices[i]] != geno)
		{
			return QByteArray();
		}
	}

	return geno;
}

FilterGenotypeAffected::FilterGenotypeAffected()
{
	name_ = "Genotype affected";
	description_ = QStringList() << "Filter for genotype(s) of the 'affected' sample(s)." << "Variants pass if 'affected' samples have the same genotype and the genotype is in the list selected genotype(s).";
	params_ << FilterParameter("genotypes", FilterParameterType::STRINGLIST, QStringList(), "Genotype(s)");
	params_.last().constraints["valid"] = "wt,het,hom,n/a,comp-het";
	params_.last().constraints["not_empty"] = "";

	checkIsRegistered();
}

QString FilterGenotypeAffected::toText() const
{
	return name() + " " + getStringList("genotypes", false).join(",");
}

void FilterGenotypeAffected::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	QStringList genotypes = getStringList("genotypes");

	//get affected column indices
	QList<int> geno_indices = variants.getSampleHeader().sampleColumns(true);
	if (geno_indices.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without affected samples!");


	//filter
	if (!genotypes.contains("comp-het"))
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QByteArray geno_all = checkSameGenotype(geno_indices, variants[i]);
			if (geno_all.isEmpty() || !genotypes.contains(geno_all))
			{
				result.flags()[i] = false;
			}
		}
	}
	else
	{
		int i_gene = annotationColumn(variants, "gene");

		//(1) filter for all genotypes but comp-het
		//(2) count heterozygous passing variants per gene
		QHash<QByteArray, int> gene_to_het;
		FilterResult result_other(variants.count());
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i])
			{
				result_other.flags()[i] = false;
				continue;
			}

			QByteArray geno_all = checkSameGenotype(geno_indices, variants[i]);
			result_other.flags()[i] = !geno_all.isEmpty() && genotypes.contains(geno_all);

			if (geno_all=="het")
			{
				QList<QByteArray> genes = variants[i].annotations()[i_gene].toUpper().split(',');
				foreach(const QByteArray& gene, genes)
				{
					gene_to_het[gene.trimmed()] += 1;
				}

			}
		}

		//apply combined results from above
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			//other filter pass => pass
			if (result_other.flags()[i]) continue;

			//check for comp-het
			bool pass = false;
			QByteArray geno_all = checkSameGenotype(geno_indices, variants[i]);
			if (geno_all=="het")
			{
				QList<QByteArray> genes = variants[i].annotations()[i_gene].toUpper().split(',');
				foreach(const QByteArray& gene, genes)
				{
					if (gene_to_het[gene.trimmed()]>=2)
					{
						pass = true;
						break;
					}
				}
			}
			result.flags()[i] = pass;
		}
	}
}

QByteArray FilterGenotypeAffected::checkSameGenotype(const QList<int>& geno_indices, const Variant& v) const
{
	QByteArray geno = v.annotations()[geno_indices[0]];
	for (int i=1; i<geno_indices.count(); ++i)
	{
		if (v.annotations()[geno_indices[i]] != geno)
		{
			return QByteArray();
		}
	}

	return geno;
}

FilterColumnMatchRegexp::FilterColumnMatchRegexp()
{
	name_ = "Column match";
	description_ = QStringList() << "Filter that matches the content of a column against a perl-compatible regular expression." << "For details about regular expressions, see http://perldoc.perl.org/perlretut.html";

	params_ << FilterParameter("pattern", FilterParameterType::STRING, "", "Pattern to match to column");
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("column", FilterParameterType::STRING, "", "Column to filter");
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("action", FilterParameterType::STRING, "KEEP", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,FILTER,REMOVE";

	checkIsRegistered();
}

QString FilterColumnMatchRegexp::toText() const
{
	return name() + " " + getString("action", false) + ": " + getString("column", false) + " '" + getString("pattern", false) + "'";
}

void FilterColumnMatchRegexp::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	index = annotationColumn(variants, getString("column"));
	regexp.setPattern(getString("pattern"));

	QString action = getString("action");
	if (action=="REMOVE")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = !match(variants[i]);
		}
	}
	else if (action=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
}

bool FilterColumnMatchRegexp::match(const Variant& v) const
{
	QByteArray content = v.annotations()[index].trimmed();
	return regexp.match(content).hasMatch();
}


FilterAnnotationPathogenic::FilterAnnotationPathogenic()
{
	name_ = "Annotated pathogenic";
	description_ = QStringList() << "Filter that matches variants annotated to be pathogenic by ClinVar or HGMD.";

	params_ << FilterParameter("sources", FilterParameterType::STRINGLIST, QStringList() << "ClinVar" << "HGMD", "Sources of pathogenicity to use");
	params_.last().constraints["valid"] = "ClinVar,HGMD";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("also_likely_pathogenic", FilterParameterType::BOOL, false, "Also consider likely pathogenic variants");
	params_ << FilterParameter("action", FilterParameterType::STRING, "KEEP", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,FILTER";

	checkIsRegistered();
}

QString FilterAnnotationPathogenic::toText() const
{
	return name() + " " + getString("action", false) + ": " + getStringList("sources", false).join(",") + " " + (getBool("also_likely_pathogenic") ? " (also likely pathogenic)" : "");
}

void FilterAnnotationPathogenic::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	QStringList sources = getStringList("sources");
	also_likely_pathogenic = getBool("also_likely_pathogenic");
	i_clinvar = sources.contains("ClinVar") ? annotationColumn(variants, "ClinVar") : -1;
	i_hgmd = sources.contains("HGMD") ? annotationColumn(variants, "HGMD", false) : -1;

	if (getString("action")=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = annotatedPathogenic(variants[i]);
		}

	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;

			result.flags()[i] = annotatedPathogenic(variants[i]);
		}
	}
}

bool FilterAnnotationPathogenic::annotatedPathogenic(const Variant& v) const
{
	//ClinVar
	if (i_clinvar!=-1)
	{
		const QByteArray& clinvar = v.annotations()[i_clinvar];
		if (clinvar.contains("[pathogenic"))
		{
			return true;
		}
		if (also_likely_pathogenic && clinvar.contains("[likely pathogenic"))
		{
			return true;
		}
	}

	//HGMD
	if (i_hgmd!=-1)
	{
		const QByteArray& hgmd = v.annotations()[i_hgmd];
		if (hgmd.contains("CLASS=DM")) //matches both "DM" and "DM?"
		{
			if (also_likely_pathogenic)
			{
				return true;
			}
			else if (!hgmd.contains("CLASS=DM?"))
			{
				return true;
			}
		}
	}

	return false;
}

FilterPredictionPathogenic::FilterPredictionPathogenic()
{
	name_ = "Predicted pathogenic";
	description_ = QStringList() << "Filter for variants predicted to be pathogenic." << "Prediction scores included are: phyloP>=1.6, Sift=D, PolyPhen=D, fathmm-MKL>=0.5, CADD>=20 and REVEL>=0.5.";
	params_ << FilterParameter("min", FilterParameterType::INT, 1, "Minimum number of pathogenic predictions");
	params_.last().constraints["min"] = "1";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,FILTER";

	checkIsRegistered();
}

QString FilterPredictionPathogenic::toText() const
{
	return name() + " " + getString("action", false) + " &ge; " + QString::number(getInt("min", false));
}

void FilterPredictionPathogenic::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	min = getInt("min");
	i_phylop = annotationColumn(variants, "phyloP");
	i_sift = annotationColumn(variants, "Sift");
	i_polyphen = annotationColumn(variants, "PolyPhen");
	i_fathmm = annotationColumn(variants, "fathmm-MKL");
	i_cadd = annotationColumn(variants, "CADD");
	i_revel = annotationColumn(variants, "REVEL");

	if (getString("action")=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = predictedPathogenic(variants[i]);
		}
	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;

			result.flags()[i] = predictedPathogenic(variants[i]);
		}
	}
}

bool FilterPredictionPathogenic::predictedPathogenic(const Variant& v) const
{
	int count = 0;

	if (v.annotations()[i_sift].contains("D"))
	{
		++count;
	}

	if ( v.annotations()[i_polyphen].contains("D"))
	{
		++count;
	}

	if (v.annotations()[i_fathmm].contains(","))
	{
		QByteArrayList parts = v.annotations()[i_fathmm].split(',');
		foreach(const QByteArray& part, parts)
		{
			bool ok = true;
			double value = part.toDouble(&ok);
			if (ok && value>=0.5)
			{
				++count;
				break;
			}
		}
	}

	bool ok;
	double value = v.annotations()[i_phylop].toDouble(&ok);
	if (ok && value>=1.6) ++count;


	value = v.annotations()[i_cadd].toDouble(&ok);
	if (ok && value>=20.0) ++count;


	value = v.annotations()[i_revel].toDouble(&ok);
	if (ok && value>=0.5) ++count;

	return count>=min;
}


FilterAnnotationText::FilterAnnotationText()
{
	name_ = "Text search";
	description_ = QStringList() << "Filter for text match in variant annotations." << "The text comparison ignores the case.";
	params_ << FilterParameter("term", FilterParameterType::STRING, QString(), "Search term");
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "FILTER,KEEP,REMOVE";

	checkIsRegistered();
}

QString FilterAnnotationText::toText() const
{
	return name() + " " + getString("action", false) + " " + getString("term", false);
}

void FilterAnnotationText::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	term = getString("term").toLatin1().trimmed().toLower();

	QString action = getString("action");
	if (action=="REMOVE")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = !match(variants[i]);
		}
	}
	else if (action=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;

			result.flags()[i] = match(variants[i]);
		}
	}
}

bool FilterAnnotationText::match(const Variant& v) const
{
	foreach(const QByteArray& anno, v.annotations())
	{
		if (anno.toLower().contains(term))
		{
			return true;
		}
	}

	return false;
}

FilterVariantType::FilterVariantType()
{
	name_ = "Variant type";
	description_ = QStringList() << "Filter for variant types as defined by sequence ontology." << "For details see http://www.sequenceontology.org/browser/obob.cgi";
	params_ << FilterParameter("HIGH", FilterParameterType::STRINGLIST, QStringList() << "frameshift_variant" << "splice_acceptor_variant" << "splice_donor_variant" << "start_lost" << "start_retained_variant" << "stop_gained" << "stop_lost", "High impact variant types");
	params_.last().constraints["valid"] = "frameshift_variant,splice_acceptor_variant,splice_donor_variant,start_lost,start_retained_variant,stop_gained,stop_lost";

	params_ << FilterParameter("MODERATE", FilterParameterType::STRINGLIST, QStringList() << "inframe_deletion" << "inframe_insertion" << "missense_variant", "Moderate impact variant types");
	params_.last().constraints["valid"] = "inframe_deletion,inframe_insertion,missense_variant";

	params_ << FilterParameter("LOW", FilterParameterType::STRINGLIST, QStringList() << "splice_region_variant", "Low impact variant types");
	params_.last().constraints["valid"] = "splice_region_variant,stop_retained_variant,synonymous_variant";

	params_ << FilterParameter("MODIFIER", FilterParameterType::STRINGLIST, QStringList(), "Lowest impact variant types");
	params_.last().constraints["valid"] = "3_prime_UTR_variant,5_prime_UTR_variant,NMD_transcript_variant,downstream_gene_variant,intergenic_variant,intron_variant,mature_miRNA_variant,non_coding_transcript_exon_variant,non_coding_transcript_variant,upstream_gene_variant";

	checkIsRegistered();
}

QString FilterVariantType::toText() const
{
	QStringList selected;
	selected << getStringList("HIGH", false);
	selected << getStringList("MODERATE", false);
	selected << getStringList("LOW", false);
	selected << getStringList("MODIFIER", false);

	return name() + " " + selected.join(",");
}

void FilterVariantType::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	QByteArrayList types;
	foreach(QString type, getStringList("HIGH"))
	{
		types.append(type.trimmed().toLatin1());
	}
	foreach(QString type, getStringList("MODERATE"))
	{
		types.append(type.trimmed().toLatin1());
	}
	foreach(QString type, getStringList("LOW"))
	{
		types.append(type.trimmed().toLatin1());
	}
	foreach(QString type, getStringList("MODIFIER"))
	{
		types.append(type.trimmed().toLatin1());
	}

	int index = annotationColumn(variants, "coding_and_splicing");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		bool match_found = false;
		foreach(const QByteArray& type, types)
		{
			if (variants[i].annotations()[index].contains(type))
			{
				match_found = true;
				break;
			}
		}
		result.flags()[i] = match_found;
	}
}


FilterVariantQC::FilterVariantQC()
{
	name_ = "Variant quality";
	description_ = QStringList() << "Filter for variant quality";
	params_ << FilterParameter("qual", FilterParameterType::INT, 250, "Minimum variant quality score (Phred)");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("depth", FilterParameterType::INT, 0, "Minimum depth");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("mapq", FilterParameterType::INT, 40, "Minimum mapping quality of alternate allele (Phred)");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("strand_bias", FilterParameterType::INT, 20, "Maximum strand bias Phred score of alternate allele (set -1 to disable)");
	params_.last().constraints["min"] = "-1";
	params_ << FilterParameter("allele_balance", FilterParameterType::INT, 40, "Maximum allele balance Phred score (set -1 to disable)");
	params_.last().constraints["min"] = "-1";

	checkIsRegistered();
}

QString FilterVariantQC::toText() const
{
	return name() + " qual&ge;" + QString::number(getInt("qual", false)) + " depth&ge;" + QString::number(getInt("depth", false)) + " mapq&ge;" + QString::number(getInt("mapq", false)) + " strand_bias&le;" + QString::number(getInt("strand_bias", false)) + " allele_balance&le;" + QString::number(getInt("allele_balance", false));
}

void FilterVariantQC::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int index = annotationColumn(variants, "quality");
	int qual = getInt("qual");
	int depth = getInt("depth");
	int mapq = getInt("mapq");
	int strand_bias = getInt("strand_bias");
	int allele_balance = getInt("allele_balance");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;
		QByteArrayList parts = variants[i].annotations()[index].split(';');
		foreach(const QByteArray& part, parts)
		{
			if (part.startsWith("QUAL="))
			{
				//also handle floats (should not be necessary, but floats were used due to a bug in the somatic single-sample pipeline)
				QByteArray qual_str = part.mid(5);
				if (qual_str.contains('.')) qual_str = qual_str.left(qual_str.indexOf('.'));

				if (qual_str.toInt()<qual)
				{
					result.flags()[i] = false;
				}
			}
			else if (part.startsWith("DP="))
			{
				if (!part.contains(',')) //single-sample analysis
				{
					if (part.mid(3).toInt()<depth)
					{
						result.flags()[i] = false;
					}
				}
				else //multi-sample analysis (comma-separed depth values)
				{
					QByteArrayList dps = part.mid(3).split(',');
					foreach(const QByteArray& dp, dps)
					{
						if (dp.toInt()<depth)
						{
							result.flags()[i] = false;
						}
					}
				}
			}
			else if (part.startsWith("MQM="))
			{
				if (part.mid(4).toInt()<mapq)
				{
					result.flags()[i] = false;
				}
			}
			else if (strand_bias>=0 && part.startsWith("SAP="))
			{
				if (part.mid(4).toInt()>strand_bias)
				{
					result.flags()[i] = false;
				}
			}
			else if (allele_balance>=0 && part.startsWith("ABP="))
			{
				if (part.mid(4).toInt()>allele_balance)
				{
					result.flags()[i] = false;
				}
			}
		}
	}
}


FilterTrio::FilterTrio()
{
	name_ = "Trio";
	description_ = QStringList() << "Filter trio variants";
	params_ << FilterParameter("types", FilterParameterType::STRINGLIST, QStringList() << "de-novo" << "recessive" << "comp-het" << "LOH" << "x-linked", "Variant types");
	params_.last().constraints["valid"] = "de-novo,recessive,comp-het,LOH,x-linked,imprinting";
	params_.last().constraints["non-empty"] = "";

	params_ << FilterParameter("gender_child", FilterParameterType::STRING, "n/a", "Gender of the child - if 'n/a', the gender from the GSvar file header is taken");
	params_.last().constraints["valid"] = "male,female,n/a";

	checkIsRegistered();
}

QString FilterTrio::toText() const
{
	return name() + " " + getStringList("types", false).join(',');
}

void FilterTrio::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	//determine child gender
	QString gender_child = getString("gender_child");
	if (gender_child=="n/a")
	{
		gender_child = variants.getSampleHeader().infoByStatus(true).gender();
	}
	if (gender_child=="n/a")
	{
		THROW(ArgumentException, "Could not determine gender of child, please set it!");
	}

	//determine column indices
	i_quality = annotationColumn(variants, "quality");
	int i_gene = annotationColumn(variants, "gene");
	SampleHeaderInfo sample_headers = variants.getSampleHeader();
	i_c = sample_headers.infoByStatus(true).column_index;
	i_f = sample_headers.infoByStatus(false, "male").column_index;
	i_m = sample_headers.infoByStatus(false, "female").column_index;

	//determine AF indices
	QList<int> tmp;
	tmp << i_c << i_f << i_m;
	std::sort(tmp.begin(), tmp.end());
	i_af_c = tmp.indexOf(i_c);
	i_af_f = tmp.indexOf(i_f);
	i_af_m = tmp.indexOf(i_m);

	//get PAR region
	BedFile par_region = NGSHelper::pseudoAutosomalRegion("hg19");

	//pre-calculate genes with heterozygous variants
	QSet<QString> types = getStringList("types").toSet();
	GeneSet genes_comphet;
	if (types.contains("comp-het"))
	{
		GeneSet het_father;
		GeneSet het_mother;

		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			const Variant& v = variants[i];
			bool diplod_chromosome = v.chr().isAutosome() || (v.chr().isX() && gender_child=="female") || (v.chr().isX() && par_region.overlapsWith(v.chr(), v.start(), v.end()));
			if (diplod_chromosome)
			{
				QByteArray geno_c, geno_f, geno_m;
				correctedGenotypes(v, geno_c, geno_f, geno_m);

				if (geno_c=="het" && geno_f=="het" && geno_m=="wt")
				{
					het_mother << GeneSet::createFromText(v.annotations()[i_gene], ',');
				}
				if (geno_c=="het" && geno_f=="wt" && geno_m=="het")
				{
					het_father << GeneSet::createFromText(v.annotations()[i_gene], ',');
				}
			}
		}
		genes_comphet = het_mother.intersect(het_father);
	}

	//load imprinting gene list
	QMap<QByteArray, ImprintingInfo> imprinting = NGSHelper::imprintingGenes();

	//apply
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		const Variant& v = variants[i];

		//get genotypes
		QByteArray geno_c, geno_f, geno_m;
		correctedGenotypes(v, geno_c, geno_f, geno_m);

		//remove variants where index is wild-type
		if (geno_c=="wt")
		{
			result.flags()[i] = false;
			continue;
		}

		//remove variants where genotype data is missing
		if (geno_c=="n/a" || geno_f=="n/a" || geno_m=="n/a")
		{
			result.flags()[i] = false;
			continue;
		}

		bool diplod_chromosome = v.chr().isAutosome() || (v.chr().isX() && gender_child=="female") || (v.chr().isX() && par_region.overlapsWith(v.chr(), v.start(), v.end()));

		//filter
		bool match = false;
		if (types.contains("de-novo"))
		{
			if (geno_f=="wt" && geno_m=="wt")
			{
				match = true;
			}
		}
		if (types.contains("recessive"))
		{
			if (diplod_chromosome)
			{
				if (geno_c=="hom" && geno_f=="het" && geno_m=="het")
				{
					match = true;
				}
			}
		}
		if (types.contains("LOH"))
		{
			if (diplod_chromosome)
			{
				if ((geno_c=="hom" && geno_f=="het" && geno_m=="wt") || (geno_c=="hom" && geno_f=="wt" && geno_m=="het"))
				{
					match = true;
				}
			}
		}
		if (types.contains("comp-het"))
		{
			if (diplod_chromosome)
			{
				if ((geno_c=="het" && geno_f=="het" && geno_m=="wt")
					||
					(geno_c=="het" && geno_f=="wt" && geno_m=="het"))
				{
					if (genes_comphet.intersectsWith(GeneSet::createFromText(v.annotations()[i_gene], ',')))
					{
						match = true;
					}
				}
			}
		}
		if (types.contains("x-linked") && v.chr().isX() && gender_child=="male")
		{
			if (geno_c=="hom" && geno_f=="wt" && geno_m=="het")
			{
				match = true;
			}
		}
		if (types.contains("imprinting"))
		{
			if (geno_c=="het" && geno_f=="het" && geno_m=="wt")
			{
				GeneSet genes = GeneSet::createFromText(v.annotations()[i_gene], ',');
				foreach(const QByteArray& gene, genes)
				{
					if (imprinting.contains(gene) && imprinting[gene].source_allele!="maternal")
					{
						match = true;
					}
				}
			}
			if (geno_c=="het" && geno_f=="wt" && geno_m=="het")
			{
				GeneSet genes = GeneSet::createFromText(v.annotations()[i_gene], ',');
				foreach(const QByteArray& gene, genes)
				{
					if (imprinting.contains(gene) && imprinting[gene].source_allele!="paternal")
					{
						match = true;
					}
				}
			}
		}

		result.flags()[i] = match;
	}
}

void FilterTrio::correctedGenotypes(const Variant& v, QByteArray& geno_c, QByteArray& geno_f, QByteArray& geno_m) const
{
	geno_c = v.annotations()[i_c];
	geno_f = v.annotations()[i_f];
	geno_m = v.annotations()[i_m];

	//correct genotypes based on AF
	QByteArrayList q_parts = v.annotations()[i_quality].split(';');
	foreach(const QByteArray& part, q_parts)
	{
		if (part.startsWith("AF="))
		{
			QByteArrayList af_parts = part.mid(3).split(',');

			if (geno_f=="wt" && af_parts[i_af_f].toDouble()>=0.05 && af_parts[i_af_f].toDouble()<=0.3)
			{
				geno_f = "het";
			}
			if (geno_m=="wt" && af_parts[i_af_m].toDouble()>=0.05 && af_parts[i_af_m].toDouble()<=0.3)
			{
				geno_m = "het";
			}
			if (geno_c=="het" && af_parts[i_af_c].toDouble()<0.1)
			{
				geno_c = "wt";
			}
		}
	}


}

FilterOMIM::FilterOMIM()
{
	name_ = "OMIM genes";
	description_ = QStringList() << "Filter for OMIM genes i.e. the 'OMIM' column is not empty.";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "REMOVE,FILTER";
	checkIsRegistered();
}

QString FilterOMIM::toText() const
{
	return name();
}

void FilterOMIM::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int index = annotationColumn(variants, "OMIM");

	QString action = getString("action");
	if (action=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (variants[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
	else //REMOVE
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (!variants[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterConservedness::FilterConservedness()
{
	name_ = "Conservedness";
	description_ = QStringList() << "Filter for variants that affect conserved bases";
	params_ << FilterParameter("min_score", FilterParameterType::DOUBLE, 1.6, "Minimum phlyoP score.");

	checkIsRegistered();
}

QString FilterConservedness::toText() const
{
	return name() + " phyloP&ge;" + QString::number(getDouble("min_score", false));
}

void FilterConservedness::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int i_phylop = annotationColumn(variants, "phyloP");
	double min_score = getDouble("min_score");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		bool ok;
		double value = variants[i].annotations()[i_phylop].toDouble(&ok);
		if (!ok || value<min_score)
		{
			result.flags()[i] = false;
		}
	}
}

FilterRegulatory::FilterRegulatory()
{
	name_ = "Regulatory";
	description_ = QStringList() << "Filter for regulatory variants, i.e. the 'regulatory' column is not empty.";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "REMOVE,FILTER";

	checkIsRegistered();
}

QString FilterRegulatory::toText() const
{
	return name() + " " + getString("action", false);
}

void FilterRegulatory::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int index = annotationColumn(variants, "regulatory");

	QString action = getString("action");
	if (action=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (variants[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
	else //REMOVE
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (!variants[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
}

/*************************************************** concrete filters for CNVs ***************************************************/

FilterCnvSize::FilterCnvSize()
{
	name_ = "CNV size";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for CNV size (kilobases).";
	params_ << FilterParameter("size", FilterParameterType::DOUBLE, 0.0, "Minimum CNV size in kilobases");
	params_.last().constraints["min"] = "0";

	checkIsRegistered();
}

QString FilterCnvSize::toText() const
{
	return name() + " size&ge;" + QString::number(getDouble("size", false), 'f', 2) + " kB";
}

void FilterCnvSize::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	double min_size_bases = getDouble("size") * 1000.0;
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (cnvs[i].size() < min_size_bases)
		{
			result.flags()[i] = false;
		}
	}
}

FilterCnvRegions::FilterCnvRegions()
{
	name_ = "CNV regions";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for number of regions/exons.";
	params_ << FilterParameter("regions", FilterParameterType::INT, 3, "Minimum number of regions");
	params_.last().constraints["min"] = "1";

	checkIsRegistered();
}

QString FilterCnvRegions::toText() const
{
	return name() + " &ge; " + QString::number(getInt("regions"));
}

void FilterCnvRegions::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	int min_regions = getInt("regions");
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (cnvs[i].regions()==0) continue; //multi-sample CNV lists sometimes don't contain region counts (e.g. for ClinCNV)

		if (cnvs[i].regions() < min_regions)
		{
			result.flags()[i] = false;
		}
	}
}

FilterCnvCopyNumber::FilterCnvCopyNumber()
{
	name_ = "CNV copy-number";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for CNV copy number.";
	params_ << FilterParameter("cn", FilterParameterType::STRING, "n/a", "Copy number");
	params_.last().constraints["valid"] = "n/a,0,1,2,3,4+";

	checkIsRegistered();
}

QString FilterCnvCopyNumber::toText() const
{
	return name() + " CN=" + getString("cn");
}

void FilterCnvCopyNumber::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	QByteArray cn_exp = getString("cn").toLatin1();
	bool cn_exp_4plus = cn_exp=="4+";
	if (cn_exp=="n/a") return;

	if (cnvs.type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE || cnvs.type()==CnvListType::CNVHUNTER_GERMLINE_MULTI)
	{
		int i_cns = cnvs.annotationIndexByName("region_copy_numbers", true);
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QByteArrayList cns = cnvs[i].annotations()[i_cns].split(',');

			bool hit = false;
			foreach (const QByteArray& cn, cns)
			{
				if ((!cn_exp_4plus && cn==cn_exp) || (cn_exp_4plus && cn.toInt()>=4))
				{
					hit = true;
					break;
				}
			}
			result.flags()[i] = hit;
		}
	}
	else
	{
		int i_cn = cnvs.annotationIndexByName("CN_change", true);
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			const QByteArray& cn = cnvs[i].annotations()[i_cn];

			if (!((!cn_exp_4plus && cn==cn_exp) || (cn_exp_4plus && cn.toInt()>=4)))
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterCnvAlleleFrequency::FilterCnvAlleleFrequency()
{
	name_ = "CNV allele frequency";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for CNV allele frequency in the analyzed cohort.";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 0.05, "Maximum allele frequency");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterCnvAlleleFrequency::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_af"), 'f', 2);
}

void FilterCnvAlleleFrequency::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	double max_af = getDouble("max_af");

	if (cnvs.type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE || cnvs.type()==CnvListType::CNVHUNTER_GERMLINE_MULTI)
	{
		int i_afs = cnvs.annotationIndexByName("region_cnv_af", true);
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QByteArrayList afs = cnvs[i].annotations()[i_afs].split(',');

			bool hit = false;
			foreach (const QByteArray& af, afs)
			{
				if (af.toDouble()<=max_af)
				{
					hit = true;
					break;
				}
			}
			result.flags()[i] = hit;
		}
	}
	else
	{
		int i_af = cnvs.annotationIndexByName("potential_AF", true);
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			const QByteArray& af = cnvs[i].annotations()[i_af];

			if (af.toDouble()>max_af)
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterCnvZscore::FilterCnvZscore()
{
	name_ = "CNV z-score";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for CNV z-score." << "The z-score determines to what degee that the region was a statistical outlier when compared to the reference samples." << "Note: for deletions z-scores lower than the negative cutoff pass." << "Note: this filter works for CnvHunter CNV lists only!";
	params_ << FilterParameter("min_z", FilterParameterType::DOUBLE, 4.0, "Minimum z-score");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "10.0";

	checkIsRegistered();
}

QString FilterCnvZscore::toText() const
{
	return name() + " &ge; " + QString::number(getDouble("min_z"), 'f', 2);
}

void FilterCnvZscore::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	if (cnvs.type()!=CnvListType::CNVHUNTER_GERMLINE_SINGLE && cnvs.type()!=CnvListType::CNVHUNTER_GERMLINE_MULTI) THROW(ArgumentException, "Filter '" + name() + "' can only be applied to CNV lists generated by CnvHunter!");

	double min_z = getDouble("min_z");
	int i_zs = cnvs.annotationIndexByName("region_zscores", true);
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QByteArrayList zs = cnvs[i].annotations()[i_zs].split(',');

		bool hit = false;
		foreach (const QByteArray& z, zs)
		{
			double z_num = fabs(z.toDouble());
			if (z_num>=min_z)
			{
				hit = true;
				break;
			}
		}
		result.flags()[i] = hit;
	}
}

FilterCnvLoglikelihood::FilterCnvLoglikelihood()
{
	name_ = "CNV log-likelihood";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for CNV log-likelihood." << "The log-likelihood is the logarithm of the ratio between likelihoods of the no CN change model vs the CN equal to the reported state model (bigger is better). If scale by region is checked the total log-likelihood value is normalized by the number of regions." << "Note: when applied to multi-sample CNV lists, each log-likelihood entry must exceed the cutuff!" << "Note: this filter works for CNV lists generated by ClinCNV only!" << "Note: log-likelihood scaling can only be applied to CNV lists with regions count";
	params_ << FilterParameter("min_ll", FilterParameterType::DOUBLE, 20.0, "Minimum log-likelihood");
	params_.last().constraints["min"] = "0.0";
	params_ << FilterParameter("scale_by_regions", FilterParameterType::BOOL, false, "Scale log-likelihood by number of regions.");

	checkIsRegistered();
}

QString FilterCnvLoglikelihood::toText() const
{
	return name() + " &ge; " + QString::number(getDouble("min_ll"), 'f', 2) + QString(getBool("scale_by_regions")?" (scaled by regions)":"");
}

void FilterCnvLoglikelihood::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	double min_ll = getDouble("min_ll");
	bool scale_by_regions = getBool("scale_by_regions");
	int i_ll = cnvs.annotationIndexByName("loglikelihood", true);
	if (cnvs.type()==CnvListType::CLINCNV_GERMLINE_SINGLE || cnvs.type()==CnvListType::CLINCNV_TUMOR_NORMAL_PAIR || cnvs.type()==CnvListType::CLINCNV_TUMOR_ONLY)
	{
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (scale_by_regions)
			{
				int number_of_regions = cnvs[i].regions();
				if (number_of_regions < 0) THROW(FileParseException, "Invalid/unset number of regions!");
				double scaled_ll = cnvs[i].annotations()[i_ll].toDouble() / number_of_regions;
				if (scaled_ll < min_ll)
				{
					result.flags()[i] = false;
				}
			}
			else
			{
				if (cnvs[i].annotations()[i_ll].toDouble()<min_ll)
				{
					result.flags()[i] = false;
				}
			}
		}
	}
	else if (cnvs.type()==CnvListType::CLINCNV_GERMLINE_MULTI)
	{
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QByteArrayList lls = cnvs[i].annotations()[i_ll].split(',');
			foreach(const QByteArray& ll, lls)
			{
				if (scale_by_regions)
				{
					int number_of_regions = cnvs[i].regions();
					if (number_of_regions < 0) THROW(FileParseException, "Invalid/unset number of regions!");
					double scaled_ll = ll.toDouble() / number_of_regions;
					if (scaled_ll < min_ll)
					{
						result.flags()[i] = false;
						break;
					}
				}
				else
				{
					if (ll.toDouble()<min_ll)
					{
						result.flags()[i] = false;
						break;
					}
				}
			}
		}
	}
	else
	{
		THROW(ArgumentException, "Filter '" + name() + "' can only be applied to CNV lists generated by ClinCNV!");
	}
}

FilterCnvQvalue::FilterCnvQvalue()
{
	name_ = "CNV q-value";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for CNV q-value." << "The q-value is the p-value corrected for the number of CNVs detected (smaller is better)" << "Note: when applied to multi-sample CNV lists, each q-value must be below the cutuff!" << "Note: this filter works for CNV lists generated by ClinCNV only!";
	params_ << FilterParameter("max_q", FilterParameterType::DOUBLE, 1.0, "Maximum q-value");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterCnvQvalue::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_q"), 'f', 2);
}

void FilterCnvQvalue::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	double max_q = getDouble("max_q");
	int i_q = cnvs.annotationIndexByName("qvalue", true);

	if (cnvs.type()==CnvListType::CLINCNV_GERMLINE_SINGLE || cnvs.type()==CnvListType::CLINCNV_TUMOR_ONLY)
	{
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (cnvs[i].annotations()[i_q].toDouble()>max_q)
			{
				result.flags()[i] = false;
			}
		}
	}
	else if (cnvs.type()==CnvListType::CLINCNV_GERMLINE_MULTI)
	{
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QByteArrayList qs = cnvs[i].annotations()[i_q].split(',');
			foreach(const QByteArray& q, qs)
			{
				if (q.toDouble()>max_q)
				{
					result.flags()[i] = false;
					break;
				}
			}
		}
	}
	else
	{
		THROW(ArgumentException, "Filter '" + name() + "' can only be applied to CNV lists generated by ClinCNV!");
	}

}

FilterCnvCompHet::FilterCnvCompHet()
{
	name_ = "CNV compound-heterozygous";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for compound-heterozygous CNVs." << "Mode 'CNV-CNV' detects genes with two or more CNV hits." << "Mode 'CNV-SNV/INDEL' detectes genes with exactly one CNV and exactly one small variant hit (after other filters are applied).";
	params_ << FilterParameter("mode", FilterParameterType::STRING, "n/a", "Compound-heterozygotes detection mode.");
	params_.last().constraints["valid"] = "n/a,CNV-CNV,CNV-SNV/INDEL";

	checkIsRegistered();
}

QString FilterCnvCompHet::toText() const
{
	return name() + " " + getString("mode");
}

void FilterCnvCompHet::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	QString mode = getString("mode");
	if (mode=="n/a") return;

	//count hits per gene for CNVs
	QMap<QByteArray, int> gene_count;
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		foreach(const QByteArray& gene, cnvs[i].genes())
		{
			gene_count[gene] += 1;
		}
	}

	GeneSet comphet_hit;

	//two CNV hits
	if (mode=="CNV-CNV")
	{
		for(auto it=gene_count.cbegin(); it!=gene_count.cend(); ++it)
		{
			if (it.value()>1)
			{
				comphet_hit.insert(it.key());
			}
		}
	}

	//one CNV and one SNV/INDEL hit
	else if (mode=="CNV-SNV/INDEL")
	{
		GeneSet single_hit_cnv;
		for(auto it=gene_count.cbegin(); it!=gene_count.cend(); ++it)
		{
			if (it.value()==1)
			{
				single_hit_cnv.insert(it.key());
			}
		}

		foreach(const QByteArray& gene, single_hit_cnv)
		{
			if (het_hit_genes_.contains(gene))
			{
				comphet_hit.insert(gene);
			}
		}
	}

	//flag passing CNVs
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = cnvs[i].genes().intersectsWith(comphet_hit);
	}
}

FilterCnvOMIM::FilterCnvOMIM()
{
	name_ = "CNV OMIM genes";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for OMIM genes i.e. the 'OMIM' column is not empty.";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "REMOVE,FILTER";

	checkIsRegistered();
}

QString FilterCnvOMIM::toText() const
{
	return name();
}

void FilterCnvOMIM::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	int index = cnvs.annotationIndexByName("omim", true);
	QString action = getString("action");
	if (action=="FILTER")
	{
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (cnvs[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
	else //REMOVE
	{
		for(int i=0; i<cnvs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (!cnvs[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterCnvCnpOverlap::FilterCnvCnpOverlap()
{
	name_ = "CNV polymorphism region";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for overlap with CNP regions.";
	params_ << FilterParameter("column", FilterParameterType::STRING, "overlap af_genomes_imgag", "CNP column name");
	params_ << FilterParameter("max_ol", FilterParameterType::DOUBLE, 0.95, "Maximum overlap");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterCnvCnpOverlap::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_ol"), 'f', 2) + " (column: " + getString("column") + ")";
}

void FilterCnvCnpOverlap::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	int index = cnvs.annotationIndexByName(getString("column").toUtf8(), true);
	double max_ol = getDouble("max_ol");

	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (cnvs[i].annotations()[index].left(5).toDouble()>max_ol)
		{
			result.flags()[i] = false;
		}
	}
}

FilterCnvGeneConstraint::FilterCnvGeneConstraint()
{
	name_ = "CNV gene constraint";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter based on gene constraint (gnomAD o/e score for LOF variants)." << "Note that gene constraint is most helpful for early-onset severe diseases." << "For details on gnomAD o/e, see https://macarthurlab.org/2018/10/17/gnomad-v2-1/";

	params_ << FilterParameter("max_oe_lof", FilterParameterType::DOUBLE, 0.35, "Maximum gnomAD o/e score for LoF variants");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterCnvGeneConstraint::toText() const
{
	return name() + " o/e&le;" + QString::number(getDouble("max_oe_lof", false), 'f', 2);
}

void FilterCnvGeneConstraint::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	//get column indices
	int i_geneinfo = cnvs.annotationIndexByName("gene_info", true);
	double max_oe_lof = getDouble("max_oe_lof");

	//filter
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//parse gene_info entry - example: 34P13.14 (region=complete oe_lof=), ...
		QByteArrayList gene_entries= cnvs[i].annotations()[i_geneinfo].split(',');
		bool any_gene_passed = false;
		foreach(const QByteArray& gene, gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
			foreach(const QByteArray& term, term_entries)
			{
				if (term.startsWith("oe_lof="))
				{
					bool ok;
					double oe = term.mid(7).toDouble(&ok);
					if (!ok) oe = 1.0; // value 'n/a' > pass
					if (oe<=max_oe_lof)
					{
						any_gene_passed = true;
					}
				}
			}
		}
		result.flags()[i] = any_gene_passed;
	}
}

FilterCnvGeneOverlap::FilterCnvGeneOverlap()
{
	name_ = "CNV gene overlap";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter based on gene overlap.";

	params_ << FilterParameter("complete", FilterParameterType::BOOL, true , "Overlaps the complete gene.");
	params_ << FilterParameter("exonic/splicing", FilterParameterType::BOOL, true , "Overlaps the coding or splicing region of the gene.");
	params_ << FilterParameter("intronic/intergenic", FilterParameterType::BOOL, false , "Overlaps the intronic/intergenic region of the gene only.");

	checkIsRegistered();
}

QString FilterCnvGeneOverlap::toText() const
{
	return name() + " " + selectedOptions().join(", ");
}

void FilterCnvGeneOverlap::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	int i_geneinfo = cnvs.annotationIndexByName("gene_info", true);
	QByteArrayList selected = selectedOptions();

	//filter
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//parse gene_info entry - example: 34P13.14 (region=complete oe_lof=), ...
		QByteArrayList gene_entries = cnvs[i].annotations()[i_geneinfo].split(',');
		bool any_gene_passed = false;
		foreach(const QByteArray& gene, gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
			foreach(const QByteArray& term, term_entries)
			{
				if (term.startsWith("region="))
				{
					if (selected.contains(term.mid(7)))
					{
						any_gene_passed = true;
					}
				}
			}
		}
		result.flags()[i] = any_gene_passed;
	}
}

QByteArrayList FilterCnvGeneOverlap::selectedOptions() const
{
	QByteArrayList output;
	if (getBool("complete")) output << "complete";
	if (getBool("exonic/splicing")) output << "exonic/splicing";
	if (getBool("intronic/intergenic")) output << "intronic/intergenic";

	return output;
}

FilterCnvPathogenicCnvOverlap::FilterCnvPathogenicCnvOverlap()
{
	name_ = "CNV pathogenic CNV overlap";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter for overlap with pathogenic CNVs from the NGSD i.e. the 'ngsd_pathogenic_cnvs' column is not empty.";

	checkIsRegistered();
}

QString FilterCnvPathogenicCnvOverlap::toText() const
{
	return name();
}

void FilterCnvPathogenicCnvOverlap::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	int index = cnvs.annotationIndexByName("ngsd_pathogenic_cnvs", true);

	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (cnvs[i].annotations()[index].trimmed().isEmpty())
		{
			result.flags()[i] = false;
		}
	}
}


/*************************************************** concrete filters for SVs ***************************************************/

FilterSvType::FilterSvType()
{
	name_ = "SV type";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on SV types.";
	params_ << FilterParameter("Structural variant type", FilterParameterType::STRINGLIST, QStringList(), "Structural variant type");
	params_.last().constraints["valid"] = "DEL,DUP,INS,INV,BND";
	params_.last().constraints["not_empty"] = "";

	checkIsRegistered();
}

QString FilterSvType::toText() const
{
	return name() + " " + getStringList("Structural variant type", false).join(",");
}

void FilterSvType::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	// get selected types
	QStringList sv_types = getStringList("Structural variant type");

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = sv_types.contains(StructuralVariantTypeToString(svs[i].type()));
	}
}

FilterSvRemoveChromosomeType::FilterSvRemoveChromosomeType()
{
	name_ = "SV remove chr type";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Removes all structural variants which contains non-standard/standard chromosomes.";
	params_ << FilterParameter("chromosome type", FilterParameterType::STRING, "special chromosomes", "Structural variants containing non-standard/standard chromosome are removed.");
	params_.last().constraints["valid"] = "special chromosomes,standard chromosomes";
	params_.last().constraints["not_empty"] = "";
	checkIsRegistered();
}

QString FilterSvRemoveChromosomeType::toText() const
{
	return name() + ": Remove " + getString("chromosome type");
}

void FilterSvRemoveChromosomeType::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	bool remove_special_chr = (getString("chromosome type") == "special chromosomes");


	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;
		if (remove_special_chr)
		{
			// only pass if both positions are located on standard chromosomes
			result.flags()[i] = svs[i].chr1().isNonSpecial() && svs[i].chr2().isNonSpecial();
		}
		else
		{
			// only pass if both positions are located on special chromosomes
			result.flags()[i] = !svs[i].chr1().isNonSpecial() && !svs[i].chr2().isNonSpecial();
		}
	}
}

FilterSvGenotypeControl::FilterSvGenotypeControl()
{
	name_ = "SV genotype control";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter structural variants of control samples based on their genotype.";
	params_ << FilterParameter("genotypes", FilterParameterType::STRINGLIST, QStringList(), "Structural variant genotype(s)");
	params_.last().constraints["valid"] = "wt,het,hom,n/a";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("same_genotype", FilterParameterType::BOOL, false, "Also check that all 'control' samples have the same genotype.");

	checkIsRegistered();
}

QString FilterSvGenotypeControl::toText() const
{
	return name() + ": " + getStringList("genotypes", false).join(",");
}

void FilterSvGenotypeControl::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	QList<int> format_data_indices = svs.sampleHeaderInfo().sampleColumns(false);
	if (format_data_indices.size() < 1) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without control samples!");

	// get genotypes
	QStringList genotypes = getStringList("genotypes");
	bool same_genotype = getBool("same_genotype");

	int format_col_index = svs.annotationIndexByName("FORMAT");

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get genotype for each control sample

		// get format keys and values
		QByteArrayList format_keys = svs[i].annotations()[format_col_index].split(':');
		int genotype_idx = format_keys.indexOf("GT");

		QSet<QString> genotypes_all;
		foreach (int data_idx, format_data_indices)
		{
			QByteArrayList format_values = svs[i].annotations()[data_idx].split(':');

			QByteArray sv_genotype_string = format_values[genotype_idx].trimmed();
			QString sv_genotype;

			// convert genotype into GSvar format
			if (sv_genotype_string == "0/1" || (sv_genotype_string == "1/0")) sv_genotype = "het";
			else if (sv_genotype_string == "1/1") sv_genotype = "hom";
			else if (sv_genotype_string == "0/0") sv_genotype = "wt";
			else sv_genotype = "n/a";

			if (!genotypes.contains(sv_genotype))
			{
				result.flags()[i] = false;
			}

			// store determined genotype in set
			genotypes_all.insert(sv_genotype);
		}

		// check if all SVs have the same genotype
		if (same_genotype && genotypes_all.size() > 1) result.flags()[i] = false;
	}
}

FilterSvGenotypeAffected::FilterSvGenotypeAffected()
{
	name_ = "SV genotype affected";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter structural variants (of affected samples) based on their genotype.";
	params_ << FilterParameter("genotypes", FilterParameterType::STRINGLIST, QStringList(), "Structural variant genotype(s)");
	params_.last().constraints["valid"] = "wt,het,hom,n/a";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("same_genotype", FilterParameterType::BOOL, false, "Also check that all 'control' samples have the same genotype.");

	checkIsRegistered();
}

QString FilterSvGenotypeAffected::toText() const
{
	return name() + ": " + getStringList("genotypes", false).join(",");
}

void FilterSvGenotypeAffected::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	// get genotypes
	QStringList genotypes = getStringList("genotypes");
	bool same_genotype = getBool("same_genotype");

	int format_col_index = svs.annotationIndexByName("FORMAT");

	QList<int> format_data_indices;
	format_data_indices << format_col_index + 1;
	if ((svs.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI) || (svs.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO))
	{
		format_data_indices = svs.sampleHeaderInfo().sampleColumns(true);
		if (format_data_indices.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without affected samples!");
	}


	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get genotype for each control sample

		// get format keys and values
		QByteArrayList format_keys = svs[i].annotations()[format_col_index].split(':');
		int genotype_idx = format_keys.indexOf("GT");

		QSet<QString> genotypes_all;
		foreach (int data_idx, format_data_indices)
		{
			QByteArrayList format_values = svs[i].annotations()[data_idx].split(':');

			QByteArray sv_genotype_string = format_values[genotype_idx].trimmed();
			QString sv_genotype;

			// convert genotype into GSvar format
			if (sv_genotype_string == "0/1" || (sv_genotype_string == "1/0")) sv_genotype = "het";
			else if (sv_genotype_string == "1/1") sv_genotype = "hom";
			else if (sv_genotype_string == "0/0") sv_genotype = "wt";
			else sv_genotype = "n/a";

			if (!genotypes.contains(sv_genotype))
			{
				result.flags()[i] = false;
			}

			// store determined genotype in set
			genotypes_all.insert(sv_genotype);
		}

		// check if all SVs have the same genotype
		if (same_genotype && genotypes_all.size() > 1) result.flags()[i] = false;
	}
}

FilterSvQuality::FilterSvQuality()
{
	name_ = "SV quality";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter structural variants based on their quality.";
	params_ << FilterParameter("quality", FilterParameterType::INT, 0, "Minimum quality score");
	params_.last().constraints["min"] = "0";

	checkIsRegistered();
}

QString FilterSvQuality::toText() const
{
	return name() + " &ge; " + QByteArray::number(getInt("quality", false));
}

void FilterSvQuality::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;
	if (svs.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		// ignore filter if applied to tumor-normal sample
		THROW(ArgumentException, "Filter '" + name() +"' cannot be applied to somatic tumor normal sample!");
		return;
	}

	// get quality threshold
	int min_quality = getInt("quality");

	// get quality column index
	int quality_col_index = svs.annotationIndexByName("QUAL");

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = Helper::toDouble(svs[i].annotations()[quality_col_index]) >= min_quality;
	}
}

FilterSvFilterColumn::FilterSvFilterColumn()
{
	name_ = "SV filter columns";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter structural variants based on the entries of the 'FILTER' column.";

	params_ << FilterParameter("entries", FilterParameterType::STRINGLIST, QStringList(), "Filter column entries");
	//params_.last().constraints["valid"] ="PASS,MinSomaticScore,HomRef,MaxDepth,MaxMQ0Frac,MinGQ,MinQUAL,NoPairSupport,Ploidy,SampleFT,off-target";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("action", FilterParameterType::STRING, "REMOVE", "Action to perform");
	params_.last().constraints["valid"] = "REMOVE,FILTER,KEEP";

	checkIsRegistered();
}

QString FilterSvFilterColumn::toText() const
{
	return name() + " " + getString("action", false) + ": " + getStringList("entries", false).join(",");
}

void FilterSvFilterColumn::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	QSet<QString> filter_entries = getStringList("entries").toSet();
	QString action = getString("action");
	int filter_col_index = svs.annotationIndexByName("FILTER");

	if (action=="REMOVE")
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QSet<QString> sv_entries = QString(svs[i].annotations()[filter_col_index]).split(';').toSet();
			// check if intersection of both list == 0 -> remove entry otherwise
			result.flags()[i] = (sv_entries.intersect(filter_entries).size() == 0);
		}
	}
	else if (action=="FILTER")
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QSet<QString> sv_entries = QString(svs[i].annotations()[filter_col_index]).split(';').toSet();
			// compute intersection
			if (!sv_entries.intersects(filter_entries))
			{
				result.flags()[i] = false;
			}
		}
	}
	else if (action=="KEEP")
	{
		for(int i=0; i<svs.count(); ++i)
		{
			QSet<QString> sv_entries = QString(svs[i].annotations()[filter_col_index]).split(';').toSet();
			// iterate over list of required entries
			foreach (QString filter_entry, filter_entries)
			{
				if (sv_entries.contains(filter_entry))
				{
					// display SV if at least one of the provided filter entries match
					result.flags()[i] = true;
					break;
				}
			}
		}
	}
	else
	{
		THROW(NotImplementedException, "Invalid action '" + action +"'provided!");
	}
}

FilterSvPairedReadAF::FilterSvPairedReadAF()
{
	name_ = "SV paired read AF";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Show only SVs with a certain Paired Read Allele Frequency +/- 10%";
    description_ << "(In trio/multi sample all (affected) samples must meet the requirements.)";
	params_ << FilterParameter("Paired Read AF", FilterParameterType::DOUBLE, 0.0, "Paired Read Allele Frequency +/- 10%");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
	params_ << FilterParameter("only_affected", FilterParameterType::BOOL, false , "Apply filter only to affected Samples.");


	checkIsRegistered();
}

QString FilterSvPairedReadAF::toText() const
{
	return name() + " = " + QByteArray::number(getDouble("Paired Read AF", false), 'f', 2) + " &plusmn; 10%" + ((getBool("only_affected"))?" (only affected)": "");
}

void FilterSvPairedReadAF::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;
	if (svs.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		// ignore filter if applied to tumor-normal sample
		THROW(ArgumentException, "Filter '" + name() +"' cannot be applied to somatic tumor normal sample!");
		return;
	}

	// get allowed interval
	double upper_limit = getDouble("Paired Read AF", false) + 0.1;
	double lower_limit = getDouble("Paired Read AF", false) - 0.1;
    bool only_affected = getBool("only_affected");


	int format_col_index = svs.annotationIndexByName("FORMAT");

	// determine analysis type
	int sample_count = 1;
	bool is_multisample = false;
	if ((svs.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI) || (svs.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO))
	{
		// get sample count for multisample
		sample_count = svs.sampleHeaderInfo().size();
		is_multisample = true;
	}

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get format keys and values
		QByteArrayList format_keys = svs[i].annotations()[format_col_index].split(':');



		for (int sample_idx = 0; sample_idx < sample_count; ++sample_idx)
        {
			// skip on control samples
            if (is_multisample && only_affected && !svs.sampleHeaderInfo().at(sample_idx).isAffected()) continue;

			QByteArrayList format_values = svs[i].annotations()[format_col_index + sample_idx + 1].split(':');

            // compute allele frequency
            QByteArrayList pr_af_entry = format_values[format_keys.indexOf("PR")].split(',');
            if (pr_af_entry.size() != 2) THROW(FileParseException, "Invalid paired read entry (PR) in sv " + QByteArray::number(i) + "!")
            int count_ref = Helper::toInt(pr_af_entry[0]);
            int count_alt = Helper::toInt(pr_af_entry[1]);
            double pr_af = 0;
            if (count_alt + count_ref != 0)
            {
                pr_af =  (double)count_alt / (count_alt+count_ref);
            }

            // compare AF with filter
            if(pr_af > upper_limit || pr_af < lower_limit)
            {
                result.flags()[i] = false;
                break;
            }
        }
	}
}

FilterSvSplitReadAF::FilterSvSplitReadAF()
{
	name_ = "SV split read AF";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Show only SVs with a certain Split Read Allele Frequency +/- 10%";
    description_ << "(In trio/multi sample all (affected) samples must meet the requirements.)";
	params_ << FilterParameter("Split Read AF", FilterParameterType::DOUBLE, 0.0, "Split Read Allele Frequency +/- 10%");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
	params_ << FilterParameter("only_affected", FilterParameterType::BOOL, false , "Apply filter only to affected Samples.");



	checkIsRegistered();
}

QString FilterSvSplitReadAF::toText() const
{
	return name() + " = " + QByteArray::number(getDouble("Split Read AF", false), 'f', 2) + " &plusmn; 10%"  + ((getBool("only_affected"))?" (only affected)" : "");
}

void FilterSvSplitReadAF::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;
	if (svs.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		// ignore filter if applied to tumor-normal sample
		THROW(ArgumentException, "Filter '" + name() +"' cannot be applied to somatic tumor normal sample!");
		return;
	}

	// get allowed interval
	double upper_limit = getDouble("Split Read AF", false) + 0.1;
	double lower_limit = getDouble("Split Read AF", false) - 0.1;
    bool only_affected = getBool("only_affected");

	int format_col_index = svs.annotationIndexByName("FORMAT");

	// determine analysis type
	int sample_count = 1;
	bool is_multisample = false;
	if ((svs.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI) || (svs.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO))
	{
		// get sample count for multisample
		sample_count = svs.sampleHeaderInfo().size();
		is_multisample = true;
	}

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get format keys and values
		QByteArrayList format_keys = svs[i].annotations()[format_col_index].split(':');

        // compute allele frequency
        int sr_idx = format_keys.indexOf("SR");
        if (sr_idx == -1)
        {
            // remove all SVs which does not contain any split read information (e.g. DUP)
            result.flags()[i] = false;
            continue;
        }

		for (int sample_idx = 0; sample_idx < sample_count; ++sample_idx)
        {
			// skip on control samples
            if (is_multisample && only_affected && !svs.sampleHeaderInfo().at(sample_idx).isAffected()) continue;

			QByteArrayList format_values = svs[i].annotations()[format_col_index + sample_idx + 1].split(':');

            QByteArrayList sr_af_entry = format_values[sr_idx].split(',');
            if (sr_af_entry.size() != 2) THROW(FileParseException, "Invalid split read entry (SR) in sv " + QByteArray::number(i) + "!")
            int count_ref = Helper::toInt(sr_af_entry[0]);
            int count_alt = Helper::toInt(sr_af_entry[1]);
            double sr_af = 0;
            if (count_alt + count_ref != 0)
            {
                sr_af =  (double)count_alt / (count_alt+count_ref);
            }

            // compare AF with filter
            if(sr_af > upper_limit || sr_af < lower_limit)
            {
                result.flags()[i] = false;
                break;
            }
        }
	}
}

FilterSvPeReadDepth::FilterSvPeReadDepth()
{
	name_ = "SV PE read depth";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Show only SVs with at least a certain number of Paired End Reads";
    description_ << "(In trio/multi sample all (affected) samples must meet the requirements.)";
	params_ << FilterParameter("PE Read Depth", FilterParameterType::INT, 0, "minimal number of Paired End Reads");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("only_affected", FilterParameterType::BOOL, false , "Apply filter only to affected Samples.");


	checkIsRegistered();
}

QString FilterSvPeReadDepth::toText() const
{
	return name() + " &ge; " + QByteArray::number(getInt("PE Read Depth", false))  + ((getBool("only_affected"))? " (only affected)": "");
}

void FilterSvPeReadDepth::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;
	if (svs.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		// ignore filter if applied to tumor-normal sample
		THROW(ArgumentException, "Filter '" + name() +"' cannot be applied to somatic tumor normal samples!");
		return;
	}

	// get min PE read depth
	int min_read_depth = getInt("PE Read Depth", false);
    bool only_affected = getBool("only_affected");

	int format_col_index = svs.annotationIndexByName("FORMAT");

	// determine analysis type
	int sample_count = 1;
	bool is_multisample = false;
	if ((svs.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI) || (svs.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO))
	{
		// get sample count for multisample
		sample_count = svs.sampleHeaderInfo().size();
		is_multisample = true;
	}

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get format keys and values
		QByteArrayList format_keys = svs[i].annotations()[format_col_index].split(':');

		for (int sample_idx = 0; sample_idx < sample_count; ++sample_idx)
        {
			// skip on control samples
            if (is_multisample && only_affected && !svs.sampleHeaderInfo().at(sample_idx).isAffected()) continue;

			QByteArrayList format_values = svs[i].annotations()[format_col_index + sample_idx + 1].split(':');

            // get total read number
            QByteArrayList pe_read_entry = format_values[format_keys.indexOf("PR")].split(',');
            if (pe_read_entry.size() != 2) THROW(FileParseException, "Invalid paired read entry (PR) in sv " + QByteArray::number(i) + "!")
            int pe_read_depth = Helper::toInt(pe_read_entry[1]);

            // compare AF with filter
            if(pe_read_depth < min_read_depth)
            {
                result.flags()[i] = false;
                break;
            }
        }
	}
}


FilterSvSomaticscore::FilterSvSomaticscore()
{
	name_ = "SV SomaticScore";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Show only SVs with at least a certain Somaticscore";
	params_ << FilterParameter("Somaticscore", FilterParameterType::INT, 0, "min. Somaticscore");
	params_.last().constraints["min"] = "0";

	checkIsRegistered();
}

QString FilterSvSomaticscore::toText() const
{
	return name() + " &ge; " + QByteArray::number(getInt("Somaticscore", false));
}

void FilterSvSomaticscore::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	if (svs.format() != BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		// ignore filter if applied to non-tumor-normal sample
		THROW(ArgumentException, "Filter '" + name() +"' can only be applied to somatic tumor normal samples!");
		return;
	}


	// get min somaticscore
	int min_somaticscore = getInt("Somaticscore", false);

	int i_somaticscore = svs.annotationIndexByName("SOMATICSCORE");

	if (i_somaticscore == -1) THROW(FileParseException, "No SOMATICSCORE column found in BEDPE file!");

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get somaticscore
		double somaticscore = Helper::toInt(svs[i].annotations()[i_somaticscore], "Somaticscore", QString::number(i));
		// compare AF with filter
		result.flags()[i] = (min_somaticscore <= somaticscore);
	}
}

FilterSvGeneConstraint::FilterSvGeneConstraint()
{
	name_ = "SV gene constraint";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on gene constraint (gnomAD o/e score for LOF variants)." << "Note that gene constraint is most helpful for early-onset severe diseases." << "For details on gnomAD o/e, see https://macarthurlab.org/2018/10/17/gnomad-v2-1/";

	params_ << FilterParameter("max_oe_lof", FilterParameterType::DOUBLE, 0.35, "Maximum gnomAD o/e score for LoF variants");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterSvGeneConstraint::toText() const
{
	return name() + " o/e&le;" + QString::number(getDouble("max_oe_lof", false), 'f', 2);
}

void FilterSvGeneConstraint::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	//get column indices
	int i_gene_info = svs.annotationIndexByName("GENE_INFO", true);

	//throw exception if gene info not found
	if (i_gene_info == -1)
	{
		THROW(FileParseException, "No 'GENE_INFO' column found in BEDPE file! Please reannotate structural variant file.")
	}

	double max_oe_lof = getDouble("max_oe_lof");

	//filter
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//parse gene_info entry - example: 34P13.14 (region=complete oe_lof=), ...
		QByteArrayList gene_entries= svs[i].annotations()[i_gene_info].split(',');
		bool any_gene_passed = false;
		foreach(const QByteArray& gene, gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
			foreach(const QByteArray& term, term_entries)
			{
				if (term.startsWith("oe_lof="))
				{
					bool ok;
					double oe = term.mid(7).toDouble(&ok);
					if (!ok) oe = 1.0; // value 'n/a' > pass
					if (oe<=max_oe_lof)
					{
						any_gene_passed = true;
						break;
					}
				}
				if (any_gene_passed) break;
			}
		}
		result.flags()[i] = any_gene_passed;
	}
}

FilterSvGeneOverlap::FilterSvGeneOverlap()
{
	name_ = "SV gene overlap";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on gene overlap.";

	params_ << FilterParameter("complete", FilterParameterType::BOOL, true , "Overlaps the complete gene.");
	params_ << FilterParameter("exonic/splicing", FilterParameterType::BOOL, true , "Overlaps the coding or splicing region of the gene.");
	params_ << FilterParameter("intronic/intergenic", FilterParameterType::BOOL, false , "Overlaps the intronic/intergenic region of the gene only.");

	checkIsRegistered();
}

QString FilterSvGeneOverlap::toText() const
{
	return name() + " " + selectedOptions().join(", ");
}

void FilterSvGeneOverlap::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	int i_gene_info = svs.annotationIndexByName("GENE_INFO", true);

	//throw exception if gene info not found
	if (i_gene_info == -1)
	{
		THROW(FileParseException, "No 'GENE_INFO' column found in BEDPE file! Please reannotate structural variant file.")
	}

	QByteArrayList selected = selectedOptions();

	//filter
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//parse gene_info entry - example: 34P13.14 (region=complete oe_lof=), ...
		QByteArrayList gene_entries = svs[i].annotations()[i_gene_info].split(',');
		bool any_gene_passed = false;
		foreach(const QByteArray& gene, gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
			foreach(const QByteArray& term, term_entries)
			{
				if (term.startsWith("region="))
				{
					if (selected.contains(term.mid(7)))
					{
						any_gene_passed = true;
						break;
					}
				}
				if (any_gene_passed) break;
			}
		}
		result.flags()[i] = any_gene_passed;
	}
}

QByteArrayList FilterSvGeneOverlap::selectedOptions() const
{
	QByteArrayList output;
	if (getBool("complete")) output << "complete";
	if (getBool("exonic/splicing")) output << "exonic/splicing";
	if (getBool("intronic/intergenic")) output << "intronic/intergenic";

	return output;
}

// Filter SV min size
FilterSvSize::FilterSvSize()
{
	name_ = "SV size";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter for SV size in the given range.";
	params_ << FilterParameter("min_size", FilterParameterType::INT, 0, "Minimum SV size (absolute size).");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("max_size", FilterParameterType::INT, 0, "Maximum SV size (absolute size). Select 0 for infinity.");
	params_.last().constraints["min"] = "0";

	checkIsRegistered();
}

QString FilterSvSize::toText() const
{
	int min_size = getInt("min_size", false);
	int max_size = getInt("max_size", false);
	if (max_size != 0)
	{
		return name() + " between " + QString::number(min_size) + " and " + QString::number(max_size) + " bases";
	}
	return name() + " &ge; " + QString::number(min_size) + " bases";
}

void FilterSvSize::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	int min_size = getInt("min_size", false);
	int max_size = getInt("max_size", false);

	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		// get SV length
		int sv_length = svs.estimatedSvSize(i);
		if (sv_length < min_size) result.flags()[i] = false;
		if (max_size != 0 && sv_length > max_size) result.flags()[i] = false;
	}
}

FilterSvOMIM::FilterSvOMIM()
{
	name_ = "SV OMIM genes";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter for OMIM genes i.e. the 'OMIM' column is not empty.";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "REMOVE,FILTER";
	checkIsRegistered();
}

QString FilterSvOMIM::toText() const
{
	return name();
}

void FilterSvOMIM::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	int index = svs.annotationIndexByName("OMIM", true);
	QString action = getString("action");
	if (action=="FILTER")
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (svs[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
	else
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (!svs[i].annotations()[index].trimmed().isEmpty())
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterSvCompHet::FilterSvCompHet()
{
	name_ = "SV compound-heterozygous";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter for compound-heterozygous SVs." << "Mode 'SV-SV' detects genes with two or more SV hits." << "Mode 'SV-SNV/INDEL' detectes genes with exactly one SV and exactly one small variant hit (after other filters are applied).";
	params_ << FilterParameter("mode", FilterParameterType::STRING, "n/a", "Compound-heterozygotes detection mode.");
	params_.last().constraints["valid"] = "n/a,SV-SV,SV-SNV/INDEL";

	checkIsRegistered();
}

QString FilterSvCompHet::toText() const
{
	return name() + " " + getString("mode");
}

void FilterSvCompHet::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	QString mode = getString("mode");
	if (mode=="n/a") return;

	// get column index for genes
	int i_genes = svs.annotationIndexByName("GENES");

	//count hits per gene for SVs
	QMap<QByteArray, int> gene_count;
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		GeneSet genes;
		genes << svs[i].annotations()[i_genes].split(';');

		foreach(const QByteArray& gene, genes)
		{
			gene_count[gene] += 1;
		}
	}

	GeneSet comphet_hit;

	//two SV hits
	if (mode=="SV-SV")
	{
		for(auto it=gene_count.cbegin(); it!=gene_count.cend(); ++it)
		{
			if (it.value()>1)
			{
				comphet_hit.insert(it.key());
			}
		}
	}

	//one SV and one SNV/INDEL hit
	else if (mode=="SV-SNV/INDEL")
	{
		GeneSet single_hit_sv;
		for(auto it=gene_count.cbegin(); it!=gene_count.cend(); ++it)
		{
			if (it.value()==1)
			{
				single_hit_sv.insert(it.key());
			}
		}

		foreach(const QByteArray& gene, single_hit_sv)
		{
			if (het_hit_genes_.contains(gene))
			{
				comphet_hit.insert(gene);
			}
		}
	}

	//flag passing SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		GeneSet genes;
		genes << svs[i].annotations()[i_genes].split(';');
		result.flags()[i] = genes.intersectsWith(comphet_hit);
	}
}

FilterSvCountNGSD::FilterSvCountNGSD()
{
	name_ = "SV count NGSD";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on the occurances of a structural variant in the NGSD.";
	params_ << FilterParameter("max_count", FilterParameterType::INT, 20, "Maximum NGSD SV count");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("overlap_matches", FilterParameterType::BOOL, false, "If set, overlaping SVs are considered also.");

	checkIsRegistered();
}

QString FilterSvCountNGSD::toText() const
{
	return name() + " &le; " + QString::number(getInt("max_count", false)) + (getBool("overlap_matches") ? " (overlap_matches)" : "");
}

void FilterSvCountNGSD::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	int max_count = getInt("max_count");
	bool overlap_match = getBool("overlap_matches");

	int ngsd_col_index;
	if (overlap_match)
	{
		ngsd_col_index = svs.annotationIndexByName("NGSD_COUNT_OVERLAP");

	}
	else
	{
		ngsd_col_index = svs.annotationIndexByName("NGSD_COUNT");
	}

	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		int ngsd_count;

		if (overlap_match)
		{
			ngsd_count = Helper::toInt(svs[i].annotations()[ngsd_col_index], "NGSD count overlap column", QString::number(i));
		}
		else
		{
			ngsd_count = Helper::toInt(svs[i].annotations()[ngsd_col_index].split('(')[0], "NGSD count column", QString::number(i));
		}

		result.flags()[i] = ngsd_count <= max_count;
	}

}

FilterSvAfNGSD::FilterSvAfNGSD()
{
	name_ = "SV allele frequency NGSD";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on the allele frequency of this structural variant in the NGSD."
								 << "Note: this filter should only be used for whole genome samples.";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 1.0, "Maximum allele frequency in %");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "100.0";

	checkIsRegistered();
}

QString FilterSvAfNGSD::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_af", false)) + "%";
}

void FilterSvAfNGSD::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	double max_af = getDouble("max_af")/100.0;

	int ngsd_col_index = svs.annotationIndexByName("NGSD_COUNT");

	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		result.flags()[i] = Helper::toDouble(svs[i].annotations()[ngsd_col_index].split('(')[1].split(')')[0], "NGSD count column", QString::number(i)) <= max_af;
	}
}

FilterSvTrio::FilterSvTrio()
{
    name_ = "SV trio";
	type_ = VariantType::SVS;
    description_ = QStringList() << "Filter trio structural variants";
	params_ << FilterParameter("types", FilterParameterType::STRINGLIST, QStringList() << "de-novo" << "recessive" << "comp-het" << "LOH" << "x-linked", "Variant types");
    params_.last().constraints["valid"] = "de-novo,recessive,comp-het,LOH,x-linked,imprinting";
    params_.last().constraints["non-empty"] = "";

	params_ << FilterParameter("gender_child", FilterParameterType::STRING, "n/a", "Gender of the child - if 'n/a', the gender from the GSvar file header is taken");
    params_.last().constraints["valid"] = "male,female,n/a";

    checkIsRegistered();
}

QString FilterSvTrio::toText() const
{
    return name() + " " + getStringList("types", false).join(',');
}

void FilterSvTrio::apply(const BedpeFile &svs, FilterResult &result) const
{
    if (!enabled_) return;

    if (svs.format() != BedpeFileFormat::BEDPE_GERMLINE_TRIO) THROW(FileParseException, "Trio filter can only be applied to trio SV samples!");

    //determine child gender
    QString gender_child = getString("gender_child");
    if (gender_child=="n/a")
    {
        gender_child = svs.sampleHeaderInfo().infoByStatus(true).gender();
    }
    if (gender_child=="n/a")
    {
        THROW(ArgumentException, "Could not determine gender of child, please set it!");
    }

    //determine column indices
    int i_gene = svs.annotationIndexByName("GENES");
    SampleHeaderInfo sample_headers = svs.sampleHeaderInfo();
	int i_c = sample_headers.infoByStatus(true).column_index;
	int i_f = sample_headers.infoByStatus(false, "male").column_index;
	int i_m = sample_headers.infoByStatus(false, "female").column_index;
	int i_format_col = svs.annotationIndexByName("FORMAT");

    //get PAR region
    BedFile par_region = NGSHelper::pseudoAutosomalRegion("hg19");

    //pre-calculate genes with heterozygous variants
    QSet<QString> types = getStringList("types").toSet();
    GeneSet genes_comphet;
    if (types.contains("comp-het"))
    {
        GeneSet het_father;
        GeneSet het_mother;

        for(int i=0; i<svs.count(); ++i)
        {
            if (!result.flags()[i]) continue;

            const BedpeLine& sv = svs[i];
            BedFile sv_region = sv.affectedRegion();
            bool diplod_chromosome = sv.chr1().isAutosome() || (sv.chr1().isX() && gender_child=="female") || (sv.chr1().isX() && par_region.overlapsWith(sv_region[0].chr(), sv_region[0].start(), sv_region[0].end()));
            // special handling of BNDs
            if (sv.type() == StructuralVariantType::BND)
            {
                diplod_chromosome = diplod_chromosome || sv.chr2().isAutosome() || (sv.chr2().isX() && gender_child=="female") || (sv.chr2().isX() && par_region.overlapsWith(sv_region[1].chr(), sv_region[1].start(), sv_region[1].end()));
            }

			// get genotypes
			QByteArray geno_c = determineGenotype(sv.annotations()[i_format_col], sv.annotations().at(i_c));
			QByteArray geno_f = determineGenotype(sv.annotations()[i_format_col], sv.annotations()[i_f]);
			QByteArray geno_m = determineGenotype(sv.annotations()[i_format_col], sv.annotations()[i_m]);

            if (diplod_chromosome)
            {
                if (geno_c=="het" && geno_f=="het" && geno_m=="wt")
                {
                    het_mother << GeneSet::createFromText(sv.annotations()[i_gene], ',');
                }
                if (geno_c=="het" && geno_f=="wt" && geno_m=="het")
                {
                    het_father << GeneSet::createFromText(sv.annotations()[i_gene], ',');
                }
            }
        }
        genes_comphet = het_mother.intersect(het_father);
    }

    //load imprinting gene list
	QMap<QByteArray, ImprintingInfo> imprinting = NGSHelper::imprintingGenes();

    //apply
    for(int i=0; i<svs.count(); ++i)
    {
        if (!result.flags()[i]) continue;

        const BedpeLine& sv = svs[i];

        //get genotypes
		QByteArray geno_c = determineGenotype(sv.annotations()[i_format_col], sv.annotations()[i_c]);
		QByteArray geno_f = determineGenotype(sv.annotations()[i_format_col], sv.annotations()[i_f]);
		QByteArray geno_m = determineGenotype(sv.annotations()[i_format_col], sv.annotations()[i_m]);

        //remove variants where index is wild-type
        if (geno_c=="wt")
        {
            result.flags()[i] = false;
            continue;
        }

        //remove variants where genotype data is missing
        if (geno_c=="n/a" || geno_f=="n/a" || geno_m=="n/a")
        {
            result.flags()[i] = false;
            continue;
        }

        BedFile sv_region = sv.affectedRegion();
        bool diplod_chromosome = sv.chr1().isAutosome() || (sv.chr1().isX() && gender_child=="female") || (sv.chr1().isX() && par_region.overlapsWith(sv_region[0].chr(), sv_region[0].start(), sv_region[0].end()));
        // special handling of BNDs
        if (sv.type() == StructuralVariantType::BND)
        {
            diplod_chromosome = diplod_chromosome || sv.chr2().isAutosome() || (sv.chr2().isX() && gender_child=="female") || (sv.chr2().isX() && par_region.overlapsWith(sv_region[1].chr(), sv_region[1].start(), sv_region[1].end()));
        }

        //filter
        bool match = false;
        if (types.contains("de-novo"))
        {
            if (geno_f=="wt" && geno_m=="wt")
            {
                match = true;
            }
        }
        if (types.contains("recessive"))
        {
            if (diplod_chromosome)
            {
                if (geno_c=="hom" && geno_f=="het" && geno_m=="het")
                {
                    match = true;
                }
            }
        }
        if (types.contains("LOH"))
        {
            if (diplod_chromosome)
            {
                if ((geno_c=="hom" && geno_f=="het" && geno_m=="wt") || (geno_c=="hom" && geno_f=="wt" && geno_m=="het"))
                {
                    match = true;
                }
            }
        }
        if (types.contains("comp-het"))
        {
            if (diplod_chromosome)
            {
                if ((geno_c=="het" && geno_f=="het" && geno_m=="wt")
                    ||
                    (geno_c=="het" && geno_f=="wt" && geno_m=="het"))
                {
                    if (genes_comphet.intersectsWith(GeneSet::createFromText(sv.annotations()[i_gene], ',')))
                    {
                        match = true;
                    }
                }
            }
        }
        if (types.contains("x-linked") && sv.chr1().isX() && sv.chr2().isX() && gender_child=="male")
        {
            if (geno_c=="hom" && geno_f=="wt" && geno_m=="het")
            {
                match = true;
            }
        }
        if (types.contains("imprinting"))
        {
            if (geno_c=="het" && geno_f=="het" && geno_m=="wt")
            {
                GeneSet genes = GeneSet::createFromText(sv.annotations()[i_gene], ',');
                foreach(const QByteArray& gene, genes)
                {
					if (imprinting.contains(gene) && imprinting[gene].source_allele!="maternal")
                    {
                        match = true;
                    }
                }
            }
            if (geno_c=="het" && geno_f=="wt" && geno_m=="het")
            {
                GeneSet genes = GeneSet::createFromText(sv.annotations()[i_gene], ',');
                foreach(const QByteArray& gene, genes)
                {
					if (imprinting.contains(gene) && imprinting[gene].source_allele!="paternal")
                    {
                        match = true;
                    }
                }
            }
        }

        result.flags()[i] = match;
	}
}

QByteArray FilterSvTrio::determineGenotype(const QByteArray& format_col, const QByteArray& data_col) const
{
	QByteArrayList keys = format_col.split(':');
	QByteArrayList values = data_col.split(':');
	int genotype_idx = keys.indexOf("GT");
	if (genotype_idx < 0) THROW(FileParseException, "No genotype entry found for SV!");
	QByteArray genotype_string = values.at(genotype_idx);
	QByteArray genotype;
	// convert genotype into GSvar format
	if ((genotype_string == "0/1") || (genotype_string == "1/0")) genotype = "het";
	else if (genotype_string == "1/1") genotype = "hom";
	else if (genotype_string == "0/0") genotype = "wt";
	else genotype = "n/a";

	return genotype;
}

FilterSomaticAlleleFrequency::FilterSomaticAlleleFrequency()
{
	name_ = "Somatic allele frequency";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the allele frequency of variants in tumor/normal samples.";
	params_ << FilterParameter("min_af_tum", FilterParameterType::DOUBLE, 5.0, "Minimum allele frequency in tumor sample [%]");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "100.0";
	params_ << FilterParameter("max_af_nor", FilterParameterType::DOUBLE, 1.0, "Maximum allele frequency in normal sample [%]");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "100.0";

	checkIsRegistered();
}

QString FilterSomaticAlleleFrequency::toText() const
{
	QString text = name();

	double min_af_tum = getDouble("min_af_tum", false);
	if (min_af_tum>0.0)
	{
		text += " min_af_tum&ge;" + QString::number(min_af_tum) + "%";
	}

	double max_af_nor = getDouble("max_af_nor", false);
	if (max_af_nor<1.0)
	{
		text += " max_af_nor&le;" + QString::number(max_af_nor) + "%";
	}

	return text;
}

void FilterSomaticAlleleFrequency::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double min_af_tum = getDouble("min_af_tum")/100.0;
	if (min_af_tum>0.0)
	{
		int i_af = annotationColumn(variants, "tumor_af");
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (variants[i].annotations()[i_af].toDouble()<min_af_tum)
			{
				result.flags()[i] = false;
			}
		}
	}

	double max_af_nor = getDouble("max_af_nor")/100.0;
	if (max_af_nor<1.0)
	{
		int i_af = annotationColumn(variants, "normal_af");
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (variants[i].annotations()[i_af].toDouble()>max_af_nor)
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterTumorOnlyHomHet::FilterTumorOnlyHomHet()
{
	name_ = "Tumor zygosity";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the zygosity of tumor-only samples. Filters out germline het/hom calls.";
	params_ << FilterParameter("het_af_range", FilterParameterType::DOUBLE, 0.0, "Consider allele frequencies of 50% ± het_af_range as heterozygous and thus as germline.");
	params_.last().constraints["min"] = "0";
	params_.last().constraints["max"] = "49.9";
	params_ << FilterParameter("hom_af_range", FilterParameterType::DOUBLE, 0.0, "Consider allele frequencies of 100% ± hom_af_range as homozygous and thus as germline.");
	params_.last().constraints["min"] = "0";
	params_.last().constraints["max"] = "99.9";

	checkIsRegistered();
}

QString FilterTumorOnlyHomHet::toText() const
{
	QString text = name();

	double het_af_range = getDouble("het_af_range", false);
	if(het_af_range != 0.0)
	{
		text += ", het=50%&plusmn;" + QString::number(het_af_range) + "%";
	}

	double hom_af_range = getDouble("hom_af_range", false);
	if(hom_af_range != 0.0)
	{
		text += ", hom=100%&plusmn;" + QString::number(hom_af_range) + "%";
	}

	return text;
}

void FilterTumorOnlyHomHet::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double het_af_range = getDouble("het_af_range")/100.0;

	if(het_af_range != 0.0)
	{
		int i_af = annotationColumn(variants, "tumor_af");
		for(int i=0; i<variants.count(); ++i)
		{
			if(!result.flags()[i]) continue;

			if(variants[i].annotations()[i_af].toDouble() < (0.5+het_af_range) && variants[i].annotations()[i_af].toDouble() > (0.5-het_af_range) )
			{
				result.flags()[i] = false;
			}
		}
	}

	double hom_af_range = getDouble("hom_af_range")/100.0;
	if (hom_af_range != 0.0)
	{
		int i_af = annotationColumn(variants, "tumor_af");
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			if (variants[i].annotations()[i_af].toDouble()> (1.-hom_af_range) )
			{
				result.flags()[i] = false;
			}
		}
	}
}

FilterGSvarScoreAndRank::FilterGSvarScoreAndRank()
{
	name_ = "GSvar score/rank";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based GSvar score/rank.";
	params_ << FilterParameter("top", FilterParameterType::INT, 10, "Show top X rankging variants only.");
	params_.last().constraints["min"] = "1";

	checkIsRegistered();
}

QString FilterGSvarScoreAndRank::toText() const
{
	QString text = name();

	int top = getInt("top", false);
	text += " top=" + QString::number(top);

	return text;
}

void FilterGSvarScoreAndRank::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int top = getInt("top", true);
	int i_rank = annotationColumn(variants, "GSvar_rank");
	for(int i=0; i<variants.count(); ++i)
	{
		if(!result.flags()[i]) continue;

		const QByteArray& rank = variants[i].annotations()[i_rank];
		if(rank.isEmpty() || rank.toInt()>top)
		{
			result.flags()[i] = false;
		}
	}
}
