#include <QtCore/QCoreApplication>
#include <QFile>
#include <QRegExp>
#include "src/sockslistener.h"
#include "src/optionsParser.h"
#include "src/logger.h"
#include <QDir>
#include <QFileInfo>
#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	try {
		Logger * logger = new Logger(&a);

		//Опции
		options_t options;
		OptionsParser * optionsParser = new OptionsParser();
		optionsParser->load(options);

		logger->setDebugMode(options.debug);

		if (!options.dumpDir.isEmpty() && !QFileInfo::exists(options.dumpDir)) {
			if (!QDir().mkpath(options.dumpDir)) {
				logger->messageInfoConsole(QString("Unable to create path: ") + options.dumpDir);
				QCoreApplication::exit(255);
			}
		}

		//Сервер
		SocksListener * server = new SocksListener(&a, options.usingSsl, options.crtPath, options.keyPath, options.dumpDir);
		bool onServerListen = server->startListen(QHostAddress(options.address), options.port);

		if (!onServerListen) return 0;

		logger->messageStartListen(options.address, options.port, options.usingSsl);


		return a.exec();
	}
	catch (std::exception & e) {
		std::cout << e.what() << std::endl;
	}
}

