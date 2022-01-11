#include "errordescriptor.h"


ErrorDescriptor::ErrorDescriptor(void):  mErrorType(ErrorNone) { }

ErrorDescriptor::ErrorDescriptor(const QString & msg, ErrorType errorType):mErrorString(msg), mErrorType(errorType){}

ErrorDescriptor::~ErrorDescriptor(void){}

QString ErrorDescriptor::errorString() const{
	return mErrorString;
}

ErrorDescriptor::ErrorType ErrorDescriptor::errorType() const{
	return mErrorType;
}

void ErrorDescriptor::setErrorString(const QString & errorStr,  ErrorType errorType){
	mErrorString = errorStr;
	mErrorType = errorType;
}

void ErrorDescriptor::clear(){
	mErrorString.clear();
	mErrorType = ErrorNone;
}

bool ErrorDescriptor::isError() const{
	return mErrorType != ErrorNone;
}

std::ostream & operator << (std::ostream & s, ErrorDescriptor const & ed) {
    return s << ed.errorType() << " -- " << ed.errorString().toStdString();
}

std::ostream & operator << (std::ostream & s, ErrorDescriptor::ErrorType const & et){
	switch(et){
	case ErrorDescriptor::ErrorNone: return s << "SUCCESS";
	case ErrorDescriptor::ErrorCommon: return s << "Common error";
	case ErrorDescriptor::ErrorNetwork: return s << "Network error";
	case ErrorDescriptor::ErrorAuthentication: return s << "Auth error";
	case ErrorDescriptor::ErrorProxy: return s << "Proxy error";
    default: return s << "Unknown error type (" << static_cast<int>(et) << ")";
	}
}

//ErrorDescriptor & ErrorDescriptorSupport::errorDescription(){
//	return mError;
//}

