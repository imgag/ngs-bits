#include "FilterCascade.h"
#include "Exceptions.h"
#include "GeneSet.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "Log.h"
#include "GeneSet.h"
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

    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (value.metaType()!=rhs.value.metaType()) return false;
    #else
    if (value.type()!=rhs.value.type()) return false;
    #endif

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
	if (pass.count()!=variants.count()) THROW(ProgrammingException, "Variant and filter result count not equal in FilterResult::removeFlagged!");

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
	if (pass.count()!=variants.count()) THROW(ProgrammingException, "Variant and filter result count not equal in FilterResult::removeFlagged!");

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
	variants.restrictTo(to_index);

	//update flags
	pass = QBitArray(variants.count(), true);
}

void FilterResult::removeFlagged(CnvList& cnvs)
{
	if (pass.count()!=cnvs.count()) THROW(ProgrammingException, "CNV and filter result count not equal in FilterResult::removeFlagged!");

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
	if (pass.count()!=svs.count()) THROW(ProgrammingException, "SV and filter result count not equal in FilterResult::removeFlagged!");

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

void FilterResult::tagNonPassing(VariantList& variants, const QByteArray& tag, const QByteArray& description)
{
	if (pass.count()!=variants.count()) THROW(ProgrammingException, "Variant and filter result count not equal in FilterResult::tagNonPassing!");

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

void FilterResult::tagNonPassing(VcfFile& variants, const QByteArray& tag, const QString& description)
{
	if (pass.count()!=variants.count()) THROW(ProgrammingException, "Variant and filter result count not equal in FilterResult::tagNonPassing!");

	//add tag description (if missing)
	if (!variants.vcfHeader().filterIdDefined(tag))
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

        for (const FilterParameter& p : params_)
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
        setStringList(name, value.split(',', QT_SKIP_EMPTY_PARTS));
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

bool FilterBase::hasParameter(const QString& name, FilterParameterType type) const
{
	for (int i=0; i<params_.count(); ++i)
	{
		if (params_[i].name==name && params_[i].type==type) return true;
	}

	return false;
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
            for (const QString& value : list)
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
    QElapsedTimer timer;
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
    QElapsedTimer timer;
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
    QElapsedTimer timer;
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
	operator=(fromText(lines));
}

void FilterCascade::store(QString filename)
{
	QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
	file->write(toText().join("\n").toUtf8());
	file->close();
}

QStringList FilterCascade::toText()
{
	QStringList lines;
    for (QSharedPointer<FilterBase> filter : filters_)
	{
		QStringList params;
        for (const FilterParameter& param : filter->parameters())
		{
			params << param.name + "=" + param.valueAsString();
		}
		if (!filter->enabled()) params << "disabled";

		lines << filter->name() + "\t" + params.join("\t");
	}

	return lines;
}

FilterCascade FilterCascade::fromText(const QStringList& lines)
{
	FilterCascade output;

    for (QString line : lines)
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

    for (QString line : Helper::loadTextFile(filename, true, QChar::Null, true))
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
    for (const QString& line : filter_file)
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
    for (const QString& param : parameters)
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
    QStringList filtered_names = registry.keys();

    for (const QString& name : registry.keys())
	{
		QSharedPointer<FilterBase> filter = QSharedPointer<FilterBase>(registry[name]());
		if (filter->type()!=subject)
		{
            filtered_names.removeAll(name);
		}
	}

    return filtered_names;
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
		output["CNV maximum log-likelihood"] = &createInstance<FilterCnvMaxLoglikelihood>;
		output["CNV log-likelihood"] = &createInstance<FilterCnvLoglikelihood>;
		output["CNV q-value"] = &createInstance<FilterCnvQvalue>;
		output["CNV compound-heterozygous"] = &createInstance<FilterCnvCompHet>;
		output["CNV OMIM genes"] = &createInstance<FilterCnvOMIM>;
		output["CNV polymorphism region"] = &createInstance<FilterCnvCnpOverlap>;
		output["CNV gene constraint"] = &createInstance<FilterCnvGeneConstraint>;
		output["CNV gene overlap"] = &createInstance<FilterCnvGeneOverlap>;
		output["CNV tumor CN change"] = &createInstance<FilterCnvTumorCopyNumberChange>;
		output["CNV clonality"] = &createInstance<FilterCnvClonality>;
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
		output["SV break point density NGSD"] = &createInstance<FilterSvBreakpointDensityNGSD>;
        output["SV trio"] = &createInstance<FilterSvTrio>;
		output["SV CNV overlap"] = &createInstance<FilterSvCnvOverlap>;
		output["Splice effect"] = &createInstance<FilterSpliceEffect>;
		output["RNA ASE allele frequency"] = &createInstance<FilterVariantRNAAseAlleleFrequency>;
		output["RNA ASE depth"] = &createInstance<FilterVariantRNAAseDepth>;
		output["RNA ASE alternative count"] = &createInstance<FilterVariantRNAAseAlt>;
		output["RNA ASE p-value"] = &createInstance<FilterVariantRNAAsePval>;
		output["RNA aberrant splicing fraction"] = &createInstance<FilterVariantRNAAberrantSplicing>;
		output["RNA gene expression"] = &createInstance<FilterVariantRNAGeneExpression>;
		output["RNA expression fold-change"] = &createInstance<FilterVariantRNAExpressionFC>;
		output["RNA expression z-score"] = &createInstance<FilterVariantRNAExpressionZScore>;
		output["lr short-read overlap"] = &createInstance<FilterVariantLrSrOverlap>;
		//SV lrGS
		output["SV-lr AF"] = &createInstance<FilterSvLrAF>;
		output["SV-lr support reads"] = &createInstance<FilterSvLrSupportReads>;
	}

	return output;
}

/*************************************************** concrete filters for small variants ***************************************************/

FilterAlleleFrequency::FilterAlleleFrequency()
{
	name_ = "Allele frequency";
	description_ = QStringList() << "Filter based on overall allele frequency given by gnomAD and if available 1000g.";
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
	int i_gnomad = annotationColumn(variants, "gnomAD");
	int i_1000g = annotationColumn(variants, "1000g", false);

	//filter
	if (i_1000g == -1)
	{
		for(int i=0; i<variants.count(); ++i)
		{
			result.flags()[i] = result.flags()[i]
				&& variants[i].annotations()[i_gnomad].toDouble()<=max_af;
		}
	}
	else
	{
		for(int i=0; i<variants.count(); ++i)
		{
			result.flags()[i] = result.flags()[i]
				&& variants[i].annotations()[i_1000g].toDouble()<=max_af
				&& variants[i].annotations()[i_gnomad].toDouble()<=max_af;
		}
	}

}

FilterGenes::FilterGenes()
{
	name_ = "Genes";
	description_ = QStringList() << "Filter that preserves a gene set.";
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
        QRegularExpression reg(genes.join('|').replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			GeneSet var_genes = GeneSet::createFromText(variants[i].annotations()[i_gene], ',');
			bool match_found = false;
            for (const QByteArray& var_gene : var_genes)
			{
                if (reg.match(var_gene).hasMatch())
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

		result.flags()[i] = variants[i].filtersPassed();
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
        for (const QByteArray& part : parts)
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
	QByteArrayList impacts = getStringList("impact").join(":,:").prepend(":").append(":").toUtf8().split(',');

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		bool pass_impact = false;
        for (const QByteArray& impact : impacts)
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
	params_ << FilterParameter("ignore_genotype", FilterParameterType::BOOL, false, "If set, all variants in NGSD are counted independent of the genotype. Otherwise, for homozygous variants only homozygous NGSD variants are counted and for heterozygous variants homozygous and heterozygous NGSD variants are counted.");
	params_ << FilterParameter("mosaic_as_het", FilterParameterType::BOOL, false, "If set, mosaic variants are counted as heterozygous. Otherwise, they are not counted.");

	checkIsRegistered();
}

QString FilterVariantCountNGSD::toText() const
{
	return name() + " &le; " + QString::number(getInt("max_count", false)) + (getBool("ignore_genotype") ? " (ignore genotype)" : "") + + (getBool("mosaic_as_het") ? " (mosaic as het)" : "");
}

void FilterVariantCountNGSD::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int max_count = getInt("max_count");

	int i_ihdb_hom = annotationColumn(variants, "NGSD_hom");
	int i_ihdb_het = annotationColumn(variants, "NGSD_het");
	int i_ihdb_mosaic = annotationColumn(variants, "NGSD_mosaic", false);
	bool mosaic_as_het = getBool("mosaic_as_het");

	if (getBool("ignore_genotype"))
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			int count = variants[i].annotations()[i_ihdb_het].toInt() + variants[i].annotations()[i_ihdb_hom].toInt();
			if (mosaic_as_het && i_ihdb_mosaic!=-1) count += variants[i].annotations()[i_ihdb_mosaic].toInt();

			result.flags()[i] = count <= max_count;
		}
	}
	else
	{
		//get affected column indices
		QList<int> geno_indices = variants.getSampleHeader().sampleColumns(true);
		geno_indices.removeAll(-1);
		if (geno_indices.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without affected samples!");

		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			bool var_is_hom = false;
            for (int index : geno_indices)
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

			int count = variants[i].annotations()[i_ihdb_hom].toInt();
			if (!var_is_hom) count += variants[i].annotations()[i_ihdb_het].toInt();
			if (!var_is_hom && mosaic_as_het && i_ihdb_mosaic!=-1) count += variants[i].annotations()[i_ihdb_mosaic].toInt();

			result.flags()[i] = count <= max_count;
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
    for (const QString& entry : getStringList("entries"))
	{
		entries.append(entry.toUtf8());
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
    for (const QByteArray& f :  v.filters())
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
	params_.last().constraints["valid"] = "1,2,3,4,5,M,R";
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
	params_.last().constraints["valid"] = "AR,AD,XLR,XLD,MT,n/a";
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
        for (const QByteArray& gene : genes)
		{
			int start = gene.indexOf('(');
			QByteArrayList entries = gene.mid(start+1, gene.length()-start-2).split(' ');
            for (const QByteArray& entry : entries)
			{
				if (entry.startsWith("inh="))
				{
					QStringList modes = QString(entry.mid(4)).split('+');
                    for (const QString& mode : modes)
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
        for (const QByteArray& gene : genes)
		{
			int start = gene.indexOf('(');
			QByteArrayList entries = gene.mid(start+1, gene.length()-start-2).split(' ');
            for (const QByteArray& entry : entries)
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
	geno_indices.removeAll(-1);
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
            for (int index : geno_indices)
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
	description_ = QStringList() << "Filter for genotype(s) of the 'affected' sample(s)." << "Variants pass if 'affected' samples have the same genotype and the genotype is in the list selected genotype(s)."
								 << "comp-het works on unphased data (short-read) and keeps all het variants where are at least two (remaining) variants per gene."
								 << "comp-het (phased) only works on phased data (long-read) on completely phased genes and keeps all het variants where are at least one het variant on each allele per gene."
								 << "comp-het (unphased) only works on phased data (long-read) on genes with at least one unphased variant or multiple phasing blocks and keeps all het variants where are at least two het variant per gene (inverse of com-het (phased))."
								 << "You can only select one of the three above at a time.";
	params_ << FilterParameter("genotypes", FilterParameterType::STRINGLIST, QStringList(), "Genotype(s)");
	params_.last().constraints["valid"] = "wt,het,hom,n/a,comp-het,comp-het (phased),comp-het (unphased)";
	params_.last().constraints["not_empty"] = "";
	params_ << FilterParameter("same_genotype", FilterParameterType::BOOL, false, "Also check that all 'control' samples have the same genotype.");

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

	//check input
	if (((int)genotypes.contains("comp-het") + (int)genotypes.contains("comp-het (phased)") + (int)genotypes.contains("comp-het (unphased)")) > 1)
	{
		THROW(ArgumentException, "You can select only one of comp-het, comp-het (phased) and comp-het (unphased)!");
	}

	//get affected column indices
	QList<int> geno_indices = variants.getSampleHeader().sampleColumns(true);
	geno_indices.removeAll(-1);
	if (geno_indices.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without affected samples!");

	//get index of phasing entry
	int i_phasing = variants.annotationIndexByName("genotype_phased", true, false);

	if (genotypes.contains("comp-het (phased)") || genotypes.contains("comp-het (unphased)"))
	{
		if (variants.getSampleHeader().sampleColumns(true).size() > 1) THROW(ArgumentException, "Cannot apply phased filter '" + name() + "' to variant list with multiple affected samples!");
		if (i_phasing < 0) THROW(ArgumentException, "Cannot apply phased filter '" + name() + "' to variant list without phasing information!");
	}


	//filter
	if (!(genotypes.contains("comp-het") || genotypes.contains("comp-het (phased)") || genotypes.contains("comp-het (unphased)")))
	{
		bool same_genotype = getBool("same_genotype");
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
                for (int index : geno_indices)
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
	else
	{
		int i_gene = annotationColumn(variants, "gene");

		//(1) filter for all genotypes but comp-het
		//(2) count heterozygous passing variants per gene
		QHash<QByteArray, int> gene_to_het;
		QHash<QByteArray, int> gene_to_het_phase1;
		QHash<QByteArray, int> gene_to_het_phase2;
		QHash<QByteArray, int> gene_to_het_unphased;
		QHash<QByteArray, QSet<int>> gene_to_phasing_block;
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
				GeneSet genes = GeneSet::createFromText(variants[i].annotations()[i_gene], ',');
                for (const QByteArray& gene : genes)
				{
					//init hash:
					if (!gene_to_het_phase1.contains(gene.trimmed())) gene_to_het_phase1[gene.trimmed()] = 0;
					if (!gene_to_het_phase2.contains(gene.trimmed())) gene_to_het_phase2[gene.trimmed()] = 0;
					if (!gene_to_het_unphased.contains(gene.trimmed())) gene_to_het_unphased[gene.trimmed()] = 0;
					if (!gene_to_phasing_block.contains(gene.trimmed())) gene_to_phasing_block[gene.trimmed()] = QSet<int>();

					gene_to_het[gene.trimmed()] += 1; //old method
					if (!genotypes.contains("comp-het"))
					{
						//get phasing info:
						QStringList phasing_entry = QString(variants[i].annotations()[i_phasing]).split(' ');
						if (phasing_entry.size() < 2)
						{
							 gene_to_het_unphased[gene.trimmed()] += 1;
							 gene_to_phasing_block[gene.trimmed()] << -1;
						}
						else
						{
							QByteArray phased_genotype = phasing_entry.at(0).toUtf8();
							QString pb_raw = phasing_entry.at(1);
                            int phasing_block = Helper::toInt(pb_raw.remove(QRegularExpression("[()]")), "Phasing block", QString::number(i));

							if (phased_genotype == "1|0") gene_to_het_phase1[gene.trimmed()] += 1;
							else gene_to_het_phase2[gene.trimmed()] += 1;
							gene_to_phasing_block[gene.trimmed()] << phasing_block;
						}
					}
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
				GeneSet genes = GeneSet::createFromText(variants[i].annotations()[i_gene], ',');
                for (const QByteArray& gene : genes)
				{
					if (genotypes.contains("comp-het"))
					{
						//old method
						if (gene_to_het[gene.trimmed()]>=2)
						{
							pass = true;
							break;
						}

					}
					else if (genotypes.contains("comp-het (phased)"))
					{
						//look only on variants of completely phased genes
						if ((gene_to_het_phase1[gene.trimmed()] >= 1) && (gene_to_het_phase2[gene.trimmed()] >= 1)
							&& (gene_to_phasing_block[gene.trimmed()].size() < 2) && (gene_to_het_unphased[gene.trimmed()] == 0))
						{
							pass = true;
							break;
						}


					}
					else if (genotypes.contains("comp-het (unphased)"))
					{
						//look only on (partly) unphased genes
						if ((gene_to_phasing_block[gene.trimmed()].size() > 1) || (gene_to_het_unphased[gene.trimmed()] > 0))
						{
							//old method
							if (gene_to_het[gene.trimmed()]>=2)
							{
								pass = true;
								break;
							}
						}

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
	description_ = QStringList() << "Filter for variants predicted to be pathogenic." << "Pathogenicity predictions used by this filter are: phyloP, CADD, REVEL and AlphaMissense.";
	params_ << FilterParameter("min", FilterParameterType::INT, 1, "Minimum number of pathogenic predictions");
	params_.last().constraints["min"] = "1";
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,FILTER";
	params_ << FilterParameter("skip_high_impact", FilterParameterType::BOOL, false, "Do not apply this filter to variants with impact 'HIGH'.");
	//cutoffs
	params_ << FilterParameter("cutoff_phylop", FilterParameterType::DOUBLE, 1.6, "Minimum phyloP score for a pathogenic prediction. The phyloP score is not used if set to -10.0.");
	params_ << FilterParameter("cutoff_cadd", FilterParameterType::DOUBLE, 20.0, "Minimum CADD score for a pathogenic prediction. The CADD score is not used if set to 0.0.");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("cutoff_revel", FilterParameterType::DOUBLE, 0.9, "Minimum REVEL score for a pathogenic prediction. The REVEL score is not used if set to 0.0.");
	params_.last().constraints["min"] = "0";
	params_.last().constraints["max"] = "1";
	params_ << FilterParameter("cutoff_alphamissense", FilterParameterType::DOUBLE, 0.564, "Minimum AlphaMissense score for a pathogenic prediction. The AlphaMissense score is not used if set to 0.0.");
	params_.last().constraints["min"] = "0";
	params_.last().constraints["max"] = "1";

	checkIsRegistered();
}

QString FilterPredictionPathogenic::toText() const
{
	return name() + " " + getString("action", false) + " min&ge; " + QString::number(getInt("min", false)) + (skip_high_impact ? " skip_high_impact" : "");
}

void FilterPredictionPathogenic::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	min = getInt("min");
	i_phylop = annotationColumn(variants, "phyloP");
	i_cadd = annotationColumn(variants, "CADD");
	i_revel = annotationColumn(variants, "REVEL");
	i_alphamissense =  annotationColumn(variants, "AlphaMissense", false); //optional to support old GSvar files without AlphaMissense

	skip_high_impact = getBool("skip_high_impact");
	i_co_sp = annotationColumn(variants, "coding_and_splicing");

	cutoff_cadd = getDouble("cutoff_cadd");
	cutoff_revel = getDouble("cutoff_revel");
	cutoff_phylop = getDouble("cutoff_phylop");
	cutoff_alphamissense = getDouble("cutoff_alphamissense");

	if (getString("action")=="FILTER")
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!result.flags()[i]) continue;
			if (skip_high_impact && variants[i].annotations()[i_co_sp].contains(":HIGH:")) continue;

			result.flags()[i] = predictedPathogenic(variants[i]);
		}
	}
	else //KEEP
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (result.flags()[i]) continue;
			if (skip_high_impact && variants[i].annotations()[i_co_sp].contains(":HIGH:")) continue;

			result.flags()[i] = predictedPathogenic(variants[i]);
		}
	}
}

bool FilterPredictionPathogenic::predictedPathogenic(const Variant& v) const
{
	int count = 0;

	if (cutoff_phylop>-10)
	{
		bool ok;
		double value = v.annotations()[i_phylop].toDouble(&ok);
		if (ok && value>=cutoff_phylop)
		{
			++count;
		}
	}


	if (cutoff_cadd>0)
	{
		bool ok;
		double value = v.annotations()[i_cadd].toDouble(&ok);
		if (ok && value>=cutoff_cadd)
		{
			++count;
		}
	}


	if (cutoff_revel>0)
	{
		bool ok;
		double value = v.annotations()[i_revel].toDouble(&ok);
		if (ok && value>=cutoff_revel)
		{
			++count;
		}
	}

	if (i_alphamissense>=0 && cutoff_alphamissense>0) //optional to support old GSvar files without AlphaMissense
	{
		bool ok;
		double value = v.annotations()[i_alphamissense].toDouble(&ok);
		if (ok && value>=cutoff_alphamissense)
		{
			++count;
		}
	}

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

	term = getString("term").toUtf8().trimmed().toLower();

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
    for (const QByteArray& anno : v.annotations())
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
    for (QString type : getStringList("HIGH"))
	{
		types.append(type.trimmed().toUtf8());
	}
    for (QString type : getStringList("MODERATE"))
	{
		types.append(type.trimmed().toUtf8());
	}
    for (QString type : getStringList("LOW"))
	{
		types.append(type.trimmed().toUtf8());
	}
    for (QString type : getStringList("MODIFIER"))
	{
		types.append(type.trimmed().toUtf8());
	}

	int index = annotationColumn(variants, "coding_and_splicing");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		bool match_found = false;
        for (const QByteArray& type : types)
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
	params_ << FilterParameter("min_occurences", FilterParameterType::INT, 1, "Minimum occurences of the variant per strand");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("min_af", FilterParameterType::DOUBLE, 0.0, "Minimum allele frequency of the variant in the sample");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 1.0, "Maximum allele frequency of the variant in the sample");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterVariantQC::toText() const
{
	return name() + " qual&ge;" + QString::number(getInt("qual", false)) + " depth&ge;" + QString::number(getInt("depth", false)) + " mapq&ge;" + QString::number(getInt("mapq", false)) + " strand_bias&le;" + QString::number(getInt("strand_bias", false)) + " allele_balance&le;" + QString::number(getInt("allele_balance", false)) + " min_occurences&ge;" + QString::number(getInt("min_occurences", false)) + " min_af&ge;" + QString::number(getDouble("min_af", false)) + " max_af&le;" + QString::number(getDouble("max_af", false));
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
	double min_af = getDouble("min_af");
	double max_af = getDouble("max_af");
	int min_occ = getInt("min_occurences");


	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;
		QByteArrayList parts = variants[i].annotations()[index].split(';');
        for (const QByteArray& part : parts)
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
                    for (const QByteArray& dp : dps)
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
			else if (min_occ > 0 && (part.startsWith("SAR=") || part.startsWith("SAF=")))
			{
				if (part.mid(4).toInt() < min_occ)
				{
					result.flags()[i] = false;
				}
			}
			else if ((min_af > 0 || max_af < 1) && part.startsWith("AF="))
			{
				QByteArrayList afs = part.mid(3).split(',');
				bool af_in_interval = false;
                for (const QByteArray& entry : afs)
				{
					double af = entry.toDouble();
					if (af >= min_af && max_af >= af) af_in_interval = true;

				}
				if (!af_in_interval) result.flags()[i] = false;
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

	params_ << FilterParameter("build", FilterParameterType::STRING, "hg38", "Genome build used for pseudoautosomal region coordinates");
	params_.last().constraints["valid"] = "hg19,hg38";

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
	BedFile par_region = NGSHelper::pseudoAutosomalRegion(stringToBuild(getString("build")));

	//pre-calculate genes with heterozygous variants
    QSet<QString> types = LIST_TO_SET(getStringList("types"));
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
                for (const QByteArray& gene : genes)
				{
					if (imprinting.contains(gene) && imprinting[gene].expressed_allele!="maternal")
					{
						match = true;
					}
				}
			}
			if (geno_c=="het" && geno_f=="wt" && geno_m=="het")
			{
				GeneSet genes = GeneSet::createFromText(v.annotations()[i_gene], ',');
                for (const QByteArray& gene : genes)
				{
					if (imprinting.contains(gene) && imprinting[gene].expressed_allele!="paternal")
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
    for (const QByteArray& part : q_parts)
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

		int number_of_regions = cnvs[i].regions();
		if (number_of_regions<1) THROW(FileParseException, "Invalid/unset number of regions!");

		if (number_of_regions< min_regions)
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
	params_ << FilterParameter("cn", FilterParameterType::STRINGLIST, QStringList(), "Copy number");
	params_.last().constraints["valid"] = "0,1,2,3,4,5+";
	params_.last().constraints["not_empty"] = "";

	checkIsRegistered();
}

QString FilterCnvCopyNumber::toText() const
{
	return name() + " CN=" + getStringList("cn").join(",");
}

void FilterCnvCopyNumber::apply(const CnvList& cnvs, FilterResult& result) const
{
	if (!enabled_) return;

	bool cn_5plus = false;
	QSet<QByteArray> cn_exp;
    for (QString cn : getStringList("cn"))
	{
		cn_exp << cn.toUtf8();
		if (cn=="5+") cn_5plus = true;
	}

	int i_cn = cnvs.annotationIndexByName("CN_change", true);
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		const QByteArray& cn = cnvs[i].annotations()[i_cn];

		result.flags()[i] = cn_exp.contains(cn) || (cn_5plus && cn.toInt()>=5);
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

FilterCnvMaxLoglikelihood::FilterCnvMaxLoglikelihood()
{
	name_ = "CNV maximum log-likelihood";
	type_ = VariantType::CNVS;
	description_ << QStringList() << "Filter for maximum log-likelihood" << "Can be used to display artefact CNVs only" << "Works only for tumor-normal pairs" ;
	params_ << FilterParameter("max_ll", FilterParameterType::DOUBLE, 200.0, "Maixmum log-likelihood");
	params_.last().constraints["min"] = "0.0";
	params_ << FilterParameter("scale_by_regions", FilterParameterType::BOOL, false, "Scale log-likelihood by number of regions.");
	checkIsRegistered();
}

QString FilterCnvMaxLoglikelihood::toText() const
{
	return name() + " max_ll=" + QString::number(getDouble("max_ll"), 'f', 2) + QString(getBool("scale_by_regions")?" (scaled by regions)": "");
}

void FilterCnvMaxLoglikelihood::apply(const CnvList &cnvs, FilterResult &result) const
{
	if(!enabled_) return;
	if(cnvs.type() != CnvListType::CLINCNV_TUMOR_NORMAL_PAIR) return;

	double max_ll = getDouble("max_ll");
	bool scale_by_regions = getBool("scale_by_regions");
	int i_ll = cnvs.annotationIndexByName("loglikelihood", true);
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (scale_by_regions)
		{
			int number_of_regions = cnvs[i].regions();
			if (number_of_regions<1) THROW(FileParseException, "Invalid/unset number of regions!");
			double scaled_ll = cnvs[i].annotations()[i_ll].toDouble() / number_of_regions;
			if (scaled_ll > max_ll)
			{
				result.flags()[i] = false;
			}
		}
		else
		{
			if (cnvs[i].annotations()[i_ll].toDouble() > max_ll)
			{
				result.flags()[i] = false;
			}
		}
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
				if (number_of_regions<1) THROW(FileParseException, "Invalid/unset number of regions!");
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
            for (const QByteArray& ll : lls)
			{
				if (scale_by_regions)
				{
					int number_of_regions = cnvs[i].regions();
					if (number_of_regions<1) THROW(FileParseException, "Invalid/unset number of regions!");
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
            for (const QByteArray& q : qs)
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

        for (const QByteArray& gene : cnvs[i].genes())
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

        for (const QByteArray& gene : single_hit_cnv)
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
        for (const QByteArray& gene : gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
            for (const QByteArray& term : term_entries)
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

FilterCnvTumorCopyNumberChange::FilterCnvTumorCopyNumberChange()
{
	name_ = "CNV tumor CN change";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter based on CNV tumor copy number.";
	params_ << FilterParameter("min_tumor_cn", FilterParameterType::INT, 0, "Minimum tumor copy number of the CNV");
	params_ << FilterParameter("max_tumor_cn", FilterParameterType::INT, 10, "Maximum tumor copy number of the CNV.");

	checkIsRegistered();
}

QString FilterCnvTumorCopyNumberChange::toText() const
{
	return name() + " min_tumor_cn=" + QString::number(getInt("min_tumor_cn")) + ", max_tumor_cn=" + QString::number(getInt("max_tumor_cn"));

}

void FilterCnvTumorCopyNumberChange::apply(const CnvList& cnvs, FilterResult &result) const
{
	if(!enabled_) return;

	int i_tumor_cn = cnvs.annotationIndexByName("tumor_CN_change", true);
	int min_cn = getInt("min_tumor_cn");
	int max_cn = getInt("max_tumor_cn");
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;
		bool ok = false;
		int tumor_cn = cnvs[i].annotations()[i_tumor_cn].trimmed().toDouble(&ok);
		if(!ok) continue;
		result.flags()[i] = (tumor_cn >= min_cn) && (tumor_cn <= max_cn);
	}
}

FilterCnvClonality::FilterCnvClonality()
{
	name_ = "CNV clonality";
	type_ = VariantType::CNVS;
	description_ = QStringList() << "Filter based on CNV clonality.";
	params_ << FilterParameter("min_clonality", FilterParameterType::DOUBLE, 0., "Minimum Clonality of the CNV ");
	params_ << FilterParameter("max_clonality", FilterParameterType::DOUBLE, 1., "Maximum Clonality of the CNV ");

	checkIsRegistered();
}

QString FilterCnvClonality::toText() const
{
	return name() + " min_clonality=" + QString::number(getDouble("min_clonality")) + ", max_clonality=" + QString::number(getDouble("max_clonality"));
}

void FilterCnvClonality::apply(const CnvList &cnvs, FilterResult &result) const
{
	if(!enabled_) return;
	int i_clonality = cnvs.annotationIndexByName("tumor_clonality", true);
	double min_clonality= getDouble("min_clonality");
	double max_clonality = getDouble("max_clonality");

	//filter
	for(int i=0; i<cnvs.count(); ++i)
	{
		if (!result.flags()[i]) continue;
		bool ok = false;
		double tumor_clonality = cnvs[i].annotations()[i_clonality].trimmed().toDouble(&ok);
		if(!ok) continue;

		result.flags()[i] = (tumor_clonality > min_clonality) && (tumor_clonality < max_clonality);
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
        for (const QByteArray& gene : gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
            for (const QByteArray& term : term_entries)
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
			//passes if one breakpoint is located on special chromosomes
			result.flags()[i] = !svs[i].chr1().isNonSpecial() || !svs[i].chr2().isNonSpecial();
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
	format_data_indices.removeAll(-1);
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

		if(genotype_idx == -1)
		{
			THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list because could not find GT field in format column.");
		}

		QSet<QString> genotypes_all;
        for (int data_idx : format_data_indices)
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
		format_data_indices.removeAll(-1);
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

		if(genotype_idx == -1)
		{
			THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list because could not find GT field in format column.");
		}

		QSet<QString> genotypes_all;
        for (int data_idx : format_data_indices)
		{
			QByteArrayList format_values = svs[i].annotations()[data_idx].split(':');

			QByteArray sv_genotype_string = format_values[genotype_idx].trimmed();
			QString sv_genotype;

			// convert genotype into GSvar format
			if (sv_genotype_string == "0/1" || sv_genotype_string == "1/0" || sv_genotype_string == "0|1" || sv_genotype_string == "1|0") sv_genotype = "het";
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

    QSet<QString> filter_entries = LIST_TO_SET(getStringList("entries"));
	QString action = getString("action");
	int filter_col_index = svs.annotationIndexByName("FILTER");

	if (action=="REMOVE")
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

            QSet<QString> sv_entries = LIST_TO_SET(QString(svs[i].annotations()[filter_col_index]).split(';'));
			if (sv_entries.intersects(filter_entries))
			{
				result.flags()[i] = false;
			}
		}
	}
	else if (action=="FILTER")
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

            QSet<QString> sv_entries = LIST_TO_SET(QString(svs[i].annotations()[filter_col_index]).split(';'));
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
            QSet<QString> sv_entries = LIST_TO_SET(QString(svs[i].annotations()[filter_col_index]).split(';'));
			if (sv_entries.intersects(filter_entries))
			{
				result.flags()[i] = true;
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
	if (format_col_index==-1) THROW(ProgrammingException, "Missing column FORMAT");

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
			int pr_index = format_keys.indexOf("PR");
			if (pr_index==-1) THROW(FileParseException, "Missing paired read entry (PR) in SV " + svs[i].toString(true) + "!");
			QByteArrayList pe_read_entry = format_values[pr_index].split(',');
			if (pe_read_entry.size() != 2) THROW(FileParseException, "Invalid paired read entry (PR) in SV " + svs[i].toString(true) + "!")
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
        for (const QByteArray& gene : gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
            for (const QByteArray& term : term_entries)
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
	params_ << FilterParameter("intronic/near gene", FilterParameterType::BOOL, false , "Overlaps the intronic region or less than 5kb up/down stream of the gene .");

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
        for (const QByteArray& gene : gene_entries)
		{
			int start = gene.indexOf('(');
			QByteArrayList term_entries = gene.mid(start+1, gene.length()-start-2).split(' ');
            for (const QByteArray& term : term_entries)
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
	if (getBool("intronic/near gene")) output << "intronic/intergenic";

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

		GeneSet genes = GeneSet::createFromText(svs[i].annotations()[i_genes], ';');
        for (const QByteArray& gene : genes)
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

        for (const QByteArray& gene : single_hit_sv)
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

		GeneSet genes = GeneSet::createFromText(svs[i].annotations()[i_genes], ';');
		result.flags()[i] = genes.intersectsWith(comphet_hit);
	}
}

FilterSvCountNGSD::FilterSvCountNGSD()
{
	name_ = "SV count NGSD";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on the hom/het occurances of a structural variant in the NGSD.";
	params_ << FilterParameter("max_count", FilterParameterType::INT, 20, "Maximum NGSD SV count");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("ignore_genotype", FilterParameterType::BOOL, false, "If set, all NGSD entries are counted independent of the variant genotype. Otherwise, for homozygous variants only homozygous NGSD entries are counted and for heterozygous variants all NGSD entries are counted.");

	checkIsRegistered();
}

QString FilterSvCountNGSD::toText() const
{
	return name() + " &le; " + QString::number(getInt("max_count", false)) + (getBool("ignore_genotype") ? " (ignore genotype)" : "");
}

void FilterSvCountNGSD::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	int max_count = getInt("max_count");
	bool ignore_genotype = getBool("ignore_genotype");

	//fallback for annotations before 24.03.22
	int idx_old = svs.annotationIndexByName("NGSD_COUNT", false);
	if (idx_old!=-1 && svs.annotationIndexByName("NGSD_HOM",false)==-1)
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QString text = svs[i].annotations()[idx_old];
			if (text.contains('(')) text = text.split('(')[0];
			int count = Helper::toInt(text, "NGSD count", QString::number(i));
			result.flags()[i] = count <= max_count;
		}

		return;
	}

	int idx_ngsd_hom = svs.annotationIndexByName("NGSD_HOM");
	int idx_ngsd_het = svs.annotationIndexByName("NGSD_HET");

	if (ignore_genotype)
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			int ngsd_count_hom = Helper::toInt(svs[i].annotations()[idx_ngsd_hom], "NGSD count hom", QString::number(i));
			int ngsd_count_het = Helper::toInt(svs[i].annotations()[idx_ngsd_het], "NGSD count het", QString::number(i));
			result.flags()[i] = (ngsd_count_hom + ngsd_count_het) <= max_count;
		}
	}
	else
	{
		//get genotype indices
		int idx_format = svs.annotationIndexByName("FORMAT");
		if (idx_format < 0) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to structural variant list without 'FORMAT' column!");
		QList<int> indices_format_data;
		indices_format_data << idx_format + 1; //single sample
		if ((svs.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI) || (svs.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO))
		{
			indices_format_data = svs.sampleHeaderInfo().sampleColumns(true);
			indices_format_data.removeAll(-1);
			if (indices_format_data.isEmpty()) THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list without affected samples!");
		}

		// iterate over all SVs
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			// get format keys and values
			QByteArrayList format_keys = svs[i].annotations()[idx_format].split(':');
			int idx_genotype = format_keys.indexOf("GT");

			if(idx_genotype == -1)
			{
				THROW(ArgumentException, "Cannot apply filter '" + name() + "' to variant list because could not find GT field in format column.");
			}


			//get NGSD counts
			int ngsd_count_hom = Helper::toInt(svs[i].annotations()[idx_ngsd_hom], "NGSD count hom", QString::number(i));
			int ngsd_count_het = Helper::toInt(svs[i].annotations()[idx_ngsd_het], "NGSD count het", QString::number(i));

            for (const int& idx_format_data : indices_format_data)
			{
				QByteArrayList format_values = svs[i].annotations()[idx_format_data].split(':');

				QByteArray sv_genotype_string = format_values[idx_genotype].trimmed();
				result.flags()[i] = false;

				if (sv_genotype_string == "1/1")
				{
					if(ngsd_count_hom <= max_count)
					{
						result.flags()[i] = true;
						break;
					}
				}
				else
				{
					if(ngsd_count_het <= max_count)
					{
						result.flags()[i] = true;
						break;
					}
				}

			}
		}
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
	params_.last().constraints["max"] = "200.0";

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

	//fallback for annotations before 24.03.22
	int idx_old = svs.annotationIndexByName("NGSD_COUNT", false);
	if (idx_old!=-1 && svs.annotationIndexByName("NGSD_AF",false)==-1)
	{
		for(int i=0; i<svs.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			QString text = svs[i].annotations()[idx_old];
			if (text.contains('(')) text = text.split('(')[0];
			if (text.contains(')')) text = text.split(')')[0];
			double af = Helper::toDouble(text, "NGSD AF", QString::number(i));
			result.flags()[i] = af <= max_af;
		}

		return;
	}

	int idx_ngsd_af = svs.annotationIndexByName("NGSD_AF");

	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;
		//allow empty NGSD af entry
		if (svs[i].annotations()[idx_ngsd_af].trimmed().isEmpty())
		{
			result.flags()[i] = true;
		}
		else
		{
			result.flags()[i] = Helper::toDouble(svs[i].annotations()[idx_ngsd_af], "NGSD AF") <= max_af;
		}

	}
}

FilterSvBreakpointDensityNGSD::FilterSvBreakpointDensityNGSD()
{
	name_ = "SV break point density NGSD";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter based on the density of SV break points in the NGSD in the CI of the structural variant.";
	params_ << FilterParameter("max_density", FilterParameterType::INT, 20, "Maximum density in the confidence interval of the SV");
	params_.last().constraints["min"] = "0";
	params_ << FilterParameter("remove_strict", FilterParameterType::BOOL, false, "Remove also SVs in which only one break point is above threshold.");
	params_ << FilterParameter("only_system_specific", FilterParameterType::BOOL, false, "Filter only based on the density of breakpoint of the current processing system.");

	checkIsRegistered();
}

QString FilterSvBreakpointDensityNGSD::toText() const
{
	return name() + " &le; " + QString::number(getInt("max_density", false)) + QByteArray((getBool("remove_strict"))?" (remove_strict)":"") + QByteArray((getBool("only_system_specific"))?" (only_system_specific)":"");
}

void FilterSvBreakpointDensityNGSD::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	int max_density = getInt("max_density");
	bool remove_strict = getBool("remove_strict");
	bool only_system_specific = getBool("only_system_specific");

	int idx_ngsd_density = (only_system_specific)? svs.annotationIndexByName("NGSD_SV_BREAKPOINT_DENSITY_SYS") : svs.annotationIndexByName("NGSD_SV_BREAKPOINT_DENSITY");

	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QByteArray density = svs[i].annotations()[idx_ngsd_density];

		if (density.trimmed().isEmpty()) continue; //skip empty entries
		QByteArrayList densities = density.split('/');
		if (densities.size() == 1)
		{
			//only one break point (INS)
			result.flags()[i] = Helper::toInt(density, "NGSD_SV_BREAKPOINT_DENSITY(_SYS)") <= max_density;
		}
		else
		{
			//2 break points
			if (remove_strict)
			{
				result.flags()[i] = (Helper::toInt(densities.at(0), "NGSD_SV_BREAKPOINT_DENSITY(_SYS) (BP1)") <= max_density)
						&& (Helper::toInt(densities.at(1), "NGSD_SV_BREAKPOINT_DENSITY(_SYS) (BP2)") <= max_density);
			}
			else
			{
				result.flags()[i] = (Helper::toInt(densities.at(0), "NGSD_SV_BREAKPOINT_DENSITY(_SYS) (BP1)") <= max_density)
						|| (Helper::toInt(densities.at(1), "NGSD_SV_BREAKPOINT_DENSITY(_SYS) (BP2)") <= max_density);
			}
		}

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

	params_ << FilterParameter("build", FilterParameterType::STRING, "hg19", "Genome build used for pseudoautosomal region coordinates");
	params_.last().constraints["valid"] = "hg19,hg38";

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
	BedFile par_region = NGSHelper::pseudoAutosomalRegion(stringToBuild(getString("build")));

    //pre-calculate genes with heterozygous variants
    QSet<QString> types = LIST_TO_SET(getStringList("types"));
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
                for (const QByteArray& gene : genes)
                {
					if (imprinting.contains(gene) && imprinting[gene].expressed_allele!="maternal")
                    {
                        match = true;
                    }
                }
            }
            if (geno_c=="het" && geno_f=="wt" && geno_m=="het")
            {
                GeneSet genes = GeneSet::createFromText(sv.annotations()[i_gene], ',');
                for (const QByteArray& gene : genes)
                {
					if (imprinting.contains(gene) && imprinting[gene].expressed_allele!="paternal")
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

FilterSpliceEffect::FilterSpliceEffect()
{
	name_="Splice effect";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the predicted change in splice effect";
	params_ << FilterParameter("SpliceAi", FilterParameterType::DOUBLE, 0.5, "Minimum SpliceAi score. Disabled if set to zero.");
	params_.last().constraints["min"] = "0";
	params_.last().constraints["max"] = "1";
	params_ << FilterParameter("MaxEntScan", FilterParameterType::STRING, "HIGH", "Minimum predicted splice effect. Disabled if set to LOW.");
	params_.last().constraints["valid"] = "HIGH,MODERATE,LOW";
	params_ << FilterParameter("splice_site_only", FilterParameterType::BOOL, true, "Use native splice site predictions only and skip de-novo acceptor/donor predictions.");
	params_ << FilterParameter("action", FilterParameterType::STRING, "FILTER", "Action to perform");
	params_.last().constraints["valid"] = "KEEP,FILTER";
	checkIsRegistered();
}

QString FilterSpliceEffect::toText() const
{
	QString text = this->name() + " " + getString("action");

	double min_sai = getDouble("SpliceAi", false);
	if (min_sai>0)
	{
		text += " SpliceAi>=" + QString::number(min_sai, 'f', 2);
	}

	QString min_mes = getString("MaxEntScan", false);
	if (min_mes!="LOW")
	{
		text += " maxEntScan>=" + min_mes;
	}

	if (getBool("splice_site_only"))
	{
		text += " (splice_site_only)";
	}

	return text;
}

void FilterSpliceEffect::apply(const VariantList &variant_list, FilterResult &result) const
{
	if (!enabled_) return;

	int idx_sai = annotationColumn(variant_list, "SpliceAi");
	double min_sai = getDouble("SpliceAi");

	int idx_mes = annotationColumn(variant_list, "MaxEntScan");
	MaxEntScanImpact min_mes = MaxEntScanImpact::LOW;
	QByteArray min_mes_str = getString("MaxEntScan").toUtf8();
	if (min_mes_str=="MODERATE") min_mes = MaxEntScanImpact::MODERATE;
	if (min_mes_str=="HIGH") min_mes = MaxEntScanImpact::HIGH;

	bool splice_site_only = getBool("splice_site_only");

	// if all filters are deactivated return
	if (min_sai==0 && min_mes==MaxEntScanImpact::LOW) return;

	// action FILTER
	if (getString("action") == "FILTER")
	{
		for(int i=0; i<variant_list.count(); ++i)
		{
			if (!result.flags()[i]) continue;

			//If the variant has no value for all possible filters remove it
			QByteArray sai_anno = variant_list[i].annotations()[idx_sai].trimmed();
			QByteArray mes_anno = variant_list[i].annotations()[idx_mes].trimmed();
			if (sai_anno.isEmpty() && mes_anno.isEmpty())
			{
				result.flags()[i] = false;
				continue;
			}

			//apply filters
			if (applySpliceAi_(sai_anno, min_sai, splice_site_only)) continue;
			if (applyMaxEntScanFilter_(mes_anno, min_mes, splice_site_only)) continue;

			result.flags()[i] = false;
		}
	}
	// actio KEEP
	else if (getString("action") == "KEEP")
	{
		for(int i=0; i<variant_list.count(); ++i)
		{
			if (result.flags()[i]) continue;

			if (applySpliceAi_(variant_list[i].annotations()[idx_sai].trimmed(), min_sai, splice_site_only))
			{
				result.flags()[i] = true;
				continue;
			}

			if (applyMaxEntScanFilter_(variant_list[i].annotations()[idx_mes].trimmed(), min_mes, splice_site_only))
			{
				result.flags()[i] = true;
				continue;
			}
		}
	}
}

bool FilterSpliceEffect::applyMaxEntScanFilter_(const QByteArray& mes_anno, MaxEntScanImpact min_mes, bool splice_site_only) const
{
	if (!mes_anno.isEmpty() && min_mes!=MaxEntScanImpact::LOW)
	{
        for (const QByteArray& entry : mes_anno.split(','))
		{
			QByteArray details;
			MaxEntScanImpact impact = NGSHelper::maxEntScanImpact(entry.split('/'), details, splice_site_only);
			if (impact==MaxEntScanImpact::HIGH) return true;
			if (impact==MaxEntScanImpact::MODERATE && min_mes==MaxEntScanImpact::MODERATE) return true;
		}
	}

	return false;
}

bool FilterSpliceEffect::applySpliceAi_(const QByteArray& sai_anno, double min_sai, bool splice_site_only) const
{
	if (!sai_anno.isEmpty() && min_sai>0)
	{
		//old format - maximum score for all transcripts/genes only
		bool ok = false;
		double max_score = sai_anno.toDouble(&ok);
		if (ok) return max_score>=min_sai;

		//new format - comma-speparated list of predictions, e.g. BABAM1|0.03|0.00|0.01|0.00|-2|2|41|2,CTD-2278I10.6|0.03|0.00|0.01|0.00|-2|2|41|2 (GENE|DS_AG|DS_AL|DS_DG|DS_DL|DP_AG|DP_AL|DP_DG|DP_DL)
		 max_score = 0.0;
        for (const QByteArray& entry : sai_anno.split(','))
		{
			QByteArrayList parts = entry.split('|');
			if (parts.count()!=9) THROW(ProgrammingException, "Invalid SpliceAI annotation - not 9 parts: " + entry);

			//determine maximum score
			QList<int> indices;
			indices << 2 << 4;
			if (!splice_site_only) indices << 1 << 3;
            for (const int& i : indices)
			{
				QString score = parts[i];
				if (parts.count()!=9) THROW(ProgrammingException, "Invalid SpliceAI annotation - score with index "+QString::number(i)+" is not numeric: " + entry);

				bool ok = false;
				double score_val = score.toDouble(&ok);
				if (!ok || score_val<0 || score_val>1) continue;
				max_score = std::max(score_val, max_score);
			}
		}
		return max_score>=min_sai;
	}

	return false;
}

FilterVariantRNAAseAlleleFrequency::FilterVariantRNAAseAlleleFrequency()
{
	name_ = "RNA ASE allele frequency";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the allele specific expression allele frequency.";
	params_ << FilterParameter("min_af", FilterParameterType::DOUBLE, 0.0, "Minimal expression allele frequency.");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 1.0, "Maximal expression allele frequency.");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterVariantRNAAseAlleleFrequency::toText() const
{
	return name() + " between " + QString::number(getDouble("min_af", false), 'f', 2) + " and " + QString::number(getDouble("max_af", false), 'f', 2);
}

void FilterVariantRNAAseAlleleFrequency::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double min_af = getDouble("min_af");
	double max_af = getDouble("max_af");

	int idx_ase_af = annotationColumn(variants, "ASE_af");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//skip not covered variants
		QString ase_af_string = variants[i].annotations()[idx_ase_af].trimmed();
		if(ase_af_string.isEmpty() || ase_af_string.startsWith("n/a"))
		{
			result.flags()[i] = false;
			continue;
		}

		double ase_af = Helper::toDouble(ase_af_string, "ASE_af", QString::number(i));
		result.flags()[i] = (ase_af >= min_af) && (ase_af <= max_af);
	}
}

FilterVariantRNAAseDepth::FilterVariantRNAAseDepth()
{
	name_ = "RNA ASE depth";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the allele specific expression depth.";
	params_ << FilterParameter("min_depth", FilterParameterType::INT, 20, "Minimal expression depth.");
	params_.last().constraints["min"] = "0";

	checkIsRegistered();
}

QString FilterVariantRNAAseDepth::toText() const
{
	return name() + " &ge; " + QString::number(getInt("min_depth", false));
}

void FilterVariantRNAAseDepth::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int min_depth = getInt("min_depth");

	int idx_ase_depth = annotationColumn(variants, "ASE_depth");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		int ase_depth = Helper::toInt(variants[i].annotations()[idx_ase_depth], "ASE_depth", QString::number(i));
		result.flags()[i] = ase_depth >= min_depth;
	}
}

FilterVariantRNAAseAlt::FilterVariantRNAAseAlt()
{
	name_ = "RNA ASE alternative count";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the allele specific expression alternative count.";
	params_ << FilterParameter("min_ac", FilterParameterType::INT, 5, "Minimal expression alternative count.");
	params_.last().constraints["min"] = "0";

	checkIsRegistered();
}

QString FilterVariantRNAAseAlt::toText() const
{
	return name() + " &ge; " + QString::number(getInt("min_ac", false));
}

void FilterVariantRNAAseAlt::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	int min_ac = getInt("min_ac");

	int idx_ase_ac = annotationColumn(variants, "ASE_alt");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//skip not covered variants
		QString ase_ac_string = variants[i].annotations()[idx_ase_ac].trimmed();
		if(ase_ac_string.isEmpty() || ase_ac_string.startsWith("n/a"))
		{
			result.flags()[i] = false;
			continue;
		}

		int ase_ac = Helper::toInt(ase_ac_string, "ASE_alt", QString::number(i));
		result.flags()[i] = ase_ac >= min_ac;
	}
}

FilterVariantRNAAsePval::FilterVariantRNAAsePval()
{
	name_ = "RNA ASE p-value";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the allele specific expression p-value.";
	params_ << FilterParameter("max_pval", FilterParameterType::DOUBLE, 0.05, "Maximal expression p-value.");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterVariantRNAAsePval::toText() const
{
	return name() + " &le; " + QString::number(getDouble("max_pval", false), 'f', 2);
}

void FilterVariantRNAAsePval::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double max_pval = getDouble("max_pval");

	int idx_ase_pval = annotationColumn(variants, "ASE_pval");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//skip not covered variants
		QString ase_pval_string = variants[i].annotations()[idx_ase_pval].trimmed();
		if(ase_pval_string.isEmpty() || ase_pval_string.startsWith("n/a"))
		{
			result.flags()[i] = false;
			continue;
		}

		double ase_pval = Helper::toDouble(ase_pval_string, "ASE_pval", QString::number(i));
		result.flags()[i] = ase_pval <= max_pval;
	}
}

FilterVariantRNAAberrantSplicing::FilterVariantRNAAberrantSplicing()
{
	name_ = "RNA aberrant splicing fraction";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the fraction of aberrant splicing reads.";
	params_ << FilterParameter("min_asf", FilterParameterType::DOUBLE, 0.01, "Minimal aberrant splicing fraction.");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
}

QString FilterVariantRNAAberrantSplicing::toText() const
{
	return name() + " &ge; " + QString::number(getDouble("min_asf", false), 'f', 3);
}

void FilterVariantRNAAberrantSplicing::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double min_asf = getDouble("min_asf");

	int idx_asf = annotationColumn(variants, "aberrant_splicing");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QList<QByteArray> fraction_strings = variants[i].annotations()[idx_asf].split(',');
		result.flags()[i] = false;
        for (const QByteArray& fraction_string : fraction_strings)
		{
			if(fraction_string.isEmpty() || fraction_string.startsWith("n/a")) continue;

			double fraction_value = Helper::toDouble(fraction_string, "aberrant_splicing", QString::number(i));
			if (fraction_value >= min_asf)
			{
				result.flags()[i] = true;
				break;
			}
		}
	}
}

FilterVariantRNAGeneExpression::FilterVariantRNAGeneExpression()
{
	name_ = "RNA gene expression";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the gene expression in transcripts-per-million";
	params_ << FilterParameter("min_tpm", FilterParameterType::DOUBLE, 5.0, "Minimal gene expression in transcripts-per-million.");
	params_.last().constraints["min"] = "0.0";
}

QString FilterVariantRNAGeneExpression::toText() const
{
	return name() + " &ge; " + QString::number(getDouble("min_tpm", false), 'f', 2) + "(tpm)";
}

void FilterVariantRNAGeneExpression::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double min_tpm = getDouble("min_tpm");

	int idx_asf = annotationColumn(variants, "tpm");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QList<QByteArray> fraction_strings = variants[i].annotations()[idx_asf].split(',');
		result.flags()[i] = false;
        for (const QByteArray& fraction_string : fraction_strings)
		{
            if(fraction_string.isEmpty() || fraction_string.startsWith("n/a")) continue;

            double fraction_value = Helper::toDouble(fraction_string, "tpm", QString::number(i));
			if (fraction_value >= min_tpm)
			{
				result.flags()[i] = true;
				break;
			}
		}
	}
}

FilterVariantRNAExpressionFC::FilterVariantRNAExpressionFC()
{
	name_ = "RNA expression fold-change";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the absolute gene expression log2 fold-change.";
	params_ << FilterParameter("min_fc", FilterParameterType::DOUBLE, 2.0, "Minimal absolute fold-change.");
	params_.last().constraints["min"] = "0.0";
}

QString FilterVariantRNAExpressionFC::toText() const
{
	return name() + " (abs) &ge; " + QString::number(getDouble("min_fc", false), 'f', 2);
}

void FilterVariantRNAExpressionFC::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double min_fc = getDouble("min_fc");

	int idx_fc = annotationColumn(variants, "expr_log2fc");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QList<QByteArray> fc_strings = variants[i].annotations()[idx_fc].split(',');
		result.flags()[i] = false;
        for (const QByteArray& fc_string : fc_strings)
		{
			if(fc_string.isEmpty() || fc_string.startsWith("n/a")) continue;

			double fraction_value = fabs(Helper::toDouble(fc_string, "expr_log2fc", QString::number(i)));
			if (fraction_value >= min_fc)
			{
				result.flags()[i] = true;
				break;
			}
		}
	}
}

