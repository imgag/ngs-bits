#include "SharedData.h"
#include "Settings.h"
#include <QFileInfo>
#include <QMessageBox>
#include <cmath>
#include <limits>

QList<Parameter> SharedData::getParameters()
{
	return {
		Parameter("label width", "Width of the label area on the left of tracks (in pixels).", ParameterType::INT, {{ConstraintType::MIN, 50}}),
		Parameter("min window size", "Minimum number of bases to show.", ParameterType::INT, {{ConstraintType::MIN, 21}}),
		Parameter("transcript padding", "Number of bases added around genes/transcripts, e.g. after search.", ParameterType::INT, {{ConstraintType::MIN, 0}}),
		Parameter("BAM max visibility region", "Initial visibility range for new BAM alignment and coverage tracks (in bases).", ParameterType::INT, {{ConstraintType::MIN, 1000}}),
		Parameter("BAM insert size lower percentile", "Lower insert-size cutoff for coloring reads as outliers.", ParameterType::DOUBLE, {{ConstraintType::MIN, 0.0}, {ConstraintType::MAX, 1.0}, {ConstraintType::DISPLAY_DIGITS, 3}}),
		Parameter("BAM insert size upper percentile", "Upper insert-size cutoff for coloring reads as outliers.", ParameterType::DOUBLE, {{ConstraintType::MIN, 0.0}, {ConstraintType::MAX, 1.0}, {ConstraintType::DISPLAY_DIGITS, 3}})
	};
}

QHash<QByteArray, QVariant> SharedData::getParameterDefaults()
{
	return {{"label width", 165}, {"min window size", 41}, {"transcript padding", 2000},
		{"BAM max visibility region", 50000}, {"BAM insert size lower percentile", 0.005}, {"BAM insert size upper percentile", 0.995}};
}

void SharedData::writeSettingsToXml(QXmlStreamWriter& writer)
{
	for (const Parameter& parameter : parameters().parameters())
	{
		writer.writeStartElement("Settings");
		writer.writeAttribute("name", QString::fromUtf8(parameter.name()));
		writer.writeAttribute("type", QString::fromUtf8(Parameter::toString(parameter.type())));
		const QString value = parameter.type() == ParameterType::DOUBLE
			? QString::number(parameter.value().toDouble(), 'g', std::numeric_limits<double>::max_digits10)
			: parameter.value().toString();
		writer.writeTextElement("Value", value);
		writer.writeEndElement(); //Settings
	}
}

void SharedData::loadSettingsFromXml(const QDomElement& general)
{
	//Validate against the global schema, independent of the current percentile bounds.
	ParameterList validated("Global settings", getParameters(), getParameterDefaults());
	QHash<QByteArray, QVariant> values;
	for (QDomElement item = general.firstChildElement("Settings"); !item.isNull(); item = item.nextSiblingElement("Settings"))
	{
		const QByteArray key = item.attribute("name").toUtf8();
		if (values.contains(key)) THROW(ArgumentException, "Duplicate global setting '" + key + "'.");
		const Parameter& parameter = validated.parameter(key);
		if (item.attribute("type").toUtf8() != Parameter::toString(parameter.type()))
		{
			THROW(ArgumentException, "Type mismatch for global setting '" + key + "'.");
		}
		const QDomElement entry = item.firstChildElement("Value");
		if (entry.isNull() || !entry.nextSiblingElement("Value").isNull())
		{
			THROW(ArgumentException, "Expected exactly one Value for global setting '" + key + "'.");
		}
		bool ok = false;
		switch (parameter.type())
		{
			case ParameterType::INT:
			{
				const int number = entry.text().toInt(&ok);
				if (!ok) THROW(ArgumentException, "Invalid integer for global setting '" + key + "'.");
				validated.setInt(key, number);
				break;
			}
			case ParameterType::DOUBLE:
			{
				const double number = entry.text().toDouble(&ok);
				if (!ok) THROW(ArgumentException, "Invalid double for global setting '" + key + "'.");
				validated.setDouble(key, number);
				break;
			}
			default: THROW(ProgrammingException, "Unsupported global setting type for '" + key + "'.");
		}
		values.insert(key, validated.parameter(key).value());
	}
	for (const Parameter& parameter : validated.parameters())
	{
		if (!values.contains(parameter.name())) THROW(ArgumentException, "Missing global setting '" + parameter.name() + "'.");
	}
	const double lower = validated.getDouble("BAM insert size lower percentile");
	const double upper = validated.getDouble("BAM insert size upper percentile");
	if (lower > upper) THROW(ArgumentException, "Lower BAM insert-size percentile exceeds upper percentile.");
	validated.overrideConstraint("BAM insert size lower percentile", ConstraintType::MAX, upper);
	validated.overrideConstraint("BAM insert size upper percentile", ConstraintType::MIN, lower);
	parameters().reset(validated.parameters(), values);
}

