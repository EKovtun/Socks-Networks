#include "ssltcpserver.h"
#include "logger.h"
#include <QSslSocket>

SslTcpServer::SslTcpServer(QObject *parent)
	: QTcpServer(parent)
{}

SslTcpServer::SslTcpServer(QObject *parent, const QString & keyPath, const QString & crtPath)
	: QTcpServer(parent)
	, keyPath(keyPath)
	, crtPath(crtPath)

{}

SslTcpServer::~SslTcpServer()
{}


void SslTcpServer::incomingConnection(qintptr socketDescriptor) {
	QSslSocket * s = new QSslSocket(this);
	if (s->setSocketDescriptor(socketDescriptor)) {
		s->setPeerVerifyMode(QSslSocket::VerifyNone);

		if (!crtPath.isEmpty() && !keyPath.isEmpty()) {
			s->setLocalCertificate(crtPath);
			s->setPrivateKey(keyPath);
		}

		s->setProtocol(QSsl::AnyProtocol);

		addPendingConnection(s);
	}
	else
		delete s;
}