FilterVariantRNAExpressionZScore::FilterVariantRNAExpressionZScore()
{
	name_ = "RNA expression z-score";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter based on the absolute gene expression z-score.";
	params_ << FilterParameter("min_zscore", FilterParameterType::DOUBLE, 2.0, "Minimal absolute z-score.");
	params_.last().constraints["min"] = "0.0";
}

QString FilterVariantRNAExpressionZScore::toText() const
{
	return name() + " (abs) &ge; " + QString::number(getDouble("min_zscore", false), 'f', 2);
}

void FilterVariantRNAExpressionZScore::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	double min_zscore = getDouble("min_zscore");

	int idx_zscore = annotationColumn(variants, "expr_zscore");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		QList<QByteArray> zscore_strings = variants[i].annotations()[idx_zscore].split(',');
		result.flags()[i] = false;
        for (const QByteArray& zscore_string : zscore_strings)
		{
			if(zscore_string.isEmpty() || zscore_string.startsWith("n/a")) continue;

			double zscore = fabs(Helper::toDouble(zscore_string, "expr_zscore", QString::number(i)));
			if (zscore >= min_zscore)
			{
				result.flags()[i] = true;
				break;
			}
		}
	}
}


FilterVariantLrSrOverlap::FilterVariantLrSrOverlap()
{
	name_ = "lr short-read overlap";
	type_ = VariantType::SNVS_INDELS;
	description_ = QStringList() << "Filter that preserves variants if they were called in short-read WGS sample only.";
	params_ << FilterParameter("invert", FilterParameterType::BOOL, false, "If set, removes all variants if they were called in short-read WGS sample.");

}

