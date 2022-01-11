#include "logger.h"
#include <QtCore/QCoreApplication>
#include <QDateTime>
#include <QTextStream>
#include <QRegExp>
#include <QDebug>

bool debug_mod;

Logger::Logger(QObject * parent, const bool & debugMode) : QObject(parent) {
	debug_mod = debugMode;
}

void Logger::setDebugMode(const bool & debugMode) {
	debug_mod = debugMode;
}

//MESSAGES_______________________________________________________________________________________
void Logger::messageInfoConsole(const QString & message) {
	QTextStream out(stdout);
	out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
	out << " " << message << endl;
	out.flush();
}

void Logger::messageDebugConsole(const QString & message) {
	if (!debug_mod) return;

	QTextStream out(stdout);
	out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
	out << " " << message << endl;
	out.flush();
}


void Logger::messageStartListen(const QString & address, const quint16 & port, const bool & usingSSL) {
	messageInfoConsole("START");

	messageInfoConsole(QString("Debug mode: ") + (debug_mod ? "true" : "false"));
	messageInfoConsole(QString("Inspect SSL: ") + (usingSSL ? "true" : "false"));

	messageInfoConsole("Listen on " + address + ":" + QString::number(port));
}
