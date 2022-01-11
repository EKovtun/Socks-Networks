#include "proxysocks.h"
#include "socksproto.h"
#include "socksutils.h"
#include "logger.h"
#include "hton.h"
#include <QByteArray>
#include <QNetworkProxy>

using namespace SOCKS;
using namespace SOCKS::detail;


ProxySocks::ProxySocks(QObject *parent, QSslSocket * clientLink, const bool & usingSSL, const QString & dumpDir)
	: QObject(parent)
	, port(0)
	, version(0)
	, stage(StageBegin)
	, clientLink(clientLink)
	, targetLink(nullptr)
	, mToRead(0)
	, mConnectionDenied(false)
	, usingSSL(usingSSL)
	, clientMessage(true)
	, direction(Unknown)
{
	mRecvBuff.reserve(SOCKS::MAX_SOCSK5_CMD);
	
	fileDumper = new FileDumper(this, dumpDir);
	message = new QByteArray();
}

ProxySocks::~ProxySocks() {}

void ProxySocks::start() {
	setStage(StageBegin);

	if (clientLink->bytesAvailable()) {
		emit clientLink->readyRead();
	}
}

void ProxySocks::setStage(Stage stage, ErrorDescriptor ed) {
	this->stage = stage;

	switch (this->stage) {
	case StageBegin:
		clientTraceInfo = QString("CLIENT ") + clientLink->peerAddress().toString() + ":" + QString::number(clientLink->peerPort());
		Logger::messageInfoConsole(clientTraceInfo + " accepted");

		mToRead = 0;

		connect(clientLink, &QSslSocket::readyRead, this, &ProxySocks::onReadyRead);
		connect(clientLink, &QSslSocket::disconnected, this, &ProxySocks::onDisconnect);

		targetLink = new QSslSocket(this);
		targetLink->setPeerVerifyMode(QSslSocket::VerifyNone);
		targetLink->setProtocol(QSsl::AnyProtocol);
		targetLink->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));

		connect(targetLink, &QSslSocket::connected, this, &ProxySocks::onConnect);
		connect(targetLink, static_cast<void (QSslSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &ProxySocks::onError);

		break;
	case StageSocks4Command:
		Logger::messageDebugConsole(clientTraceInfo + " USING SOCKS version 4");

		mToRead = sizeof(SOCKS4req) + 1; //плюс минимум один байт на терминатор USERID
		break;
	case StageSocks5Methods:
		Logger::messageDebugConsole(clientTraceInfo + " USING SOCKS version 5");

		mToRead = sizeof(SOCKS5NegotiationRequest);
		break;
	case StageSocks5Auth:
		// unsupported
		break;
	case StageSocks5Command:
		mToRead = sizeof(SOCKS5CommandRequest);
		break;
	case StageConnectionWaiting:
		targetTraceInfo = QString("TARGET ") + QString::fromStdString(address) + ":" + QString::number(port);
		Logger::messageDebugConsole(clientTraceInfo + " has requested a connection to " + targetTraceInfo);

		// Отключаем обработчик ReadyRead и не обращаем внимание на клиента пока не установится соедниение или не возникнет ошибка
		disconnect(clientLink, &QSslSocket::readyRead, this, &ProxySocks::onReadyRead);
		break;
	case StageTransparent:
		// Подключаем новые обработчики, обеспечивающие прозрачную пересылку данных
		connect(clientLink, &QSslSocket::readyRead, this, &ProxySocks::onTransparentRedirect);
		connect(targetLink, &QSslSocket::readyRead, this, &ProxySocks::onTransparentRedirect);
		connect(targetLink, &QSslSocket::disconnected, this, &ProxySocks::onDisconnect);

		// Если за момент как мы прицепили сигнал удаленных хост отвалился
		if (targetLink->state() == QAbstractSocket::UnconnectedState) {
			Logger::messageDebugConsole("targetLink has disconnected from host, and ProxySocks missed that disconnected() signal");
			fileDumper->messageTextDebug("targetLink has disconnected from host, and ProxySocks missed that disconnected() signal");
			close_links();
		}
		break;
	case StageBroken:
		Logger::messageInfoConsole(clientTraceInfo + " - stage broken. Error: " + ed.errorString() + ". Close connections");

		close_links();
	}

	// clear удаляет исходный буфер, resize не изменяет capacity
	mRecvBuff.resize(0);
}


