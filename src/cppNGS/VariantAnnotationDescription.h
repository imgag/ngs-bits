#ifndef VARIANTANNOTATIONDESCRIPTION_H
#define VARIANTANNOTATIONDESCRIPTION_H

#include "cppNGS_global.h"
#include <QString>
#include <QHash>
#include <QSharedPointer>

///stores properties of a annotation.

class CPPNGSSHARED_EXPORT VariantAnnotationDescription
{
public:
	///Types (for VCF).
	enum AnnotationType {INTEGER,FLOAT,FLAG,CHARACTER,STRING};

	///Constructor.
	VariantAnnotationDescription();
	VariantAnnotationDescription(const QString& name, const QString& description, AnnotationType type=STRING);

	///==Operator (two different INFO or FORMAT annotation can't have same ID in vcf)
	bool operator==(VariantAnnotationDescription b)
	{
		return (this->name_==b.name_);
	}
	///Returns the name of the annotation.
	const QString& name() const
	{
		return name_;
	}
	///Sets the name of the annotation.
	void setName(const QString& name)
	{
		name_ = name;
	}
	///Returns the description of the annotation.
	const QString& description() const
	{
		return description_;
	}
	///Sets the description of the annotation.
	void setDescription(const QString& description)
	{
		description_ = description;
	}

	///Returns the type of the annotation.
	AnnotationType type() const
	{
		return type_;
	}
	///Sets the type of the annotation.
	void setType(AnnotationType type)
	{
		type_ = type;
	}

protected:
	///Name of the annotation (nearly unique, sample-specific and -independent annotations may have the same name).
	QString name_;
	///Description of the annotation.
	QString description_;
	///The annotation type (see above)
	AnnotationType type_;
};

class CPPNGSSHARED_EXPORT VariantAnnotationHeader
{
public:
	VariantAnnotationHeader(const QString& name);

	///==Operator (two different INFO or FORMAT annotation can't have same ID in vcf)
	bool operator==(VariantAnnotationHeader b)
	{
		return (this->name_==b.name_);
	}

	const QString& name() const
	{
		return name_;
	}

	void setName(const QString& name)
	{
		name_=name;
	}

	const VariantAnnotationDescription& description() const
	{
		return *description_;
	}

	void setDescription(VariantAnnotationDescription& description)
	{
		description_ = QSharedPointer<VariantAnnotationDescription>(&description);
	}

protected:
	QString name_;
	QSharedPointer<VariantAnnotationDescription> description_;

};

#endif // VARIANTANNOTATIONDESCRIPTION_H
