#include "NGSHelper.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VariantFilter.h"
#include "BasicStatistics.h"

#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <cmath>

Pileup NGSHelper::getPileup(BamReader& reader, const Chromosome& chr, int pos, int indel_window, int min_mapq, bool anom, int min_baseq)
{
	//init
	Pileup output;
	int reads_mapped = 0;
	int reads_mapq0 = 0;

	//restrict region
	reader.setRegion(chr, pos, pos);

	//iterate through all alignments and create counts
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		if (!al.isProperPair() && anom==false) continue;
		if (al.isSecondaryAlignment()) continue;
		if (al.isDuplicate()) continue;
		if (al.isUnmapped()) continue;

		reads_mapped += 1;
		if (al.mappingQuality()==0) reads_mapq0 += 1;

		if (al.mappingQuality()<min_mapq) continue;

		//snps
		QPair<char, int> base = NGSHelper::extractBaseByCIGAR(al, pos);
		if (base.second>=min_baseq)
		{
			output.inc(base.first);
		}

		//indels
		if (indel_window>=0)
		{
			QVector<Sequence> indels;
			NGSHelper::extractIndelsByCIGAR(indels, al, pos, indel_window);
			output.addIndels(indels);
		}
	}

	output.setMapq0Frac((double)reads_mapq0 / reads_mapped);

	return output;
}

void NGSHelper::getPileups(QList<Pileup>& pileups, BamReader& reader, const Chromosome& chr, int start, int end, int min_mapq)
{
	//init empty pileups
	pileups.clear();
	pileups.reserve(end-start+1);
	for (int i=start; i<=end; ++i)
	{
		pileups.append(Pileup());
	}

	//restrict region
	reader.setRegion(chr, start, end);

	//iterate through all alignments and create counts
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		if (al.isDuplicate()) continue;
		if (!al.isProperPair()) continue;
		if (al.isSecondaryAlignment()) continue;
		if (al.isUnmapped() || al.mappingQuality()<min_mapq) continue;

		//look up indels
		int read_pos = 0;
		int genome_pos = al.start();
		const QList<CigarOp> cigar_data = al.cigarData();
		const QByteArray sequence = al.bases();
		foreach(const CigarOp& op, cigar_data)
		{
			//update positions
			if (op.Type==BAM_CMATCH)
			{
				for (int j=0; j<op.Length; ++j)
				{
					if (genome_pos>=start)
					{
						if (genome_pos>end) break;
						pileups[genome_pos-start].inc(sequence[read_pos]);
					}

					genome_pos += 1;
					read_pos += 1;
				}
			}
			else if(op.Type==BAM_CINS)
			{
				if (genome_pos>=start)
				{
					pileups[genome_pos-start].addIndel(QByteArray("+") + sequence.mid(read_pos, op.Length));
				}

				read_pos += op.Length;
			}
			else if(op.Type==BAM_CDEL)
			{
				if (genome_pos>=start)
				{
					pileups[genome_pos-start].addIndel("-" + QByteArray::number(op.Length));
				}
				for (int j=0; j<op.Length; ++j)
				{
					if (genome_pos>=start)
					{
						if (genome_pos>end) break;
						pileups[genome_pos-start].inc('-');
					}

					genome_pos += 1;
				}
			}
			else if(op.Type==BAM_CREF_SKIP) //skipped reference bases (for RNA)
			{
				genome_pos += op.Length;
			}
			else if(op.Type==BAM_CSOFT_CLIP) //soft-clipped (only at the beginning/end)
			{
				read_pos += op.Length;
			}
			else if(op.Type==BAM_CHARD_CLIP) //hard-clipped (only at the beginning/end)
			{
				//can be ignored as hard-clipped bases are not considered in the position or sequence
			}
			else
			{
				THROW(Exception, "Unknown CIGAR operation " + QString::number(op.Type) + "!");
			}

			//abort if we are right of the ROI
			if (genome_pos>end) break;
		}
	}
}

