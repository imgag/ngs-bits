#pragma once

#include "Statistics.h"
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

struct VariantInfo
{
	VariantList variants;

	QString in_file_name;
	QString out_file_name;

	VariantInfo(QString in_file_name_, QString out_file_name_)
	{
		in_file_name = in_file_name_;
		out_file_name = out_file_name_;
	}

	void writeData()
	{
		variants.store(out_file_name);
	}
};

