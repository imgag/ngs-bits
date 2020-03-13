#include "VariantConversionWidget.h"
#include "GUIHelper.h"
#include "VariantList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Settings.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

VariantConversionWidget::VariantConversionWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, mode_(NONE)
{
	ui_.setupUi(this);
	GUIHelper::styleSplitter(ui_.splitter);
	connect(ui_.convert_btn, SIGNAL(clicked(bool)), this, SLOT(convert()));
	connect(ui_.load_btn, SIGNAL(clicked(bool)), this, SLOT(loadInputFromFile()));
}

void VariantConversionWidget::setMode(VariantConversionWidget::ConversionMode mode)
{
	mode_ = mode;

	if (mode==VCF_TO_GSVAR)
	{
		ui_.input_label->setText("Input (VCF):");
		ui_.output_label->setText("Output (GSvar):");
		ui_.load_btn->setVisible(true);
	}
	else if (mode==HGVSC_TO_GSVAR)
	{
		ui_.input_label->setText("Input (HGVS.c):");
		ui_.output_label->setText("Output (GSvar):");
		ui_.load_btn->setVisible(false);
	}
}

void VariantConversionWidget::loadInputFromFile()
{
	//load
	try
	{
		QString path = Settings::path("path_variantlists");
		QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "VCF files (*.vcf);;All files (*.*)");
		if (filename=="") return;

		ui_.input->clear();

		QStringList lines = Helper::loadTextFile(filename, true, '#', true);
		ui_.input->setPlainText(lines.join("\n"));
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "File error", e.message());
	}

	//auto-convert
	convert();
}

