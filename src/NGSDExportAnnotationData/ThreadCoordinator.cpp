#include "ThreadCoordinator.h"
#include "ExportWorker.h"
#include "NGSD.h"
#include <QElapsedTimer>

ThreadCoordinator::ThreadCoordinator(QObject* parent, const GermlineParameters& params)
	: QObject(parent)
	, params_(params)
	, shared_data_()
	, thread_pool_()
	, out_(stdout)
{
	thread_pool_.setMaxThreadCount(params.threads);

	NGSD db(params.use_test_db);
	log("coordinator", "Parameters: " + params_.toString());

	//cache processed sample infos
	log("coordinator", "Caching sample data");
	SqlQuery query = db.getQuery();
	query.exec("SELECT ps.id, ps.quality, s.id, s.disease_status, s.disease_group FROM processed_sample ps, sample s WHERE ps.sample_id=s.id");
	while(query.next())
	{
		int id = query.value(0).toInt();
		ProcessedSampleInfo info;
		info.bad_quality = query.value(1).toString()=="bad";
		info.s_id = query.value(2).toInt();
		info.affected = query.value(3).toString()=="Affected";
		info.disease_group = query.value(4).toString();
		shared_data_.ps_infos.insert(id, info);
	}

	//cache classification data
	log("coordinator", "Caching classification data");
	query.exec("SELECT variant_id, class, comment FROM variant_classification");
	while(query.next())
	{
		int variant_id =query.value(0).toInt();
		ClassificationData info;
		info.classification = query.value(1).toByteArray().trimmed().replace("n/a", "");
		info.comment = VcfFile::encodeInfoValue(query.value(2).toByteArray()).toUtf8();
		shared_data_.class_infos.insert(variant_id, info);
	}

	//start analysis
	log("coordinator", "Starting export of variants");
	shared_data_.chrs = db.getEnum("variant", "chr");
	foreach (QString chr, shared_data_.chrs)
	{
		QString chr_tmp_file = params_.output_file.mid(0, params_.output_file.length()-4) + "_" + chr + ".vcf";
		shared_data_.chr2vcf.insert(chr,  chr_tmp_file);

		ExportWorker* worker = new ExportWorker(chr, params_, shared_data_);
		connect(worker, SIGNAL(log(QString,QString)), this, SLOT(log(QString,QString)));
		connect(worker, SIGNAL(done(QString)), this, SLOT(done(QString)));
		connect(worker, SIGNAL(error(QString,QString)), this, SLOT(error(QString,QString)));
		thread_pool_.start(worker);
	}
}

ThreadCoordinator::~ThreadCoordinator()
{
	if (params_.verbose) log("coordinator", "~ThreadCoordinator");
}

void ThreadCoordinator::log(QString chr, QString message)
{
	out_ << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "\t" << chr << "\t" << message << endl;
}

void ThreadCoordinator::error(QString chr, QString message)
{
	//QTextStream(stdout) << "ThreadCoordinator::error " << i << " " << message << endl;
	THROW(Exception, "Exception in worker for " + chr + ": " + message);
}

