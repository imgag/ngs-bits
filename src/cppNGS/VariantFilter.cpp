#include "VariantFilter.h"
#include "VariantList.h"
#include "Exceptions.h"
#include <QBitArray>

VariantFilter::VariantFilter()
	: name_()
	, criteria_()
{
}

VariantFilter::VariantFilter(const QString& name, const QString& criteria)
	: name_(name)
	, criteria_(criteria)
{
	checkValid();
}

void VariantFilter::checkValid()
{
	//check if valid
	QStringList o_crits = criteria_.split("||", QString::SkipEmptyParts);
	foreach(const QString& o_crit, o_crits)
	{
		//OR parts
		QStringList a_crits = o_crit.split("&&", QString::SkipEmptyParts);
		foreach(const QString& a_crit, a_crits)
		{
			//split single criterion (field, op, value)
			QStringList parts = a_crit.split(" ", QString::SkipEmptyParts);
			if (parts.count()<2)
			{
				//qDebug() << __LINE__;
				THROW(ArgumentException, "Invalid variant filter criterion '" + a_crit + "'. It must contain at lease field name, operation and value!");
			}

			//value may be empty
			if (parts.count()<3) parts.append("");

			//check field value
			QString field_name = parts[0];
			if (QRegExp("a-zA-Z0-9_-").exactMatch(field_name))
			{
				//qDebug() << __LINE__ << field_name;
				THROW(ArgumentException, "Invalid field name '" + field_name + "' in filter criterion: '" + a_crit + "'");
			}

			//operations
			QString op = parts[1];
			if (op==">=" || op==">" || op=="==" || op=="<" || op=="<=")
			{
				bool ok = false;
				QStringList(parts.mid(2)).join(' ').toDouble(&ok);
				if (!ok)
				{
					//qDebug() << __LINE__ << QStringList(parts.mid(2)).join(' ');
					THROW(ArgumentException, "Invalid filter value in NUMERIC filter criterion: '" + a_crit + "'");
				}
			}
			else if (op=="IS" || op=="IS_NOT" || op=="CONTAINS" || op=="CONTAINS_NOT")
			{
				//nothing to check for strings
			}
			else
			{
				//qDebug() << __LINE__ << op;
				THROW(ArgumentException, "Invalid filter operation '" + op + "' in filter criterion: '" + a_crit + "'");
			}
		}
	}
}

bool VariantFilter::pass(const VariantList& list, int index) const
{
	QHash<QString, int> anno_index_cache;
	return pass(list, index, anno_index_cache);
}

bool VariantFilter::pass(const VariantList& list, int index, QHash<QString, int>& anno_index_cache) const
{
	const Variant& variant = list[index];
	bool result = false;

	//AND parts
	QStringList o_crits = criteria_.split("||", QString::SkipEmptyParts);
	foreach(const QString& o_crit, o_crits)
	{
		//OR parts
		bool and_result = true;
		QStringList a_crits = o_crit.split("&&", QString::SkipEmptyParts);
		foreach(const QString& a_crit, a_crits)
		{
			QStringList parts = a_crit.split(" ", QString::SkipEmptyParts);

			//check field value
			QString field_name = parts[0];
			QString field_string = "";
			if (anno_index_cache.contains(field_name))
			{
				field_string = variant.annotations()[anno_index_cache[field_name]];
			}
			else if (field_name=="chr")
			{
				field_string = variant.chr().str();
			}
			else if (field_name=="start")
			{
				field_string = QString::number(variant.start());
			}
			else if (field_name=="end")
			{
				field_string = QString::number(variant.end());
			}
			else if (field_name=="ref")
			{
				field_string = variant.ref();
			}
			else if (field_name=="obs")
			{
				field_string = variant.obs();
			}
			else if (field_name.startsWith("*") && field_name.endsWith("*"))
			{
				field_name = field_name.mid(1, field_name.count()-2);
				int anno_index = list.annotationIndexByName(field_name, false, true);
				field_string = variant.annotations()[anno_index];
				anno_index_cache.insert("*" + field_name + "*", anno_index);
			}
			else
			{
				int anno_index = list.annotationIndexByName(field_name, true, true);
				field_string = variant.annotations()[anno_index];
				anno_index_cache.insert(field_name, anno_index);
			}

			//operations
			bool single_result = true;
			QString op = parts[1];
			QString op_string = QStringList(parts.mid(2)).join(' ');
			if (op==">=" || op==">" || op=="==" || op=="<" || op=="<=")
			{
				double op_value = op_string.toDouble();
				bool ok = true;
				double field_value = field_string.toDouble(&ok);
				if (!ok) THROW(ArgumentException, "Invalid variant annotation in field '" + field_string + "' for NUMERIC filter criterion '" + a_crit + "'");
				if (op==">=") single_result = (field_value>=op_value);
				if (op==">") single_result = (field_value>op_value);
				if (op=="==") single_result = (field_value==op_value);
				if (op=="<") single_result = (field_value<op_value);
				if (op=="<=") single_result = (field_value<=op_value);
			}
			else if (op=="IS")
			{
				single_result = (field_string==op_string);
			}
			else if (op=="IS_NOT")
			{
				single_result = (field_string!=op_string);
			}
			else if (op=="CONTAINS")
			{
				single_result = (field_string.contains(op_string));
			}
			else if (op=="CONTAINS_NOT")
			{
				single_result = (!field_string.contains(op_string));
			}

			if (!single_result)
			{
				and_result = false;
				break;
			}
		}

		if (and_result)
		{
			result = true;
			break;
		}
	}

	return result;
}

QBitArray VariantFilter::multiPass(const VariantList& list, const QVector<VariantFilter>& filters)
{
	QBitArray output(list.count(), true);

	//optimization: use annotation index cache
	QHash<QString, int> anno_index_cache;
	foreach(const VariantFilter& filter, filters)
	{
		for(int i=0; i<list.count(); ++i)
		{
			//optimization: perform check only if the variant passed so far
			if (output[i])
			{
				output.setBit(i, filter.pass(list, i, anno_index_cache));
			}
		}
	}

	return output;
}
