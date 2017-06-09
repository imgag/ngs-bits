#include "VariantFilter.h"
#include "ChromosomalIndex.h"
#include "Exceptions.h"
#include "VariantList.h"
#include "GeneSet.h"

VariantFilter::VariantFilter(VariantList& vl)
	: variants(vl)
{
	clear();
}

void VariantFilter::flagByAllelFrequency(double max_af)
{
	//check input
	if (max_af<0 || max_af>1)
	{
		THROW(ArgumentException, "Invalid MAF '" + QString::number(max_af) + "'. Must be between 0 and 1!");
	}

	//get column indices
	int i_1000g = variants.annotationIndexByName("1000g", true, true);
	int i_exac = variants.annotationIndexByName("ExAC", true, true);
	int i_kaviar = variants.annotationIndexByName("Kaviar", true, true);

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		pass[i] = pass[i]
				&& variants[i].annotations()[i_1000g].toDouble()<=max_af
				&& variants[i].annotations()[i_exac].toDouble()<=max_af
				&& variants[i].annotations()[i_kaviar].toDouble()<=max_af;
	}
}

void VariantFilter::flagByImpact(QStringList impacts)
{
	//check input
	QStringList valid;
	valid << "HIGH" << "MODERATE" << "LOW" << "MODIFIER";
	foreach(QString impact, impacts)
	{
		if (!valid.contains(impact))
		{
			THROW(ArgumentException, "Invalid SnpEff impact '" + impact + "'. Valid are:" + valid.join(", "));
		}
	}

	//get column indices
	int i_co_sp = variants.annotationIndexByName("coding_and_splicing", true, true);

	//prepare impacts list
	QList<QByteArray> impacts2;
	foreach(const QString& impact, impacts)
	{
		impacts2.append(":" + impact.toLatin1() + ":");
	}

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		bool pass_impact = false;
		foreach(const QByteArray& impact, impacts2)
		{
			if (variants[i].annotations()[i_co_sp].contains(impact))
			{
				pass_impact = true;
				break;
			}
		}
		pass[i] = pass_impact;
	}
}

void VariantFilter::flagByGenotype(QString genotype, bool invert, QStringList genotype_columns)
{
	//check input
	if (genotype!="hom" && genotype!="het" && genotype!="wt") THROW(ArgumentException, "Invalid genotype '" + genotype + "'!");

	//get column indices
	QList<int> geno_indices;
	foreach(const QString& column, genotype_columns)
	{
		geno_indices << variants.annotationIndexByName(column, true, true);
	}

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		if (invert)
		{
			foreach(int index, geno_indices)
			{
				if (variants[i].annotations()[index] == genotype)
				{
					pass[i] = false;
					break;
				}
			}
		}
		else
		{
			foreach(int index, geno_indices)
			{
				if (variants[i].annotations()[index] != genotype)
				{
					pass[i] = false;
					break;
				}
			}
		}
	}
}

void VariantFilter::flagByIHDB(int max_count, QStringList genotype_columns)
{
	//check input
	if (max_count<1)
	{
		THROW(ArgumentException, "Invalid ihdb count '" + QString::number(max_count) + "'. Must be be positive!");
	}

	//get column indices
	int i_ihdb_hom = variants.annotationIndexByName("ihdb_allsys_hom", true, true);
	int i_ihdb_het = variants.annotationIndexByName("ihdb_allsys_het", true, true);
	QList<int> geno_indices;
	foreach(const QString& column, genotype_columns)
	{
		geno_indices << variants.annotationIndexByName(column, true, true);
	}

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		bool var_is_hom = false;
		foreach(int index, geno_indices)
		{
			const QByteArray& var_geno = variants[i].annotations()[index];
			if (var_geno=="hom")
			{
				var_is_hom = true;
				break;
			}
			else if (var_geno!="het" && var_geno!="wt")
			{
				THROW(ProgrammingException, "Unknown genotype '" + var_geno + "'!");
			}
		}

		pass[i] = (variants[i].annotations()[i_ihdb_hom].toInt() + (var_is_hom ? 0 : variants[i].annotations()[i_ihdb_het].toInt())) <= max_count;
	}
}

void VariantFilter::flagByFilterColumn()
{
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		pass[i] = variants[i].filters().isEmpty();
	}
}

void VariantFilter::flagByFilterColumnMatching(QStringList remove)
{
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		foreach(const QByteArray& f,  variants[i].filters())
		{
			if (remove.contains(f))
			{
				pass[i] = false;
				break;
			}
		}
	}
}

void VariantFilter::flagByClassification(int min_class)
{
	//check input
	if (min_class<1 || min_class>5)
	{
		THROW(ArgumentException, "Invalid classification '" + QString::number(min_class) + "'!");
	}

	//get column indices
	int i_class = variants.annotationIndexByName("classification", true, true);

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		bool ok = false;
		int classification_value = variants[i].annotations()[i_class].toInt(&ok);
		if (!ok) continue;

		pass[i] = (classification_value>=min_class);
	}
}

