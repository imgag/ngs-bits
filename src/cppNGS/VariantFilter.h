#ifndef VARIANTFILTER_H
#define VARIANTFILTER_H

#include "cppNGS_global.h"
#include <QBitArray>
#include <QHash>
#include <QString>
class VariantList;

/**
	@brief Variant filter

	A criterion is composed of field name, operation and value.
	Critera can be concatenated by '&&' and '||'.

	Valid operations are >=, >, ==, <, <= for numbers and IS, IS_NOT, CONTAINS, CONTAINS_NOT for strings.
*/
class CPPNGSSHARED_EXPORT VariantFilter
{
public:
	///Default constructor. Many containers need the elements to be default-constructible...
	VariantFilter();
	///Constructor.
	VariantFilter(const QString& name, const QString& criteria);

	///Return the filter name.
    const QString& name() const
    {
        return name_;
    }
	///Returns the filter criteria string.
    const QString& criteria() const
    {
        return criteria_;
    }

	///Checks if a variant passes a filter criterion.
	bool pass(const VariantList& list, int index) const;

	///Checks which variants from a variant list pass all filters.
	static QBitArray multiPass(const VariantList& list, const QVector<VariantFilter>& filters);

protected:
	///Checks if a variant passes a filter criterion using an annotation index cache for speedup.
	bool pass(const VariantList& list, int index, QHash<QString, int>& annotation_indices) const;
	///Checks if a filter criterion is valid. Throws a ArgumentException otherwise.
	void checkValid();

	QString name_;
	QString criteria_;
};

#endif // VARIANTFILTER_H
