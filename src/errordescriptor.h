#pragma once
//#include <iosfwd>
#include <ostream>
#include <QString>
#include <QMetaType>

class ErrorDescriptor {
public:
	enum ErrorType {
		ErrorNone,
		ErrorCommon,
		ErrorNetwork,
		ErrorAuthentication,
		ErrorProxy
	};

	ErrorDescriptor();
	explicit ErrorDescriptor(const QString &, ErrorType = ErrorCommon);
	~ErrorDescriptor();

	QString errorString() const;
	ErrorType errorType() const;
	void setErrorString(const QString &, ErrorType = ErrorCommon);
	void clear();
	bool isError() const;

protected:
	/**
	*	setErrorString
	*	Функция должна использоваться только потомками для установки текста с описанием возникшей ошибки перед посылкой сигнала error()
	*/

private:
	//int		mErrorCode;
	QString	mErrorString;
	ErrorType mErrorType;
};

std::ostream & operator << (std::ostream & , ErrorDescriptor const &);
std::ostream & operator << (std::ostream & , ErrorDescriptor::ErrorType const &);

Q_DECLARE_METATYPE(ErrorDescriptor)

//class ErrorDescriptorSupport {
//public:
//	ErrorDescriptor & errorDescription();
//private:
//	ErrorDescriptor mError;
//	ErrorDescriptorSupport();
//};
