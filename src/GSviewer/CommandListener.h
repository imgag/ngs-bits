#ifndef COMMANDLISTENER_H
#define COMMANDLISTENER_H

#include <QLocalServer>
#include <QObject>
#include <QTcpServer>

class CommandListener : public QObject
{
	Q_OBJECT

protected:
	explicit CommandListener(QString addr, QObject* parent = nullptr);

public:
	static std::unique_ptr<CommandListener> fromString(const QString& address, QObject* parent = nullptr);

	virtual void bind() = 0;
	virtual void terminate() = 0;

protected:
	QString address_;

signals:
	void newConnection(QIODevice* socket);
	void closedConnection(QIODevice* socket);
};

class TcpListener : public CommandListener
{
private:
	QTcpServer sock_;
	quint16 port_;

public:
	TcpListener(const quint16 port, QObject* parent = nullptr);

	void bind() override;
	void terminate() override;
};

class LocalListener : public CommandListener
{
private:
	QLocalServer sock_;

public:
	LocalListener(const QString& path, QObject* parent);

	void bind() override;
	void terminate() override;
};

#endif // COMMANDLISTENER_H
