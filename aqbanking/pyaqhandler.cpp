#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gwenhywfar/version.h>
#include <gwenhywfar/cgui.h>
#include <gwenhywfar/gui_be.h>
#include <gwenhywfar/debug.h>
#include <gwen-gui-cpp/cppdialog.hpp>
#include <gwen-gui-cpp/cppwidget.hpp>
#include "pyaqhandler.hpp"

PyAqHandler::PyAqHandler() : CppGui()
{
	GWEN_Logger_SetLevel(0, GWEN_LoggerLevel_Verbous);
	GWEN_Gui_SetGui(this->_gui);
	GWEN_Gui_AddFlags(_gui, GWEN_GUI_FLAGS_DIALOGSUPPORTED);
	GWEN_Gui_SetName(_gui, "pyaq-gui");
	this->callbackLog = NULL;
	this->callbackPassword = NULL;
	this->callbackCheckCert = NULL;
	this->callbackPasswordStatus = NULL;
}

int PyAqHandler::setupDialog(GWEN_WIDGET *w) {
	CppWidget *xw=NULL;
	printf("GWEN Dialog Widget: %d (%s)\n\n", 
		GWEN_Widget_GetType(w), 
		GWEN_Widget_Type_toString(GWEN_Widget_GetType(w)));


	switch(GWEN_Widget_GetType(w)) {
		case GWEN_Widget_TypeLabel:
			xw = new CppWidget(w);
			const char *s;
			const char *name;
			name=xw->getName();
			s=xw->getText(0);
			printf("Got text in label %s: %s\n", name, s);
			break;
		case GWEN_Widget_TypeDialog:
		case GWEN_Widget_TypeVLayout:
		case GWEN_Widget_TypeHLayout:
		case GWEN_Widget_TypeGridLayout:
		case GWEN_Widget_TypeLineEdit:
		case GWEN_Widget_TypeVSpacer:
		case GWEN_Widget_TypeHSpacer:
		case GWEN_Widget_TypePushButton:
		case GWEN_Widget_TypeHLine:
		case GWEN_Widget_TypeVLine:
		case GWEN_Widget_TypeTextEdit:
		case GWEN_Widget_TypeComboBox:
		case GWEN_Widget_TypeTabBook:
		case GWEN_Widget_TypeTabPage:
		case GWEN_Widget_TypeCheckBox:
		case GWEN_Widget_TypeGroupBox:
		case GWEN_Widget_TypeWidgetStack:
		case GWEN_Widget_TypeTextBrowser:
		case GWEN_Widget_TypeScrollArea:
		case GWEN_Widget_TypeProgressBar:
		case GWEN_Widget_TypeListBox:
		case GWEN_Widget_TypeRadioButton:
		case GWEN_Widget_TypeSpinBox:
		default:
			DBG_ERROR(GWEN_LOGDOMAIN, "Unhandled widget type %d (%s)",
				GWEN_Widget_GetType(w), GWEN_Widget_Type_toString(GWEN_Widget_GetType(w)));
			break;
	}

	GWEN_WIDGET *wChild;
	wChild=GWEN_Widget_Tree_GetFirstChild(w);
	while(wChild) {
		setupDialog(wChild);
		wChild=GWEN_Widget_Tree_GetNext(wChild);
	}

	return 0;
}

int PyAqHandler::openDialog(GWEN_DIALOG *dlg, uint32_t guiid) {
	printf("PyAqHandler::OpenDialog\n");
	// Finally this should be passed sometime to python.
	// For now we need also to extract data.
	CppDialog *cppDlg = new CppDialog(dlg);
	GWEN_DIALOG *gwd = cppDlg->getCInterface();
	GWEN_WIDGET *w;
	GWEN_WIDGET_TREE *wtree = GWEN_Dialog_GetWidgets(gwd);
	// We need a tree
	if (wtree==NULL) {
		DBG_ERROR(GWEN_LOGDOMAIN, "No widget tree in dialog");
		return GWEN_ERROR_GENERIC;
	}

	// First child.
	w=GWEN_Widget_Tree_GetFirst(wtree);
	if (w==NULL) {
		DBG_ERROR(GWEN_LOGDOMAIN, "No widgets in dialog");
		return GWEN_ERROR_GENERIC;
	}
	printf("GWEN Dialog Widget: %d (%s)\n\n", 
		GWEN_Widget_GetType(w), 
		GWEN_Widget_Type_toString(GWEN_Widget_GetType(w)));

	w=GWEN_Widget_Tree_GetFirstChild(w);
	while(w) {
		setupDialog(w);
		w=GWEN_Widget_Tree_GetNext(w);
	}

	return 0;
}



int PyAqHandler::closeDialog(GWEN_DIALOG *dlg) {
	printf("PyAqHandler::CloseDialog\n");
  	return 0;
}

int PyAqHandler::setPasswordStatus(const char *token, const char *pin, 
	GWEN_GUI_PASSWORD_STATUS status, uint32_t guiid) {
	int pystat = 0;
	if (token == NULL && pin == NULL && status == GWEN_Gui_PasswordStatus_Remove) {
		pystat = 9;
	} else {
		if (status == GWEN_Gui_PasswordStatus_Bad) {
			pystat = 1;
		} else if (status == GWEN_Gui_PasswordStatus_Remove) {
			pystat = 2;
		}
	}

	if (this->callbackPasswordStatus != NULL) {
		// Call the callback.
		// If user does not give a callback, we can't give him a feedback about the PW status.
		PyObject *arglist = Py_BuildValue("ssi", token, pin, pystat);
		PyObject_CallObject(this->callbackPasswordStatus, arglist);
		Py_DECREF(arglist);
	}
	return 0;
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
}
