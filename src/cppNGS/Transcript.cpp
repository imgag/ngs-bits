#include "Transcript.h"
#include "Exceptions.h"
#include "NGSHelper.h"
#include <QRegularExpression>

Transcript::Transcript()
	: version_(-1)
	, strand_(INVALID)
	, start_(-1)
	, end_(-1)
	, is_preferred_transcript_(false)
	, is_mane_select_(false)
	, is_mane_plus_clinical_(false)
	, coding_start_(0)
	, coding_end_(0)
{
}

QStringList Transcript::flags(bool add_square_brackets) const
{
	QStringList output;

	if (isPreferredTranscript()) output += "NGSD preferred transcript";
	if (isGencodeBasicTranscript()) output << "GENCODE basic";
	if (isEnsemblCanonicalTranscript()) output << "Ensembl canonical";
	if (isManeSelectTranscript()) output << "MANE select";
	if (isManePlusClinicalTranscript()) output << "MANE plus clinical";

	if (add_square_brackets)
	{
		for(int i=0; i<output.count(); ++i)
		{
			output[i] =  "[" + output[i] + "]";
		}
	}

	return output;
}

void Transcript::setRegions(const BedFile& regions, int coding_start, int coding_end)
{
	//check
	if (strand_==INVALID) THROW(ProgrammingException, "Transcript::setRegions must be called after Transcript::setStrand!");
	if (regions.count()==0) THROW(ProgrammingException, "Transcript '" + name_ + "' must have at least one exon!");

	//determine chr/start/end and perform more checks
	start_ = std::numeric_limits<int>::max();
	end_ = -1;
	for (int i=0; i<regions.count(); ++i)
	{
		const BedLine& line = regions[i];
		start_ = std::min(start_, line.start());
		end_ = std::max(end_, line.end());
		if(i==0)
		{
			chr_ = line.chr();
		}
		else
		{
			if (line.chr()!=chr_) THROW(ArgumentException, "Transcript regions must be on one chromosome!");
			if (regions[i-1].end()>=line.start()) THROW(ArgumentException, "Transcript regions are not sorted/merged properly!");
		}
	}

	//init
	regions_ = regions;
	coding_start_ = coding_start;
	coding_end_ = coding_end;
	coding_regions_.clear();

	//check if coding
	if (!isCoding()) return;
	if (strand_==PLUS && coding_start>coding_end) THROW(ProgrammingException, "Coding start position must be smaller than coding end position on forward strand!");
	if (strand_==MINUS && coding_start<coding_end) THROW(ProgrammingException, "Coding start position must be bigger than coding end position on reverse strand!");

	//create coding and UTR regions
	for (int i=0; i<regions_.count(); ++i)
	{
		const BedLine& region = regions_[i];

		if (strand_==PLUS)
		{
			//completely UTR
			if (region.end()<coding_start_)
			{
				utr_5prime_.append(region);
			}
			else if (region.start()>coding_end_)
			{
				utr_3prime_.append(region);
			}
			//completely coding
			else if (region.start()>=coding_start_ && region.end()<=coding_end_)
			{
				coding_regions_.append(region);
			}
			//part coding and part UTR
			else
			{
				coding_regions_.append(BedLine(region.chr(), std::max(region.start(), coding_start_), std::min(region.end(), coding_end_)));
				if (region.start()<coding_start_)
				{
					utr_5prime_.append(BedLine(region.chr(), region.start(), coding_start_-1));
				}
				if (region.end()>coding_end_)
				{
					utr_3prime_.append(BedLine(region.chr(), coding_end_+1, region.end()));
				}
			}
		}
		else
		{
			//completely UTR
			if (region.end()<coding_end_)
			{
				utr_3prime_.append(region);
			}
			else if (region.start()>coding_start_)
			{
				utr_5prime_.append(region);
			}
			//completely coding
			else if (region.start()>=coding_end_ && region.end()<=coding_start_)
			{
				coding_regions_.append(region);
			}
			//part coding and part UTR
			else
			{
				coding_regions_.append(BedLine(region.chr(), std::max(region.start(), coding_end_), std::min(region.end(), coding_start_)));
				if (region.start()<coding_end_)
				{
					utr_3prime_.append(BedLine(region.chr(), region.start(), coding_end_-1));
				}
				if (region.end()>coding_start_)
				{
					utr_5prime_.append(BedLine(region.chr(), coding_start_+1, region.end()));
				}
			}
		}
	}

	//sort and merge regions
	utr_3prime_.merge();
	utr_5prime_.merge();
	coding_regions_.merge();
}