void VariantFilter::flagByGenes(const GeneSet& genes)
{
	//get column indices
	int i_gene = variants.annotationIndexByName("gene", true, true);

	//filter
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		bool contained = false;
		QList<QByteArray> genes_variant = variants[i].annotations()[i_gene].split(',');
		foreach(const QByteArray& gene, genes_variant)
		{
			if (genes.contains(gene))
			{
				contained = true;
				break;
			}
		}
		pass[i] = contained;
	}
}

void VariantFilter::flagByRegions(const BedFile& regions)
{
	//check regions
	if(!regions.isMergedAndSorted())
	{
		THROW(ArgumentException, "Cannot filter variant list by regions that are not merged/sorted!");
	}

	//create region index
	ChromosomalIndex<BedFile> regions_idx(regions);

	//filter
	for (int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		const Variant& v = variants[i];
		int index = regions_idx.matchingIndex(v.chr(), v.start(), v.end());
		pass[i] = (index!=-1);
	}
}

void VariantFilter::flagByRegion(const BedLine& region)
{
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		pass[i] = variants[i].overlapsWith(region);
	}
}

void VariantFilter::flagGeneric(QString criteria)
{
	//check that criteria are valid
	QStringList o_crits = criteria.split("||", QString::SkipEmptyParts);
	foreach(const QString& o_crit, o_crits)
	{
		//OR parts
		QStringList a_crits = o_crit.split("&&", QString::SkipEmptyParts);
		foreach(const QString& a_crit, a_crits)
		{
			//split single criterion (field, op, value)
			QStringList parts = a_crit.split(" ", QString::SkipEmptyParts);
			if (parts.count()<2)
			{
				//qDebug() << __LINE__;
				THROW(ArgumentException, "Invalid variant filter criterion '" + a_crit + "'. It must contain at lease field name, operation and value!");
			}

			//value may be empty
			if (parts.count()<3) parts.append("");

			//check field value
			QString field_name = parts[0];
			if (QRegExp("a-zA-Z0-9_-").exactMatch(field_name))
			{
				//qDebug() << __LINE__ << field_name;
				THROW(ArgumentException, "Invalid field name '" + field_name + "' in filter criterion: '" + a_crit + "'");
			}

			//operations
			QString op = parts[1];
			if (op==">=" || op==">" || op=="==" || op=="<" || op=="<=")
			{
				bool ok = false;
				QStringList(parts.mid(2)).join(' ').toDouble(&ok);
				if (!ok)
				{
					//qDebug() << __LINE__ << QStringList(parts.mid(2)).join(' ');
					THROW(ArgumentException, "Invalid filter value in NUMERIC filter criterion: '" + a_crit + "'");
				}
			}
			else if (op=="IS" || op=="IS_NOT" || op=="CONTAINS" || op=="CONTAINS_NOT")
			{
				//nothing to check for strings
			}
			else
			{
				//qDebug() << __LINE__ << op;
				THROW(ArgumentException, "Invalid filter operation '" + op + "' in filter criterion: '" + a_crit + "'");
			}
		}
	}

	//init
	QHash<QString, int> anno_index_cache;

	for (int i=0; i<variants.count(); ++i)
	{
		const Variant& variant = variants[i];

		bool result = false;

		//AND parts
		QStringList o_crits = criteria.split("||", QString::SkipEmptyParts);
		foreach(const QString& o_crit, o_crits)
		{
			//OR parts
			bool and_result = true;
			QStringList a_crits = o_crit.split("&&", QString::SkipEmptyParts);
			foreach(const QString& a_crit, a_crits)
			{
				QStringList parts = a_crit.split(" ", QString::SkipEmptyParts);

				//check field value
				QString field_name = parts[0];
				QString field_string = "";
				if (anno_index_cache.contains(field_name))
				{
					field_string = variant.annotations()[anno_index_cache[field_name]];
				}
				else if (field_name=="chr")
				{
					field_string = variant.chr().str();
				}
				else if (field_name=="start")
				{
					field_string = QString::number(variant.start());
				}
				else if (field_name=="end")
				{
					field_string = QString::number(variant.end());
				}
				else if (field_name=="ref")
				{
					field_string = variant.ref();
				}
				else if (field_name=="obs")
				{
					field_string = variant.obs();
				}
				else if (field_name.startsWith("*") && field_name.endsWith("*"))
				{
					field_name = field_name.mid(1, field_name.count()-2);
					int anno_index = variants.annotationIndexByName(field_name, NULL, false, true);
					field_string = variant.annotations()[anno_index];
					anno_index_cache.insert("*" + field_name + "*", anno_index);
				}
				else
				{
					int anno_index = variants.annotationIndexByName(field_name, NULL, true, true);
					field_string = variant.annotations()[anno_index];
					anno_index_cache.insert(field_name, anno_index);
				}

				//operations
				bool single_result = true;
				QString op = parts[1];
				QString op_string = QStringList(parts.mid(2)).join(' ');
				if (op==">=" || op==">" || op=="==" || op=="<" || op=="<=")
				{
					double op_value = op_string.toDouble();
					bool ok = true;
					double field_value = field_string.toDouble(&ok);
					if (!ok) THROW(ArgumentException, "Invalid variant annotation in field '" + field_string + "' for NUMERIC filter criterion '" + a_crit + "'");
					if (op==">=") single_result = (field_value>=op_value);
					if (op==">") single_result = (field_value>op_value);
					if (op=="==") single_result = (field_value==op_value);
					if (op=="<") single_result = (field_value<op_value);
					if (op=="<=") single_result = (field_value<=op_value);
				}
				else if (op=="IS")
				{
					single_result = (field_string==op_string);
				}
				else if (op=="IS_NOT")
				{
					single_result = (field_string!=op_string);
				}
				else if (op=="CONTAINS")
				{
					single_result = (field_string.contains(op_string));
				}
				else if (op=="CONTAINS_NOT")
				{
					single_result = (!field_string.contains(op_string));
				}

				if (!single_result)
				{
					and_result = false;
					break;
				}
			}

			if (and_result)
			{
				result = true;
				break;
			}
		}

		pass[i] =  result;
	}
}