void ProxySocks::onReadyRead() {
	while (auto avail = clientLink->bytesAvailable()) {
		try {
			// Вычитываем данные
			// Проверяем что достаточно места в буфере
			qint64 free = mRecvBuff.capacity() - mRecvBuff.size();
			if (free < mToRead) {
				setStage(StageBroken, ErrorDescriptor("SOCKS proxy buffer is too small"));
				return;
			}
			// Дописываем новую порцию данных в буфер и изменяем его размер
			if (mToRead) {
				auto readed = clientLink->read(mRecvBuff.data() + mRecvBuff.size(), qMin(free, mToRead));
				if (readed == -1) {
					setStage(StageBroken, ErrorDescriptor("Read error", ErrorDescriptor::ErrorCommon));
				}if (readed) {
					mRecvBuff.resize(mRecvBuff.size() + readed);
					mToRead -= readed;
				}
			}


			// в зависимости от состояния проверяем что будем делать
			switch (stage) {
			case StageBegin:
				parseSocksVersion();
				break;
			case StageSocks4Command:
				parseSocks4CommandRequest();
				break;
			case StageSocks4Userid:
				if (mRecvBuff.at(mRecvBuff.size() - 1))
					mToRead = 1;	// Читаем по одному символу пока не встретим null-терминатор
				else
					parseSocks4CommandRequest();
				break;
			case StageSocks4aDNS:
				if (mRecvBuff.at(mRecvBuff.size() - 1))
					mToRead = 1;	// Читаем по одному символу пока не встретим null-терминатор
				else
					parseSocks4CommandRequest();
				break;
			case StageSocks5Methods:
				//Согласование методов происходит только в SOCKS5
				parseSocks5MethodsRequest();
				break;
			case StageSocks5Auth:
				// Наш проки интерфейс работает (пока) без аутентификации
				break;
			case StageSocks5Command:
				parseSocks5CommandRequest();
				break;
			case StageBroken:
				// Обрываем клиентское соединение
				clientLink->readAll();
				return;
			case StageConnectionWaiting:
			case StageTransparent:
				// В этих состояниях мы не должны находиться в этом обработчике
				return;
			default:
				// Ошибка не должна появляться в рантайме
				setStage(StageBroken, ErrorDescriptor("Invalid internal ProxySocks state"));
				break;
			}

		}
		catch (NeedDataException & e) {
			mToRead = e.size();
		}
	}
}

void ProxySocks::parseSocksVersion() {
	// Проверяем первый байт не вычитывая его из буфера
	clientLink->peek((char*)&version, sizeof(version));

	switch (version) {
	case 0x04:
		setStage(StageSocks4Command, ErrorDescriptor());
		return;
	case 0x05:
		setStage(StageSocks5Methods, ErrorDescriptor());
		return;
	default:
		version = 0;
		setStage(StageBroken, ErrorDescriptor("Unsupported SOCKS version"));
		return;
	}
}

void ProxySocks::parseSocks4CommandRequest() {

	BuffReader buff(mRecvBuff.data(), mRecvBuff.data() + mRecvBuff.size());

	// Получаем ссылку на структуру SOCKS4-запроса 
	SOCKS::SOCKS4req * request = buff.get<SOCKS::SOCKS4req>();


	// Вычитывем userid если есть
	stage = StageSocks4Userid;
	auto userid = buff.getString();

	// Преобразуем адрес
	QHostAddress addr(Ntohl(request->addr));

	// Проверяем что адрес не 0.0.0.x
	if (addr.toIPv4Address() & 0xffffff00) {
		// если это нормальный IP-адрес - то действуем как обычно
		address = addr.toString().toStdString();
	}
	else {
		// иначе это расширение SOCKS4a и после USERID следует DNS-имя с null-терминатором
		stage = StageSocks4aDNS;
		address = buff.getString();
	}

	// Сохраняем номер порта
	port = Ntohs(request->port);

	switch (request->command) {
	case 0x01:
	{
		if (!mConnectionDenied) {
			// Запрос распарсили нормально - пытаемся установить соединение с целевым хостом
			targetLink->connectToHost(QString::fromStdString(address), port);

			// Переход в состояние когда нам надо дождаться ответа от сервера или получить отлуп
			setStage(StageConnectionWaiting);
		}
		else {
			// Иначе отправляем клиента восвояси
			sendSocks4CommandResponse(0x5b); // request rejected
			setStage(StageBroken, ErrorDescriptor("Exceeds the number of connections"));
		}
	}
	break;
	default:
	{
		// Иначе отправляем клиента восвояси
		sendSocks4CommandResponse(0x5b); // request rejected
		setStage(StageBroken, ErrorDescriptor("Unsupported SOCKS command"));
	}
	break;
	}
}