int Transcript::exonNumber(int start, int end) const
{
	QSet<int> matches;
	for(int i=0; i<regions_.count(); ++i)
	{
		if(regions_[i].overlapsWith(start, end))
		{
			if(strand_==STRAND::PLUS) matches << i+1;
			else if(strand_==STRAND::MINUS) matches << regions_.count()-i;
		}
	}

	if (matches.count()==0) return -1;
	if (matches.count()>1) return -2;

	return *(matches.begin());
}

QString Transcript::sourceToString(Transcript::SOURCE source)
{
	switch(source)
	{
		case CCDS:
			return "CCDS";
		case ENSEMBL:
			return "ENSEMBL";
	}

	THROW(ProgrammingException, "Unhandled transcript source enum value '" + QString::number(source) + "!");
}

Transcript::SOURCE Transcript::stringToSource(QString source)
{
	source = source.toUpper();
	if (source=="CCDS")
	{
		return CCDS;
	}
	if (source=="ENSEMBL")
	{
		return ENSEMBL;
	}

	THROW(ProgrammingException, "Unknown transcript source string '" + source + "!");
}

QByteArray Transcript::strandToString(Transcript::STRAND strand)
{
	switch(strand)
	{
		case PLUS:
			return "+";
		case MINUS:
			return "-";
		case INVALID:
			return "n/a";
	}

	THROW(ProgrammingException, "Unhandled transcript strand enum value '" + QString::number(strand) + "!");
}

Transcript::STRAND Transcript::stringToStrand(QByteArray strand)
{
	strand = strand.toUpper();
	if (strand=="+")
	{
		return PLUS;
	}
	if (strand=="-")
	{
		return MINUS;
	}

	THROW(ProgrammingException, "Unknown transcript strand string '" + strand + "'!");
}

QByteArray Transcript::biotypeToString(Transcript::BIOTYPE biotype)
{
	if (biotype==IG_C_GENE) return "IG C gene";
	else if (biotype==IG_C_PSEUDOGENE) return "IG C pseudogene";
	else if (biotype==IG_D_GENE) return "IG D gene";
	else if (biotype==IG_J_GENE) return "IG J gene";
	else if (biotype==IG_J_PSEUDOGENE) return "IG J pseudogene";
	else if (biotype==IG_V_GENE) return "IG V gene";
	else if (biotype==IG_V_PSEUDOGENE) return "IG V pseudogene";
	else if (biotype==IG_PSEUDOGENE) return "IG pseudogene";
	else if (biotype==MT_RRNA) return "Mt rRNA";
	else if (biotype==MT_TRNA) return "Mt tRNA";
	else if (biotype==TEC) return "TEC";
	else if (biotype==TR_C_GENE) return "TR C gene";
	else if (biotype==TR_D_GENE) return "TR D gene";
	else if (biotype==TR_J_GENE) return "TR J gene";
	else if (biotype==TR_J_PSEUDOGENE) return "TR J pseudogene";
	else if (biotype==TR_V_GENE) return "TR V gene";
	else if (biotype==TR_V_PSEUDOGENE) return "TR V pseudogene";
	else if (biotype==LNCRNA) return "lncRNA";
	else if (biotype==MIRNA) return "miRNA";
	else if (biotype==MISC_RNA) return "misc RNA";
	else if (biotype==NON_STOP_DECAY) return "non stop decay";
	else if (biotype==NONSENSE_MEDIATED_DECAY) return "nonsense mediated decay";
	else if (biotype==PROTEIN_CODING_LOF) return "protein coding LoF";
	else if (biotype==PROCESSED_PSEUDOGENE) return "processed pseudogene";
	else if (biotype==PROCESSED_TRANSCRIPT) return "processed transcript";
	else if (biotype==PROTEIN_CODING) return "protein coding";
	else if (biotype==PSEUDOGENE) return "pseudogene";
	else if (biotype==RRNA) return "rRNA";
	else if (biotype==RRNA_PSEUDOGENE) return "rRNA pseudogene";
	else if (biotype==RETAINED_INTRON) return "retained intron";
	else if (biotype==RIBOZYME) return "ribozyme";
	else if (biotype==SRNA) return "sRNA";
	else if (biotype==SCRNA) return "scRNA";
	else if (biotype==SCARNA) return "scaRNA";
	else if (biotype==SNRNA) return "snRNA";
	else if (biotype==SNORNA) return "snoRNA";
	else if (biotype==TRANSCRIBED_PROCESSED_PSEUDOGENE) return "transcribed processed pseudogene";
	else if (biotype==TRANSCRIBED_UNITARY_PSEUDOGENE) return "transcribed unitary pseudogene";
	else if (biotype==TRANSCRIBED_UNPROCESSED_PSEUDOGENE) return "transcribed unprocessed pseudogene";
	else if (biotype==TRANSLATED_PROCESSED_PSEUDOGENE) return "translated processed pseudogene";
	else if (biotype==TRANSLATED_UNPROCESSED_PSEUDOGENE) return "translated unprocessed pseudogene";
	else if (biotype==UNITARY_PSEUDOGENE) return "unitary pseudogene";
	else if (biotype==UNPROCESSED_PSEUDOGENE) return "unprocessed pseudogene";
	else if (biotype==VAULTRNA) return "vaultRNA";
	else if (biotype==ARTIFACT) return "artifact";
	else if (biotype==PROTEIN_CODING_CDS_NOT_DEFINED) return "protein coding CDS not defined";

	THROW(ProgrammingException, "Unhandled transcript biotype enum value '" + QString::number(biotype) + "!");
}

