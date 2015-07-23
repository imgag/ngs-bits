#include "VariantAnnotationDescription.h"

VariantAnnotationDescription::VariantAnnotationDescription(const QString& name, const QString& description, AnnotationType type, bool sample_specific, QString number)
    : name_(name)
	, description_(description)
	, type_(type)
    , sample_specific_(sample_specific)
	, number_(number)
{
}
