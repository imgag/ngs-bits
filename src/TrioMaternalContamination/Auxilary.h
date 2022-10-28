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
struct hash<const  VcfLine>
{
	size_t
	operator()(const  VcfLine & obj) const
	{
		return hash<string>()(obj.toString().toStdString());
	}
};

template<>
struct hash< VcfLine>
{
	size_t
	operator()(const  VcfLine & obj) const
	{
		return hash<string>()(obj.toString().toStdString());
	}
};
}

struct VariantInfo
{
	QString in_file_name;
	std::unordered_map<const  VcfLine, double> variants;

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
