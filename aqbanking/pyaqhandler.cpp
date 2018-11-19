#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gwenhywfar/version.h>
#include <gwenhywfar/cgui.h>
#include <gwenhywfar/gui_be.h>
#include "pyaqhandler.hpp"

PyAqHandler::PyAqHandler() : CppGui()
{
	GWEN_Logger_SetLevel(0, GWEN_LoggerLevel_Verbous);
	GWEN_Gui_SetGui(this->_gui);
	this->callbackLog = NULL;
	this->callbackPassword = NULL;
	this->callbackCheckCert = NULL;
}

/**
 * Gwen Gui progress log callback (which will then send to python callback!)
 */
int PyAqHandler::logHook(const char *logDomain, GWEN_LOGGER_LEVEL priority, const char *s)
{
	int priint = 0;
	switch(priority)
	{
		case GWEN_LoggerLevel_Emergency:
			priint = 0;
			break;
		case GWEN_LoggerLevel_Alert:
			priint = 1;
			break;
		case GWEN_LoggerLevel_Critical:
			priint = 2;
			break;
		case GWEN_LoggerLevel_Error:
			priint = 4;
			break;
		case GWEN_LoggerLevel_Warning:
			priint = 8;
			break;
		case GWEN_LoggerLevel_Notice:
			priint = 16;
			break;
		case GWEN_LoggerLevel_Info:
			priint = 32;
			break;
		case GWEN_LoggerLevel_Debug:
			priint = 64;
			break;
		case GWEN_LoggerLevel_Verbous:
			priint = 128;
			break;
		case GWEN_LoggerLevel_Unknown:
			priint = 9999;
			break;
	}
	if (this->callbackLog == NULL) {
		fprintf(stderr, "%s", "No loghook python CB defined! \n");
		return 1;
	}

	PyObject *arglist = Py_BuildValue("sis", logDomain, priint, s);
	PyObject_CallObject(this->callbackLog, arglist);
	Py_DECREF(arglist);

	return 0;
}

int PyAqHandler::getPassword(uint32_t flags, const char *token, const char *title, const char *text, char *buffer, int minLen, int maxLen, uint32_t guiid)
{
	const char *passwordPy;
	PyObject *result;
	if (this->callbackPassword == NULL) {
		fprintf(stderr, "%s", "No password python CB defined! \n");
		return 1;
	}
	PyObject *arglist = Py_BuildValue("Isssii", flags, token, title, text, minLen, maxLen);
	result = PyObject_CallObject(this->callbackPassword, arglist);
	Py_DECREF(arglist);

	// So we need a result! So now lets check it.
	if (result == NULL) {
		fprintf(stderr, "%s", "No password returned!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		PyErr_Print(); 
		return 1;
	} else {
		// Now convert the object to int.
#if PY_VERSION_HEX >= 0x03030000
		passwordPy = PyUnicode_AsUTF8(result);
#else
		PyObject *s = _PyUnicode_AsDefaultEncodedString(result, NULL);
		passwordPy = PyBytes_AS_STRING(s);
#endif
		strcpy(buffer, passwordPy);
		Py_DECREF(result);
	}

	return 0;
}

/**
 * If no callback always return: yes mam!
 */
int PyAqHandler::checkCert(const GWEN_SSLCERTDESCR *cd, GWEN_SYNCIO *sio, uint32_t guiid) 
{
	// FIXME: implement something better in this case, like GWEN_SslCertDescr_GetStatusFlags
	if (this->callbackCheckCert == NULL) {
        fprintf(stderr, "%s", "No certificate check python CB defined! \nThis is insecure!\n");
        return 0;
    }

	PyObject *result;
	const char *commonName = GWEN_SslCertDescr_GetCommonName(cd);
	const char *statusText = GWEN_SslCertDescr_GetStatusText(cd);
	const int status = GWEN_SslCertDescr_GetStatusFlags(cd);
	const char *md5 = GWEN_SslCertDescr_GetFingerPrint(cd);

#if GWENHYWFAR_VERSION_MAJOR > 4 || (GWENHYWFAR_VERSION_MAJOR == 4 && GWENHYWFAR_VERSION_MINOR >= 18)
	const char *sha512 = GWEN_SslCertDescr_GetFingerPrintSha512(cd);

	PyObject *arglist = Py_BuildValue("({s:s, s:s, s:s, s:s, s:i})", "commonName", commonName, "md5", md5, "sha512", sha512, "statusText", statusText, "status", status);
#else
	PyObject *arglist = Py_BuildValue("({s:s, s:s, s:s, s:i})", "commonName", commonName, "md5", md5, "statusText", statusText, "status", status);
#endif
	result = PyObject_CallObject(this->callbackCheckCert, arglist);
	Py_DECREF(arglist);

	if (result == NULL) {
		fprintf(stderr, "%s", "No answer on cert check returned!\n");
		PyErr_Print();
		return GWEN_ERROR_SSL_SECURITY;
	} else {
		int cert_valid = PyObject_IsTrue(result);
		Py_DECREF(result);
		return cert_valid? 0 : GWEN_ERROR_SSL_SECURITY;
	}

	return 0;
}

