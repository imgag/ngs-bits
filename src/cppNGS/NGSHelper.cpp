#include "NGSHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VariantFilter.h"
#include "BasicStatistics.h"
#include "BamReader.h"

#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <cmath>

VariantList NGSHelper::getKnownVariants(bool only_snvs, double min_af, double max_af, const BedFile* roi)
{
	VariantList output;
	output.load(":/Resources/GRCh37_snps.vcf", VariantList::VCF, roi);

	//only SNVs
	if (only_snvs)
	{
		VariantFilter filter(output);
		for (int i=0; i<output.count(); ++i)
		{
			filter.flags()[i] = output[i].isSNV();
		}
		filter.removeFlagged();
	}

	//filter my min AF
	if (min_af<0.0 || min_af>1.0)
	{
		THROW(ArgumentException, "Minumum allele frequency out of range (0.0-1.0): " + QByteArray::number(min_af));
	}
	if (max_af<0.0 || max_af>1.0)
	{
		THROW(ArgumentException, "Maximum allele frequency out of range (0.0-1.0): " + QByteArray::number(max_af));
	}
	bool min_set = min_af>0.0;
	bool max_set = max_af<1.0;
	if (min_set || max_set)
	{
		VariantFilter filter(output);
		int i_af = output.annotationIndexByName("AF");
		for (int i=0; i<output.count(); ++i)
		{
			double af = output[i].annotations()[i_af].toDouble();
			filter.flags()[i] = (!min_set || af>min_af) && (!max_set || af<max_af);
		}
		filter.removeFlagged();
	}

	return output;
}

