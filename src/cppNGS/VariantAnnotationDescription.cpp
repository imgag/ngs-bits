#include "VariantAnnotationDescription.h"

VariantAnnotationDescription::VariantAnnotationDescription()
	: name_()
	, description_()
	, type_()
{
}

VariantAnnotationDescription::VariantAnnotationDescription(const QString& name, const QString& description, AnnotationType type)
    : name_(name)
	, description_(description)
	, type_(type)
{
}

VariantAnnotationHeader::VariantAnnotationHeader(const QString& name)
	: name_(name)
{
}
