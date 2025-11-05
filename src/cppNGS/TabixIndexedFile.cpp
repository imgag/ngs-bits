#include "TabixIndexedFile.h"
#include "Exceptions.h"
#include "Chromosome.h"

TabixIndexedFile::TabixIndexedFile()
	: file_(nullptr)
	, tbx_(nullptr)
{
}

TabixIndexedFile::~TabixIndexedFile()
{
	clear();
}

void TabixIndexedFile::load(QByteArray filename)
{
	clear();

	//store file name
	filename_ = filename;

	//open data file
	file_ = hts_open(filename.data(), "r");
	if (file_ == nullptr) THROW(FileParseException, "Could not open data file " + filename_);

	//load index
	tbx_ = tbx_index_load(filename.data());
	if (tbx_ == nullptr) THROW(FileParseException, "Could not load tabix index of " + filename_);
	//create dictionary of chromosome identifiers
	int nseq;
	const char** seq = tbx_seqnames(tbx_, &nseq);
	for (int i=0; i<nseq; i++)
	{
		int tabix_id = tbx_name2id(tbx_, seq[i]);
		int ngsbits_id = Chromosome(seq[i]).num();
		chr2chr_[ngsbits_id] = tabix_id;
	}
	free(seq);
}

void TabixIndexedFile::clear()
{
	filename_.clear();

	if (tbx_!=nullptr) tbx_destroy(tbx_);
	tbx_ = nullptr;

	if (file_!=nullptr) hts_close(file_);
	file_ = nullptr;

	chr2chr_.clear();
}

QByteArrayList TabixIndexedFile::getMatchingLines(const Chromosome& chr, int start, int end, bool ignore_missing_chr) const
{
	QByteArrayList output;

	//get chromsome identifier
	int chr_id = chr2chr_.value(chr.num(), -1);
	if (chr_id==-1)
	{
		if (ignore_missing_chr)
		{
			return output;
		}
		else
		{
			THROW(ProgrammingException, "Chromosome '"+chr.str() + "' not found in tabix index of " + filename_);
		}
	}

	hts_itr_t* itr = tbx_itr_queryi(tbx_, chr_id, start-1, end);

	if (!itr) THROW(FileParseException, "Error while parsing the index file for " + filename_ + ".");

	kstring_t str = {0, 0, nullptr};
	int r;
	while(r=tbx_itr_next(file_, tbx_, itr, &str), r>=0)
	{
		output << QByteArray(str.s);
	}
	tbx_itr_destroy(itr);
	free(str.s);
	if (r < -1) THROW(FileParseException, "Error while accessing file through the index file for " + filename_ + ".");

	return output;
}
