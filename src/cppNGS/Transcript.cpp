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

Variant Transcript::hgvsToVariant(QString hgvs_c)
{
	//init
	static QRegularExpression exp_snv = QRegularExpression("^(\\d+)([-+]\\d+)?([ACGT])[><]([ACGT])$");
	int start = -1;
	int end = -1;
	int offset1 = 0;
	int offset2 = 0;
	Sequence ref;
	Sequence obs;

	//check prerequisites
	if (regions_.count()==0) THROW(ProgrammingException, "Transcript '" + name_ + "' has no regions() defined!");

	//remove prefix
	hgvs_c = hgvs_c.trimmed();
	if (hgvs_c.startsWith("c.")) hgvs_c = hgvs_c.mid(2);
	qDebug() << hgvs_c;

	//SNV
	QRegularExpressionMatch match = exp_snv.match(hgvs_c);
	if(match.hasMatch())
	{
		//qDebug() << "SNV:" << match.capturedRef(1) << match.capturedRef(2) << match.capturedRef(3) << match.capturedRef(4);
		start = cDnaToGenomic(match.capturedRef(1).toInt());
		end = start;
		if (!match.capturedRef(2).isEmpty())
		{
			offset1 = match.capturedRef(2).toInt();
			offset2 = offset1;
		}
		ref = match.capturedRef(3).toLatin1().toUpper();
		obs = match.capturedRef(4).toLatin1().toUpper();
	}

	if(strand_==Transcript::PLUS)
	{
		start += offset1;
		end += offset2;
		//TODO??? if($obs=="-" && empty($ref)) $ref = get_ref_seq($build,$chr,$start,$end);
	}
	else
	{
		start -= offset1;
		end -= offset2;

		//convert reference
		//TODO??? if($obs=="-" && empty($ref))	$ref = strtoupper(get_ref_seq($build,$chr,$start,$end));
		ref = NGSHelper::changeSeq(ref, true, true);
		obs = NGSHelper::changeSeq(obs, true, true);
	}
	qDebug() << start << end << ref << obs;

	//check reference length
	int length = end - start + 1;
	int bases = ref.length();
	if(length!=bases)
	{
		THROW(ProgrammingException, "HGVS.c '" + name_ + ":" + hgvs_c + "': reference length of coordinates (" + QString::number(length) + ") and sequence (" + QString::number(bases) + ") do not match!");
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

	return Variant(regions()[0].chr(), start, end, ref, obs);
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
	else if(preg_match("/^c\.(?<start>\d+)(?<offset1>[\-\+]\d+)?_?(?<end>\d+)?(?<offset2>[\-\+]\d+)?dup(?<obs>[CATG]+)?$/i",$cdna,$matches)!=0)	//Duplication
	{
		if(empty($matches["end"]))	$matches["end"] = $matches["start"];

		$result = convert_coding2genomic($transcript, $matches["start"], $matches["end"],$error);
		if(is_array($result))	list($chr,$start,$end,$strand) = $result;
		else	$e = $result;

		if($strand == "+")	$end = --$start;
		if($strand == "-")	$start = $end;
		//if on - strand move insertion to the right
		if(!empty($matches["offset1"]))	$offset1 = $matches["offset1"];
		if(!empty($matches["offset2"]))	$offset2 = $matches["offset2"];
		$ref = "-";
		$obs = get_ref_seq($build,$chr,$start,$end);
		if(!empty($matches["obs"]))	$obs = $matches["obs"];
		if(strlen($obs)==1)	$start=--$end;
	}

*/
