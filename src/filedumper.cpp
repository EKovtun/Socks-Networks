#include "filedumper.h"
#include "logger.h"
#include <QDateTime>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>

FileDumper::FileDumper(QObject * parent, const QString & dumpPath) 
	: QObject(parent)
	, dumpPath(dumpPath)
	, file(nullptr) 
{}

FileDumper::~FileDumper() {
	if (file) {
		file->close();
		file->deleteLater();
	}
}

void FileDumper::startWriting(const QString & clientName, const QString & serverName) {
	if (dumpPath.isEmpty()) return;
	//QFileInfo fileInfo(dumpPath + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss.zzz") + "_" + clientName + "-" + serverName + ".bin");
	QFileInfo fileInfo(dumpPath + "/" + serverName +"_"+ QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss.zzz") + ".bin");
	
	// Открываем файл на запись
	file = new QFile(fileInfo.absoluteFilePath(), this);
	if (!file->open(QFile::WriteOnly | QFile::Text)) {
		Logger::messageDebugConsole("ERROR: file " + fileInfo.absolutePath() + " cannot be created.");
		file->deleteLater();
		file = nullptr;
	}
}

void FileDumper::messageTextInfo(const QString & message) {
	if (!file) return;

	QTextStream out(file);
	out << endl << endl << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ") << "INF: " << message << endl << endl;
	//out.flush();
}

void FileDumper::messageTextDebug(const QString & message) {
	if (!file) return;

	QTextStream out(file);
	out << endl << endl << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ") << "DBG: " << message << endl << endl;
	//out.flush();
}

void FileDumper::writeDataChunk(const char * buff, qint64 len) {
	if (!file) return;
	file->write(buff, len);
}
