#include "sockslistener.h"
#include "src/proxysocks.h"
#include "logger.h"
#include <QSslSocket>
#include <QFile>

SocksListener::SocksListener(QObject *parent, const bool & usingSSL, const QString & crtPath, const QString & keyPath, const QString & dumpDir)
	: QObject(parent)
	, usingSSL(usingSSL)
	, error(false)
	, dumpDir(dumpDir)
{
	if (usingSSL) {
		if (!QFile::exists(crtPath)) {
			Logger::messageInfoConsole("Certificate " + crtPath + " does not exist");
			error = true;
		}

		if (!QFile::exists(keyPath)) {
			Logger::messageInfoConsole("Key " + keyPath + " does not exist");
			error = true;
		}
	}
		
	listener = new SslTcpServer(this, keyPath, crtPath);
	connect(listener, &QTcpServer::newConnection, this, &SocksListener::onNewConnection);
}

SocksListener::~SocksListener()
{
}


bool SocksListener::startListen(const QHostAddress & addr, quint16 port) {
	if (error) return false;

	bool listen = listener->listen(addr, port);

	if (!listen)
		Logger::messageInfoConsole(listener->errorString());

	return listen;
}


void SocksListener::onNewConnection() {
	while (listener->hasPendingConnections()) {
		auto s = static_cast<QSslSocket*>(listener->nextPendingConnection());
		auto p = new ProxySocks(this, s, usingSSL, dumpDir);
		connect(p, &ProxySocks::finished, [p]() -> void { p->deleteLater(); });
		p->start();
	}
}
