#include "TabixIndexedFile.h"
#include "Exceptions.h"
#include "Chromosome.h"
#include "QFile"
#include <htslib/bgzf.h>

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

	//determine index file
	filename_index_ = filename + ".csi";
	if (!QFile::exists(filename_index_)) filename_index_ = filename + ".tbi";
	if (!QFile::exists(filename_index_)) THROW(FileAccessException, "Could not determine tabix index of file " + filename_);

	//load index
	tbx_ = tbx_index_load3(filename.data(), filename_index_.data(), HTS_IDX_SAVE_REMOTE);
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

QByteArray TabixIndexedFile::format() const
{
	int fmt = hts_idx_fmt(tbx_->idx);
	if (fmt==HTS_FMT_CSI) return "CSI";
	if (fmt==HTS_FMT_TBI) return "TBI";
	THROW(ProgrammingException, "Invalid tabix index format: " + QString::number(fmt));
}

int TabixIndexedFile::minShift() const
{
	struct CsiHeader {
		uint32_t magic;
		int32_t min_shift;
		int32_t depth;
		int32_t aux_len;
	};
	CsiHeader hdr;

	BGZF *fp = bgzf_open(filename_index_.data(), "r");
	if (!fp) THROW(FileAccessException, "Could not open index file for reading: " + filename_index_);

	if (bgzf_read(fp, &hdr, sizeof(hdr)) != sizeof(hdr))
	{
		bgzf_close(fp);
		THROW(FileAccessException, "Could not read header of index file: " + filename_index_);
	}
	bgzf_close(fp);

	// magic bytes are stored little-endian
	if (hdr.magic != 0x01495343) THROW(FileAccessException, "Not a valid CSI header " + QString::number(hdr.magic, 16).rightJustified(8, '0') + " in " + filename_index_);

	return hdr.min_shift;
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
