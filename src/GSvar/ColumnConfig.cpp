#include "ColumnConfig.h"
#include "Exceptions.h"

ColumnConfig::ColumnConfig()
	: columns_()
	, infos_()
{
}

void ColumnConfig::append(const QString& name, const ColumnInfo& info)
{
	//check
	if (name.trimmed().isEmpty()) THROW(ArgumentException, "Column '" + name + "' is not valid in column config!");
	if (contains(name)) THROW(ArgumentException, "Column '" + name + "' already contained in column config!");
	if (info.min_width<0) THROW(ArgumentException, "Minimum width for '" + name + "' is invalid: " + QString::number(info.min_width));
	if (info.max_width<0) THROW(ArgumentException, "Minimum width for '" + name + "' is invalid: " + QString::number(info.max_width));

	//add
	columns_ << name;
	infos_.insert(name, info);
}

QString ColumnConfig::toString() const
{
	QString output;

	foreach(const QString& name, columns_)
	{
		if (!output.isEmpty()) output.append("\t");
		output.append(name);
		output.append("|");
		const ColumnInfo& info = infos_[name];
		output.append(QString::number(info.min_width));
		output.append("|");
		output.append(QString::number(info.max_width));
		output.append("|");
		output.append(info.hidden ? "hidden" : "visible");
	}

	return output;
}

ColumnConfig ColumnConfig::fromString(const QString& text)
{
	ColumnConfig output;
	if (text.trimmed().isEmpty()) return output;

	QStringList parts = text.split("\t");
	foreach(const QString& part, parts)
	{
		QStringList values = part.split("|");
		if (values.count()!=4) THROW(ArgumentException, "Column config text contains invalid column info part '" + part + "'!");
		ColumnInfo info;
		info.min_width = values[1].toInt();
		info.max_width = values[2].toInt();
		info.hidden = values[3].trimmed()=="hidden";
		output.append(values[0], info);
	}

	return output;
}