void ThreadCoordinator::writeVcf()
{
	QElapsedTimer timer;
	timer.start();

	log("coordinator", "Starting the merge of VCF files");

	FastaFileIndex reference_file(params_.ref_file);
	NGSD db(params_.use_test_db);

	QSharedPointer<QFile> vcf_file = Helper::openFileForWriting(params_.output_file, true);
	QTextStream vcf_stream(vcf_file.data());
	vcf_stream << "##fileformat=VCFv4.2\n";
	vcf_stream << "##fileDate=" << QDate::currentDate().toString("yyyyMMdd") << "\n";
	vcf_stream << "##source=NGSDExportAnnotationData " << params_.version << "\n";
	vcf_stream << "##reference=" << params_.ref_file << "\n";

	foreach (const QString& chr_name, shared_data_.chrs)
	{
		int chr_length = reference_file.lengthOf(Chromosome(chr_name));
		vcf_stream << "##contig=<ID=" << chr_name << ",length=" << chr_length << ">\n";
	}

	vcf_stream << "##INFO=<ID=COUNTS,Number=3,Type=Integer,Description=\"Homozygous/Heterozygous/Mosaic variant counts in NGSD.\">\n";

	// create info column entry for all disease groups
	QStringList disease_groups = db.getEnum("sample", "disease_group");
	for(int i = 0; i < disease_groups.size(); i++)
	{
		vcf_stream << "##INFO=<ID=GSC" << QByteArray::number(i + 1).rightJustified(2, '0') << ",Number=2,Type=Integer,Description=\"" << "Homozygous/Heterozygous variant counts in NGSD for " << disease_groups[i].toLower() << ".\">\n";
	}
	vcf_stream << "##INFO=<ID=HAF,Number=0,Type=Flag,Description=\"Indicates a allele frequency above a threshold of " << QString::number(params_.max_af, 'f', 2) << ".\">\n";
	vcf_stream << "##INFO=<ID=CLAS,Number=1,Type=String,Description=\"Classification from the NGSD.\">\n";
	vcf_stream << "##INFO=<ID=CLAS_COM,Number=1,Type=String,Description=\"Classification comment from the NGSD.\">\n";
	vcf_stream << "##INFO=<ID=COM,Number=1,Type=String,Description=\"Variant comments from the NGSD.\">\n";

	// write header line
	vcf_stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\n";

	//merge VCFs and delete temporary files
	foreach(QString chr, shared_data_.chrs)
	{
		auto infile = Helper::openFileForReading(shared_data_.chr2vcf[chr]);
		while(!infile->atEnd())
		{
			QByteArray line = infile->readLine();
			if (line.length()>4) vcf_stream << line;
		}
	}

	log("coordinator", "Runtime VCF merge: " + getTimeString(timer.elapsed()));
}

void ThreadCoordinator::done(QString chr)
{
	chrs_done_ << chr;

	bool variant_export_done = shared_data_.chr2vcf.count()==chrs_done_.count();
	if (!variant_export_done) return;

	//merge VCFs
	writeVcf();

	//export gene information
	if (!params_.genes.isEmpty())
	{
		exportGeneInformation();
	}

	//emit finished signal to termine main application
	emit finished();
}



void ThreadCoordinator::exportGeneInformation()
{
	QElapsedTimer timer;
	timer.start();

	log("coordinator", "Starting gene export");

	//init
	BedFile output_bed_file;

	//process all genes
	NGSD db(params_.use_test_db);
	GeneSet genes = db.approvedGeneNames();
	foreach (const QByteArray& gene, genes)
	{
		//get gene infos
		GeneInfo gene_info = db.geneInfo(gene);
		QByteArrayList annotations;
		annotations << gene + " (inh=" + gene_info.inheritance.toUtf8()  + " oe_syn=" + gene_info.oe_syn.toUtf8() + " oe_mis="+ gene_info.oe_mis.toUtf8() + " oe_lof=" + gene_info.oe_lof.toUtf8() + ")";

		//calculate region - several is sometimes also possible
		BedFile gene_region = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
		gene_region.extend(params_.gene_offset);
		gene_region.merge();

		//write regions with annotations
		for(int i=0; i<gene_region.count(); ++i)
		{
			output_bed_file.append(BedLine(gene_region[i].chr().strNormalized(true), gene_region[i].start(), gene_region[i].end(), annotations));
		}
	}

	// sort bed flie and write to file
	output_bed_file.sort();
	output_bed_file.store(params_.genes);

	log("coordinator", "Finished gene export");
	log("coordinator", "Exported genes: " + QString::number(genes.count()));
	log("coordinator", "Runtime gene export: " + getTimeString(timer.elapsed()));
}
