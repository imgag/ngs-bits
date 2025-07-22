#include "VariantAnnotator.h"
#include "LoginManager.h"
#include "ApiCaller.h"
#include "GlobalServiceProvider.h"
#include "Settings.h"

VariantAnnotator::VariantAnnotator(const VariantList& variants)
	: BackgroundWorkerBase("Variant annnotation")
	, variants_(variants)
{
}

void VariantAnnotator::process()
{
	if (!LoginManager::active()) THROW(ArgumentException, "This is supported in client-server mode only after successfull login!");

	QString gsvar_file = Helper::tempFileName(".GSvar");

	FastaFileIndex genome_idx(Settings::string("reference_genome"));
	VcfFile vcf;
	vcf.vcfHeader().addCommentLine(VcfHeaderLine{"SAMPLE", "<ID=DUMMY,DiseaseStatus=affected>"});
	vcf.vcfHeader().addCommentLine(VcfHeaderLine{"FORMAT", "<ID=GT,Number=1,Type=String,Description=\"Genotype\">"});
	vcf.setSampleNames(QByteArrayList() << "DUMMY");
	for(int i=0; i<variants_.count(); ++i)
	{
		VcfLine line = variants_[i].toVCF(genome_idx);
		line.setFormatKeys(QByteArrayList() << "GT");
		line.addFormatValues(QByteArrayList() << "0/1");
		vcf.append(line);
	}

	//call API
	QByteArray reply = ApiCaller().post("variant_annotation", RequestUrlParams(), HttpHeaders(), vcf.toText(), true, false, true);

	//store GSvar
	Helper::storeTextFile(gsvar_file, QStringList() << reply);

	//open GSvar
	GlobalServiceProvider::openGSvarFile(gsvar_file);
}