void NGSHelper::createSampleOverview(QStringList in, QString out, int indel_window, bool cols_auto, QStringList cols)
{
	//determine columns contained in all samples from file headers (keep order)
	if (cols_auto)
	{
		bool init = true;
		foreach(QString filename, in)
		{
			auto file = Helper::openFileForReading(filename, false);
			while (!file->atEnd())
			{
				QString line = file->readLine();
				if (!line.startsWith('#')) break;
				if (line.startsWith("#chr"))
				{
					if (init)
					{
						QStringList parts = line.trimmed().split('\t');
						foreach(QString part, parts)
						{
							//skip base columns
							if (part=="#chr" || part=="start" || part=="end" || part=="ref" || part=="obs") continue;

							//skip sample-specific germline columns
							if (part=="genotype" || part=="quality") continue;

							//skip sample-specific somatic columns
							if (part=="tumor_af" || part=="tumor_dp" || part=="normal_af" || part=="normal_dp")	continue;

							cols.append(part);
						}
						init = false;
					}
					else
					{
						QSet<QString> parts = line.trimmed().split('\t').toSet();
						for (int i=cols.count()-1; i>=0; --i)
						{
							if (!parts.contains(cols[i]))
							{
								cols.removeAt(i);
							}
						}
					}
				}
			}
			file->close();
		}
	}

	//load variant lists
	QVector<VariantList> vls;
	QVector<QVector<int> > vls_anno_indices;
	QList <VariantAnnotationDescription> vls_anno_descriptions;
	foreach(QString filename, in)
	{
		VariantList vl;
		vl.load(filename, VariantList::TSV);

		//check the all required fields are present in the input file
		QVector<int> anno_indices;
		foreach(QString col, cols)
		{
			if (col=="genotype") continue;
			int index = vl.annotationIndexByName(col, true, true);
			anno_indices.append(index);

			foreach(VariantAnnotationDescription vad, vl.annotationDescriptions())
			{
				if(vad.name()==col)
				{
					bool already_found = false;
					foreach(VariantAnnotationDescription vad2, vls_anno_descriptions)
					{
						if(vad2.name()==col) already_found = true;
					}

					if(!already_found) vls_anno_descriptions.append(vad);
				}
			}
		}

		vls_anno_indices.append(anno_indices);
		vls.append(vl);
	}

	//set up combined variant list (annotation and filter descriptions)
	VariantList vl_merged;
	foreach(const VariantList& vl, vls)
	{
		auto it = vl.filters().begin();
		while(it!=vl.filters().end())
		{
			if (!vl_merged.filters().contains(it.key()))
			{
				vl_merged.filters().insert(it.key(), it.value());
			}
			++it;
		}
	}
	foreach(int index, vls_anno_indices[0])
	{
		vl_merged.annotations().append(vls[0].annotations()[index]);
	}
	foreach(VariantAnnotationDescription vad, vls_anno_descriptions)
	{
		vl_merged.annotationDescriptions().append(vad);
	}

	//merge variants
	vl_merged.reserve(2 * vls[0].count());
	for (int i=0; i<vls.count(); ++i)
	{
		for(int j=0; j<vls[i].count(); ++j)
		{
			Variant v = vls[i][j];
			QList<QByteArray> annos = v.annotations();
			v.annotations().clear();
			foreach(int index, vls_anno_indices[i])
			{
				v.annotations().append(annos[index]);
			}
			vl_merged.append(v);
		}
	}

	//remove duplicates from variant list
	vl_merged.removeDuplicates(true);

	//append sample columns
	for (int i=0; i<vls.count(); ++i)
	{
		//get genotype index
		int geno_index = vls[i].annotationIndexByName("genotype", true, false);
		if(geno_index==-1)	geno_index = vls[i].annotationIndexByName("tumor_af", true, true);

		//add column header
		vl_merged.annotationDescriptions().append(VariantAnnotationDescription(QFileInfo(in[i]).baseName(), ""));
		vl_merged.annotations().append(VariantAnnotationHeader(QFileInfo(in[i]).baseName()));

		//create index over variant list to speed up the search
		const VariantList& vl = vls[i];
		ChromosomalIndex<VariantList> cidx(vl);

		//add sample-specific columns
		for (int j=0; j<vl_merged.count(); ++j)
		{
			Variant& v = vl_merged[j];
			QByteArray entry = "no";
			if (v.isSNV()) //SNP
			{
				QVector<int> matches = cidx.matchingIndices(v.chr(), v.start(), v.end());
				for (int k=0; k<matches.count(); ++k)
				{
					int match = matches[k];
					if (match!=-1 && vl[match].ref()==v.ref() && vl[match].obs()==v.obs())
					{
						entry = "yes (" + vl[match].annotations()[geno_index] + ")";
					}
				}
			}
			else //indel
			{
				QVector<int> matches = cidx.matchingIndices(v.chr(), v.start()-indel_window, v.end()+indel_window);
				if (matches.count()>0)
				{
					//exact match (start, obs, ref)
					bool done = false;
					for (int k=0; k<matches.count(); ++k)
					{
						const Variant& v2 = vl[matches[k]];
						if (!done && v2.start()==v.start() && v2.ref()==v.ref() && v2.obs()==v.obs())
						{
							entry = "yes (" + v2.annotations()[geno_index] + ")";
							done = true;
						}
					}

					//same indel nearby (ref, obs)
					for (int k=0; k<matches.count(); ++k)
					{
						const Variant& v2 = vl[matches[k]];
						if (!done && v2.ref()==v.ref() && v2.obs()==v.obs())
						{
							entry = "near (" + v2.annotations()[geno_index] + ")";
							done = true;
						}
					}

					//different indel nearby
					for (int k=0; k<matches.count(); ++k)
					{
						const Variant& v2 = vl[matches[k]];
						if (!done && !v2.isSNV())
						{
							entry = "different (" + v2.annotations()[geno_index] + ")";
							done = true;
						}
					}
				}
			}
			v.annotations().append(entry);
		}
	}

	vl_merged.store(out, VariantList::TSV);
}

SampleHeaderInfo NGSHelper::getSampleHeader(const VariantList& vl, QString gsvar_file)
{
	SampleHeaderInfo output;

	foreach(QString line, vl.comments())
	{
		line = line.trimmed();

		if (line.startsWith("##SAMPLE=<"))
		{
			auto parts = line.mid(10, line.length()-11).split(',');
			QString name;
			foreach(const QString& part, parts)
			{
				int sep_idx = part.indexOf('=');
				if (sep_idx==-1)
				{
					qDebug() << "Invalid sample header entry " << part << " in " << line;
					continue;
				}

				QString key = part.left(sep_idx);
				QString value = part.mid(sep_idx+1);
				if (key=="ID")
				{
					name = value;
					output[name].column_name = value;
				}
				else
				{
					output[name].properties[key] = value;
				}
			}
		}
	}

	//special handling of single-sample analysis
	for (int i=0; i<vl.annotations().count(); ++i)
	{
		if (vl.annotations()[i].name()=="genotype")
		{
			if (output.count()==0) //old single-sample analysis without '#SAMPLE header'. Old trio/somatic variant lists are no longer supported.
			{
				QString name = QFileInfo(gsvar_file).baseName();
				output[name].properties["Status"] = "Affected";
			}

			if (output.count()==1)
			{
				output.first().column_name = "genotype";
			}
		}
		break;
	}

	return output;
}