void SharedData::updateSettingsConstraints()
{
	//Reject inverted percentile ranges before a value or its linked member changes.
	settings_.overrideConstraint("BAM insert size lower percentile", ConstraintType::MAX, fast_settings_.bam_insert_size_upper_percentile);
	settings_.overrideConstraint("BAM insert size upper percentile", ConstraintType::MIN, fast_settings_.bam_insert_size_lower_percentile);
}

void SharedData::setTranscripts(const TranscriptList& transcripts)
{
	SharedData* inst = instance();

	//copy Ensembl transcripts
	inst->transcripts_.clear();
	for(const Transcript& t: transcripts)
    {
        if (t.source()!=Transcript::ENSEMBL) continue;
		inst->transcripts_ << t;
    }

	//sort transcripts
	if (!inst->transcripts_.isSorted())
	{
		inst->transcripts_.sortByPosition();
	}

	//index transcripts
	inst->transcripts_index_.createIndex();

	emit inst->transcriptsChanged();
}

void SharedData::setRegion(const Chromosome& chr, int start, int end)
{
	if (end < start) THROW(ArgumentException, "Invalid chromosome range - end before start: "+QString::number(start) + "-" + QString::number(end));

	//extend region to minimal region size
	int size = end-start+1;
	if (size<instance()->fast_settings_.min_window_size)
	{
		int missing = instance()->fast_settings_.min_window_size-size;
		start -= missing/2;
		end += missing/2;
		if (missing%2!=0)
		{
			start -= 1;
			end += 1;
		}
		size = end-start+1;
	}

	//make sure the region is in chromosome boundaries
	if (start<1)
	{
		start = 1;
		end = start + size - 1;
	}
	int genome_end = SharedData::genome().lengthOf(chr);
	if (end>genome_end)
	{
		end = genome_end;
		start = end - size + 1;
		if (start<1) start = 1; //if size is bigger than chromosome, this can happen
	}

	//check new region is different from old
	BedLine new_reg = BedLine(chr, start, end);
	if (new_reg==instance()->region_) return;

	instance()->region_ = new_reg;
	emit instance()->regionChanged();
}

SharedData* SharedData::instance()
{
	static SharedData s;
	return &s;
}

void SharedData::mouseMoved(int x, int width)
{
	int label_width = SharedData::settings().label_width;
	const BedLine& region = SharedData::region();
	double pixels_per_base_ =  (double)(width-label_width-4) / (double)region.length();

	//show
	if (x>label_width + 2 && x<width - 2)
	{
		int coordinate = region.start() + std::floor((double)(x-label_width - 2) / pixels_per_base_);
		emit instance()->updateGenomicCoordinate(region.chr().strNormalized(true) + ":" + QString::number(coordinate));
	}
	else
	{
		emit instance()->updateGenomicCoordinate("");
	}
}

SharedData::SharedData(QObject* parent)
	: QObject(parent)
	, genome_index_(Settings::string("reference_genome", false))
	, transcripts_()
	, transcripts_index_(transcripts_)
	, settings_("Global settings", getParameters(), getParameterDefaults())
	, fast_settings_()
	, region_(Chromosome("chr7"), 5971876, 5984933) //just some random default to make sure it is not a invalid region
{
	settings_.link(&fast_settings_);
	updateSettingsConstraints();
	connect(&settings_, &ParameterList::parameterChanged, this, [this]()
	{
		updateSettingsConstraints();
		emit settingsChanged();
	});
}
