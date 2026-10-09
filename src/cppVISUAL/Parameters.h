#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "cppVISUAL_global.h"
#include <QHash>
#include <QVariant>
#include <QByteArrayList>
#include <QObject>

//Parameter type
enum class ParameterType
{
	INT,
	FLOAT,
	BOOL,
	STRING
};

//Constraint types
enum class ConstraintType
{
	MIN,
	MAX,
	ALLOWED_VALUES
};

//Parameter description
class CPPVISUALSHARED_EXPORT ParameterDescription
{
public:
	//constructor
	ParameterDescription(QByteArray name, QByteArrayList description, ParameterType type, QHash<ConstraintType, QVariant> constraints);

	//members
	const QByteArray& name() const { return name_; }
	const QByteArrayList& description() const { return description_; }
	ParameterType type() const { return type_; }
	const QHash<ConstraintType, QVariant>& constraints() const { return constraints_; }

	//Converts parameter type to string
	static QByteArray toString(ParameterType type);
	//Converts constraint type to string
	static QByteArray toString(ConstraintType type);

private:
	QByteArray name_;
	QByteArrayList description_;
	ParameterType type_;
	QHash<ConstraintType, QVariant> constraints_;
};

//Parameter configuration that defines the order of parameters in UI and provides meta data about parameters
class CPPVISUALSHARED_EXPORT ParameterConfig
{
public:
	ParameterConfig(QByteArray track_name);
	const QByteArray& trackName() const {return track_name_;}

	//Clears all entries
	void clear();
	//Appends a entry
	void append(const ParameterDescription& param);
	//Returns the metadata for a parameter by name.
	const ParameterDescription& parameter(const QByteArray& name) const;
	const QList<ParameterDescription>& parameters() const { return entries_; }

protected:
	QByteArray track_name_;
	QList<ParameterDescription> entries_;
};

//Parameter
class CPPVISUALSHARED_EXPORT Parameters
	: public QObject
{
	Q_OBJECT

public:
	Parameters(const ParameterConfig& config, const QHash<QByteArray, QVariant>& defaults);

	int getInt(const QByteArray& name) const { return checkedValue(name, ParameterType::INT).toInt(); }
	void setInt(const QByteArray& name, int value) { setValue(name, ParameterType::INT, value); }
	double getFloat(const QByteArray& name) const { return checkedValue(name, ParameterType::FLOAT).toDouble(); }
	void setFloat(const QByteArray& name, double value) { setValue(name, ParameterType::FLOAT, value); }
	bool getBool(const QByteArray& name) const { return checkedValue(name, ParameterType::BOOL).toBool(); }
	void setBool(const QByteArray& name, bool value) { setValue(name, ParameterType::BOOL, value); }
	QByteArray getString(const QByteArray& name) const { return checkedValue(name, ParameterType::STRING).toByteArray(); }
	void setString(const QByteArray& name, const QByteArray& value) { setValue(name, ParameterType::STRING, value); }

signals:
	void parameterChanged();

protected:
	const ParameterConfig config_;

private:
	QHash<QByteArray, QVariant> values_;

	const ParameterDescription& checkParameter(const QByteArray& name, ParameterType type) const;
	const QVariant& checkedValue(const QByteArray& name, ParameterType type) const;
	void setValue(const QByteArray& name, ParameterType type, const QVariant& value, bool emit_changed_signal = true);
};



#endif // PARAMETERS_H