void VariantFilter::flagCompoundHeterozygous(QStringList genotype_columns, bool hom_also_passes)
{
	//get column indices
	int i_gene = variants.annotationIndexByName("gene", true, true);
	QList<int> geno_indices;
	foreach(const QString& column, genotype_columns)
	{
		geno_indices << variants.annotationIndexByName(column, true, true);
	}

	//get set of homozygous variant indices
	QSet<int> hom_indices;
	if (hom_also_passes)
	{
		for(int i=0; i<variants.count(); ++i)
		{
			if (!pass[i]) continue;

			bool var_hom = true;
			foreach(int index, geno_indices)
			{
				if (variants[i].annotations()[index]!="hom")
				{
					var_hom = false;
					break;
				}
			}

			if(var_hom)
			{
				hom_indices.insert(i);
			}
		}
	}

	//count heterozygous passing variants per gene
	QHash<QByteArray, int> gene_to_het;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;

		bool var_het = true;
		foreach(int index, geno_indices)
		{
			if (variants[i].annotations()[index]!="het")
			{
				var_het = false;
				break;
			}
		}

		if (var_het)
		{
			QList<QByteArray> genes = variants[i].annotations()[i_gene].toUpper().split(',');
			foreach(const QByteArray& gene, genes)
			{
				gene_to_het[gene.trimmed()] += 1;
			}

		}
		else if (!hom_also_passes || !hom_indices.contains(i))
		{
			pass[i] = false;
		}
	}

	//filter out variants of genes with less than two heterozygous variants
	for(int i=0; i<variants.count(); ++i)
	{
		if (!pass[i]) continue;
		if (hom_also_passes && hom_indices.contains(i)) continue;

		pass[i] = false;
		QList<QByteArray> genes = variants[i].annotations()[i_gene].toUpper().split(',');
		foreach(const QByteArray& gene, genes)
		{
			if (gene_to_het[gene.trimmed()]>=2)
			{
				pass[i] = true;
				break;
			}
		}
	}
}

void VariantFilter::clear()
{
	pass = QBitArray(variants.count(), true);
}

void VariantFilter::removeFlagged()
{
	//move passing variant to the front of the variant list
	int to_index = 0;
	for (int i=0; i<variants.count(); ++i)
	{
		if (pass[i])
		{
			if (to_index!=i)
			{
				variants[to_index] = variants[i];
			}
			++to_index;
		}
	}

	//resize to new size
	variants.resize(to_index);

	//re-init flags in case filtering goes on
	clear();
}

void VariantFilter::tagFlagged(QByteArray tag, QByteArray description)
{
	//create 'filter' column (if missing)
	int index = variants.annotationIndexByName("filter", true, false);
	if (index==-1)
	{
		index = variants.addAnnotation("filter", "Filter column.");
	}

	//add tag description (if missing)
	if (!variants.filters().contains(tag))
	{
		variants.filters().insert(tag, description);
	}

	//tag variants that did not pass
	for (int i=0; i<variants.count(); ++i)
	{
		if (!pass[i])
		{
			variants[i].addFilter(tag, index);
		}
	}

	//re-init flags in case filtering goes on
	clear();
}