QByteArray NGSHelper::expandAminoAcidAbbreviation(QChar amino_acid_change_in)
{
	const static QHash<QChar,QByteArray> dictionary = {{'A',"Ala"},{'R',"Arg"},{'N',"Asn"},{'D',"Asp"},{'C',"Cys"},{'E',"Glu"},
		{'Q',"Gln"},{'G',"Gly"},{'H',"His"},{'I',"Ile"},{'L',"Leu"},{'K',"Lys"},{'M',"Met"},{'F',"Phe"},{'P',"Pro"},{'S',"Ser"},
		{'T',"Thr"},{'W',"Trp"},{'Y',"Tyr"},{'V',"Val"},{'*',"*"}};

	QByteArray amino_acid_change_out;
	if(dictionary.keys().contains(amino_acid_change_in))
	{
		amino_acid_change_out = dictionary.value(amino_acid_change_in);
	}
	else
	{
		amino_acid_change_out = "";
	}

	return amino_acid_change_out;
}

void NGSHelper::softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos)
{
	QList<CigarOp> old_CIGAR = al.cigarData();

	//backup old CIGAR string
	al.addTag("BS", 'Z', al.cigarDataAsString());

	//check preconditions
	if(start_ref_pos > end_ref_pos)
	{
		THROW(ToolFailedException, "End position is smaller than start position.");
	}
	if(start_ref_pos < al.start() || start_ref_pos > al.end())
	{
		THROW(ToolFailedException, "Start position " + QString::number(start_ref_pos) + " not within alignment (" + QString::number(al.start()) + ":" + QString::number(al.end()) + ").");
	}

	if(end_ref_pos < al.start() || end_ref_pos > al.end())
	{
		THROW(ToolFailedException, "End position " + QString::number(end_ref_pos) + " not within alignment (" + QString::number(al.start()) + ":" + QString::number(al.end()) + ").");
	}
	for(int i=0;i<old_CIGAR.size(); ++i)
	{
		if(old_CIGAR[i].Type!=BAM_CDEL && old_CIGAR[i].Type!=BAM_CSOFT_CLIP && old_CIGAR[i].Type!=BAM_CMATCH && old_CIGAR[i].Type!=BAM_CINS && old_CIGAR[i].Type!=BAM_CHARD_CLIP)
		{
			THROW(ToolFailedException, "Unsupported CIGAR type '" + QString(old_CIGAR[i].Type) + "'");
		}
	}

	//generate CIGAR char matrix from CIGAR
	QList<QPair<int,int>> matrix;
	for (int i=0; i<old_CIGAR.size(); ++i)
	{
		for(int j=0; j<old_CIGAR[i].Length; ++j)
		{
			matrix.append(qMakePair(old_CIGAR[i].Type, old_CIGAR[i].Type));
		}
	}

	//soft clip bases in matrix according to given ref_positions
	int j = 0;
	int current_ref_pos = al.start();
	while(current_ref_pos<=al.end())
	{
		if(j>=matrix.size())
		{
			THROW(ToolFailedException, "Index out of boundary!");
		}

		if(matrix[j].first!=BAM_CHARD_CLIP)
		{
			if(current_ref_pos>=start_ref_pos && current_ref_pos<=end_ref_pos)
			{
				matrix[j].second = BAM_CSOFT_CLIP;
			}
			if(matrix[j].first==BAM_CDEL || matrix[j].first==BAM_CMATCH)
			{
				++current_ref_pos;
			}
		}

		++j;
	}

	//summarize chars within matrix > generate new CIGAR string
	QList<CigarOp> new_CIGAR;
	int tmp_char = -1;
	int tmp_count = 0;
	for(int i=0; i<matrix.size(); ++i)
	{
		//skip soft-clipped deletions
		if(matrix[i].first==BAM_CDEL && matrix[i].second==BAM_CSOFT_CLIP) continue;

		if(matrix[i].second!=tmp_char)
		{
			if(tmp_char!=-1)
			{
				new_CIGAR.append(CigarOp {tmp_char, tmp_count});
			}

			tmp_char = matrix[i].second;
			tmp_count = 0;
		}
		++tmp_count;
	}
	new_CIGAR.append(CigarOp {tmp_char, tmp_count});

	//clean up cigar string; insertions and deletion around soft-clipped regions
	for(int i=1; i<new_CIGAR.size(); ++i)
	{
		bool redo = false;

		// 1. remove deleted bases around soft-clipped bases
		if(new_CIGAR[i-1].Type==BAM_CSOFT_CLIP && new_CIGAR[i].Type==BAM_CDEL)
		{
			new_CIGAR.erase(new_CIGAR.begin()+i);
			redo = true;
		}
		else if(new_CIGAR[i-1].Type==BAM_CDEL && new_CIGAR[i].Type==BAM_CSOFT_CLIP)
		{
			new_CIGAR.erase(new_CIGAR.begin()+(i-1));
			redo = true;
		}
		//2. remove inserted bases around soft-clipped bases
		else if(new_CIGAR[i-1].Type==BAM_CSOFT_CLIP && new_CIGAR[i].Type==BAM_CINS)
		{
			new_CIGAR[i-1].Length += new_CIGAR[i].Length;
			new_CIGAR.erase(new_CIGAR.begin()+i);
			redo = true;
		}
		else if(new_CIGAR[i-1].Type==BAM_CINS && new_CIGAR[i].Type==BAM_CSOFT_CLIP)
		{
			new_CIGAR[i].Length += new_CIGAR[i-1].Length;
			new_CIGAR.erase(new_CIGAR.begin()+(i-1));
			redo = true;
		}

		if(redo)
		{
			--i;
		}
	}

	//correct left-most position if first bases are soft-clipped, consider bases that were already softclipped previously
	int start_index = 0;
	while(matrix[start_index].second==BAM_CHARD_CLIP && start_index < matrix.size())
	{
		++start_index;
	}
	if(matrix[start_index].second==BAM_CSOFT_CLIP)
	{
		int offset = 0;
		while(start_index<matrix.size() && matrix[start_index].second==BAM_CSOFT_CLIP)
		{
			if(matrix[start_index].first==BAM_CMATCH|| matrix[start_index].first==BAM_CDEL)
			{
				++offset;
			}
			++start_index;
		}
		al.setStart(al.start() + offset);
	}

	al.setCigarData(new_CIGAR);
}

