
#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__


class Exception /*: public std::exception*/ {
	ttstr message_;
public:
	Exception(const ttstr& mes) : message_(mes) {
	}
	virtual const ttstr& what() const {
		return message_;
	}
};

class EAbort : public Exception {
public:
	EAbort(const ttstr& mes) : Exception(mes) {
	}
};

#endif // __EXCEPTION_H__