void VariantConversionWidget::convert()
{
	//clear
	ui_.output->clear();

	//convert
	try
	{
		QStringList lines = ui_.input->toPlainText().split("\n");
		QStringList output;
		foreach(QString line, lines)
		{
			line = line.trimmed();
			if (line=="" || line[0]=="#")
			{
				output << "";
			}
			else if (mode_==VCF_TO_GSVAR)
			{
				QStringList parts = line.split("\t");
				if (parts.count()<5) THROW(ArgumentException, "Invalid VCF variant '" + line + "' - too few tab-separated parts!");

				int start = Helper::toInt(parts[1], "VCF start position", line);
				Sequence ref_bases = parts[3].toLatin1().toUpper();
				int end = start + ref_bases.length()-1;
				Variant variant(parts[0], start, end, ref_bases, parts[4].toLatin1().toUpper());

				variant.normalize("-", true);

				output << variant.toString(true).replace(" ", "\t");
			}
			else if (mode_==HGVSC_TO_GSVAR)
			{
/*


//@TODO implement *-syntax for HGVS compliance (end of coding)
function convert_hgvs2genomic($build, $transcript, $cdna, $error = true)
{
	//extract cDNA position and
	$chr = null;
	$start = null;
	$offset1 = 0;
	$end = null;
	$offset2 = 0;
	$strand = null;
	$ref = null;
	$obs = null;
	$matches = array();	//for preg_match
	$e = null;
	if(preg_match("/^c\.(?<start>\d+)(?<offset1>[\-\+]\d+)?(?<ref>[ACGT])[\>\<](?<obs>[ACGT])$/i",$cdna,$matches)!=0)	//SNV
	{
		$result = convert_coding2genomic($transcript, $matches["start"], $matches["start"],$error);
		if(is_array($result))	list($chr,$start,$end,$strand) = $result;
		else	$e = $result;
		if(!empty($matches["offset1"]))	$offset1 = $matches["offset1"];
		$offset2 = $offset1;
		$ref = $matches["ref"];
		$obs = $matches["obs"];
	}
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
	else	//default (not identifiable)
	{
		if($error)	trigger_error("Could not identify HGVS for variant: $cdna.",E_USER_ERROR);
		return "Could not identify variant HGVS for variant: $cdna.";
	}

	if(!is_null($e))
	{
		return $e;
	}

	if($strand=="+")
	{
		$start += $offset1;
		$end += $offset2;
		if($obs=="-" && empty($ref))	$ref = get_ref_seq($build,$chr,$start,$end);
		$ref = strtoupper($ref);
		$obs = strtoupper($obs);
	}
	if($strand == "-")
	{
		$start -= $offset2;
		$end -= $offset1;

		//convert reference
		if($obs=="-" && empty($ref))	$ref = strtoupper(get_ref_seq($build,$chr,$start,$end));
		else if($ref!="-")	$ref = strtoupper(rev_comp($ref));

		//convert obs
		if($obs!="-")	$obs = strtoupper(rev_comp($obs));
	}

	//check if reference is valid
	$r = get_ref_seq($build,$chr,$start,$end);	//adopt for different builds
	if(!empty($chr) && !empty($ref) && $ref!="-" && strtoupper($r)!=strtoupper($ref))
	{
		if($error)	trigger_error("Wrong reference sequence for HGVS '$transcript:$cdna': is '$ref', should be '".$r."' ($chr:$start-$end).",E_USER_ERROR);
		return "Wrong reference sequence for HGVS '$transcript:$cdna': is '$ref', should be '".$r."' ($chr:$start-$end).";
	}

	//check
	$l = $end - $start + 1;
	$b = strlen($ref);
	if($l!=$b)
	{
		if($error)	trigger_error("HGVS ref does not match lenght of variant '$transcript:$cdna': $chr:$start-$end, ref is '$ref', obs is '$obs'.",E_USER_ERROR);
		return "HGVS ref does not match lenght of variant '$transcript:$cdna': $chr:$start-$end, ref is '$ref', obs is '$obs'.";
	}

	return array($chr,$start,$end,$ref,$obs);
}

//SNVs
#--strand,coding and splicing	chr3	195508046	195508046	C	G	MUC4:NM_018406.6:missense:MODERATE:exon2/25:c.10405G>C:p.Asp3469His,MUC4:NM_004532.5:intron:MODIFIER:exon1/23:c.83-2720G>C:,MUC4:NM_138297.4:intron:MODIFIER:exon1/22:c.83-6870G>C:
check(convert_hgvs2genomic("GRCh37", "NM_018406.6", "c.10405G>C"),array("chr3",195508046,195508046,"C","G"));
check(convert_hgvs2genomic("GRCh37", "NM_004532.5", "c.83-2720G>C"),array("chr3",195508046,195508046,"C","G"));
#+-strand, coding	chr8	30924557	30924557	C	T	het	QUAL=7686;DP=561;AF=0.44;MQM=60	WRN	synonymous	WRN:NM_000553.4:synonymous:LOW:exon6/35:c.513C>T:p.Cys171Cys
check(convert_hgvs2genomic("GRCh37", "NM_000553.4", "c.513C>T"),array("chr8",30924557,30924557,"C","T"));
#--strand, coding	chr7	6013049	6013049	C	G	hom	QUAL=2763;DP=92;AF=1.00;MQM=35	PMS2	missense	PMS2:NM_000535.5:missense:MODERATE:exon15/15:c.2570G>C:p.Gly857Ala
check(convert_hgvs2genomic("GRCh37", "NM_000535.5", "c.2570G>C"),array("chr7",6013049,6013049,"C","G"));
#+-strand, splicing	chr9	17135434	17135434	G	C	het	QUAL=4662;DP=350;AF=0.44;MQM=60	CNTLN	splice_region&intron,intron	CNTLN:NM_017738.3:splice_region&intron:LOW:exon1/25:c.360+11G>C:,CNTLN:NM_001286984.1:splice_region&intron:LOW:exon1/2:c.360+11G>C:,CNTLN:NM_001114395.2:splice_region&intron:LOW:exon1/6:c.360+11G>C:,CNTLN:NM_001286985.1:intron:MODIFIER:exon1/2:c.171+200G>C:
check(convert_hgvs2genomic("GRCh37", "NM_017738.3", "c.360+11G>C"),array("chr9",17135434,17135434,"G","C"));
#+-strand, splicing	chr2	47630550	47630550	C	G	het	QUAL=2300;DP=168;AF=0.46;MQM=60	MSH2	splice_region&intron	MSH2:NM_000251.2:splice_region&intron:LOW:exon1/15:c.211+9C>G:,MSH2:NM_001258281.1:splice_region&intron:LOW:exon2/16:c.13+9C>G:
check(convert_hgvs2genomic("GRCh37", "NM_000251.2", "c.211+9C>G"),array("chr2",47630550,47630550,"C","G"));
#--strand;splicing:	chr18	21111585	21111585	C	T	het	QUAL=0	C18orf8,NPC1	splice_region&intron,3'UTR	C18orf8:NM_013326.4:splice_region&intron:LOW:exon19/19:c.1895-4C>T:,C18orf8:NM_001276342.1:splice_region&intron:LOW:exon17/17:c.1627-4C>T:,C18orf8:NR_075075.1:splice_region&intron:LOW:exon16/16:n.1815-4C>T:,C18orf8:NR_075076.1:splice_region&intron:LOW:exon18/18:n.1835-4C>T:,NPC1:NM_000271.4:3'UTR:MODIFIER:exon25/25:c.*581G>A:
check(convert_hgvs2genomic("GRCh37", "NM_013326.4", "c.1895-4C>T"),array("chr18",21111585,21111585,"C","T"));
#--strand,splicing:	chr18	21112158	21112158	C	A	NPC1	c.3838+8G>T
check(convert_hgvs2genomic("GRCh37", "NM_000271.4", "c.3837+8G>T"),array("chr18",21112158,21112158,"C","A"));


//Ins
#+-strand, splicing	chr11	108121410	108121410	-	T	het	QUAL=1626;DP=203;AF=0.30;MQM=57	ATM	splice_region&intron	ATM:NM_000051.3:splice_region&intron:LOW:exon9/62:c.1236-18_1236-17insT:
check(convert_hgvs2genomic("GRCh37", "NM_000051.3", "c.1236-18_1236-17insT"),array("chr11",108121410,108121410,"-","T"));
#c.3755-5_3755-4insTC, --strand,splicing:	chr18	21112158	21112158	C	A	NPC1	c.3838+8G>T
check(convert_hgvs2genomic("GRCh37", "NM_000271.4", "c.3755-5_3755-4insTC"),array("chr18",21112252,21112252,"-","GA"));
#--strand, splicing: chr3	142241692	142241692	-	A	het	QUAL=390;DP=391;AF=0.12;MQM=60	ATR	splice_region&intron	ATR:NM_001184.3:splice_region&intron:LOW:exon22/46:c.4153-10_4153-9insT:
check(convert_hgvs2genomic("GRCh37", "NM_001184.3", "c.4153-10_4153-9insT"),array("chr3",142241692,142241692,"-","A"));
#+-strand chr5	72743299	72743299	-	GC	hom	QUAL=206;DP=10;AF=0.80;MQM=60	FOXD1	frameshift	FOXD1:NM_004472.2:frameshift:HIGH:exon1/1:c.888_889insGC:p.Arg297fs
check(convert_hgvs2genomic("GRCh37", "NM_004472.2", "c.888_889insGC"),array("chr5",72743299,72743299,"-","GC"));
#chr3	75790810	75790810	-	T	het	QUAL=377;DP=20;AF=0.70;MQM=21	ZNF717,MIR4273	frameshift,5'UTR,downstream_gene	ZNF717:NM_001128223.1:frameshift:HIGH:exon3/5:c.134_135insA:p.Leu46fs,ZNF717:NM_001290210.1:frameshift:HIGH:exon3/6:c.134_135insA:p.Leu46fs,ZNF717:NM_001290208.1:frameshift:HIGH:exon3/5:c.134_135insA:p.Leu46fs,ZNF717:NM_001290209.1:5'UTR:MODIFIER:exon3/5:c.-17_-16insA:,MIR4273:NR_036235.1:downstream_gene:MODIFIER::n.*84_*84insT:
check(convert_hgvs2genomic("GRCh37", "NM_001128223.1", "c.134_135insA"),array("chr3",75790810,75790810,"-","T"));


//Del
#--strand, splicing	chr9	32986031	32986031	A	-	het	QUAL=2196;DP=105;AF=0.49;MQM=58	APTX	splice_region&intron	APTX:NM_001195248.1:splice_region&intron:LOW:exon4/7:c.526-3delT:,APTX:NM_001195249.1:splice_region&intron:LOW:exon4/7:c.484-3delT:,APTX:NM_001195254.1:splice_region&intron:LOW:exon3/6:c.322-3delT:,APTX:NM_001195250.1:splice_region&intron:LOW:exon3/6:c.364-3delT:,APTX:NM_001195251.1:splice_region&intron:LOW:exon5/8:c.484-3delT:,APTX:NM_001195252.1:splice_region&intron:LOW:exon4/7:c.310-3delT:,APTX:NM_175069.2:splice_region&intron:LOW:exon4/7:c.526-3delT:,APTX:NM_175073.2:splice_region&intron:LOW:exon5/8:c.484-3delT:
check(convert_hgvs2genomic("GRCh37", "NM_001195248.1", "c.526-3delT"),array("chr9",32986031,32986031,"A","-"));
#+-strand, splicing: chr17	18205751	18205760	GGAGAGTGAA	-	het	QUAL=781;DP=116;AF=0.32;MQM=60	TOP3A	splice_region&intron	TOP3A:NM_004618.3:splice_region&intron:LOW:exon6/18:c.644-12_644-3delTTCACTCTCC:
check(convert_hgvs2genomic("GRCh37", "NM_004618.3", "c.644-12_644-3delTTCACTCTCC"),array("chr17",18205751,18205760,"GGAGAGTGAA","-"));
#--strand, splicing: chr18	21111528	21111528	na	na	NPC1	c.3837+634_3837+637del	NM_000271.4
check(convert_hgvs2genomic("GRCh37", "NM_000271.4", "c.3837+634_3837+637del"),array("chr18",21111529,21111532,"CTTT","-"));
#--strand,splicing:	chr18	21112158	21112158	C	A	NPC1	c.3838+8G>T
check(convert_hgvs2genomic("GRCh37", "NM_000271.4", "c.3744_3747delCAGT"),array("chr18",21113326,21113329,"ACTG","-"));
#
check(convert_hgvs2genomic("GRCh37", "NM_004618.3","c.644-12del16"),array("chr17",18205745,18205760,"GCTCCTGGAGAGTGAA","-"));


//Dup
#chr1	54605318	54605318	-	G	het	QUAL=390;DP=18;AF=0.56;MQM=60	CDCP2	frameshift	CDCP2:NM_201546.3:frameshift:HIGH:exon4/4:c.1224dupC:p.Met409fs
check(convert_hgvs2genomic("GRCh37", "NM_201546.3", "c.1224dupC"),array("chr1",54605318,54605318,"-","G"));
#--strand:	chr8	48805816	48805816	-	G	hom	QUAL=12693;DP=451;AF=0.95;MQM=60	PRKDC	frameshift	PRKDC:NM_006904.6:frameshift:HIGH:exon31/86:c.3729dupC:p.Phe1244fs,PRKDC:NM_001081640.1:frameshift:HIGH:exon31/85:c.3729dupC:p.Phe1244fs
//@TODO error in cDNA position/conversion? Conversion fails... check(convert_hgvs2genomic("GRCh37", "NM_006904.6", "c.3729dupC"),array("chr8",48805816,48805816,"-","G"));
#+-strand: chr17	48452978	48452978	-	AGC	het	QUAL=9702;DP=943;AF=0.38;MQM=60	EME1	disruptive_inframe_insertion	EME1:NM_001166131.1:disruptive_inframe_insertion:MODERATE:exon2/9:c.410_412dupAGC:p.Lys137_Pro138insGln,EME1:NM_152463.2:disruptive_inframe_insertion:MODERATE:exon2/9:c.410_412dupAGC:p.Lys137_Pro138insGln
check(convert_hgvs2genomic("GRCh37", "NM_001166131.1", "c.410_412dupAGC"),array("chr17",48452978,48452978,"-","AGC"));
#WARNING: 'Start of variant in row 73 does not match converted start (c.3570_3573dupACTT, converted: chr18:21114432-21114432, previous: chr18:21114431-21114431). Skipping.' in /mnt/SRV017/users/ahschrc1/sandbox/NPC1_miriam/combine_annotate.php:94.
check(convert_hgvs2genomic("GRCh37", "NM_000271.4", "c.3570_3573dupACTT"),array("chr18",21114431,21114431,"-","AAGT"));


//@TODO NPC1:
#WARNING: 'Start of variant in row 144 does not match converted start (c.3245+1dupG, converted: chr18:21116638-21116637, previous: chr18:21115664-21115664). Skipping.' in /mnt/SRV017/users/ahschrc1/sandbox/NPC1_miriam/combine_annotate.php:94.
check(convert_hgvs2genomic("GRCh37", "NM_007294.3", "c.1A>G"),array("chr17",41276113,41276113,"T","C"));

end_test();

*/
			}
		}

		ui_.output->setPlainText(output.join("\n"));
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Conversion error", e.message());
	}
}