QByteArray NGSHelper::changeSeq(const QByteArray& seq, bool rev, bool comp)
{
	QByteArray output(seq);

	if (rev)
	{
		std::reverse(output.begin(), output.end());
	}

	if (comp)
	{
		for (int i=0; i<output.count(); ++i)
		{
			switch(output.at(i))
			{
				case 'A':
					output[i] = 'T';
					break;
				case 'C':
					output[i] = 'G';
					break;
				case 'T':
					output[i] = 'A';
					break;
				case 'G':
					output[i] = 'C';
					break;
				case 'N':
					output[i] = 'N';
					break;
				default:
					THROW(ProgrammingException, "Could not convert base " + QString(seq.at(i)) + " to complement!");
			}
		}
	}

	return output;
}

char NGSHelper::complement(char base)
{
	switch(base)
	{
		case 'A':
			return 'T';
		case 'C':
			return 'G';
		case 'T':
			return 'A';
		case 'G':
			return 'C';
		case 'N':
			return 'N';
		default:
			THROW(ProgrammingException, "Could not convert base " + QString(base) + " to complement!");
	}
}


bool SampleInfo::isAffected() const
{
	auto it = properties.cbegin();
	while(it != properties.cend())
	{
		//support for old and new disease status annotations
		if ((it.key().toLower()=="diseasestatus" || it.key().toLower()=="status") && it.value().toLower()=="affected")
		{
			return true;
		}

		++it;
	}

	return false;
}


QStringList SampleHeaderInfo::sampleColumns() const
{
	QStringList output;
	foreach(const SampleInfo& info, *this)
	{
		output << info.column_name;
	}

	return output;
}

QStringList SampleHeaderInfo::sampleColumns(bool affected) const
{
	QStringList output;
	foreach(const SampleInfo& info, *this)
	{
		if (affected==info.isAffected())
		{
			output << info.column_name;
		}
	}

	return output;
}