Transcript::BIOTYPE Transcript::stringToBiotype(QByteArray biotype_orig)
{
	QByteArray biotype = biotype_orig.toUpper();
	biotype.replace(' ', '_');

	if (biotype=="IG_C_GENE") return IG_C_GENE;
	else if (biotype=="IG_C_PSEUDOGENE") return IG_C_PSEUDOGENE;
	else if (biotype=="IG_D_GENE") return IG_D_GENE;
	else if (biotype=="IG_J_GENE") return IG_J_GENE;
	else if (biotype=="IG_J_PSEUDOGENE") return IG_J_PSEUDOGENE;
	else if (biotype=="IG_V_GENE") return IG_V_GENE;
	else if (biotype=="IG_V_PSEUDOGENE") return IG_V_PSEUDOGENE;
	else if (biotype=="IG_PSEUDOGENE") return IG_PSEUDOGENE;
	else if (biotype=="MT_RRNA") return MT_RRNA;
	else if (biotype=="MT_TRNA") return MT_TRNA;
	else if (biotype=="TEC") return TEC;
	else if (biotype=="TR_C_GENE") return TR_C_GENE;
	else if (biotype=="TR_D_GENE") return TR_D_GENE;
	else if (biotype=="TR_J_GENE") return TR_J_GENE;
	else if (biotype=="TR_J_PSEUDOGENE") return TR_J_PSEUDOGENE;
	else if (biotype=="TR_V_GENE") return TR_V_GENE;
	else if (biotype=="TR_V_PSEUDOGENE") return TR_V_PSEUDOGENE;
	else if (biotype=="LNCRNA") return LNCRNA;
	else if (biotype=="MIRNA") return MIRNA;
	else if (biotype=="MISC_RNA") return MISC_RNA;
	else if (biotype=="NON_STOP_DECAY") return NON_STOP_DECAY;
	else if (biotype=="NONSENSE_MEDIATED_DECAY") return NONSENSE_MEDIATED_DECAY;
	else if (biotype=="PROTEIN_CODING_LOF") return PROTEIN_CODING_LOF;
	else if (biotype=="PROCESSED_PSEUDOGENE") return PROCESSED_PSEUDOGENE;
	else if (biotype=="PROCESSED_TRANSCRIPT") return PROCESSED_TRANSCRIPT;
	else if (biotype=="PROTEIN_CODING") return PROTEIN_CODING;
	else if (biotype=="PSEUDOGENE") return PSEUDOGENE;
	else if (biotype=="RRNA") return RRNA;
	else if (biotype=="RRNA_PSEUDOGENE") return RRNA_PSEUDOGENE;
	else if (biotype=="RETAINED_INTRON") return RETAINED_INTRON;
	else if (biotype=="RIBOZYME") return RIBOZYME;
	else if (biotype=="SRNA") return SRNA;
	else if (biotype=="SCRNA") return SCRNA;
	else if (biotype=="SCARNA") return SCARNA;
	else if (biotype=="SNRNA") return SNRNA;
	else if (biotype=="SNORNA") return SNORNA;
	else if (biotype=="TRANSCRIBED_PROCESSED_PSEUDOGENE") return TRANSCRIBED_PROCESSED_PSEUDOGENE;
	else if (biotype=="TRANSCRIBED_UNITARY_PSEUDOGENE") return TRANSCRIBED_UNITARY_PSEUDOGENE;
	else if (biotype=="TRANSCRIBED_UNPROCESSED_PSEUDOGENE") return TRANSCRIBED_UNPROCESSED_PSEUDOGENE;
	else if (biotype=="TRANSLATED_PROCESSED_PSEUDOGENE") return TRANSLATED_PROCESSED_PSEUDOGENE;
	else if (biotype=="TRANSLATED_UNPROCESSED_PSEUDOGENE") return TRANSLATED_UNPROCESSED_PSEUDOGENE;
	else if (biotype=="UNITARY_PSEUDOGENE") return UNITARY_PSEUDOGENE;
	else if (biotype=="UNPROCESSED_PSEUDOGENE") return UNPROCESSED_PSEUDOGENE;
	else if (biotype=="VAULTRNA") return VAULTRNA;
	else if (biotype=="VAULT_RNA") return VAULTRNA; //bug in Ensembl GFF in version 105
	else if (biotype=="ARTIFACT") return ARTIFACT;
	else if (biotype=="PROTEIN_CODING_CDS_NOT_DEFINED") return PROTEIN_CODING_CDS_NOT_DEFINED;

	THROW(ProgrammingException, "Unknown transcript biotype string '" + biotype_orig + "'!");
}

