#pragma once
#include <QTcpServer>
#include <QSslKey>
#include <QSslCertificate>

class SslTcpServer : public QTcpServer
{
	Q_OBJECT

public:
	SslTcpServer(QObject *parent = nullptr);
	SslTcpServer(QObject *parent, const QString & keyPath, const QString & crtPath);
	~SslTcpServer();
protected:
	void incomingConnection(qintptr socketDescriptor);
private:
	QString keyPath;
	QString crtPath;
};