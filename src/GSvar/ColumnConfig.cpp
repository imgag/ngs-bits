#include "ColumnConfig.h"
#include "Exceptions.h"
#include "GUIHelper.h"

ColumnConfig::ColumnConfig()
	: columns_()
	, infos_()
{
}

void ColumnConfig::append(QString name, const ColumnInfo& info)
{
	name = name.trimmed();
	//check
	if (name.isEmpty()) THROW(ArgumentException, "Column '" + name + "' is not valid in column config!");
	if (contains(name)) THROW(ArgumentException, "Column '" + name + "' already contained in column config!");
	if (info.min_width<0) THROW(ArgumentException, "Minimum width for '" + name + "' is invalid: " + QString::number(info.min_width));
	if (info.max_width<0) THROW(ArgumentException, "Maximum width for '" + name + "' is invalid: " + QString::number(info.max_width));

	//add
	columns_ << name;
	infos_.insert(name, info);
}

void ColumnConfig::getOrder(const VariantList& variants, QStringList& col_order, QList<int>& anno_index_order)
{
	QStringList cols_gt;
	foreach(SampleInfo info, variants.getSampleHeader(false))
	{
		cols_gt << info.name;
	}
	AnalysisType type = variants.type();
	if (type==SOMATIC_PAIR || type==SOMATIC_SINGLESAMPLE || type==CFDNA)
	{
		cols_gt << "tumor_af" << "tumor_dp" << "normal_af" << "normal_dp";
	}

	//check if columns are contained in this config
	QHash<QString, int> col2index;
	QSet<QString> contained;
	QStringList not_contained;
	for (int i=0; i<variants.annotations().count(); ++i)
	{
		QString col = variants.annotations()[i].name();
		col2index[col] = i;

		//skip genotype cols
		if (cols_gt.contains(col)) continue;

		if (!infos_.contains(col)) not_contained << col;
		else contained << col;
	}

	//determine column order
	col_order.clear();
	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		col_order.append(cols_gt);
	}
	else if (type==SOMATIC_PAIR)
	{
		col_order << "tumor_af" << "tumor_dp" << "normal_af" << "normal_dp";
	}
	else if (type==SOMATIC_SINGLESAMPLE || type==CFDNA)
	{
		col_order << "tumor_af" << "tumor_dp";
	}
	foreach(const QString& col, columns_)
	{
		if (contained.contains(col)) col_order << col;
	}
	col_order.append(not_contained);

	//determine index order
	anno_index_order.clear();
	foreach(const QString& col, col_order)
	{
		if (col2index.contains(col)) anno_index_order << col2index[col];;
	}
}

void ColumnConfig::getOrder(const CnvList& cnvs, QStringList& col_order, QList<int>& anno_index_order)
{
	//check if columns are contained in this config
	QHash<QString, int> col2index;
	QSet<QString> contained;
	QStringList not_contained;
	for (int i=0; i<cnvs.annotationHeaders().count(); ++i)
	{
		QString col = cnvs.annotationHeaders()[i];
		col2index[col] = i;

		if (!infos_.contains(col)) not_contained << col;
		else contained << col;
	}

	//determine column order
	col_order.clear();
	foreach(const QString& col, columns_)
	{
		if (contained.contains(col)) col_order << col;
	}
	col_order.append(not_contained);

	//determine index order
	anno_index_order.clear();
	foreach(const QString& col, col_order)
	{
		if (col2index.contains(col)) anno_index_order << col2index[col];;
	}
}

void ColumnConfig::getOrder(const BedpeFile& svs, QStringList& col_order, QList<int>& anno_index_order)
{
	QStringList cols_gt;
	foreach(const SampleInfo& info, svs.sampleHeaderInfo())
	{
		cols_gt << info.name;
	}

	//check if columns are contained in this config
	QHash<QString, int> col2index;
	QSet<QString> contained;
	QStringList not_contained;
	for (int i=0; i<svs.annotationHeaders().count(); ++i)
	{
		QString col = svs.annotationHeaders()[i];
		if(col.startsWith("STRAND_") || col.startsWith("NAME_") || col=="ID" || col=="FORMAT" || col=="INFO_A" || col=="INFO_B") continue;

		//skip genotype cols
		if (cols_gt.contains(col)) continue;

		col2index[col] = i;

		if (!infos_.contains(col)) not_contained << col;
		else contained << col;
	}

	//determine column order
	col_order.clear();
	foreach(const QString& col, columns_)
	{
		if (contained.contains(col)) col_order << col;
	}
	col_order.append(not_contained);

	//determine index order
	anno_index_order.clear();
	foreach(const QString& col, col_order)
	{
		if (col2index.contains(col)) anno_index_order << col2index[col];;
	}
}

void ColumnConfig::applyColumnWidths(QTableWidget* table, int max_width_for_not_contained)
{
	//general resize
	GUIHelper::resizeTableCellWidths(table, -1, 1000);
	GUIHelper::resizeTableCellHeightsToMinimum(table);

	//apply column settings
	for (int i=0; i<table->columnCount(); ++i)
	{
		int width = table->columnWidth(i);
		QString col = table->horizontalHeaderItem(i)->text();
		if (contains(col))
		{
			ColumnInfo info = this->info(col);
			if (width<info.min_width)
			{
				table->setColumnWidth(i, info.min_width);
			}
			if (info.max_width>0 && width>info.max_width)
			{
				table->setColumnWidth(i, info.max_width);
			}
		}
		else
		{
			if (width>max_width_for_not_contained)
			{
				table->setColumnWidth(i, max_width_for_not_contained);
			}
		}
	}
}

void ColumnConfig::applyHidden(QTableWidget* table)
{
	for (int c=0; c<table->columnCount(); ++c)
	{
		QString col = table->horizontalHeaderItem(c)->text();
		if (contains(col) && infos_[col].hidden)
		{
			table->setColumnHidden(c, true);
		}
	}
}


QString ColumnConfig::toString() const
{
	QString output;

	foreach(const QString& name, columns_)
	{
		if (!output.isEmpty()) output.append("\t");
		output.append(name);
		output.append("|");
		const ColumnInfo& info = infos_[name];
		output.append(QString::number(info.min_width));
		output.append("|");
		output.append(QString::number(info.max_width));
		output.append("|");
		output.append(info.hidden ? "hidden" : "visible");
	}

	return output;
}

ColumnConfig ColumnConfig::fromString(const QString& text)
{
	ColumnConfig output;
	if (text.trimmed().isEmpty()) return output;

	QStringList parts = text.split("\t");
	foreach(const QString& part, parts)
	{
		QStringList values = part.split("|");
		if (values.count()!=4) THROW(ArgumentException, "Column config text contains invalid column info part '" + part + "'!");
		ColumnInfo info;
		info.min_width = values[1].toInt();
		info.max_width = values[2].toInt();
		info.hidden = values[3].trimmed()=="hidden";
		output.append(values[0], info);
	}

	return output;
}