int Transcript::cDnaToGenomic(int coord) const
{
	if (coord<1) THROW(ArgumentException, "Invalid cDNA coordinate " + QString::number(coord) + " given for transcript " + name_ +"!");

	int tmp = coord;
	if(strand_==PLUS)
	{
		for (int i=0; i<coding_regions_.count(); ++i)
		{
			const BedLine& line = coding_regions_[i];
			int start = line.start();
			int end = line.end();
			int length = end - start + 1;

			tmp -= length;
			//qDebug() << start << end << length << tmp;

			if (tmp<=0) return end + tmp;
		}
	}
	else
	{
		for (int i=coding_regions_.count()-1; i>=0; --i)
		{
			const BedLine& line = coding_regions_[i];
			int start = line.start();
			int end = line.end();
			int length = end - start + 1;

			tmp -= length;
			//qDebug() << start << end << length << tmp;

			if (tmp<=0) return start - tmp;
		}
	}

	THROW(ArgumentException, "Invalid cDNA coordinate " + QString::number(coord) + " (bigger than coding region) given for transcript " + name_ +"!");
}

int Transcript::nDnaToGenomic(int coord) const
{
	if (coord<1) THROW(ArgumentException, "Invalid non-coding DNA coordinate " + QString::number(coord) + " given for transcript " + name_ +"!");

	int tmp = coord;
	if(strand_==PLUS)
	{
		for (int i=0; i<regions_.count(); ++i)
		{
			const BedLine& line = regions_[i];
			int start = line.start();
			int end = line.end();
			int length = end - start + 1;

			tmp -= length;
			//qDebug() << start << end << length << tmp;

			if (tmp<=0) return end + tmp;
		}
	}
	else
	{
		for (int i=regions_.count()-1; i>=0; --i)
		{
			const BedLine& line = regions_[i];
			int start = line.start();
			int end = line.end();
			int length = end - start + 1;

			tmp -= length;
			//qDebug() << start << end << length << tmp;

			if (tmp<=0) return start - tmp;
		}
	}

	THROW(ArgumentException, "Invalid non-coding DNA coordinate " + QString::number(coord) + " (bigger than non-coding region) given for transcript " + name_ +"!");

}

