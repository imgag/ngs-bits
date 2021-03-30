#include "TestFramework.h"
#include "TestFrameworkNGS.h"
#include "VcfFile.h"

TEST_CLASS(VcfLine_Test)
{
    Q_OBJECT
    private slots:

    void constructLineWithoutFile()
    {
        QVector<Sequence> alt;
        alt.push_back("T");

        QByteArrayList format_ids;
        format_ids.push_back("GT");
        format_ids.push_back("X");

        QByteArrayList sample_ids;
        sample_ids.push_back("sample_1");
        sample_ids.push_back("sample_2");

        QList<QByteArrayList> list_of_format_values;
        QByteArrayList list;
        list.push_back("1/1");
        list.push_back("A");
        list_of_format_values.push_back(list);
        list.clear();
        list.push_back("0/0");
        list.push_back("B");
        list_of_format_values.push_back(list);

        VcfLine variant(Chromosome("chr4"), 777, "A", alt, format_ids, sample_ids, list_of_format_values);
        X_EQUAL(variant.chr(), Chromosome("chr4"));
        I_EQUAL(variant.start(), 777);
        S_EQUAL(variant.ref(), "A");
        S_EQUAL(variant.altString(), "T");
        QByteArrayList format = variant.formatKeys();
        S_EQUAL(format.at(0), "GT");
        S_EQUAL(format.at(1), "X");
        S_EQUAL(format.size(), 2);

        S_EQUAL(variant.formatValueFromSample("GT", "sample_1"), "1/1");
        S_EQUAL(variant.formatValueFromSample("X", "sample_1"), "A");
        S_EQUAL(variant.formatValueFromSample("GT", "sample_2"), "0/0");
        S_EQUAL(variant.formatValueFromSample("X", "sample_2"), "B");
    }


    void infoLinefromHeader()
    {
        VcfHeader header;
        InfoFormatLine added_info;
        for(int i = 0; i < 10; ++i)
        {
            added_info.id = QByteArray::number(i);
            added_info.description = "description of info " + QByteArray::number(i);
            header.addInfoLine(added_info);
        }

        //test for existing info
        InfoFormatLine received_info = header.infoLineByID("3");
        S_EQUAL(received_info.description, "description of info 3");

        //test for empty info
        received_info = header.infoLineByID("X", false);
        S_EQUAL(received_info.id, "");
    }

    void formatLineFromHeader()
    {
        VcfHeader header;
        InfoFormatLine added_format;
        for(int i = 0; i < 10; ++i)
        {
            added_format.id = QByteArray::number(i);
            added_format.description = "description of format " + QByteArray::number(i);
            header.addFormatLine(added_format);
        }

        InfoFormatLine received_format = header.formatLineByID("3");
        S_EQUAL(received_format.description, "description of format 3");
    }

    void filterLineFromHeader()
    {
        VcfHeader header;
        FilterLine added_filter;
        for(int i = 0; i < 10; ++i)
        {
            added_filter.id = QByteArray::number(i);
            added_filter.description = "description of filter " + QByteArray::number(i);
            header.addFilterLine(added_filter);
        }

        FilterLine received_filter = header.filterLineByID("3");
        S_EQUAL(received_filter.description, "description of filter 3");
    }

    void infoFromInfoId()
    {
        VcfLine variant;
        QByteArrayList info;
        InfoIDToIdxPtr info_ptr = InfoIDToIdxPtr(new OrderedHash<QByteArray, int>);
        for(int i = 0; i < 10; ++i)
        {
            QByteArray key = "key of " + QByteArray::number(i);
            QByteArray value = "value of " + QByteArray::number(i);
            info.push_back(value);
            info_ptr->push_back(key, static_cast<unsigned char>(i));
        }
        variant.setInfo(info);
        variant.setInfoIdToIdxPtr(info_ptr);
        S_EQUAL(variant.info("key of 3"), "value of 3");
        I_EQUAL(variant.infoKeys().size(), 10);
        I_EQUAL(variant.infoValues().size(), 10);

        QByteArrayList received_info_values = variant.infoValues();
        S_EQUAL(received_info_values.at(4), "value of 4");
        S_EQUAL(variant.info("key of 4"), "value of 4");
    }

    void formatEntryForSampleId()
    {
        QByteArray vcf_line = "chr17	72196817	.	G	GA	.	.	.	GT:PL:GQ	0/1:255,0,123:99	1/1:255,84,0:33";
        QSet<QByteArray> empty_set;
        VcfFile vcf_file;
        QByteArray header_line = "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsample_1\tsample_2";
        vcf_file.parseHeaderFields(header_line, true);

        vcf_file.parseVcfEntry(1, vcf_line, empty_set, empty_set, empty_set, true, nullptr);
        I_EQUAL(vcf_file.count(), 1);

        QList<QByteArrayList> all_samples = vcf_file[0].samples();
        I_EQUAL(all_samples.size(), 2);
        //FormatIDToValueHash single_sample = vcf_file[0].sample("sample_1");
        QByteArrayList single_sample = vcf_file[0].sample("sample_1");
        I_EQUAL(single_sample.size(), 3);
        S_EQUAL(vcf_file[0].formatValueFromSample("GT", "sample_1"), "0/1");
        single_sample = vcf_file[0].sample(1);
        I_EQUAL(single_sample.size(), 3);
        S_EQUAL(vcf_file[0].formatValueFromSample("GQ", "sample_2"), "33");

        QByteArray format_value = vcf_file[0].formatValueFromSample("GT", "sample_1");
        S_EQUAL(format_value, "0/1");
        format_value = vcf_file[0].formatValueFromSample("PL");
        S_EQUAL(format_value, "255,0,123");
        format_value = vcf_file[0].formatValueFromSample("PL", 1);
        S_EQUAL(format_value, "255,84,0");
    }

    void isMultiAllelicOrInDEl()
    {
        VcfFile file;
        file.load(TESTDATA("data_in/variantList_removeDuplicates.vcf"));

        VcfLine line = file.vcfLine(5);
        IS_TRUE(line.isMultiAllelic());
        line = file.vcfLine(6);
        IS_FALSE(line.isMultiAllelic());

        line = file.vcfLine(0);
        IS_TRUE(line.isInDel());
        line = file.vcfLine(1);
        IS_FALSE(line.isInDel());
    }

    //test same variants as in Variant::leftAlign()
    void leftNormalize()
    {
        QString ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") SKIP("Test needs the reference genome!");

        VariantList v_list;
        Variant v;

        //SNP > no change
        //leftNormalize inside conversion of GSvar to VCF
        v = Variant("chr17", 41246534, 41246534, "T", "A");
        v_list.append(v);
        VcfFile vcf_file = VcfFile::convertGSvarToVcf(v_list, ref_file);
        I_EQUAL(vcf_file.count(), 1);
        VcfLine v_line = vcf_file[0];
        I_EQUAL(v_line.start(), 41246534);
        I_EQUAL(v_line.end(), 41246534);
        S_EQUAL(v_line.ref(), "T");
        S_EQUAL(v_line.alt(0), "A");

        //leftNormalize of VCF line
        v_line.setRef("T");
        v_line.setSingleAlt("A");
        v_line.setChromosome("chr17");
        v_line.setPos(41246534);
        v_line.leftNormalize(ref_file);
        I_EQUAL(v_line.start(), 41246534);
        I_EQUAL(v_line.end(), 41246534);
        S_EQUAL(v_line.ref(), "T");
        S_EQUAL(v_line.alt(0), "A");

        //INS (one base block shift)
        //leftNormalize inside conversion of GSvar to VCF
        v = Variant("chr17", 41246534, 41246534, "-", "T");
        v_list.clear();
        v_list.append(v);
        vcf_file = VcfFile::convertGSvarToVcf(v_list, ref_file);
        I_EQUAL(vcf_file.count(), 1);
        v_line = vcf_file[0];
        I_EQUAL(v_line.start(), 41246531);
        I_EQUAL(v_line.end(), 41246531);
        S_EQUAL(v_line.ref(), "C");
        S_EQUAL(v_line.alt(0), "CT");

        //leftNormalize of VCF line
        v_line.setRef("T");
        v_line.setSingleAlt("TT");
        v_line.setChromosome("chr17");
        v_line.setPos(41246534);
        v_line.leftNormalize(ref_file);
        I_EQUAL(v_line.start(), 41246531);
        I_EQUAL(v_line.end(), 41246531);
        S_EQUAL(v_line.ref(), "C");
        S_EQUAL(v_line.alt(0), "CT");

        //INS (no block shift)
        //leftNormalize inside conversion of GSvar to VCF
        v = Variant("chr3", 195307240, 195307240, "-", "TTC");
        v_list.clear();
        v_list.append(v);
        vcf_file = VcfFile::convertGSvarToVcf(v_list, ref_file);
        I_EQUAL(vcf_file.count(), 1);
        v_line = vcf_file[0];
        I_EQUAL(v.start(), 195307240);
        I_EQUAL(v_line.end(), 195307240);
        S_EQUAL(v_line.ref(), "T");
        S_EQUAL(v_line.alt(0), "TTTC");

        //leftNormalize of VCF line
        v_line.setRef("C");
        v_line.setSingleAlt("CTTC");
        v_line.setChromosome("chr3");
        v_line.setPos(195307240);
        v_line.leftNormalize(ref_file);
        I_EQUAL(v_line.start(), 195307240);
        I_EQUAL(v_line.end(), 195307240);
        S_EQUAL(v_line.ref(), "T");
        S_EQUAL(v_line.alt(0), "TTTC");

        //DEL (two base block shift)
        //leftNormalize inside conversion of GSvar to VCF
        v = Variant("chr3", 195956747, 195956748, "AG", "-");
        v_list.clear();
        v_list.append(v);
        vcf_file = VcfFile::convertGSvarToVcf(v_list, ref_file);
        I_EQUAL(vcf_file.count(), 1);
        v_line = vcf_file[0];
        I_EQUAL(v_line.start(), 195956726);
        I_EQUAL(v_line.end(), 195956728);
        S_EQUAL(v_line.ref(), "AAG");
        S_EQUAL(v_line.alt(0), "A");

        //leftNormalize of VCF line
        v_line.setRef("GAG");
        v_line.setSingleAlt("G");
        v_line.setChromosome("chr3");
        v_line.setPos(195956746);
        v_line.leftNormalize(ref_file);
        I_EQUAL(v_line.start(), 195956726);
        I_EQUAL(v_line.end(), 195956728);
        S_EQUAL(v_line.ref(), "AAG");
        S_EQUAL(v_line.alt(0), "A");

        //DEL (no block shift, but part of sequence before and after match)
        //leftNormalize inside conversion of GSvar to VCF
        v = Variant("chr4", 88536883, 88536900, "TAGCAGTGACAGCAGCAA", "-");
        v_list.clear();
        v_list.append(v);
        vcf_file = VcfFile::convertGSvarToVcf(v_list, ref_file);
        I_EQUAL(vcf_file.count(), 1);
        v_line = vcf_file[0];
        I_EQUAL(v_line.start(), 88536868);
        I_EQUAL(v_line.end(), 88536886);
        S_EQUAL(v_line.ref(), "TAGTGACAGCAGCAATAGC");
        S_EQUAL(v_line.alt(0), "T");

        //leftNormalize of VCF line
        v_line.setRef("ATAGCAGTGACAGCAGCAA");
        v_line.setSingleAlt("A");
        v_line.setChromosome("chr4");
        v_line.setPos(88536882);
        v_line.leftNormalize(ref_file);
        I_EQUAL(v_line.start(), 88536868);
        I_EQUAL(v_line.end(), 88536886);
        S_EQUAL(v_line.ref(), "TAGTGACAGCAGCAATAGC");
        S_EQUAL(v_line.alt(0), "T");
    }

    void overlapsWithComplete()
    {
        VcfLine variant;
        variant.setChromosome("chr1");
        variant.setPos(5);
        variant.setRef("NNNNNN");
        variant.setSingleAlt("NNNNNN");
        IS_TRUE(!variant.overlapsWith("chr2", 5, 10));
        IS_TRUE(!variant.overlapsWith("chr1", 1, 4));
        IS_TRUE(!variant.overlapsWith("chr1", 11, 20));
        IS_TRUE(variant.overlapsWith("chr1", 1, 5));
        IS_TRUE(variant.overlapsWith("chr1", 5, 10));
        IS_TRUE(variant.overlapsWith("chr1", 6, 8));
        IS_TRUE(variant.overlapsWith("chr1", 10, 20));
        IS_TRUE(variant.overlapsWith("chr1", 1, 20));
    }

    void overlapsWithPosition()
    {
        VcfLine variant;
        variant.setChromosome("chr1");
        variant.setPos(5);
        variant.setRef("NNNNNN");
        variant.setSingleAlt("NNNNNN");
        IS_TRUE(variant.overlapsWith(5, 10));
        IS_TRUE(!variant.overlapsWith(1, 4));
        IS_TRUE(!variant.overlapsWith(11, 20));
        IS_TRUE(variant.overlapsWith(1, 5));
        IS_TRUE(variant.overlapsWith(5, 10));
        IS_TRUE(variant.overlapsWith(6, 8));
        IS_TRUE(variant.overlapsWith(10, 20));
        IS_TRUE(variant.overlapsWith(1, 20));
    }


    void overlapsWithBedLine()
    {
        VcfLine variant;
        variant.setChromosome("chr1");
        variant.setPos(5);
        variant.setRef("NNNNNN");
        variant.setSingleAlt("NNNNNN");		IS_TRUE(!variant.overlapsWith(BedLine("chr2", 5, 10)));
        IS_TRUE(!variant.overlapsWith(BedLine("chr1", 1, 4)));
        IS_TRUE(!variant.overlapsWith(BedLine("chr1", 11, 20)));
        IS_TRUE(variant.overlapsWith(BedLine("chr1", 1, 5)));
        IS_TRUE(variant.overlapsWith(BedLine("chr1", 5, 10)));
        IS_TRUE(variant.overlapsWith(BedLine("chr1", 6, 8)));
        IS_TRUE(variant.overlapsWith(BedLine("chr1", 10, 20)));
        IS_TRUE(variant.overlapsWith(BedLine("chr1", 1, 20)));
    }

    void operator_lessthan()
    {
        VcfLine variant_1;
        variant_1.setChromosome("chr1");
        variant_1.setPos(1);
        variant_1.setRef("NNNNN");
        variant_1.setSingleAlt("NNNNN");

        VcfLine variant_2;
        variant_2.setChromosome("chr1");
        variant_2.setPos(5);
        variant_2.setRef("NNNNN");
        variant_2.setSingleAlt("NNNNN");

        VcfLine variant_3;
        variant_3.setChromosome("chr2");
        variant_3.setPos(1);
        variant_3.setRef("NNNNN");
        variant_3.setSingleAlt("NNNNN");

        VcfLine variant_4;
        variant_4.setChromosome("chr2");
        variant_4.setPos(5);
        variant_4.setRef("NNNNN");
        variant_4.setSingleAlt("NNNNN");

        IS_FALSE(variant_1 < variant_1);
        IS_TRUE(variant_1 <variant_2);
        IS_FALSE(variant_3 < variant_1);
        IS_TRUE(variant_1 < variant_4);
    }

};
