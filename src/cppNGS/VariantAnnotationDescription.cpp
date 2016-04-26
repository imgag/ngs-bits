#include "VariantAnnotationDescription.h"

VariantAnnotationDescription::VariantAnnotationDescription()
	: name_()
	, description_()
	, type_()
	, sample_specific_()
	, number_()
	, print_ ()
{
}

VariantAnnotationDescription::VariantAnnotationDescription(const QString& name, const QString& description, AnnotationType type, bool sample_specific, QString number, bool print)
    : name_(name)
	, description_(description)
	, type_(type)
    , sample_specific_(sample_specific)
	, number_(number)
	, print_ (print)
{
}

VariantAnnotationHeader::VariantAnnotationHeader(const QString& name)
	: name_(name)
	, sample_id_()
{
}

VariantAnnotationHeader::VariantAnnotationHeader(const QString& name, const QString& sample_id)
	: name_(name)
	, sample_id_(sample_id)
{
}