Variant Transcript::hgvsToVariant(QString hgvs_c, const FastaFileIndex& genome_idx)
{
	//init
	const Chromosome& chr = regions()[0].chr();
	int start = -1;
	int end = -1;
	Sequence ref;
	Sequence obs;

	//check prerequisites
	if (regions_.count()==0) THROW(ProgrammingException, "Transcript '" + name_ + "' has no regions() defined!");

	//check prefix
	hgvs_c = hgvs_c.trimmed();
	bool non_coding = false;
	if (hgvs_c.startsWith("c."))
	{
		hgvs_c = hgvs_c.mid(2);
	}
	else if (hgvs_c.startsWith("n."))
	{
		hgvs_c = hgvs_c.mid(2);
		non_coding = true;
	}
	else
	{
		THROW(ProgrammingException, "Invalid HGVS.c prefix '" + hgvs_c.left(2) + "'. Must be 'c.' or 'n.'!");
	}

	//fix unneeded suffixes at the end of 'dup' and 'del' entries
	hgvs_c.replace(QRegExp("dup[ACGTN]+"), "dup");
	hgvs_c.replace(QRegExp("del[ACGTN]+"), "del");
	hgvs_c.replace(QRegExp("dup[0-9]+"), "dup");
	hgvs_c.replace(QRegExp("del[0-9]+"), "del");

	int length = hgvs_c.length();
	if (length<4) THROW(ProgrammingException, "Invalid cDNA change '" + hgvs_c + "'!");
	//qDebug() << "### cDNA:" << hgvs_c << "###";

	//SNV
	if(hgvs_c.at(length-4).isDigit() && hgvs_c.at(length-3).isLetter() && hgvs_c.at(length-2)=='>' && hgvs_c.at(length-1).isLetter())
	{
		//detemine position
		QString position = hgvs_c.left(length-3);
		int offset = 0;
		hgvsParsePosition(position, non_coding, start, offset);
		start += strand_==Transcript::PLUS ? offset : -1 * offset;
		end = start;

		//set sequence
		ref.append(hgvs_c[length-3].toUpper());
		obs.append(hgvs_c[length-1].toUpper());

		//convert reference to correct strand
		if(strand_==Transcript::MINUS)
		{
			ref.reverseComplement();
			obs.reverseComplement();
		}
	}
	//DUP e.g. "39-286dup" or "289-102_289-100dup"
	else if (hgvs_c.endsWith("dup"))
	{
		//coordinates (of dup)
		QString position = hgvs_c.left(hgvs_c.length()-3);
		int pos_underscore = position.indexOf('_');
		if (pos_underscore!=-1)
		{
			int offset1 = 0;
			hgvsParsePosition(position.left(pos_underscore), non_coding, start, offset1);
			start += (strand_==Transcript::PLUS ? offset1 : -1 * offset1);
			int offset2 = 0;
			hgvsParsePosition(position.mid(pos_underscore+1), non_coding, end, offset2);
			end += (strand_==Transcript::PLUS ? offset2 : -1 * offset2);

			if (start>end)
			{
				int tmp = start;
				start = end;
				end = tmp;
			}
		}
		else
		{
			int offset = 0;
			hgvsParsePosition(position, non_coding, start, offset);
			start += (strand_==Transcript::PLUS ? offset : -1 * offset);
			end = start;
		}

		//sequence
		ref.append('-');
		obs = genome_idx.seq(chr, start, end-start+1);

		//coordinates
		start = start-1; //in GSvar format insertions are to the right of the coordinate
		end = start;
	}
	//DEL e.g. "c.134-3651del" or "c.134-1926_134-1925del"
	else if (hgvs_c.endsWith("del"))
	{
		//coordinates
		QString position = hgvs_c.left(hgvs_c.length()-3);
		int pos_underscore = position.indexOf('_');
		if (pos_underscore!=-1)
		{
			int offset1 = 0;
			hgvsParsePosition(position.left(pos_underscore), non_coding, start, offset1);
			start += (strand_==Transcript::PLUS ? offset1 : -1 * offset1);
			int offset2 = 0;
			hgvsParsePosition(position.mid(pos_underscore+1), non_coding, end, offset2);
			end += (strand_==Transcript::PLUS ? offset2 : -1 * offset2);

			if (start>end)
			{
				int tmp = start;
				start = end;
				end = tmp;
			}
		}
		else
		{
			int offset = 0;
			hgvsParsePosition(position, non_coding, start, offset);
			start += (strand_==Transcript::PLUS ? offset : -1 * offset);
			end = start;
		}

		//sequence
		ref = genome_idx.seq(chr, start, end-start+1);
		obs.append('-');
	}
	//INS+DEL
	else if (hgvs_c.contains("delins"))
	{
		int delins_pos = hgvs_c.indexOf("delins");

		//coordinates
		QString position = hgvs_c.left(delins_pos);
		int pos_underscore = position.indexOf('_');
		if (pos_underscore!=-1)
		{
			int offset1 = 0;
			hgvsParsePosition(position.left(pos_underscore), non_coding, start, offset1);
			start += (strand_==Transcript::PLUS ? offset1 : -1 * offset1);
			int offset2 = 0;
			hgvsParsePosition(position.mid(pos_underscore+1), non_coding, end, offset2);
			end += (strand_==Transcript::PLUS ? offset2 : -1 * offset2);

			if (start>end)
			{
				int tmp = start;
				start = end;
				end = tmp;
			}
		}
		else
		{
			int offset = 0;
			hgvsParsePosition(position, non_coding, start, offset);
			start += (strand_==Transcript::PLUS ? offset : -1 * offset);
			end = start;
		}

		//sequence
		ref = genome_idx.seq(chr, start, end-start+1);
		obs = hgvs_c.mid(delins_pos + 6).toUtf8();

		//convert reference to correct strand
		if(strand_==Transcript::MINUS)
		{
			obs.reverseComplement();
		}
	}
	//INS
	else if (hgvs_c.contains("ins") && hgvs_c.indexOf('_')!=-1)
	{
		int ins_pos = hgvs_c.indexOf("ins");

		if (hgvs_c.mid(ins_pos+3) == "") THROW(ArgumentException, "Insertion '" + hgvs_c + "' does not specify what was inserted!")

		//coordinates
		QString position = hgvs_c.left(ins_pos);
		int pos_underscore = position.indexOf('_');
		int offset1 = 0;
		hgvsParsePosition(position.left(pos_underscore), non_coding, start, offset1);
		start += (strand_==Transcript::PLUS ? offset1 : -1 * offset1);
		int offset2 = 0;
		hgvsParsePosition(position.mid(pos_underscore+1), non_coding, end, offset2);
		end += (strand_==Transcript::PLUS ? offset2 : -1 * offset2);

		if (start>end)
		{
			int tmp = start;
			start = end;
			end = tmp;
		}

		if (start!=end-1) THROW(ArgumentException, "Insertion '" + hgvs_c + "' has coordinates that are not next to each other: " + QString::number(start) + "/" + QString::number(end) + "!");
		end = start;

		//sequence
		ref.append('-');
		obs.append(hgvs_c.mid(ins_pos+3));

		//convert reference to correct strand
		if(strand_==Transcript::MINUS)
		{
			obs.reverseComplement();
		}
	}
	else
	{
		QStringList lines;
		lines << "Unsupported cDNA change '" + hgvs_c + "'. Please note:";
		lines << "- Adjacent SNVs changes e.g. 'c.1234CA>TC' are not supported. Spit them in single base changes or format them as 'delins'.";
		lines << "- Duplication must end with 'dup'. Remove everyhing after.";
		lines << "- Deletions must end with 'del'. Remove everyhing after.";
		THROW(ArgumentException, lines.join("\n"));
	}

	//check reference length
	int length_pos = end - start + 1;
	int length_bases = ref.length();
	if(length_pos!=length_bases)
	{
		THROW(ProgrammingException, "HGVS.c '" + name_ + ":" + hgvs_c + "': reference length of coordinates (" + QString::number(length_pos) + ") and sequence (" + QString::number(length_bases) + ") do not match!");
	}

	//left-align (GSVar variants are always left-aligned)
	Variant variant(chr, start, end, ref, obs);
	//qDebug() << "  After conversion:" << variant.toString(true, -1, true);
	variant.leftAlign(genome_idx);
	//qDebug() << "  Left aligned    :" << variant.toString(true, -1, true);

	return variant;
}

