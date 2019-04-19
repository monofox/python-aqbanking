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
	PyObject *callbackLog;
	PyObject *callbackPassword;
	PyObject *callbackCheckCert;
	PyObject *callbackPasswordStatus;

protected:
	virtual int logHook(const char* logDomain, GWEN_LOGGER_LEVEL level, const char *s);
	virtual int getPassword(uint32_t flags, const char *token, const char *title, const char *text, char *buffer, int minLen, int maxLen, uint32_t guiid);
	virtual int setPasswordStatus(const char *token, const char *pin, GWEN_GUI_PASSWORD_STATUS status, uint32_t guiid);
	virtual int checkCert(const GWEN_SSLCERTDESCR *cd, GWEN_SYNCIO *sio, uint32_t guiid);
	virtual int setupDialog(GWEN_WIDGET *w);
	virtual int openDialog(GWEN_DIALOG *dlg, uint32_t guiid);
	virtual int closeDialog(GWEN_DIALOG *dlg);
};

#endif /* PYAQHANDLER_HPP */
