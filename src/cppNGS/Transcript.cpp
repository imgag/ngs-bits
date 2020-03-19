#include "Transcript.h"
#include "Exceptions.h"
#include "NGSHelper.h"
#include <QRegularExpression>

Transcript::Transcript()
	: coding_start_(0)
	, coding_end_(0)
{
}

void Transcript::setRegions(const BedFile& regions, int coding_start, int coding_end)
{
	regions_ = regions;

	coding_start_ = coding_start;
	coding_end_ = coding_end;

	if (isCoding())
	{
		coding_regions_.clear();

		for (int i=0; i<regions_.count(); ++i)
		{
			const BedLine& line = regions_[i];

			int start = std::max(line.start(), coding_start_);
			int end = std::min(line.end(), coding_end_);

			if (end<coding_start_ || start>coding_end_) continue;

			coding_regions_.append(BedLine(line.chr(), start, end));
		}

		coding_regions_.merge();
	}
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

	THROW(ProgrammingException, "Unknown transcript source enum value '" + QString::number(source) + "!");
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
	}

	THROW(ProgrammingException, "Unknown transcript strand enum value '" + QString::number(strand) + "!");
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

	THROW(ProgrammingException, "Unknown transcript strand string '" + strand + "!");
}

int Transcript::cDnaToGenomic(int coord)
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

	//remove prefix
	hgvs_c = hgvs_c.trimmed();
	if (hgvs_c.startsWith("c.")) hgvs_c = hgvs_c.mid(2);
	int length = hgvs_c.length();
	if (length<4) THROW(ProgrammingException, "Invalid cDNA change '" + hgvs_c + "'!");

	//SNV
	if(hgvs_c[length-4].isDigit() && hgvs_c[length-3].isLetter() && hgvs_c[length-2]=='>' && hgvs_c[length-1].isLetter())
	{
		QString position = hgvs_c.left(length-3);
		int offset = 0;
		hgvsParsePosition(position, start, offset);
		start += strand_==Transcript::PLUS ? offset : -1 * offset;
		end = start;
		ref.append(hgvs_c[length-3].toUpper());
		obs.append(hgvs_c[length-1].toUpper());
	}
	else if (hgvs_c.endsWith("dup")) //e.g. "39-286dup" or "289-102_289-100dup"
	{
		QString position = hgvs_c.left(hgvs_c.length()-3);
		int pos_underscore = position.indexOf('_');
		if (pos_underscore!=-1)
		{
			int offset1 = 0;
			hgvsParsePosition(position.left(pos_underscore), start, offset1);
			start += (strand_==Transcript::PLUS ? offset1 : -1 * offset1);
			int offset2 = 0;
			hgvsParsePosition(position.mid(pos_underscore+1), end, offset2);
			end += (strand_==Transcript::PLUS ? offset2 : -1 * offset2);
		}
		else
		{
			int offset = 0;
			hgvsParsePosition(position, start, offset);
			start += (strand_==Transcript::PLUS ? offset : -1 * offset);
			end = start;
		}
		ref.append('-');
		obs = genome_idx.seq(chr, start, end-start+1);
	}
	else
	{
		THROW(ArgumentException, "Unsupported cDNA change '" + hgvs_c + "'!");
	}

	//convert reference
	if(strand_==Transcript::MINUS)
	{
		ref = NGSHelper::changeSeq(ref, true, true);
		obs = NGSHelper::changeSeq(obs, true, true);
	}
	qDebug() << chr.str() << start << end << ref << obs;

	//check reference length
	int length_pos = end - start + 1;
	int length_bases = ref.length();
	if(length_pos!=length_bases)
	{
		THROW(ProgrammingException, "HGVS.c '" + name_ + ":" + hgvs_c + "': reference length of coordinates (" + QString::number(length_pos) + ") and sequence (" + QString::number(length_bases) + ") do not match!");
	}

	//TODO check reference sequence
	/*
	$r = get_ref_seq($build,$chr,$start,$end);	//adopt for different builds
	if(!empty($chr) && !empty($ref) && $ref!="-" && strtoupper($r)!=strtoupper($ref))
	{
		if($error)	trigger_error("Wrong reference sequence for HGVS '$transcript:$cdna': is '$ref', should be '".$r."' ($chr:$start-$end).",E_USER_ERROR);
		return "Wrong reference sequence for HGVS '$transcript:$cdna': is '$ref', should be '".$r."' ($chr:$start-$end).";
	}
	*/

	Variant variant(chr, start, end, ref, obs);
	variant.leftAlign(genome_idx);
	return variant;
}