void Transcript::hgvsParsePosition(const QString& position, bool non_coding, int& pos, int& offset) const
{
	//determine positions of special characters
	QList<int> special_char_positions;
	for (int i=0; i<position.length(); ++i)
	{
		if (!position[i].isDigit()) special_char_positions << i;
	}

	//determine offset
	if (special_char_positions.count()==0)
	{
		if (non_coding)
		{
			pos = nDnaToGenomic(position.toInt());
		}
		else
		{
			pos = cDnaToGenomic(position.toInt());
		}
		offset = 0;
		return;
	}
	else if (special_char_positions.count()==1)
	{
		int s_pos = special_char_positions[0];
		QChar s_char = position[s_pos];

		if (s_char=='+') //e.g. "49+17"
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(position.left(s_pos).toInt());
			}
			else
			{
				pos = cDnaToGenomic(position.left(s_pos).toInt());
			}
			offset = position.mid(s_pos+1).toInt();
			return;
		}
		else if (s_char=='-' && s_pos==0) //e.g. "-49" (before the first base)
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(1);
			}
			else
			{
				pos = utr5primeEnd();
			}
			offset = -1 * position.mid(s_pos+1).toInt();

			//fix offset if UTR is split in several regions
			if (!non_coding) correct5PrimeUtrOffset(offset);
			return;
		}
		else if (s_char=='-' && s_pos>0) //e.g. "43-25"
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(position.left(s_pos).toInt());
			}
			else
			{
				pos = cDnaToGenomic(position.left(s_pos).toInt());
			}
			offset = -1 * position.mid(s_pos+1).toInt();
			return;
		}
		else if (s_char=='*') //e.g. "*49" (after the last base)
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(regions_.baseCount());
			}
			else
			{
				pos = utr3primeStart();
			}
			offset = position.mid(s_pos+1).toInt();
			correct3PrimeUtrOffset(offset);
			return;
		}
	}
	else if (special_char_positions.count()==2)
	{
		int s_pos1 = special_char_positions[0];
		QChar s_char1 = position[s_pos1];
		int s_pos2 = special_char_positions[1];
		QChar s_char2 = position[s_pos2];

		if (s_pos1==0 && s_char1=='-' && s_char2=='-') //e.g. "-15-59"
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(1);
			}
			else
			{
				pos = utr5primeEnd();
			}
			offset = -1 * position.mid(1, s_pos2-1).toInt() ;
			if (!non_coding) correct5PrimeUtrOffset(offset);
			offset -=  position.mid(s_pos2+1).toInt();
			return;
		}
		else if (s_pos1==0 && s_char1=='-' && s_char2=='+') //e.g. "-15+59"
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(1);
			}
			else
			{
				pos = utr5primeEnd();
			}
			offset = -1 * position.mid(1, s_pos2-1).toInt();
			if (!non_coding) correct5PrimeUtrOffset(offset);
			offset += position.mid(s_pos2+1).toInt();
			return;
		}
		else if (s_char1=='*' && s_char2=='+') // e.g. "*700+581"
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(regions_.baseCount());
			}
			else
			{
				pos = utr3primeStart();
			}
			offset = position.mid(1, s_pos2-1).toInt();
			correct3PrimeUtrOffset(offset);
			offset += position.mid(s_pos2+1).toInt();
			return;
		}
		else if (s_char1=='*' && s_char2=='-') //e.g. "*9-74"
		{
			if (non_coding)
			{
				pos = nDnaToGenomic(regions_.baseCount());
			}
			else
			{
				pos = utr3primeStart();
			}
			offset = position.mid(1, s_pos2-1).toInt();
			correct3PrimeUtrOffset(offset);
			offset -= position.mid(s_pos2+1).toInt();
			return;
		}
	}

	THROW(ProgrammingException, "Unsupported HGVS.c position string '" + position + "'!");
}