QString FilterVariantLrSrOverlap::toText() const
{
	return name() + (getBool("invert") ? " (invert)" : "");
}

void FilterVariantLrSrOverlap::apply(const VariantList& variants, FilterResult& result) const
{
	if (!enabled_) return;

	bool invert = getBool("invert");
	int idx_in_shortread = variants.annotationIndexByName("in_short-read");

	for(int i=0; i<variants.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		if (invert)
		{
			result.flags()[i] = (variants[i].annotations().at(idx_in_shortread).trimmed() == "");
		}
		else
		{
			result.flags()[i] = !(variants[i].annotations().at(idx_in_shortread).trimmed() == "");
		}
	}
}



FilterSvCnvOverlap::FilterSvCnvOverlap()
{
	name_ = "SV CNV overlap";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Filter the removes DEL/DUP without support from CNV calling.";
	params_ << FilterParameter("min_ol", FilterParameterType::DOUBLE, 0.50, "Minimum CNV overlap.");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
	params_ << FilterParameter("min_size", FilterParameterType::INT, 10000, "Minimum SV size in bases.");
	params_.last().constraints["min"] = "0";
}

QString FilterSvCnvOverlap::toText() const
{
	return name() + " &ge; " + QString::number(getDouble("min_ol", false), 'f', 2)+ " (size &ge; " + QString::number(getInt("min_size", false)/1000.0, 'f', 2) + "kb)";
}

