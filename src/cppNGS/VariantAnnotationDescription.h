#ifndef VARIANTANNOTATIONDESCRIPTION_H
#define VARIANTANNOTATIONDESCRIPTION_H

#include "cppNGS_global.h"
#include <QString>
#include <QHash>

///stores properties of a annotation.

class CPPNGSSHARED_EXPORT VariantAnnotationDescription
{
public:
    ///Types (for VCF).
    enum AnnotationType {INTEGER,FLOAT,FLAG,CHARACTER,STRING};

    ///Constructor.
    VariantAnnotationDescription(const QString& name, const QString& description, AnnotationType type=STRING, bool sample_specific=false, QString number="1");

	///==Operator (two different INFO or FORMAT annotation can't have same ID in vcf)
	bool operator==(VariantAnnotationDescription b)
	{
		return ((this->name_==b.name_)&&(this->sample_specific_==b.sample_specific_));
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
    ///Returns if the annotion is sample-specific (for VCF).
    bool sampleSpecific() const
    {
        return sample_specific_;
    }
    ///Sets if the annotion is sample-specific (for VCF).
    void setSampleSpecific(bool sample_specific)
    {
        sample_specific_ = sample_specific;
    }
    ///Returns the number of values of the annotation (for VCF).
    const QString& number() const
    {
        return number_;
    }
    ///Sets number of values of the annotation (for VCF).
    void setNumber(const QString& number)
    {
        number_=number;
    }

protected:
    ///Name of the annotation (nearly unique, sample-specific and -independent annotations may have the same name).
    QString name_;
    ///Description of the annotation.
    QString description_;
    ///The annotation type (see above)
    AnnotationType type_;
    ///Information whether the annotation value is sample specific (e.g. read depth) or not (e.g. hapmap2 membership).
    bool sample_specific_;
    ///The number of values the annotation consists of, may be positive int (including "0"), ".", "A" or "G".
    QString number_;
};

#endif // VARIANTANNOTATIONDESCRIPTION_H
