#include <QCoreApplication>
#include "htslib/sam.h"
#include "htslib/cram.h"
#include <QDebug>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

//	samFile *f = sam_open("https://download.imgag.de/ahsturm1/NA12878_03/NA12878_03.bam", "r");
	samFile *f = sam_open("http://srv011.img.med.uni-tuebingen.de/NA12878_03.bam", "r");

	qDebug() << f->is_cram;
	sam_hdr_t* header = sam_hdr_read(f);
	qDebug() << header;
	return a.exec();
}