void FilterSvCnvOverlap::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;

	//init
	double min_ol = getDouble("min_ol", false);
	int ol_col = svs.annotationIndexByName("CNV_OVERLAP");
	if (ol_col==-1) THROW(ProgrammingException, "Missing column CNV_OVERLAP");
	int min_size = getInt("min_size", false);

	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//skip if no overlap is annotated, i.e. not DEL/DUP
		QByteArray ol_str = svs[i].annotations()[ol_col].trimmed();
		if (ol_str.isEmpty()) continue;

		//skip too small DEL/DUP - they are hard to detect in CNV calling with 1kb window size
		int sv_length = svs.estimatedSvSize(i);
		if (sv_length < min_size) continue;

		if (ol_str.toDouble() < min_ol) result.flags()[i] = false;
	}
}


FilterSvLrAF::FilterSvLrAF()
{
	name_ = "SV-lr AF";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Show only (lr) SVs with a allele frequency between the given interval";
	params_ << FilterParameter("min_af", FilterParameterType::DOUBLE, 0.0, "minimal allele frequency");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";
	params_ << FilterParameter("max_af", FilterParameterType::DOUBLE, 1.0, "maximal allele frequency");
	params_.last().constraints["min"] = "0.0";
	params_.last().constraints["max"] = "1.0";

	checkIsRegistered();
}

