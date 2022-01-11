#ifndef PROXYSOCKS_H
#define PROXYSOCKS_H

#include <QHostAddress>
#include <QSslSocket>
#include "errordescriptor.h"
#include "filedumper.h"

class ProxySocks : public QObject
{
	Q_OBJECT

public:
	ProxySocks(QObject *parent, QSslSocket * clientLink, const bool & usingSSL = false, const QString & dumpDir = "");
	~ProxySocks();
	void start();

private:
	QString clientTraceInfo;
	QString targetTraceInfo;
	//virtual void doStart();
	enum Stage { StageBegin, StageSocks4Command, StageSocks4Userid, StageSocks4aDNS, StageSocks5Methods, StageSocks5Auth, StageSocks5Command, StageConnectionWaiting, StageTransparent, StageBroken };
	void setStage(Stage stage, ErrorDescriptor ed = ErrorDescriptor());
	Stage stage;

	void parseSocksVersion();
	void parseSocks5MethodsRequest();

	void parseSocks4CommandRequest();
	void parseSocks5CommandRequest();

	void sendSocks4CommandResponse(unsigned char status, const QHostAddress & qaddr = QHostAddress::AnyIPv4, quint16 port = 0);
	void sendSocks5CommandResponse(unsigned char status, const QHostAddress & qaddr = QHostAddress::AnyIPv4, quint16 port = 0);

	void send(const QByteArray & data);
	void close_links();

	FileDumper * fileDumper;
	QByteArray * message;
	bool clientMessage;

	QByteArray mRecvBuff;
	qint64 mToRead;

	QSslSocket * clientLink;
	QSslSocket * targetLink;

	unsigned char version;		// socks protocol version
	std::string address;		// target address
	quint16 port;				// target port
	bool mConnectionDenied;
	bool usingSSL;
	enum Direction {
		Unknown = 0,
		Upload = 1,
		Download = 2
	} direction;
signals:
	void finished();

private slots:
	void onConnect();
	void onEncrypted();
	void onError(QAbstractSocket::SocketError error);
	void onDisconnect();
	void onReadyRead();
	void onTransparentRedirect();
	void onSslErrors(const QList<QSslError> & errors);
};

#endif // PROXYSOCKS_H
