#pragma once

#include "Statistics.h"
#include "unordered_set"
#include "unordered_map"
#include <QFileInfo>

enum Member
{
	MOTHER,
	FATHER,
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
struct VariantHeritage
{
	int exclusiveVariantsOfMother = 0;
	int exclusiveVariantsOfFather = 0;
	int commonVariants = 0;
	int newVariants = 0;
};

namespace std
{
  template<>
	struct hash<const Variant>
	{
	  size_t
	  operator()(const Variant & obj) const
	  {
		return hash<string>()(obj.toString().toStdString());
	  }
	};
}

struct VariantInfo
{
	QString in_file_name;
	QString out_file_name;
	std::unordered_map<const Variant, double> variants;

	VariantInfo(QString in_file_name_, QString out_file_name_)
	{
		in_file_name = in_file_name_;
		out_file_name = out_file_name_;
	}

	void writeData()
	{
		//variants.store(out_file_name);
	}
};

