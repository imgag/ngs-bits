#include "ToolBase.h"
#include "Helper.h"
#include "VersatileFile.h"
#include <NGSHelper.h>
#include <QFileInfo.h>
#include <VcfFile.h>

class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

    //TODO also use to normalize VCFs in single sample pipelines? Support merging two het into one hom variant

	virtual void setup()
	{
        setDescription("Merges several VCF files into a multi-sample VCF file.");
        setExtendedDescription(QStringList() << "Input VCF have to be normalized (no multi-allelic variants, split into allelic primitives and indels left-aligned."
                                             << "The output has no information in the QUAL, FILTER and INFO column. It contains the following FORMAT entries: GT, DP, AF, GQ, PS");
		addInfileList("in", "Input files to merge in VCF or VCG.GZ format.", false);
		//optional
        addOutfile("out", "Output multi-sample VCF. If unset, writes to STDOUT.", true);
        addFlag("trio", "Enables trio special handling. Expected sample order: child, father, mother.");
        //TODO add BAM files, to determine depth and AF for uncalled variants
        changeLog(2026, 3, 29, "Initial implementation.");
	}

    struct VariantDetails
    {
        Chromosome chr;
        int pos;
        QByteArray ref;
        QByteArray alt;
        bool is_snv;

        bool operator<(const VariantDetails& rhs) const
        {
            if (chr<rhs.chr) return true; //compare chromosome
            else if (chr>rhs.chr) return false;
            else if (pos<rhs.pos) return true; //compare start position
            else if (pos>rhs.pos) return false;
            else if (ref<rhs.ref) return true; //compare ref sequence
            else if (ref>rhs.ref) return false;
            else if (alt<rhs.alt) return true; //compare obs sequence
            else if (alt>rhs.alt) return false;
            return false;
        }

        QByteArray tag() const
        {
            return chr.strNormalized(true)+'\t'+QByteArray::number(pos)+"\t.\t"+ref+'\t'+alt;
        }
    };

    struct FormatData
    {
        QByteArray gt = "0/0";
        QByteArray dp = ".";
        QByteArray af = ".";
        QByteArray gq = ".";
        QByteArray ps = ".";
        bool is_mosaic = false;
        bool is_low_mappability = false;
    };

    struct VcfData
    {
        //general data
        QByteArray filename; //filename without path
        QByteArray sample; //sample name
        QByteArray sample_desc; //megSAP-specific sample line '##SAMPLE=...'

        //hash from tag to FORMAT data
        QHash<QByteArray, FormatData> tag_to_format;

        //statistics data
        double chrx_het_perc = -1; //heteroyzgous chrX SNVs used to estimate the gender
        int c_snv = 0; //SNV count
        int c_indel = 0; //INDEL count
        int c_mosaic = 0; //mosaic variant count
        int c_low_mappability = 0; //low_mappability variant count
        int c_skipped_wt = 0; //variants skipped because they are wild-type
    };

    VcfData loadVcf(QString filename, QList<VariantDetails>& var_details, QHash<QByteArray, int>& var_tag_to_index)
    {
        //init
        VcfData output;
        int c_snv_x = 0;
        int c_snv_x_het = 0;

        output.filename = QFileInfo(filename).fileName().toUtf8();
        VersatileFile file(filename);
        file.open(QIODevice::Text|QFile::ReadOnly);
        while(!file.atEnd())
        {
            QByteArray line = file.readLine(true);
            if (line.isEmpty()) continue;

            //comment and header lines
            if (line[0]=='#')
            {
                //comment lines
                if (line.startsWith("##"))
                {
                    if(line.startsWith("##SAMPLE=")) output.sample_desc = line;
                    continue;
                }
                else //header line
                {
                    QByteArrayList parts = line.split('\t');

                    //check that input VCF is a single-sample file
                    if (parts.count()!=10) THROW(FileParseException, "Input file '" + filename + "' contains "+QString::number(parts.count())+" columns. Only single-sample input VCFs with 10 columns are supported.");
                    output.sample = parts[9].trimmed();
                }

                continue;
            }
            else //variant lines
            {
                //check number of columns
                QByteArrayList parts = line.split('\t');
                if (parts.count()!=10) THROW(FileParseException, "Input file '" + filename + "' variant line with other than 10 columns: "+line);

                //check not multi-allelic
                const QByteArray& alt = parts[4];
                if (alt.contains(',')) THROW(FileParseException, "Input file '" + filename + "' contains multi-allelic variant: "+line);

                //parse format data
                QByteArrayList format_keys = parts[8].split(':');
                int i_gt = format_keys.indexOf("GT");
                if (i_gt!=0) THROW(FileParseException, "Input file '" + filename + "' has invalid FORMAT data: GT is not first element: " +line);
                int i_dp = format_keys.indexOf("DP");
                int i_af = format_keys.indexOf("AF");
                int i_ao = format_keys.indexOf("AO");
                int i_gq = format_keys.indexOf("GQ");
                int i_ps = format_keys.indexOf("PS");
                QByteArrayList format_values = parts[9].split(':');
                if (format_keys.count()!=format_values.count()) THROW(FileParseException, "Input file '" + filename + "' has differing format key/value count: " +line);

                //normalize GT
                QByteArray gt = format_values[0].trimmed();
                gt = gt.replace('|', '/').replace('.', '0');
                if (gt=="1/0") gt = "0/1";
                if (gt=="1") gt = "1/1"; //Clair3 returns only one allele for chrMT
                if (gt=="0/0" || gt=="0") //WT > variant not in sample
                {
                    ++output.c_skipped_wt;
                    continue;
                }
                if (gt!="0/1" && gt!="1/1") THROW(FileParseException, "Input file '" + filename + "' has invalid unsupported 'GT' format: " +line);

                //determine variant type
                const QByteArray& ref = parts[3];
                bool is_snv = ref.length()==1 && alt.length()==1;
                if (is_snv) ++output.c_snv;
                else ++output.c_indel;

                //get index of variant in list
                Chromosome chr(parts[0]);
                int pos = Helper::toInt(parts[1], "variant position");
                VariantDetails details{chr, pos, ref, alt, is_snv};
                QByteArray tag = details.tag();
                int index = var_tag_to_index.value(tag, -1);

                //no index > insert variant to list
                if (index==-1)
                {
                    var_details << details;
                    var_tag_to_index.insert(tag, var_details.count()-1);
                }

                //determine FORMAT data
                FormatData format;
                format.gt = gt;
                if (i_dp!=-1) format.dp = format_values[i_dp];
                if (i_af!=-1) format.af = format_values[i_af];
                else if (i_ao!=-1)
                {
                    QByteArray dp = format_values[i_dp];
                    QByteArray ao = format_values[i_ao];
                    if (Helper::isNumeric(dp) && Helper::isNumeric(ao))
                    {
                        format.af = QByteArray::number(dp.toDouble() / ao.toDouble(), 'f', 3);
                    }
                }
                if (i_gq!=-1) format.gq = format_values[i_gq];
                if (i_ps!=-1) format.ps = format_values[i_ps];
                QByteArrayList filters = parts[6].split(';');
                std::for_each(filters.begin(), filters.end(), [](QByteArray& x) { x = x.trimmed(); });
                format.is_low_mappability = filters.contains("low_mappability");
                if (format.is_low_mappability) ++output.c_low_mappability;
                format.is_mosaic = filters.contains("mosaic");
                if (format.is_mosaic) ++output.c_mosaic;
                output.tag_to_format.insert(tag, format);

                //determine heterozygous SNV percentage on chrX (for gender)
                if (chr.isX() && is_snv && !format.is_mosaic && !format.is_low_mappability)
                {
                    if (!NGSHelper::pseudoAutosomalRegion(GenomeBuild::HG38).overlapsWith(chr, pos, pos))
                    {
                        ++c_snv_x;
                        if (gt=="0/1") ++c_snv_x_het;
                    }
                }
            }
        }

        //determine fraction of heterozygous SNVs on chrX
        if (c_snv_x>0)
        {
            output.chrx_het_perc = 100.0 * c_snv_x_het / c_snv_x;
        }

        return output;
    }

    void printFileDebug(const VcfData& data, QTextStream& debug)
    {
        debug << "input file: " << data.filename << "\n";
        debug << "  variants skipped (wild-type): " << QByteArray::number(data.c_skipped_wt) << "\n";
        debug << "  variants loaded: " << QByteArray::number(data.tag_to_format.count()) << "\n";
        debug << "    SNV: " << QByteArray::number(data.c_snv) << "\n";
        debug << "    INDEL: " << QByteArray::number(data.c_indel) << "\n";
        debug << "    mosaic: " << QByteArray::number(data.c_mosaic) << "\n";
        debug << "    low-mappability: " << QByteArray::number(data.c_low_mappability) << "\n";
        if (data.chrx_het_perc>=0)
        {
            debug << "  heterozygous SNVs on chrX ouside PAR: " << QByteArray::number(data.chrx_het_perc, 'f', 2) << "%\n";
        }
        debug << "\n";
    }

	virtual void main()
	{
        //init
		QStringList in_files = getInfileList("in");
		QString out = getOutfile("out");
		foreach(QString in, in_files)
		{
            if(in==out) THROW(ArgumentException, "Input and output files must be different!");
		}
        QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
        QTextStream debug(out.isEmpty() ? stderr : stdout);
        bool trio = getFlag("trio");

        //load data into memory
        QList<VariantDetails> var_details;
        QHash<QByteArray, int> var_tag_to_index; //index for fast lookup of variants in `var_details` while reading data
        QList<VcfData> data;
        foreach(QString in, in_files)
        {
            data << loadVcf(in, var_details, var_tag_to_index);
            printFileDebug(data.last(), debug);
        }

        //write comments
        out_p->write("##fileformat=VCFv4.3\n");
        out_p->write("##fileDate="+QDate::currentDate().toString("yyyyMMdd").toUtf8()+"\n");
        out_p->write("##ANALYSISTYPE=GERMLINE_MULTISAMPLE\n");
        out_p->write("##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype of variant.\">\n");
        out_p->write("##FORMAT=<ID=DP,Number=1,Type=Integer,Description=\"Read depth at variant position.\">\n");
        out_p->write("##FORMAT=<ID=AF,Number=1,Type=Float,Description=\"Allele frequency of variant.\">\n");
        out_p->write("##FORMAT=<ID=GQ,Number=1,Type=Integer,Description=\"Genotype quality.\">\n");
        out_p->write("##FORMAT=<ID=PS,Number=1,Type=Integer,Description=\"Phase set identifier.\">\n");
        for(const VcfData& entry: std::as_const(data))
        {
            if (entry.sample_desc.isEmpty()) continue;
            out_p->write(entry.sample_desc+"\n");
        }

        //write header line
        out_p->write("#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT");
        QSet<QByteArray> samples_done;
        for(const VcfData& entry: std::as_const(data))
        {
            if (samples_done.contains(entry.sample)) THROW(Exception, "Sample name '" + entry.sample + "' contained more than once in input VCFs!");
            samples_done << entry.sample;

            out_p->write("\t"+entry.sample);
        }
        out_p->write("\n");

        //sort variants
        std::sort(var_details.begin(), var_details.end());

        //write variants
        for(const VariantDetails& v: std::as_const(var_details))
		{
            QByteArray tag = v.tag();
            out_p->write(tag + "\t30\tPASS\t.\tGT:DP:AF:GQ:PS");
            for(const VcfData& entry: std::as_const(data))
            {
                FormatData format = entry.tag_to_format.value(tag);
                out_p->write("\t"+format.gt+":"+format.dp+":"+format.af+":"+format.gq+":"+format.ps);
            }
            out_p->write("\n");
        }

		//clean up
		out_p->close();

        //statistics output
        debug << "output:\n";
        debug << "  variants written: " << QByteArray::number(var_details.count()) << "\n";

        //trio: determine mendelian error rate
        if (trio)
        {
            int c_snv = 0;
            int c_snv_error = 0;
            int c_indel = 0;
            int c_indel_error = 0;
            for(const VariantDetails& v: std::as_const(var_details))
            {
                if (!v.chr.isAutosome()) continue;

                //check if mendelian error
                QByteArray tag = v.tag();
                bool is_error = false;
                QByteArray gt_c = "0/0";
                FormatData format = data[0].tag_to_format.value(tag);
                if (format.gt!="." && !format.is_low_mappability && !format.is_mosaic) gt_c = format.gt;
                QByteArray gt_f = "0/0";
                format = data[1].tag_to_format.value(tag);
                if (format.gt!="." && !format.is_low_mappability && !format.is_mosaic) gt_f = format.gt;
                QByteArray gt_m = "0/0";
                format = data[2].tag_to_format.value(tag);
                if (format.gt!="." && !format.is_low_mappability && !format.is_mosaic) gt_m = format.gt;

                //hom, hom => het/wt
                if (gt_f=="1/1" && gt_m=="1/1" && gt_c!="1/1") is_error = true;
                //hom, x => wt
                else if ((gt_f=="1/1" || gt_m=="1/1") && gt_c=="0/0") is_error = true;
                //wt, x => hom
                else if ((gt_f=="0/0" || gt_m=="0/0") && gt_c=="1/1") is_error = true;
                //wt, wt  => het/hom
                else if (gt_f=="0/0" && gt_m=="0/0" && gt_c!="0/0") is_error = true;

                if (v.is_snv)
                {
                    ++c_snv;
                    if (is_error) ++c_snv_error;
                }
                else
                {
                    ++c_indel;
                    if (is_error) ++c_indel_error;
                }
            }
            debug << "  trio mendelian error rate of SNVs: " << QByteArray::number(100.0 * c_snv_error/c_snv, 'f', 2) << "%\n";
            debug << "  trio mendelian error rate of INDELs: " << QByteArray::number(100.0 * c_indel_error/c_indel, 'f', 2) << "%\n";
        }
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