void ProxySocks::parseSocks5MethodsRequest() {
	BuffReader buff(mRecvBuff.data(), mRecvBuff.data() + mRecvBuff.size());

	SOCKS5NegotiationRequest * negRequest = buff.get<SOCKS5NegotiationRequest>();

	// Проверяем что клиент готов работать без аутентификации ("пролистываем" все методы, которые посылает клиент)
	bool without_auth(false);
	for (int i = 0; i < negRequest->nMethods; ++i) {
		if (negRequest->methods[i] == 0x00) {
			without_auth = true;
			break;
		}
	}

	// Отвечаем, что работаем без аутентификации или посылаем нафиг, если клиент так не хочет
	SOCKS5NegotiationResponse negResponse;

	if (without_auth) {
		negResponse.version = 0x05;	// Версия SOCKS
		negResponse.method = 0x00;	// Без аутентификации
		setStage(StageSocks5Command);
	}
	else {
		negResponse.version = 0x05;	// Версия SOCKS
		negResponse.method = 0xFF;	// Ничего не поддерживается, посылаем отлуп и обрываем соединение
		setStage(StageBroken, ErrorDescriptor("Client requires unsupported authentication mechanism"));
	}

	// Отправляем ответ клиенту 
	send(QByteArray::fromRawData((const char *)&negResponse, sizeof(negResponse)));
}


void ProxySocks::parseSocks5CommandRequest() {
	BuffReader buff(mRecvBuff.data(), mRecvBuff.data() + mRecvBuff.size());

	// Считываем команду на установку соединения
	SOCKS5CommandRequest * commRequest = buff.get<SOCKS5CommandRequest>();

	// Если это не SOCKS5 - материмся и выходим
	if (commRequest->version != 0x05) {
		setStage(StageBroken, ErrorDescriptor("Bad SOCKS signature"));
		return;
	}

	// Тип команды
	switch (commRequest->command) {
	case (0x01): // CONNECT
		break;
	case (0x02): // BIND
		sendSocks5CommandResponse(0x07);	// command not supported
		setStage(StageBroken, ErrorDescriptor("SOCKS5 bind is not supported"));
		return;
		break;
	case (0x03): // UDP ASSOCIATE
		sendSocks5CommandResponse(0x07);	// command not supported
		setStage(StageBroken, ErrorDescriptor("SOCKS5 udp  associate is not supported"));
		return;
		break;
	default:
		sendSocks5CommandResponse(0x07);	// command not supported
		setStage(StageBroken, ErrorDescriptor("Unknown SOCKS5 command"));
		return;
		break;
	}

	// Считываем адрес хоста назначения
	switch (commRequest->addr_type) {
	case (0x01): // IP v4
	{
		SOCKS::IPv4Addr  * addr = buff.get<SOCKS::IPv4Addr>();
		QHostAddress qaddr(Ntohl(addr->addr));
		address = qaddr.toString().toStdString();
	}
	break;
	case (0x03): // DNS 
	{
		const SOCKS::DNSName * addr = buff.get<SOCKS::DNSName>();
		address.assign((char*)addr->addr, addr->length);
	}
	break;
	case(0x04):  // IP v6
	{
		const SOCKS::IPv6Addr * addr = buff.get<SOCKS::IPv6Addr>();
		QHostAddress qaddr((quint8 *)addr->addr);
		address = qaddr.toString().toStdString();
	}
	break;
	default:
		sendSocks5CommandResponse(0x08);
		setStage(StageBroken, ErrorDescriptor("Address type given by client is unsupported"));
		return;
		break;
	}

	// Считываем номер порта
	port = Ntohs(*buff.get<quint16>());

	// Все распарсили, пытаемся подключиться к целевому хосту

	if (!mConnectionDenied) {
		// Если соединение разрешено
		targetLink->connectToHost(QString::fromStdString(address), port);

		// Переходим в режим ожидания, пока не получим ответ от сервера или отлуп
		setStage(StageConnectionWaiting);
	}
	else {
		// Иначе(если больше подключаться не можем), то отключаем клиента
		sendSocks5CommandResponse(0x02);	// 0x02 - connection is denied by ruleset
		setStage(StageBroken, ErrorDescriptor("Address type given by client is unsupported"));
	}
}