QString FilterSvLrAF::toText() const
{
	return name() + " between " + QByteArray::number(getDouble("min_af", false), 'f', 2) + " and "  + QByteArray::number(getDouble("max_af", false), 'f', 2);
}

void FilterSvLrAF::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;
	if (svs.format() == BedpeFileFormat::BEDPE_SOMATIC_TUMOR_NORMAL)
	{
		// ignore filter if applied to tumor-normal sample
		THROW(ArgumentException, "Filter '" + name() +"' cannot be applied to somatic tumor normal sample!");
		return;
	}

	// get allowed interval
	double upper_limit = getDouble("max_af", false);
	double lower_limit = getDouble("min_af", false);


	int col_index = svs.annotationIndexByName("AF");

	if ((svs.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI) || (svs.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO))
	{
		// ignore filter if applied to trio/multi samples
		THROW(ArgumentException, "Filter '" + name() +"' cannot be applied on multi-samples!");
		return;
	}

	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//some SVs do not have a AF due to insufficient coverage, keep them in
		if (svs[i].annotations()[col_index].isEmpty()) continue;

		//get AF
		double af = Helper::toDouble(svs[i].annotations()[col_index]);

		// compare AF with filter
		if(af > upper_limit || af < lower_limit) result.flags()[i] = false;
	}
}

FilterSvLrSupportReads::FilterSvLrSupportReads()
{
	name_ = "SV-lr support reads";
	type_ = VariantType::SVS;
	description_ = QStringList() << "Show only (lr) SVs with a minimum number of supporting reads";
	params_ << FilterParameter("min_support", FilterParameterType::INT, 5, "Minimum support read count");
	params_.last().constraints["min"] = "0";
	params_.last().constraints["max"] = "10000";

	checkIsRegistered();
}

QString FilterSvLrSupportReads::toText() const
{
	return name() + " &ge; " + QString::number(getInt("min_support", false), 'f', 2);
}

void FilterSvLrSupportReads::apply(const BedpeFile& svs, FilterResult& result) const
{
	if (!enabled_) return;
	int col_index = svs.annotationIndexByName("SUPPORT");
	int min_support = getInt("min_support", true);
	// iterate over all SVs
	for(int i=0; i<svs.count(); ++i)
	{
		if (!result.flags()[i]) continue;

		//get supporting read count
		int sup_reads = Helper::toInt(svs[i].annotations()[col_index]);

		// compare AF with filter
		if(sup_reads < min_support) result.flags()[i] = false;
	}
}

