#pragma once

#include "Statistics.h"
#include "unordered_map"
#include "unordered_set"
#include <QFileInfo>

enum Member
	{
	FATHER,
	MOTHER,
	CHILD
	};
struct EnumHash
{
	template <typename T>
	std::size_t	operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

namespace std
{
template<>
struct hash<const VcfFormat::VCFLine>
{
	size_t
	operator()(const VcfFormat::VCFLine & obj) const
	{
		return hash<string>()(obj.variantToString().toStdString());
	}
};

template<>
struct hash<VcfFormat::VCFLine>
{
	size_t
	operator()(const VcfFormat::VCFLine & obj) const
	{
		return hash<string>()(obj.variantToString().toStdString());
	}
};
}

struct VariantInfo
{
	QString in_file_name;
	std::unordered_map<const VcfFormat::VCFLine, double> variants;

	VariantInfo(QString in_file_name_)
	{
		in_file_name = in_file_name_;
	}
};

struct VariantInheritance
{
	double percentOfMotherToChild = 0;
	double percentOfFatherToChild = 0;
};