VariantDetails NGSHelper::getVariantDetails(BamReader& reader, const FastaFileIndex& reference, const Variant& variant)
{
	VariantDetails output;

	if (variant.isSNV()) //SVN
	{
		Pileup pileup = NGSHelper::getPileup(reader, variant.chr(), variant.start(), -1);
		output.depth = pileup.depth(true);
		if (output.depth!=0)
		{
			output.frequency = pileup.countOf(variant.obs()[0]) / (double)output.depth;
		}
		output.mapq0_frac = pileup.mapq0Frac();
	}
	else //indel
	{
		//determine region of interest for indel (important for repeat regions where more than one alignment is possible)
		QPair<int, int> reg = Variant::indelRegion(variant.chr(), variant.start(), variant.end(), variant.ref(), variant.obs(), reference);
		//qDebug() << "VAR:" << variant;
		//qDebug() << "REG:" << reg.first << reg.second;

		//get indels from region
		QVector<Sequence> indels;
		NGSHelper::getIndels(reference, reader, variant.chr(), reg.first-1, reg.second+1, indels, output.depth, output.mapq0_frac);
		//qDebug() << "INDELS:" << indels.join(" ");

		Variant variant_normalized = variant;
		variant_normalized.normalize("-");

		//count indels
		if (variant_normalized.ref()!="-" && variant_normalized.obs()!="-")
		{
			int c_ins = 0;
			int c_del = 0;
			foreach(const Sequence& indel, indels)
			{
				if (indel[0]=='+') ++c_ins;
				else if (indel[0]=='-') ++c_del;

			}
			output.frequency = std::min(c_ins, c_del);
		}
		else if (variant_normalized.ref()=="-")
		{
			output.frequency = indels.count("+" + variant_normalized.obs());
		}
		else
		{
			output.frequency = indels.count("-" + variant_normalized.ref());
		}

		//we might count more indels than depth because of the window - correct that
		output.frequency = std::min(1.0, output.frequency / output.depth);
	}

	return output;
}

void NGSHelper::getIndels(const FastaFileIndex& reference, BamReader& reader, const Chromosome& chr, int start, int end, QVector<Sequence>& indels, int& depth, double& mapq0_frac)
{
	//init
	indels.clear();
	depth = 0;
	int reads_mapped = 0;
	int reads_mapq0 = 0;

	//restrict region
	reader.setRegion(chr, start, end);

	//iterate through all alignments and create counts
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		//skip low-quality reads
		if (al.isDuplicate()) continue;
		if (!al.isProperPair()) continue;
		if (al.isSecondaryAlignment()) continue;
		if (al.isUnmapped()) continue;

		reads_mapped += 1;
		if (al.mappingQuality()==0)
		{
			reads_mapq0 += 1;
			continue;
		}

		//skip reads that do not span the whole region
		if (al.start()>start || al.end()<end ) continue;
		++depth;

		//run time optimization: skip reads that do not contain Indels
		bool contains_indels = false;
		const QList<CigarOp> cigar_data = al.cigarData();
		foreach(const CigarOp& op, cigar_data)
		{
			if (op.Type==BAM_CINS || op.Type==BAM_CDEL)
			{
				contains_indels = true;
				break;
			}
		}
		if (!contains_indels) continue;

		//look up indels
		int read_pos = 0;
		int genome_pos = al.start();
		const QList<CigarOp> cigar_data2 = al.cigarData();
		foreach(const CigarOp& op, cigar_data2)
		{
			//update positions
			if (op.Type==BAM_CMATCH)
			{
				genome_pos += op.Length;
				read_pos += op.Length;
			}
			else if(op.Type==BAM_CINS)
			{
				if (genome_pos>=start && genome_pos<=end)
				{
					indels.append(QByteArray("+") + al.bases().mid(read_pos, op.Length));
				}
				read_pos += op.Length;
			}
			else if(op.Type==BAM_CDEL)
			{
				if (genome_pos>=start && genome_pos<=end)
				{
					indels.append("-" + reference.seq(chr.str(), genome_pos, op.Length));
				}
				genome_pos += op.Length;
			}
			else if(op.Type==BAM_CREF_SKIP) //skipped reference bases (for RNA)
			{
				genome_pos += op.Length;
			}
			else if(op.Type==BAM_CSOFT_CLIP) //soft-clipped (only at the beginning/end)
			{
				read_pos += op.Length;
			}
			else if(op.Type==BAM_CHARD_CLIP) //hard-clipped (only at the beginning/end)
			{
				//can be ignored as hard-clipped bases are not considered in the position or sequence
			}
			else
			{
				THROW(Exception, "Unknown CIGAR operation " + QString::number(op.Type) + "!");
			}
		}
	}

	mapq0_frac = (double)reads_mapq0 / reads_mapped;
}

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

