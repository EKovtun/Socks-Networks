#pragma once

#include <QObject>
#include <QHostAddress>
#include "ssltcpserver.h"


class SocksListener : public QObject
{
	Q_OBJECT

public:
	SocksListener(QObject *parent, const bool & usingSSL = false, const QString & crtPath = "", const QString & keyPath = "", const QString & dumpDir = "");
	bool startListen(const QHostAddress & addr = QHostAddress::Any, quint16 port = 1080);
	
	~SocksListener();
	
private slots:
	void onNewConnection();
private:
	SslTcpServer * listener;
	QString dumpDir;
	bool usingSSL;
	bool error;
};
