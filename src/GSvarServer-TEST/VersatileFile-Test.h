#ifndef VERSATILEFILETEST_H
#define VERSATILEFILETEST_H

#include "TestFramework.h"
#include "VersatileFile.h"
#include "HttpRequestHandler.h"
#include "ServerHelper.h"
#include "NGSHelper.h"

TEST_CLASS(VersatileFile_Test)
{
	Q_OBJECT
	private slots:
		void test_metadata()
		{
			if (!ServerHelper::hasBasicSettings())
			{
				SKIP("Server has not been configured correctly");
			}
			const QString bam_file = NGSHelper::serverApiUrl() + "bam/rna.bam";

			VersatileFile file_over_https = VersatileFile(bam_file);
			IS_TRUE(file_over_https.exists());
			S_EQUAL(file_over_https.fileName(), bam_file)
			I_EQUAL(file_over_https.size(), 117570);
			IS_TRUE(file_over_https.isReadable());
		}

		void test_readability()
		{
			if (!ServerHelper::hasBasicSettings())
			{
				SKIP("Server has not been configured correctly");
			}

			const QString html_file = NGSHelper::serverApiUrl();

			QFile *local_asset_file = new QFile(":/assets/client/info.html");
			local_asset_file->open(QIODevice::ReadOnly);
			QByteArray asset_file_content = local_asset_file->readAll();

			VersatileFile index_page_file = VersatileFile(html_file);
			QByteArray index_page_content = index_page_file.readAll();
			S_EQUAL(asset_file_content, index_page_content);

			IS_TRUE(index_page_file.atEnd());
			index_page_file.seek(0);
			IS_FALSE(index_page_file.atEnd());
			QByteArray first_line = index_page_file.readLine();
			S_EQUAL(first_line.trimmed(), "<!doctype html>");
			I_EQUAL(index_page_file.pos(), 16);
			index_page_file.seek(10);
			QByteArray line_fragment = index_page_file.read(3);
			S_EQUAL(line_fragment, "html");
			I_EQUAL(index_page_file.pos(), 13);
		}
};

#endif // VERSATILEFILETEST_H