void ProxySocks::sendSocks4CommandResponse(unsigned char status, const QHostAddress & qaddr, quint16 port) {
	// Готовим пакет на отправку
	SOCKS4resp resp;
	resp.nullb = 0;
	resp.status = status;
	resp.port = Htons(port);
	resp.addr = Htonl(qaddr.toIPv4Address());

	// отправляем клиенту 
	send(QByteArray::fromRawData((const char*)&resp, sizeof(resp)));
}


void ProxySocks::sendSocks5CommandResponse(unsigned char status, const QHostAddress & qaddr, quint16 port) {
	// Выделяем статический буфер для формирования команды
	char buf[SOCKS::MAX_SOCSK5_CMD];

	SOCKS::SOCKS5CommandResponse * response = (SOCKS::SOCKS5CommandResponse *)buf;

	// Устанавиваем заголовок пакета
	response->version = 0x05;	// version
	response->reply = status;	// reply code
	response->reserved = 0;		// must be a null

	SOCKS::byte * ptr = (SOCKS::byte *)response->addr;

	// Устанавливаем BIND-адрес
	switch (qaddr.protocol()) {
	case(QAbstractSocket::IPv6Protocol):
	{
		response->addr_type = 0x04;	//IPv6
		auto taddr = qaddr.toIPv6Address();
		memcpy(ptr, taddr.c, 16);
		ptr += 16;
	}
	break;
	default:
	{
		response->addr_type = 0x01;	//IPv4
		auto taddr = Htonl(qaddr.toIPv4Address());
		memcpy(ptr, &taddr, 4);
		ptr += 4;
	}
	break;
	}

	// Устанавливаем номер порта
	quint16 nport = Htons(port);
	memcpy(ptr, &nport, 2);
	ptr += 2;

	// Отправляем ответ клиенту
	auto dataSz = ptr - (SOCKS::byte*)buf;
	send(QByteArray::fromRawData(buf, dataSz));
}


void ProxySocks::send(const QByteArray & data) {
	auto szWrited = clientLink->write(data);
	if (szWrited < data.size()) {
		setStage(StageBroken, ErrorDescriptor(clientLink->errorString()));
	}
}

void ProxySocks::onConnect() {
	
	fileDumper->startWriting(clientLink->peerAddress().toString(), targetLink->peerAddress().toString());
	
	//fileDumper->messageTextInfo(clientTraceInfo + " accepted");

	Logger::messageDebugConsole(targetTraceInfo + " connected successfully");
	//fileDumper->messageTextInfo(targetTraceInfo + " connected successfully");

	// Отправляем клиенту ответ, что всё хорошо
	switch (version) {
	case(0x04):
		sendSocks4CommandResponse(0x5a);
		break;
	case(0x05):
		sendSocks5CommandResponse(0x00);
		break;
	default:
		Q_ASSERT(false);
	}

	//Активируем SSL рукопожатия с обеими сторонами и ожидаем его завершения
	if (usingSSL) {
		connect(clientLink, &QSslSocket::encrypted, this, &ProxySocks::onEncrypted);
		connect(targetLink, &QSslSocket::encrypted, this, &ProxySocks::onEncrypted);
		connect(targetLink, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(onSslErrors(const QList<QSslError> &)));

		clientLink->startServerEncryption();
		if (!clientLink->waitForEncrypted())
			qDebug() << clientLink->errorString();
		targetLink->startClientEncryption();
		if (!targetLink->waitForEncrypted())
			qDebug() << targetLink->errorString();
		return;
	}

	// Переключаемся в новое состояние (хотя уже и беспонту, поскольку будем менять обработчик )
	setStage(StageTransparent);

	// Инициируем начало работы
	if (clientLink->bytesAvailable()) {
		emit clientLink->readyRead();
	}
	if (targetLink->bytesAvailable()) {
		emit targetLink->readyRead();
	}
}

