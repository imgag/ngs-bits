#include "RefGenDownloadDialog.h"

RefGenDownloadDialog::RefGenDownloadDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
	, proxy_type_(HttpRequestHandler::ProxyType::NONE)
{
	ui_.setupUi(this);
	connect(ui_.start_btn, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	connect(ui_.cancel_btn, SIGNAL(clicked(bool)), this, SLOT(cancelDownload()));

	ui_.message->setText("The reference genome is required to use the entire set of features");
	bool genome_found = GSvarHelper::isGenomeFound();
	if (genome_found)
	{
		ui_.message->setText("The reference genome has been found locally");
		ui_.progressBar->setValue(100);
		ui_.start_btn->setEnabled(false);
		ui_.cancel_btn->setEnabled(false);
	}
	else
	{
		ui_.start_btn->setEnabled(true);
		ui_.cancel_btn->setEnabled(false);
	}

	is_interrupted_ = false;
	if ((!Settings::string("proxy_host").isEmpty()) && (!Settings::string("proxy_port").isEmpty()))
	{
		proxy_type_ = HttpRequestHandler::ProxyType::INI;
	}
}

void RefGenDownloadDialog::startDownload()
{
	is_interrupted_ = false;
	ui_.cancel_btn->setEnabled(true);
	bool genome_found = GSvarHelper::isGenomeFound();
	if (!genome_found)
	{
		ui_.message->setText("The reference genome is being downloaded");
		ui_.start_btn->setEnabled(false);
		static HttpHandler http_handler(proxy_type_);
		QString index_file_content = http_handler.get(Settings::string("remote_reference_genome") + ".fai");
		QFile index_file(Settings::string("reference_genome") + ".fai");
		if (!index_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		{
			ui_.message->setText("Cannot save the reference genome index locally");
		}
		QTextStream index_out(&index_file);
		index_out << index_file_content;
		index_file.close();

		qint64 reply = http_handler.getFileSize(Settings::string("remote_reference_genome"));
		QFile file(Settings::string("reference_genome"));
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		{
			ui_.message->setText("Cannot save the reference genome locally");
		}
		QTextStream out(&file);

		int chunk_count = 100;
		int chunks_completed = -1;
		qint64 chunk_size = reply / chunk_count;
		int remainder = reply % chunk_count;

		for (int i = 0; i < (chunk_count+1); i++)
		{
			chunks_completed++;
			if (is_interrupted_)
			{
				break;
			}
			ui_.progressBar->setValue(i);
			HttpHeaders headers {};
			QString range = "bytes=" + QString::number(i*chunk_size) + "-" + QString::number(((i+1)*chunk_size)-1);
			if (i == (chunk_count))
			{
				range = "bytes=" + QString::number(chunk_count*chunk_size) + "-" + QString::number((chunk_count*chunk_size) + remainder);
			}
			headers.insert("Range", range.toLocal8Bit());
			QString chunk = http_handler.get(Settings::string("remote_reference_genome"), headers);
			out << chunk;
		}
		file.close();
		qDebug() << "chunk_count = " << chunk_count << ", chunks_completed = " << chunks_completed;
		if (chunk_count == chunks_completed)
		{
			ui_.message->setText("Reference genome has been downloaded");
			ui_.start_btn->setEnabled(false);
			ui_.cancel_btn->setEnabled(false);
		}
		else
		{
			ui_.message->setText("Reference genome is incomplete. Try again");
			ui_.start_btn->setEnabled(true);
		}
	}
}

void RefGenDownloadDialog::cancelDownload()
{
	is_interrupted_ = true;
	ui_.message->setText("Download has been canceled");
	ui_.cancel_btn->setEnabled(false);
}

void RefGenDownloadDialog::closeEvent(QCloseEvent* e)
{
	qDebug() << "Clicked close";
	cancelDownload();
	e->accept();
}

void RefGenDownloadDialog::keyPressEvent(QKeyEvent *e)
{
	qDebug() << "Hit Esc";
	if (e->key() == Qt::Key_Escape) return;
	QDialog::keyPressEvent(e);
}
