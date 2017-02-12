#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gwenhywfar/cgui.h>
#include <gwenhywfar/gui_be.h>
#include "pyaqhandler.hpp"

PyAqHandler::PyAqHandler() : CppGui()
{
	GWEN_Logger_SetLevel(0, GWEN_LoggerLevel_Verbous);
	GWEN_Gui_SetGui(this->_gui);
	this->callbackLog = NULL;
	this->callbackPassword = NULL;
	this->callbackPasswordStatus = NULL;
#ifdef FENQUEJOB
	this->callbackOpenDialog = NULL;
	this->callbackCloseDialog = NULL;
	this->callbackExecDialog = NULL;
	this->callbackRunDialog = NULL;
	this->callbackPrint = NULL;
#endif
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
		fprintf(stderr, "%s", "No password returned!\n");
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

int PyAqHandler::setPasswordStatus(const char *token, const char *pin, GWEN_GUI_PASSWORD_STATUS enumStatus, uint32_t guiid) {
	if (this->callbackPasswordStatus == NULL) {
		return 0;
	}

	/* map status: 
	 * 0 = OK
	 * 1 = Bad
	 * 2 = Remove
	 * 4 = Used
	 * 8 = Unused
	 * 16 = Unknown
	 */ 
	int status = 16;
	switch(enumStatus) {
		case GWEN_Gui_PasswordStatus_Ok:
			status = 0;
			break;
		case GWEN_Gui_PasswordStatus_Bad:
			status = 1;
			break;
		case GWEN_Gui_PasswordStatus_Remove:
			status = 2;
			break;
		case GWEN_Gui_PasswordStatus_Used:
			status = 4;
			break;
		case GWEN_Gui_PasswordStatus_Unused:
			status = 8;
			break;
		case GWEN_Gui_PasswordStatus_Unknown:
			status = 16;
			break;
	}

	PyObject *arglist = Py_BuildValue("ssiI", token, pin, status, guiid);
	PyObject_CallObject(this->callbackPasswordStatus, arglist);
	Py_DECREF(arglist);

	return 0;
}

/**
 * Return always: yes mam!
 */
int PyAqHandler::checkCert(const GWEN_SSLCERTDESCR *cd, GWEN_SYNCIO *sio, uint32_t guiid) 
{
	// FIXME: implement at least some basic checks => or ask user via callback!
	return 0;
}

#ifdef FENQUEJOB
int PyAqHandler::openDialog(GWEN_DIALOG *dlg, uint32_t guiid) {
	if (this->callbackOpenDialog == NULL) {
		fprintf(stderr, "PyAqHandler::openDialog not implemented yet!");
		return GWEN_ERROR_NOT_SUPPORTED;
	} else {
		PyObject *arglist = Py_BuildValue("I", guiid);
		PyObject_CallObject(this->callbackOpenDialog, arglist);
		Py_DECREF(arglist);
		return 0;
	}
}

int PyAqHandler::execDialog(GWEN_DIALOG *dlg, uint32_t guiid) {
	if (this->callbackExecDialog == NULL) {
		fprintf(stderr, "PyAqHandler::execDialog not implemented yet!");
		return GWEN_ERROR_NOT_SUPPORTED;
	} else {
		PyObject *arglist = Py_BuildValue("I", guiid);
		PyObject_CallObject(this->callbackExecDialog, arglist);
		Py_DECREF(arglist);
		return 0;
	}
}

int PyAqHandler::closeDialog(GWEN_DIALOG *dlg) {
	if (this->callbackCloseDialog == NULL) {
		fprintf(stderr, "PyAqHandler::closeDialog not implemented yet!");
		return GWEN_ERROR_NOT_SUPPORTED;
	} else {
		PyObject_CallObject(this->callbackCloseDialog, NULL);
		return 0;
	}
}

int PyAqHandler::runDialog(GWEN_DIALOG *dlg, int untilEnd) {
	if (this->callbackRunDialog == NULL) {
		fprintf(stderr, "PyAqHandler::runDialog not implemented yet!");
		return GWEN_ERROR_NOT_SUPPORTED;
	} else {
		PyObject *arglist = Py_BuildValue("i", untilEnd);
		PyObject_CallObject(this->callbackRunDialog, arglist);
		Py_DECREF(arglist);
		return 0;
	}
}

int PyAqHandler::print(const char *docTitle, const char *docType, const char *descr, const char *text, uint32_t guiid) {
	if (this->callbackPrint == NULL) {
		fprintf(stderr, "PyAqHandler::print not implemented yet!");
		return GWEN_ERROR_NOT_SUPPORTED;
	} else {
		PyObject *arglist = Py_BuildValue("ssssI", docTitle, docType, descr, text, guiid);
		PyObject_CallObject(this->callbackPrint, arglist);
		Py_DECREF(arglist);
		return 0;
	}
}
#endif
