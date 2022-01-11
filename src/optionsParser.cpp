#include "optionsParser.h"
#include "logger.h"
#include <QFile>
#include <QDebug>
#include <QTextCodec>
#include <QSettings>
#include <stdexcept>

options_t::options_t()
	: address("127.0.0.1")
	, port(1080)
	, dumpDir("")
	, crtPath("")
	, keyPath("")
	, usingSsl(false)
	, config("")
	, debug(false)
{}

namespace named_options {
	QCommandLineOption address("address", "my address (default: 127.0.0.1)", "string");
	QCommandLineOption port("port", "my port (default: 1080)", "number");

	QCommandLineOption dump_dir("dump-dir", "directory for writing log files ", "path");

	QCommandLineOption crt_path("ssl-crt", "ssl certificate path", "file path");
	QCommandLineOption key_path("ssl-key", "ssl key path", "file path");
	QCommandLineOption using_ssl("inspect-ssl", "inspect ssl traffic");

	QCommandLineOption config(QStringList() << "config" << "c", "account config path", "file path");
	QCommandLineOption debug("debug", "show debug message");
}

bool operator==(const QString & optName, const QCommandLineOption & opt) {
	foreach(const auto & name, opt.names()) {
		if (optName == name) return true;
	}
	return false;
}

template<typename T>
T extract(const QString & stringValue);

template<>
QString extract(const QString & stringValue) {
	if (stringValue.isNull())
		throw std::invalid_argument("Can not be null");
	return stringValue;
}

template<>
quint16 extract(const QString & stringValue) {
	bool ok = false;
	auto res = stringValue.toUInt(&ok);
	if (ok && res < 65536)
		return res;

	throw std::invalid_argument("Invalid port number");
}

template<>
bool extract(const QString & stringValue) {
	// Переменная просто как флажок
	if (stringValue.isNull())
		return true;

	// Если значение всеже есть
	auto str = stringValue.toLower();

	// Кастуется к числу
	bool ok = false;
	auto num = str.toUInt(&ok);
	if (ok)
		return num != 0;

	// Если это строковое значение
	if ("true" == str || "y" == str || "yes" == str)
		return true;
	else if ("false" == str || "n" == str || "no" == str)
		return false;

	throw std::invalid_argument("Invalid bool value");
}

OptionsParser::OptionsParser() {
	initParser();
}

void OptionsParser::load(options_t & options) {
	if (!cmdparser.parse(QCoreApplication::arguments())) {
		throw std::invalid_argument(cmdparser.errorText().toStdString());
	}
	cmdparser.process(*QCoreApplication::instance());

	readConfig(options); //Грузим конфиг
	readCommandLine(options); //Поверх мерджим опции командной строки
}

void OptionsParser::initParser() {
	cmdparser.setApplicationDescription("little socks proxy server");

	cmdparser.addOption(named_options::address);
	cmdparser.addOption(named_options::port);
	cmdparser.addOption(named_options::dump_dir);
	cmdparser.addOption(named_options::crt_path);
	cmdparser.addOption(named_options::key_path);
	cmdparser.addOption(named_options::using_ssl);
	cmdparser.addOption(named_options::config);
	cmdparser.addOption(named_options::debug);

	cmdparser.addHelpOption();
}

void OptionsParser::readConfig(options_t & options) {
	QString configPath;
	if (options.config.isEmpty())
		configPath = QCoreApplication::applicationDirPath() + "/littlesocks.conf";
	else
		configPath = options.config;

	if (QFile::exists(configPath)) 
		Logger::messageInfoConsole("Read config file " + configPath);
	else 
		if (!options.config.isEmpty()) 
			Logger::messageInfoConsole("Config file " + configPath + " does not exist");

	QSettings settings(configPath, QSettings::IniFormat);
	settings.setIniCodec(QTextCodec::codecForLocale());

	foreach(auto option, settings.allKeys())
			setOptionValue(options, option, settings.value(option).toString());
}

void OptionsParser::readCommandLine(options_t & options) {
	foreach(auto option, cmdparser.optionNames()) {
		setOptionValue(options, option, cmdparser.value(option));
	}
}

void OptionsParser::setOptionValue(options_t & options, const QString & option, const QString & stringValue) {
	if (option == named_options::address) options.address = extract<QString>(stringValue);
	else if (option == named_options::port) options.port = extract<quint16>(stringValue);
	else if (option == named_options::dump_dir) options.dumpDir = extract<QString>(stringValue);
	else if (option == named_options::crt_path) options.crtPath = extract<QString>(stringValue);
	else if (option == named_options::key_path) options.keyPath = extract<QString>(stringValue);
	else if (option == named_options::using_ssl) options.usingSsl = extract<bool>(stringValue);
	else if (option == named_options::debug) options.debug = extract<bool>(stringValue);
	else
	{
		Logger::messageInfoConsole("Option " + option + " unknown");
	}
}