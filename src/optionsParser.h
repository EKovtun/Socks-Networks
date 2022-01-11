#pragma once
#include <QString>
#include <QCommandLineParser>
#include <QSet>

struct options_t {
	QString address;
	quint16 port;

	QString dumpDir;
	
	QString crtPath;
	QString keyPath;
	bool usingSsl;

	QString config;
	bool debug;

	options_t();
};

class OptionsParser {
	QSet<QString> allowedOptions;

public:
	OptionsParser();
	~OptionsParser() = default;

	void load(options_t & options);

private:
	void initParser();
	void readConfig(options_t & options);
	void readCommandLine(options_t & options);
	void setOptionValue(options_t & options, const QString & option, const QString & stringValue);

private:
	QCommandLineParser cmdparser;
};