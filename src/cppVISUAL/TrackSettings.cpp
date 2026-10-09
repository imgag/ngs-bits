#include "TrackSettings.h"
#include "Exceptions.h"
#include "Helper.h"
#include <cmath>



QByteArray ParameterDescription::toString(ParameterType type)
{
	switch (type)
	{
		case ParameterType::INT: return "INT";
		case ParameterType::FLOAT: return "FLOAT";
		case ParameterType::BOOL: return "BOOL";
		case ParameterType::STRING: return "STRING";
	}
	THROW(ProgrammingException, "Unhandled parameter type: " + QString::number(static_cast<int>(type)));
}

QByteArray ParameterDescription::toString(ConstraintType type)
{
	switch (type)
	{
		case ConstraintType::MIN: return "MIN";
		case ConstraintType::MAX: return "MAX";
		case ConstraintType::ALLOWED_VALUES: return "ALLOWED_VALUES";
	}
	THROW(ProgrammingException, "Unhandled constraint type: " + QString::number(static_cast<int>(type)));
}

ParameterDescription::ParameterDescription(QByteArray name, QByteArrayList description, ParameterType type, QHash<ConstraintType, QVariant> constraints)
	: name_(name)
	, description_(description)
	, type_(type)
	, constraints_(constraints)
{
	QString parameter_name = QString::fromUtf8(name_);
	if (type_ == ParameterType::INT || type_ == ParameterType::FLOAT)
	{
		for(auto it=constraints.cbegin(); it!=constraints.cend(); ++it)
		{
			if (it.key()==ConstraintType::MIN)
			{
				QString value_as_str = it.value().toString();
				if (!Helper::isNumeric(value_as_str)) THROW(ArgumentException, "Int/float parameter '"+name_+"' has non-numeric 'MIN' constraint '" + value_as_str + "'!");
			}
			else if (it.key()==ConstraintType::MAX)
			{
				QString value_as_str = it.value().toString();
				if (!Helper::isNumeric(value_as_str)) THROW(ArgumentException, "Int/float parameter '"+name_+"' has non-numeric 'MAX' constraint '" + value_as_str + "'!");
			}
			else THROW(ArgumentException, "Int/float parameter '"+name_+"' cannot have constraint '" + ParameterDescription::toString(it.key()) + "'!");
		}
		if (constraints_.contains(ConstraintType::MIN) && constraints_.contains(ConstraintType::MAX)
			&& constraints_.value(ConstraintType::MIN).toDouble() > constraints_.value(ConstraintType::MAX).toDouble())
		{
			THROW(ArgumentException, "MIN constraint exceeds MAX constraint for parameter '" + name_ + "'.");
		}
	}
	else if (type_ == ParameterType::STRING)
	{
		for(auto it=constraints.cbegin(); it!=constraints.cend(); ++it)
		{
			if (it.key()==ConstraintType::ALLOWED_VALUES)
			{
				QByteArrayList parts = it.value().toByteArray().split('\t');
				if (parts.count()<2) THROW(ArgumentException, "String parameter '"+name_+"' has have constraint '" + ParameterDescription::toString(it.key()) + "'!");
			}
			else THROW(ArgumentException, "String parameter '"+name_+"' cannot have constraint '" + ParameterDescription::toString(it.key()) + "'!");
		}
	}
	else
	{
		for(auto it=constraints.cbegin(); it!=constraints.cend(); ++it)
		{
			THROW(ArgumentException, "String parameter '"+name_+"' cannot have constraint '" + ParameterDescription::toString(it.key()) + "'!");
		}
	}
}

ParameterConfig::ParameterConfig(QByteArray track_name)
	: track_name_(track_name)
{
}

void ParameterConfig::clear()
{
	entries_.clear();
}

void ParameterConfig::append(const ParameterDescription& param)
{
	for (const ParameterDescription& entry: std::as_const(entries_))
	{
		if (entry.name() == param.name())
		{
			THROW(ArgumentException, "Parameter '" + QString::fromUtf8(param.name()) + "' added twice to ParameterConfig.");
		}
	}
	entries_.append(param);
}

const ParameterDescription& ParameterConfig::parameter(const QByteArray& name) const
{
	for (const ParameterDescription& entry : entries_)
	{
		if (entry.name() == name) return entry;
	}
	THROW(ArgumentException, "Unknown parameter '" + name + "'.");
}

TrackSettings::TrackSettings(const ParameterConfig& config, QObject* parent)
	: QObject(parent)
	, config_(config)
{
}

const ParameterDescription& TrackSettings::checkParameter(const QByteArray& name, ParameterType type) const
{
	const ParameterDescription& parameter = config_.parameter(name);
	if (parameter.type() != type) THROW(ArgumentException, "Requested type '"+ParameterDescription::toString(type)+"' for parameter '" + name + "' with type '"+ParameterDescription::toString(parameter.type())+"' in track '"+config_.trackName() +"'.");
	return parameter;
}

const QVariant& TrackSettings::checkedValue(const QByteArray& name, ParameterType type) const
{
	checkParameter(name, type);
	auto it = values_.constFind(name);
	if (it == values_.cend()) THROW(ArgumentException, "Parameter '" + name + "' has not been set in track '"+config_.trackName()+"'.");
	return it.value();
}

void TrackSettings::setValue(const QByteArray& name, ParameterType type, const QVariant& value)
{
	const auto& constraints = checkParameter(name, type).constraints();
	if (type == ParameterType::INT || type == ParameterType::FLOAT)
	{
		double numeric_value = value.toDouble();
		if (!std::isfinite(numeric_value))
		{
			THROW(ArgumentException, "Non-finite value for parameter '" + name + "' in track '"+config_.trackName()+"'.");
		}
		for (auto it = constraints.cbegin(); it != constraints.cend(); ++it)
		{
			double limit = it.value().toDouble();
			if (it.key() == ConstraintType::MIN && numeric_value < limit)
			{
				THROW(ArgumentException, "Value '"+value.toString()+"' of parameter '"+name+"' smaller than MIN constraint ("+QString::number(limit)+") for parameter '" + name + "' in track '"+config_.trackName()+"'.");
			}
			if (it.key() == ConstraintType::MAX && numeric_value > limit)
			{
				THROW(ArgumentException, "Value '"+value.toString()+"' of parameter '"+name+"' larger than MAX constraint ("+QString::number(limit)+") for parameter '" + name + "' in track '"+config_.trackName()+"'.");
			}
		}
	}
	else if (type == ParameterType::STRING && constraints.contains(ConstraintType::ALLOWED_VALUES))
	{
		QByteArrayList allowed_values = constraints.value(ConstraintType::ALLOWED_VALUES).toByteArray().split('\t');
		if (!allowed_values.contains(value.toByteArray()))
		{
			THROW(ArgumentException, "Value '"+value.toByteArray()+"' is not allowed for parameter '" + name + "' in track '"+config_.trackName()+"'.");
		}
	}

	auto it = values_.constFind(name);
	if (it != values_.cend() && it.value() == value) return;
	values_.insert(name, value);
	emit parameterChanged();
}

void TrackSettings::setInt(const QByteArray& name, int value)
{
	setValue(name, ParameterType::INT, value);
}

void TrackSettings::setFloat(const QByteArray& name, double value)
{
	setValue(name, ParameterType::FLOAT, value);
}

void TrackSettings::setBool(const QByteArray& name, bool value)
{
	setValue(name, ParameterType::BOOL, value);
}

void TrackSettings::setString(const QByteArray& name, const QByteArray& value)
{
	setValue(name, ParameterType::STRING, value);
}
