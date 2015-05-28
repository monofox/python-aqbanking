#ifndef PYAQHANDLER_HPP
#define PYAQHANDLER_HPP

#include <Python.h>
#include <gwenhywfar/gui_be.h>
#include <gwenhywfar/i18n.h>
#include <gwen-gui-cpp/cppgui.hpp>
#include <aqbanking/banking.h>

static PyObject *AqBankingInitializeError;
static PyObject *AqBankingDeInitializeError;

/**
 * This handles all request and so it also gives the opportunity to have handlers per account!
 */
class PyAqHandler : public CppGui {
public:
	PyAqHandler();

	/**
	 * The callbacks to the python caller.
	 */
	PyObject *callbackLog = NULL;
	PyObject *callbackPassword = NULL;

protected:
	virtual int logHook(const char* logDomain, GWEN_LOGGER_LEVEL level, const char *s) override;
	virtual int getPassword(uint32_t flags, const char *token, const char *title, const char *text, char *buffer, int minLen, int maxLen, uint32_t guiid) override;
	virtual int checkCert(const GWEN_SSLCERTDESCR *cd, GWEN_SYNCIO *sio, uint32_t guiid) override;
};

#endif /* PYAQHANDLER_HPP */