void Transcript::hgvsParsePosition(const QString& position, int& pos, int& offset)
{
	//determine special characters
	int pos_plus = -1;
	int pos_minus = -1;
	int pos_star = -1;
	for (int i=0; i<position.length(); ++i)
	{
		if (position[i]=='+')
		{
			pos_plus = i;
			break;
		}
		else if (position[i]=='-')
		{
			pos_minus = i;
			break;
		}
		else if (position[i]=='*')
		{
			pos_star = i;
			break;
		}
	}

	//determine offset
	if (pos_plus!=-1)
	{
		pos = cDnaToGenomic(position.left(pos_plus).toInt());
		offset = position.mid(pos_plus+1).toInt();
	}
	else if (pos_minus!=-1)
	{
		pos = cDnaToGenomic(pos_minus==0 ? 1 : position.left(pos_minus).toInt());
		offset = -1 * position.mid(pos_minus+1).toInt();
	}
	else if (pos_star!=-1)
	{
		pos = cDnaToGenomic(pos_star==0 ? coding_regions_.baseCount() : position.left(pos_star).toInt());
		offset = position.mid(pos_star+1).toInt();
	}
	else
	{
		pos = cDnaToGenomic(position.toInt());
		offset = 0;
	}
}


/*

	...

	else if(preg_match("/^c\.(?<start>\d+)(?<offset1>[\-\+]\d+)?_?(?<end>\d+)?(?<offset2>[\-\+]\d+)?del(?<ref>[CATG]+)?$/i",$cdna,$matches)!=0)	//Deletion
	{
		if(empty($matches["end"]))	$matches["end"] = $matches["start"];	//if no end position is given

		$result = convert_coding2genomic($transcript, $matches["start"], $matches["end"],$error);
		if(is_array($result))	list($chr,$start,$end,$strand) = $result;
		else	$e = $result;

		if(!empty($matches["offset1"]))	$offset1 = $matches["offset1"];
		$offset2 = $offset1;
		if(!empty($matches["offset2"]))	$offset2 = $matches["offset2"];
		if(!empty($matches["ref"]))	$ref = $matches["ref"];
		$obs = "-";
	}
	else if(preg_match("/^c\.(?<start>\d+)(?<offset1>[\-\+]\d+)?_?(?<end>\d+)?(?<offset2>[\-\+]\d+)?del(?<ref_count>\d+)?$/i",$cdna,$matches)!=0)	//Deletion, e.g. c.644-12del16
	{
		if(empty($matches["end"]))	$matches["end"] = $matches["start"];	//if no end position is given

		$result = convert_coding2genomic($transcript, $matches["start"], $matches["end"],$error);
		if(is_array($result))	list($chr,$start,$end,$strand) = $result;
		else	$e = $result;

		if(!empty($matches["offset1"]))	$offset1 = $matches["offset1"];
		$offset2 = $offset1;
		if(!empty($matches["ref_count"]))	$offset2 += $matches["ref_count"]-1;	//if no end position is given
		$obs = "-";
	}
	else if(preg_match("/^c\.(?<start>\d+)(?<offset1>[\-\+]\d+)?_?(?<end>\d+)?(?<offset2>[\-\+]\d+)?ins(?<obs>[CATG]+)$/i",$cdna,$matches)!=0)	//Insertion
	{
		//skip end and offset2, since insertion is always next to start (both splicing and coding)
		$result = convert_coding2genomic($transcript, $matches["start"], $matches["start"],$error);
		if(is_array($result))	list($chr,$start,$end,$strand) = $result;
		else	$e = $result;

		//offsets
		if(!empty($matches["offset1"]))	$offset1 = $matches["offset1"];
		if(!empty($matches["offset2"]))	$offset2 = $matches["offset2"];
		if($strand=="+" && $offset1!=0 && $offset2!=0)	$offset1 = min($offset1, $offset2);
		if($strand=="-" && $offset1!=0 && $offset2!=0)	$offset1 = max($offset1, $offset2);
		$offset2 = $offset1;
		if($strand=="-" && empty($offset1) && empty($offset2))	$end = --$start;	//change of insertion site required for "-"-strand variants.

		//alleles
		$ref = "-";
		$obs = $matches["obs"];
	}
	else if(preg_match("/^c\.(?<start>\d+)(?<offset1>[\-\+]\d+)?_?(?<end>\d+)?(?<offset2>[\-\+]\d+)?del(?<ref>[CATG]+)?ins(?<obs>[CATG]+)$/i",$cdna,$matches)!=0)	//combined InDel
	{
		if(empty($matches["end"]))	$matches["end"] = $matches["start"];	//if no end position is given

		$result = convert_coding2genomic($transcript, $matches["start"], $matches["end"],$error);
		if(is_array($result))	list($chr,$start,$end,$strand) = $result;
		else	$e = $result;


		if(!empty($matches["offset1"]))	$offset1 = $matches["offset1"];
		if(!empty($matches["offset2"]))	$offset2 = $matches["offset2"];
		if(!empty($matches["ref"]))	$ref = $matches["ref"];
		if(empty($ref))	$ref = get_ref_seq($build,$chr,$start,$end);
		if($strand=="-")	$ref = rev_comp ($ref);
		$obs = $matches["obs"];
	}

*/
