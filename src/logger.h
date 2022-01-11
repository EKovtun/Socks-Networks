#pragma once

#include <QObject>
#include <QFile>

class Logger: public QObject {
public:
	Logger(QObject * parent, const bool & debugMode = false);
	void setDebugMode(const bool & debugMode = true);

	//MESSAGES_______________________________________________________________________________________
	static void messageInfoConsole(const QString & message);
	static void messageDebugConsole(const QString & message);
	static void messageStartListen(const QString & address, const quint16 & port, const bool & usingSSL);
};