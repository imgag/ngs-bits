#include "TestFramework.h"
#include "StringPool.h"
#include "TsvFile.h"
#include <QTemporaryFile>
#include <array>
#include <atomic>
#include <thread>

TEST_CLASS(StringPool_Test)
{
private:
	TEST_METHOD(sharingAndMutation)
	{
		StringPool::clean();
		const QString first = StringPool::get(QString("shared string"));
		const QString second = StringPool::get(QString("shared string"));
		IS_TRUE(first.constData() == second.constData());
		QString changed = first;
		changed[0] = 'X';
		S_EQUAL(first, "shared string");
		S_EQUAL(changed, "Xhared string");

		const QByteArray bytes = StringPool::get(QByteArray("shared bytes"));
		const QByteArray other_bytes = StringPool::get(QByteArray("shared bytes"));
		IS_TRUE(bytes.constData() == other_bytes.constData());
		QByteArray changed_bytes = bytes;
		changed_bytes[0] = 'X';
		S_EQUAL(bytes, "shared bytes");
		S_EQUAL(changed_bytes, "Xhared bytes");
		S_EQUAL(StringPool::get(QString(" Case ")), " Case ");
		S_EQUAL(StringPool::get(QByteArray(" Case ")), " Case ");
		IS_TRUE(StringPool::get(QString()).isNull());
		IS_FALSE(StringPool::get(QString("")).isNull());
		IS_TRUE(StringPool::get(QByteArray()).isNull());
		IS_FALSE(StringPool::get(QByteArray("")).isNull());
	}

	TEST_METHOD(inputOwnership)
	{
		StringPool::clean();
		char data[] = {'a', '\0', 'b'};
		const QByteArray bytes = StringPool::get(QByteArray::fromRawData(data, 3));
		data[0] = 'X';
		S_EQUAL(bytes, QByteArray("a\0b", 3));
		IS_TRUE(bytes.constData() == StringPool::get(QByteArray("a\0b", 3)).constData());
		QChar text[] = {QChar('a'), QChar(0), QChar(0x00e4)};
		const QString value = StringPool::get(QString::fromRawData(text, 3));
		text[0] = QChar('X');
		S_EQUAL(value, QString::fromUtf8("a\0\xc3\xa4", 4));
	}

	TEST_METHOD(clean)
	{
		StringPool::clean();
		const QString text = StringPool::get(QString("retained text"));
		const QByteArray bytes = StringPool::get(QByteArray("retained bytes"));
		StringPool::clean();
		StringPool::clean();
		S_EQUAL(text, "retained text");
		S_EQUAL(bytes, "retained bytes");
		//The old values keep their buffers alive, so these pointers must differ.
		const QString new_text = StringPool::get(QString("retained text"));
		const QByteArray new_bytes = StringPool::get(QByteArray("retained bytes"));
		IS_TRUE(text.constData() != new_text.constData());
		IS_TRUE(bytes.constData() != new_bytes.constData());
		IS_TRUE(new_text.constData() == StringPool::get(QString("retained text")).constData());
		IS_TRUE(new_bytes.constData() == StringPool::get(QByteArray("retained bytes")).constData());
	}

	TEST_METHOD(sharingAcrossTables)
	{
		QTemporaryFile file;
		IS_TRUE(file.open());
		file.write("#column\nshared table cell\n");
		file.close();
		StringPool::clean();
		TsvFile first;
		first.load(file.fileName(), true);
		TsvFile second;
		second.load(file.fileName(), true);
		IS_TRUE(first[0][0].constData() == second[0][0].constData());
		StringPool::clean();
		S_EQUAL(first[0][0], "shared table cell");
		S_EQUAL(second[0][0], "shared table cell");
	}

	TEST_METHOD(concurrentGetAndClean)
	{
		StringPool::clean();
		constexpr int count = 8;
		std::array<std::thread, count> threads;
		std::array<QString, count> texts;
		std::array<QByteArray, count> bytes;
		std::array<bool, count> success{};
		std::atomic<int> ready{0};
		for (int t = 0; t < count; ++t)
		{
			threads[t] = std::thread([&, t]()
			{
				++ready;
				while (ready.load() != count) std::this_thread::yield();
				texts[t] = StringPool::get(QString("concurrent text"));
				bytes[t] = StringPool::get(QByteArray("concurrent bytes"));
			});
		}
		for (auto& thread : threads) thread.join();
		for (int t = 0; t < count; ++t)
		{
			IS_TRUE(texts[t].constData() == texts[0].constData());
			IS_TRUE(bytes[t].constData() == bytes[0].constData());
		}

		ready = 0;
		for (int t = 0; t < count; ++t)
		{
			threads[t] = std::thread([&, t]()
			{
				++ready;
				while (ready.load() != count) std::this_thread::yield();
				success[t] = true;
				for (int i = 0; i < 1000; ++i)
				{
					const QString expected = "parallel text " + QString::number(i % 64);
					const QByteArray expected_bytes = expected.toUtf8();
					const QString text = StringPool::get(expected);
					const QByteArray value = StringPool::get(expected_bytes);
					if (t == 0 && i % 8 == 0) StringPool::clean();
					success[t] = success[t] && text == expected && value == expected_bytes;
				}
			});
		}
		for (auto& thread : threads) thread.join();
		for (bool result : success) IS_TRUE(result);
		StringPool::clean();
	}
};