QPair<char, int> NGSHelper::extractBaseByCIGAR(const BamAlignment& al, int pos)
{
	int read_pos = 0;
	int genome_pos = al.start()-1;
	const QList<CigarOp> cigar_data = al.cigarData();
	foreach(const CigarOp& op, cigar_data)
	{
		//update positions
		if (op.Type==BAM_CMATCH)
		{
			genome_pos += op.Length;
			read_pos += op.Length;
		}
		else if(op.Type==BAM_CINS)
		{
			read_pos += op.Length;
		}
		else if(op.Type==BAM_CDEL)
		{
			genome_pos += op.Length;

			//base is deleted
			if (genome_pos>=pos) return qMakePair('-', 255);
		}
		else if(op.Type==BAM_CREF_SKIP) //skipped reference bases (for RNA)
		{
			genome_pos += op.Length;

			//base is skipped
			if (genome_pos>=pos) return qMakePair('~', -1);
		}
		else if(op.Type==BAM_CSOFT_CLIP) //soft-clipped (only at the beginning/end)
		{
			read_pos += op.Length;

			//base is soft-clipped
			//Nb: reads that are mapped in paired end mode and completely soft-clipped (e.g. 7I64S, 71S) keep their original left-most (genomic) position
			if(read_pos>=al.length())	return qMakePair('~', -1);
		}
		else if(op.Type==BAM_CHARD_CLIP) //hard-clipped (only at the beginning/end)
		{
			//can be ignored as hard-clipped bases are not considered in the position or sequence
		}
		else
		{
			THROW(Exception, "Unknown CIGAR operation " + QString::number(op.Type) + "!");
		}

		if (genome_pos>=pos)
		{
			int actual_pos = read_pos - (genome_pos + 1 - pos);
			return qMakePair(al.base(actual_pos), al.quality((actual_pos)));
		}
	}

	THROW(Exception, "Could not find position  " + QString::number(pos) + " in read " + al.bases() + " with start position " + QString::number(al.start()) + "!");
}

void NGSHelper::extractIndelsByCIGAR(QVector<Sequence>& indels, BamAlignment& al, int pos, int indel_window)
{
	//init
	bool use_window = (indel_window!=0);
	int window_start = pos - indel_window;
	int window_end = pos + indel_window;

	//look up indels
	int read_pos = 0;
	int genome_pos = al.start();
	const QList<CigarOp> cigar_data = al.cigarData();
	const QByteArray sequence = al.bases();
	foreach(const CigarOp& op, cigar_data)
	{
		//update positions
		if (op.Type==BAM_CMATCH) //match or mismatch
		{
			genome_pos += op.Length;
			read_pos += op.Length;
		}
		else if(op.Type==BAM_CINS) //insert
		{
			if ((!use_window && genome_pos==pos) || (use_window && genome_pos>=window_start && genome_pos<=window_end))
			{
				indels.append(QByteArray("+") + sequence.mid(read_pos, op.Length));
			}

			read_pos += op.Length;
		}
		else if(op.Type==BAM_CDEL) //deletion
		{
			if ((!use_window && genome_pos==pos) || (use_window && genome_pos>=window_start && genome_pos<=window_end))
			{
				indels.append("-" + QByteArray::number(op.Length));
			}
			genome_pos += op.Length;
		}
		else if(op.Type==BAM_CREF_SKIP) //skipped reference bases (for RNA)
		{
			genome_pos += op.Length;
		}
		else if(op.Type==BAM_CSOFT_CLIP) //soft-clipped (only at the beginning/end)
		{
			read_pos += op.Length;
		}
		else if(op.Type==BAM_CHARD_CLIP) //hard-clipped (only at the beginning/end)
		{
			//can be ignored as hard-clipped bases are not considered in the position or sequence
		}
		else
		{
			THROW(Exception, "Unknown CIGAR operation " + QString::number(op.Type) + "!");
		}

		//abort if we are behind the indel position
		if ((!use_window && genome_pos>pos) || (use_window && genome_pos>window_end)) break;
	}
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
