#include "OutputWorker.h"

OutputWorker::OutputWorker(ReadPairPool& pair_pool, QString out1, QString out2, int compression_level)
	: QRunnable()
	, terminate_(false)
	, pair_pool_(pair_pool)
	, ostream1_(new FastqOutfileStream(out1, compression_level))
	, ostream2_(new FastqOutfileStream(out2, compression_level))
{
	setAutoDelete(false);
}

OutputWorker::OutputWorker(ReadPairPool& pair_pool, QString out, int compression_level)
	: QRunnable()
	, terminate_(false)
	, pair_pool_(pair_pool)
	, ostream1_(new FastqOutfileStream(out, compression_level))
	, ostream2_(nullptr)
{
	setAutoDelete(false);
}

void OutputWorker::run()
{
	while(!terminate_)
	{
		for (int j=0; j<pair_pool_.count(); ++j)
		{
			ReadPair& pair = pair_pool_[j];
			if (pair.status!=ReadPair::TO_BE_WRITTEN) continue;

			//write output
			try
			{
				ostream1_->write(pair.e1);
				if (!ostream2_.isNull()) ostream2_->write(pair.e2);
				pair.status = ReadPair::FREE;
			}
			catch(Exception& e)
			{
				pair.status = ReadPair::ERROR;
				pair.error_message = e.message();
				terminate();
			}
		}
	}
}

ReadPairPool::ReadPairPool(int initial_size)
{
	while(size()<initial_size)
	{
		append(ReadPair());
	}
}

ReadPair& ReadPairPool::nextFreePair()
{
	while(true)
	{
		for (int i=0; i<size(); ++i)
		{
			ReadPair& pair = operator[](i);
			if (pair.status==ReadPair::ERROR)
			{
				THROW(Exception, pair.error_message);
			}

			if (pair.status==ReadPair::FREE)
			{
				return pair;
			}
		}
	}
}

void ReadPairPool::waitAllWritten() const
{
	while(true)
	{
		int done = 0;
		for (int i=0; i<size(); ++i)
		{
			const ReadPair& pair = operator[](i);
			if (pair.status==ReadPair::ERROR)
			{
				THROW(Exception, pair.error_message);
			}

			if (pair.status==ReadPair::FREE)
			{
				++done;
			}
		}
		if (done==size()) break;
	}
}