void Transcript::correct5PrimeUtrOffset(int& offset) const
{
	//nothing to do if UTR consists of one region only
	if (utr_5prime_.count()<2) return;

	//determine gap sum
	int gap_sum = 0;
	if (strand_==PLUS)
	{
		bool first = true;
		int size_sum = 0;
		int index = utr_5prime_.count()-1;
		while(size_sum>offset && index>=0)
		{
			size_sum -= utr_5prime_[index].length();
			if (first)
			{
				first = false;
			}
			else
			{
				gap_sum += utr_5prime_[index+1].start() - utr_5prime_[index].end() - 1;
			}
			--index;
		}
	}
	else
	{
		bool first = true;
		int size_sum = 0;
		int index = 0;
		while(size_sum>offset && index<utr_5prime_.count())
		{
			size_sum -= utr_5prime_[index].length();
			if (first)
			{
				first = false;
			}
			else
			{
				gap_sum += utr_5prime_[index].start() - utr_5prime_[index-1].end() - 1;
			}

			++index;
		}
	}

	offset -= gap_sum;
}

void Transcript::correct3PrimeUtrOffset(int& offset) const
{
	//nothing to do if UTR consists of one region only
	if (utr_3prime_.count()<2) return;

	//determine gap sum
	int gap_sum = 0;
	if (strand_==PLUS)
	{
		bool first = true;
		int size_sum = 0;
		int index = 0;
		while(size_sum<offset && index<utr_3prime_.count())
		{
			size_sum += utr_3prime_[index].length();
			if (first)
			{
				first = false;
			}
			else
			{
				gap_sum += utr_3prime_[index].start() - utr_3prime_[index-1].end() - 1;
			}
			++index;
		}
	}
	else
	{
		bool first = true;
		int size_sum = 0;
		int index = utr_3prime_.count()-1;
		while(size_sum<offset && index>=0)
		{
			size_sum += utr_3prime_[index].length();
			if (first)
			{
				first = false;
			}
			else
			{
				gap_sum += utr_3prime_[index+1].start() - utr_3prime_[index].end() - 1;
			}
			--index;
		}
	}

	offset += gap_sum;
}

