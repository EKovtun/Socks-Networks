#pragma once

#include <QObject>

class QFile;

class FileDumper : public QObject {
	Q_OBJECT

public:
	FileDumper(QObject * parent = nullptr, const QString & dumpPath = "");
	~FileDumper();

	void startWriting(const QString & clientName, const QString & serverName);

	void messageTextInfo(const QString & message);
	void messageTextDebug(const QString & message);
	void writeDataChunk(const char * buff, qint64 len);

private:
	QFile * file;
	QString dumpPath;
};