void ProxySocks::onEncrypted() {
	if ((QSslSocket *)sender() == clientLink) {
		Logger::messageDebugConsole(clientTraceInfo + " connection is in ssl-inspection mode");
		//fileDumper->messageTextInfo(clientTraceInfo + " connection is in ssl-inspection mode");
	}
	else
	{
		Logger::messageDebugConsole(targetTraceInfo + " connection is in ssl-inspection mode");
		//fileDumper->messageTextInfo(targetTraceInfo + " connection is in ssl-inspection mode");
	}

	if (!clientLink->isEncrypted() || !targetLink->isEncrypted()) return; //Ожидаем безопасного подключения для обеих узлов

	// Переключаемся в новое состояние (хотя уже и беспонту, поскольку будем менять обработчик )
	setStage(StageTransparent);

	// Инициируем начало работы
	if (clientLink->bytesAvailable()) {
		emit clientLink->readyRead();
	}
	if (targetLink->bytesAvailable()) {
		emit targetLink->readyRead();
	}
}

void ProxySocks::onError(QAbstractSocket::SocketError error) {
	//BOOST_LOG_CHANNEL_SEV(flog, log_channel, severity::error) << "ProxySocks[" << stage << "]" 
	//	<< clientLink->trace_string().toStdString() 
	//	<< " error while connect: " << ed;

	Logger::messageDebugConsole(targetTraceInfo + " connection failed");
	//fileDumper->messageTextInfo(targetTraceInfo + " connection failed");

	if ((StageTransparent != stage) && (StageBroken != stage)) {
		// Отправляем клиенту ответ	что всё плохо
		switch (version) {
		case(0x04):
			sendSocks4CommandResponse(0x5b); // Request rejected or failed
			break;
		case(0x05):
		default:
			sendSocks5CommandResponse(0x05); // Connection refused
			break;
		}
	}

	close_links();
}

void ProxySocks::close_links() {
	if (clientLink && clientLink->state() == QAbstractSocket::ConnectedState) {
		clientLink->disconnectFromHost();
	}
	if (targetLink && targetLink->state() == QAbstractSocket::ConnectedState) {
		targetLink->disconnectFromHost();
	}

	if ((clientLink->state() == QTcpSocket::UnconnectedState) && (targetLink->state() == QTcpSocket::UnconnectedState))
		this->deleteLater();
}

void ProxySocks::onDisconnect() {
	if ((QSslSocket *)sender() == clientLink) {
		Logger::messageInfoConsole(clientTraceInfo + " disconnected");
		fileDumper->messageTextInfo(clientTraceInfo + " disconnected");
	}
	else
	{
		Logger::messageDebugConsole(targetTraceInfo + " disconnected");
		fileDumper->messageTextInfo(targetTraceInfo + " disconnected");
	}


	close_links();
}

void ProxySocks::onTransparentRedirect() {
	// Определяемся с читателем и писателем
	QSslSocket * m_read_link = (QSslSocket *)sender();
	QSslSocket * m_write_link = (m_read_link == clientLink) ? targetLink : clientLink;

	if (m_read_link == clientLink) {
		if (direction != Upload) {
			fileDumper->messageTextInfo(clientTraceInfo + " >>>>>>>>>>>>>>>>>>>>");
			direction = Upload;
		}
	}
	else {
		//m_read_link == targetLink
		if (direction != Download) {
			fileDumper->messageTextInfo(targetTraceInfo + " <<<<<<<<<<<<<<<<<<<<");
			direction = Download;
		}
	}

	char buff[16384];
	auto inSockSz = m_read_link->bytesAvailable();
	while (inSockSz > 0) {
		auto readSz = m_read_link->read(buff, std::min<qint64>(inSockSz, sizeof(buff)));
		if (readSz == -1) {
			close_links(); return;
		}
		inSockSz -= readSz;
		if (readSz > 0 && inSockSz < 0) {
			qWarning() << "Read more bytes that were available";
		}
		auto writtenSz = m_write_link->write(buff, readSz);
		if (writtenSz == -1) {
			close_links(); return;
		}

		fileDumper->writeDataChunk(buff, readSz);
	}
}

void ProxySocks::onSslErrors(const QList<QSslError> & errors) {
	targetLink->ignoreSslErrors(errors);
}