int Transcript::utr5primeEnd() const
{
	if (utr_5prime_.count()==0) THROW(ProgrammingException, "Cannot determine 5' UTR end for transcript " + name_ + " without 5' UTR regions!");

	if (strand_==PLUS)
	{
		return utr_5prime_[utr_5prime_.count()-1].end()+1;
	}
	else
	{
		return utr_5prime_[0].start()-1;
	}
}

int Transcript::utr3primeStart() const
{
	if (utr_3prime_.count()==0) THROW(ProgrammingException, "Cannot determine 3' UTR start for transcript " + name_ + " without 3' UTR regions!");

	if (strand_==PLUS)
	{
		return utr_3prime_[0].start()-1;
	}
	else
	{
		return utr_3prime_[utr_3prime_.count()-1].end()+1;
	}
}

bool TranscriptList::contains(const Transcript& transcript) const
{
	for (auto it=begin(); it!=end(); ++it)
	{
		if (it->name()==transcript.name()) return true;
	}

	return false;
}

bool TranscriptList::contains(const QByteArray& name) const
{
	for (auto it=begin(); it!=end(); ++it)
	{
		if (it->name()==name) return true;
	}

	return false;
}

Transcript TranscriptList::getTranscript(const QByteArray& name)
{
	for (auto it=begin(); it!=end(); ++it)
	{
		if (it->name()==name) return *it;
	}

	return Transcript();
}

void TranscriptList::sortByBases()
{
	std::stable_sort(begin(), end(), [](const Transcript& a, const Transcript& b){ return a.regions().baseCount() > b.regions().baseCount(); });
}

void TranscriptList::sortByCodingBases()
{
	std::stable_sort(begin(), end(), [](const Transcript& a, const Transcript& b){ return a.codingRegions().baseCount() > b.codingRegions().baseCount(); });
}

void TranscriptList::sortByRelevance()
{
	TranscriptRelevanceComparator comparator;
	std::stable_sort(begin(), end(), comparator);
}

void TranscriptList::sortByPosition()
{
	TranscriptPositionComparator comparator;
	std::stable_sort(begin(), end(), comparator);
}

bool TranscriptList::TranscriptPositionComparator::operator()(const Transcript& a, const Transcript& b) const
{
	if (a.chr()<b.chr()) return true;
	if (a.chr()>b.chr()) return false;

	if (a.start()<b.start()) return true;
	if (a.start()>b.start()) return false;

	if (a.end()<b.end()) return true;
	if (a.end()>b.end()) return false;

	return a.name()<b.name();
}

bool TranscriptList::TranscriptRelevanceComparator::operator()(const Transcript& a, const Transcript& b) const
{
	//gene (alphabetical)
	if (a.gene()>b.gene()) return false;
	if (a.gene()<b.gene()) return true;

	//coding length (longer first)
	long long a_coding = a.codingRegions().baseCount();
	long long b_coding = b.codingRegions().baseCount();
	if (a_coding>b_coding) return true;
	if (a_coding<b_coding) return false;

	//relevant transcript (relevant first)
	bool a_main_transcipt = a.isPreferredTranscript() || a.isManeSelectTranscript() || a.isManePlusClinicalTranscript() || a.isEnsemblCanonicalTranscript();
	bool b_main_transcipt = b.isPreferredTranscript() || b.isManeSelectTranscript() || b.isManePlusClinicalTranscript() || a.isEnsemblCanonicalTranscript();
	if (a_main_transcipt && !b_main_transcipt) return true;
	if (!a_main_transcipt && b_main_transcipt) return false;

	//non-coding length (longer first)
	long long a_noncoding = a.regions().baseCount();
	long long b_noncoding = b.regions().baseCount();
	if (a_noncoding>b_noncoding) return true;
	if (a_noncoding<b_noncoding) return false;

	//name alphabetical (alphabetical)
	return a.name()>b.name();
}
