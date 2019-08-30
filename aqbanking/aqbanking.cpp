#pragma GCC diagnostic ignored "-Wwrite-strings"
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef DEBUG
#define Py_DEBUG 1
#endif

#include <Python.h>
#include <datetime.h>
#include "unicodeobject.h"
#include "structmember.h"
#include <aqbanking/banking.h>
#include <aqbanking/jobgetbalance.h>
#include <aqbanking/jobgettransactions.h>
#include <aqbanking/jobsingletransfer.h>
#include <aqbanking/jobsepatransfer.h>
#include <aqbanking/jobsepatransfer.h>
#include "pyaqhandler.hpp"

/**
 * Here are some exceptions.
 */
static PyObject *AccountNotFound;
static PyObject *InvalidIBAN;
static PyObject *ExecutionFailed;
static PyObject *JobNotAvailable;

/**
 * Module specific AQ Handler.
 */
static PyAqHandler* aqh = NULL;
static AB_BANKING *ab = NULL;
#ifdef SUPPORT_APPREGISTRATION
static const char *fintsRegistrationKey = NULL;
static const char *fintsRegistrationKeyFB = FINTS_REGISTRATION_KEY;
#endif

/**
 * This is the structure for the Account class.
 */
typedef struct {
	PyObject_HEAD
	PyObject *no;
	PyObject *name;
	PyObject *description;
	PyObject *bank_code;
	PyObject *bank_name;

	PyAqHandler *aqh;
	AB_BANKING *ab;
} aqbanking_Account;

/**
 * This is the structure for a transaction.
 */
typedef struct {
	PyObject_HEAD
	PyObject *uniqueId;
	PyObject *date;
	PyObject *valutaDate;
	PyObject *localAccount;
	PyObject *localBank;
	PyObject *localIban;
	PyObject *localBic;
	PyObject *localName;
	PyObject *remoteAccount;
	PyObject *remoteBank;
	PyObject *remoteIban;
	PyObject *remoteBic;
	PyObject *remoteName;
	PyObject *purpose;
	PyObject *value;
	PyObject *currency;
	PyObject *transactionCode;
	PyObject *transactionText;
	PyObject *textKey;
	PyObject *textKeyExt;
	PyObject *sepaMandateId;
	PyObject *customerReference;
	PyObject *bankReference;
	PyObject *endToEndReference;
	PyObject *fiId;
	PyObject *primaNota;
	int state;
} aqbanking_Transaction;

int AB_create(aqbanking_Account *acct = NULL) {
	int rv = 0;

	// Initialisierungen GWEN
	if (acct == NULL) {
		GWEN_Gui_SetGui(aqh->getCInterface());
	} else {
		GWEN_Gui_SetGui(acct->aqh->getCInterface());
	}

	// Initialisierungen AB
	if (acct == NULL) {
		#ifdef DEBUGSTDERR
		fprintf(stderr, "Account not set, create AqBanking connection...\n");
		#endif
		ab = AB_Banking_new("python-aqbanking", 0, AB_BANKING_EXTENSION_NONE);
		#ifdef SUPPORT_APPREGISTRATION
		if (fintsRegistrationKey != NULL) {
			#ifdef DEBUGSTDERR
			fprintf(stderr, "FinTS registration key set: %s [%s]\n", fintsRegistrationKey, PACKAGE_VERSION);
			#endif
			AB_Banking_RuntimeConfig_SetCharValue(ab, "fintsRegistrationKey", fintsRegistrationKey);
			AB_Banking_RuntimeConfig_SetCharValue(ab, "fintsApplicationVersionString", PACKAGE_VERSION);
		} else {
			#ifdef DEBUGSTDERR
			fprintf(stderr, "FinTS registration key not set, fall back to: %s [%s]\n", fintsRegistrationKeyFB, PACKAGE_VERSION);
			#endif
			AB_Banking_RuntimeConfig_SetCharValue(ab, "fintsRegistrationKey", fintsRegistrationKeyFB);
			AB_Banking_RuntimeConfig_SetCharValue(ab, "fintsApplicationVersionString", PACKAGE_VERSION);
		}
		#endif
		rv = AB_Banking_Init(ab);
	} else {
		#ifdef DEBUGSTDERR
		fprintf(stderr, "Account set, create AqBanking connection...\n");
		#endif
		acct->ab = AB_Banking_new("python-aqbanking", 0, AB_BANKING_EXTENSION_NONE);
		#ifdef SUPPORT_APPREGISTRATION
		if (fintsRegistrationKey != NULL) {
			#ifdef DEBUGSTDERR
			fprintf(stderr, "FinTS registration key set: %s [%s]\n", fintsRegistrationKey, PACKAGE_VERSION);
			#endif
			AB_Banking_RuntimeConfig_SetCharValue(acct->ab, "fintsRegistrationKey", fintsRegistrationKey);
			AB_Banking_RuntimeConfig_SetCharValue(acct->ab, "fintsApplicationVersionString", PACKAGE_VERSION);
		} else {
			#ifdef DEBUGSTDERR
			fprintf(stderr, "FinTS registration key not set, fall back to: %s [%s]\n", fintsRegistrationKeyFB, PACKAGE_VERSION);
			#endif
			AB_Banking_RuntimeConfig_SetCharValue(acct->ab, "fintsRegistrationKey", fintsRegistrationKeyFB);
			AB_Banking_RuntimeConfig_SetCharValue(acct->ab, "fintsApplicationVersionString", PACKAGE_VERSION);
		}
		#endif
		rv = AB_Banking_Init(acct->ab);
	}
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not initialize (%d).", rv));
		return 2;
	}
	if (acct == NULL) {
		rv = AB_Banking_OnlineInit(ab);
	} else {
		rv = AB_Banking_OnlineInit(acct->ab);
	}
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online initialize (%d).", rv));
		return 2;
	}

	//Setzen der Call-backs
	/*GWEN_Gui_SetProgressLogFn(this->gui, zProgressLog);
	GWEN_Gui_SetCheckCertFn(this->gui, zCheckCert);
	GWEN_Gui_SetMessageBoxFn(this->gui, zMessageBox);
	GWEN_Gui_SetGetPasswordFn(this->gui, zPasswordFn);*/

	return 0;
}

int AB_free(aqbanking_Account *acct = NULL) {
	int rv = 0;

	if (acct == NULL) {
		rv = AB_Banking_OnlineFini(ab);
	} else {
		rv = AB_Banking_OnlineFini(acct->ab);
	}
	if (rv) 
	{
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online deinit. (%d).", rv));
		return 3;
	}

	if (acct == NULL) {
		rv = AB_Banking_Fini(ab);
	} else {
		rv = AB_Banking_Fini(acct->ab);
	}
	if (rv) 
	{
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do deinit. (%d).", rv));
		return 3;
	}

	if (acct == NULL) {
		AB_Banking_free(ab);
	} else {
		AB_Banking_free(acct->ab);
	}
	return 0;
}

//***** HERE THE AQBANKING PYTHON ITSELF STARTS ****
static void aqbanking_Transaction_dealloc(aqbanking_Transaction* self)
{
	Py_XDECREF(self->uniqueId);
	Py_XDECREF(self->date);
	Py_XDECREF(self->valutaDate);
	Py_XDECREF(self->localAccount);
	Py_XDECREF(self->localBank);
	Py_XDECREF(self->localIban);
	Py_XDECREF(self->localBic);
	Py_XDECREF(self->localName);
	Py_XDECREF(self->remoteAccount);
	Py_XDECREF(self->remoteBank);
	Py_XDECREF(self->remoteIban);
	Py_XDECREF(self->remoteBic);
	Py_XDECREF(self->remoteName);
	Py_XDECREF(self->purpose);
	Py_XDECREF(self->value);
	Py_XDECREF(self->currency);
	Py_XDECREF(self->transactionCode);
	Py_XDECREF(self->transactionText);
	Py_XDECREF(self->textKey);
	Py_XDECREF(self->textKeyExt);
	Py_XDECREF(self->sepaMandateId);
	Py_XDECREF(self->customerReference);
	Py_XDECREF(self->bankReference);
	Py_XDECREF(self->endToEndReference);
	Py_XDECREF(self->fiId);
	Py_XDECREF(self->primaNota);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *aqbanking_Transaction_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	aqbanking_Transaction *self;

	self = (aqbanking_Transaction *)type->tp_alloc(type, 0);
	if (self != NULL) {
		int uniqueId_default = -1;
		self->uniqueId = PyLong_FromLong(uniqueId_default);
		if (self->uniqueId == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->date = PyUnicode_FromString("");
		if (self->date == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->valutaDate = PyUnicode_FromString("");
		if (self->valutaDate == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localAccount = PyUnicode_FromString("");
		if (self->localAccount == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localBank = PyUnicode_FromString("");
		if (self->localBank == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localIban = PyUnicode_FromString("");
		if (self->localIban == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localBic = PyUnicode_FromString("");
		if (self->localBic == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteAccount = PyUnicode_FromString("");
		if (self->remoteAccount == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteBank = PyUnicode_FromString("");
		if (self->remoteBank == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteIban = PyUnicode_FromString("");
		if (self->remoteIban == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteBic = PyUnicode_FromString("");
		if (self->remoteBic == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->purpose = PyUnicode_FromString("");
		if (self->purpose == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		double startValue = 0.0;
		self->value = PyFloat_FromDouble(startValue);
		if (self->value == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->currency = PyUnicode_FromString("EUR");
		if (self->currency == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->state = 0;
	}

	return (PyObject *)self;
}

static int aqbanking_Transaction_init(aqbanking_Transaction *self, PyObject *args, PyObject *kwds)
{
	PyObject *date=NULL, *valutaDate=NULL, *localIban=NULL, *localBic=NULL, *remoteIban=NULL, *remoteBic=NULL;
	PyObject *purpose=NULL, *value=NULL, *currency=NULL, *tmp;
	static char *kwlist[] = {
		"date", "valutaDate", "localIban", "localBic", "remoteIban", "remoteBic", "purpose", "value", "currency", NULL
	};
	if (! PyArg_ParseTupleAndKeywords(
		args, kwds, "|OOOOOOOOO", kwlist, &date, &valutaDate, &localIban, &localBic, &remoteIban, &remoteBic, purpose, 
		value, currency
		))
	{
		return -1;
	}

	if (date) {
		tmp = self->date;
		Py_INCREF(date);
		self->date = date;
		Py_XDECREF(tmp);
	}

	if (valutaDate) {
		tmp = self->valutaDate;
		Py_INCREF(valutaDate);
		self->valutaDate = valutaDate;
		Py_XDECREF(tmp);
	}

	if (localIban) {
		tmp = self->localIban;
		Py_INCREF(localIban);
		self->localIban = localIban;
		Py_XDECREF(tmp);
	}

	if (localBic) {
		tmp = self->localBic;
		Py_INCREF(localBic);
		self->localBic = localBic;
		Py_XDECREF(tmp);
	}

	if (remoteIban) {
		tmp = self->remoteIban;
		Py_INCREF(remoteIban);
		self->remoteIban = remoteIban;
		Py_XDECREF(tmp);
	}

	if (remoteBic) {
		tmp = self->remoteBic;
		Py_INCREF(remoteBic);
		self->remoteBic = remoteBic;
		Py_XDECREF(tmp);
	}

	if (purpose) {
		tmp = self->purpose;
		Py_INCREF(purpose);
		self->purpose = purpose;
		Py_XDECREF(tmp);
	}

	if (value) {
		tmp = self->value;
		Py_INCREF(value);
		self->value = value;
		Py_XDECREF(tmp);
	}

	if (currency) {
		tmp = self->currency;
		Py_INCREF(currency);
		self->currency = currency;
		Py_XDECREF(tmp);
	}

	return 0;
}

static PyMemberDef aqbanking_Transaction_members[] = {
	{"uniqueId", T_OBJECT_EX, offsetof(aqbanking_Transaction, uniqueId), 0, "Unique ID of transaction"},
	{"date", T_OBJECT_EX, offsetof(aqbanking_Transaction, date), 0, "Date of transaction"},
	{"valutaDate", T_OBJECT_EX, offsetof(aqbanking_Transaction, valutaDate), 0, "Valuta date of transaction"},
	{"localAccount", T_OBJECT_EX, offsetof(aqbanking_Transaction, localAccount), 0, "Local account no."},
	{"localBank", T_OBJECT_EX, offsetof(aqbanking_Transaction, localBank), 0, "Local bank code"},
	{"localIban", T_OBJECT_EX, offsetof(aqbanking_Transaction, localIban), 0, "Local IBAN"},
	{"localBic", T_OBJECT_EX, offsetof(aqbanking_Transaction, localBic), 0, "Local BIC"},
	{"localName", T_OBJECT_EX, offsetof(aqbanking_Transaction, localName), 0, "Local owner of the bank account"},
	{"remoteAccount", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteAccount), 0, "Remote account no."},
	{"remoteBank", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteBank), 0, "Remote bank code"},
	{"remoteIban", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteIban), 0, "Remote IBAN"},
	{"remoteBic", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteBic), 0, "Remote BIC"},
	{"remoteName", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteName), 0, "Remote owner of the bank account"},
	{"purpose", T_OBJECT_EX, offsetof(aqbanking_Transaction, purpose), 0, "Purpose of transaction"},
	{"value", T_OBJECT_EX, offsetof(aqbanking_Transaction, value), 0, "Value"},
	{"currency", T_OBJECT_EX, offsetof(aqbanking_Transaction, currency), 0, "Currency (by default EUR)"},
	{"state", T_INT, offsetof(aqbanking_Transaction, state), 0, "State of the transaction"},
	{"transactionCode", T_OBJECT_EX, offsetof(aqbanking_Transaction, transactionCode), 0, "A 3-digit transaction code (Geschäftsvorfallcode)"},
	{"transactionText", T_OBJECT_EX, offsetof(aqbanking_Transaction, transactionText), 0, "Text representing the kind of transaction"},
	{"textKey", T_OBJECT_EX, offsetof(aqbanking_Transaction, textKey), 0, "A numerical transaction code (Textschlüssel)"},
	{"textKeyExt", T_OBJECT_EX, offsetof(aqbanking_Transaction, textKeyExt), 0, "An extension to the text key (Textschlüsselergänzung)"},
	{"sepaMandateId", T_OBJECT_EX, offsetof(aqbanking_Transaction, sepaMandateId), 0, "Mandate ID of a SEPA mandate for SEPA direct debits"},
	{"customerReference", T_OBJECT_EX, offsetof(aqbanking_Transaction, customerReference), 0, "Customer Reference"},
	{"bankReference", T_OBJECT_EX, offsetof(aqbanking_Transaction, bankReference), 0, "Bank Reference"},
	{"endToEndReference", T_OBJECT_EX, offsetof(aqbanking_Transaction, endToEndReference), 0, "End-To-End Reference"},
	{"fiId", T_OBJECT_EX, offsetof(aqbanking_Transaction, fiId), 0, "FiID"},
	{"primaNota", T_OBJECT_EX, offsetof(aqbanking_Transaction, primaNota), 0, "Prima Nota"},
	{NULL}
};

static PyMethodDef aqbanking_Transaction_methods[] = {
	/*{"name", (PyCFunction)aqbanking_Transaction_name, METH_NOARGS,
	},*/
	{NULL}  /* Sentinel */
};

static PyTypeObject aqbanking_TransactionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"aqbanking.Transaction", /* tp_name */
	sizeof(aqbanking_Transaction), /* tp_basicsize */
	0, /* tp_itemsize */
	(destructor)aqbanking_Transaction_dealloc, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_reserved */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash  */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Transaction", /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	aqbanking_Transaction_methods, /* tp_methods */
	aqbanking_Transaction_members, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	(initproc)aqbanking_Transaction_init, /* tp_init */
	0, /* tp_alloc */
	aqbanking_Transaction_New, /* tp_new */
};

/*
 * HERE THE ACCOUNT STARTS!
 */
static int aqbanking_Account_clear(aqbanking_Account* self)
{
	if (self->aqh != NULL) 
	{
		// clear the callbacks
		if (self->aqh->callbackLog != NULL) {
			Py_XDECREF(self->aqh->callbackLog);
			self->aqh->callbackLog = NULL;
		}
		if (self->aqh->callbackPassword != NULL) {
			Py_XDECREF(self->aqh->callbackPassword);
			self->aqh->callbackPassword = NULL;
		}
		if (self->aqh->callbackCheckCert != NULL) {
			Py_XDECREF(self->aqh->callbackCheckCert);
			self->aqh->callbackCheckCert = NULL;
		}
		if (self->aqh->callbackPasswordStatus != NULL) {
			Py_XDECREF(self->aqh->callbackPasswordStatus);
			self->aqh->callbackPasswordStatus = NULL;
		}
	}
    return 0;
}

static void aqbanking_Account_dealloc(aqbanking_Account* self)
{
	Py_XDECREF(self->no);
	Py_XDECREF(self->name);
	Py_XDECREF(self->description);
	Py_XDECREF(self->bank_code);
	Py_XDECREF(self->bank_name);
	aqbanking_Account_clear(self);
	self->aqh = NULL;
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *aqbanking_Account_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	aqbanking_Account *self;

	self = (aqbanking_Account *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->no = PyUnicode_FromString("");
		if (self->no == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->name = PyUnicode_FromString("");
		if (self->name == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->description = PyUnicode_FromString("");
		if (self->description == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->bank_code = PyUnicode_FromString("");
		if (self->bank_code == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->bank_name = PyUnicode_FromString("");
		if (self->bank_name == NULL) {
			Py_DECREF(self);
			return NULL;
		}
	}

	self->aqh = NULL;

	return (PyObject *)self;
}

static int aqbanking_Account_init(aqbanking_Account *self, PyObject *args, PyObject *kwds)
{
	PyObject *no=NULL, *name=NULL, *description=NULL, *bank_code=NULL, *bank_name=NULL, *tmp;
	static char *kwlist[] = {"no", "name", "description", "bank_code", "bank_name", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOO", kwlist,
		&no, &name, &description, &bank_code, &bank_name))
		return -1;

	if (no) {
		tmp = self->no;
		Py_INCREF(no);
		self->no = no;
		Py_XDECREF(tmp);
	}

	if (name) {
		tmp = self->name;
		Py_INCREF(name);
		self->name = name;
		Py_XDECREF(tmp);
	}

	if (description) {
		tmp = self->description;
		Py_INCREF(description);
		self->description = description;
		Py_XDECREF(tmp);
	}

	if (bank_code) {
		tmp = self->bank_code;
		Py_INCREF(bank_code);
		self->bank_code = bank_code;
		Py_XDECREF(tmp);
	}

	if (bank_name) {
		tmp = self->bank_name;
		Py_INCREF(bank_name);
		self->bank_name = bank_name;
		Py_XDECREF(tmp);
	}

	self->aqh = new PyAqHandler();

	return 0;
}

static PyObject *aqbanking_Account_name(aqbanking_Account* self)
{
	if (self->no == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "no");
	}
	if (self->name == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "name");
	}

	return PyUnicode_FromFormat("%S: %S", self->no, self->name);
}

static PyObject *aqbanking_Account_cleanup(aqbanking_Account* self, PyObject *args)
{
	aqbanking_Account_clear(self);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *aqbanking_Account_set_callbackLog(aqbanking_Account* self, PyObject *args)
{
	PyObject *result = NULL;
	PyObject *temp;

	if (PyArg_ParseTuple(args, "O:set_callbackLog", &temp)) {
		if (!PyCallable_Check(temp)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_XINCREF(temp);         /* Add a reference to new callback */
		Py_XDECREF(self->aqh->callbackLog);  /* Dispose of previous callback */
		self->aqh->callbackLog = temp;       /* Remember new callback */
		/* Boilerplate to return "None" */
		Py_INCREF(Py_None);
		result = Py_None;
	}
	return result;
}

static PyObject *aqbanking_Account_set_callbackPassword(aqbanking_Account* self, PyObject *args)
{
	PyObject *result = NULL;
	PyObject *temp;

	if (PyArg_ParseTuple(args, "O:set_callbackPassword", &temp)) {
		if (!PyCallable_Check(temp)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_XINCREF(temp);         /* Add a reference to new callback */
		Py_XDECREF(self->aqh->callbackPassword);  /* Dispose of previous callback */
		self->aqh->callbackPassword = temp;       /* Remember new callback */
		if (self->aqh->callbackPassword == NULL) {
			fprintf(stderr, "%s", "!!but is still invalid!!! \n");
		}
		/* Boilerplate to return "None" */
		Py_INCREF(Py_None);
		result = Py_None;
	}
	return result;
}

static PyObject *aqbanking_Account_set_callbackPasswordStatus(aqbanking_Account* self, PyObject *args)
{
	PyObject *result = NULL;
	PyObject *temp;

	if (PyArg_ParseTuple(args, "O:set_callbackPasswordStatus", &temp)) {
		if (!PyCallable_Check(temp)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_XINCREF(temp);         /* Add a reference to new callback */
		Py_XDECREF(self->aqh->callbackPasswordStatus);  /* Dispose of previous callback */
		self->aqh->callbackPasswordStatus = temp;       /* Remember new callback */
		if (self->aqh->callbackPasswordStatus == NULL) {
			fprintf(stderr, "%s", "!!but is still invalid!!! \n");
		}
		/* Boilerplate to return "None" */
		Py_INCREF(Py_None);
		result = Py_None;
	}
	return result;
}

static PyObject *aqbanking_Account_set_callbackCheckCert(aqbanking_Account* self, PyObject *args)
{
	PyObject *result = NULL;
	PyObject *temp;

	if (PyArg_ParseTuple(args, "O:set_callbackCheckCert", &temp)) {
		if (!PyCallable_Check(temp)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_XINCREF(temp);         /* Add a reference to new callback */
		Py_XDECREF(self->aqh->callbackCheckCert);  /* Dispose of previous callback */
		self->aqh->callbackCheckCert = temp;       /* Remember new callback */
		/* Boilerplate to return "None" */
		Py_INCREF(Py_None);
		result = Py_None;
	}
	return result;
}

static PyObject *aqbanking_Account_balance(aqbanking_Account* self, PyObject *args, PyObject *keywds)
{
	const AB_ACCOUNT_STATUS * status;
	const AB_BALANCE * bal;
	const AB_VALUE *v = 0;
	int rv;
	double balance;
	const char *bank_code; 
	const char *account_no; 
	PyObject *currency;

	// Check if all necessary data in account object is given!
	if (self->no == NULL)
	{                       
		PyErr_SetString(PyExc_ValueError, "Account number not set.");
		return NULL;
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_ValueError, "Bank code not set.");
		return NULL;
	}

#if PY_VERSION_HEX >= 0x03030000
	bank_code = PyUnicode_AsUTF8(self->bank_code);
	account_no = PyUnicode_AsUTF8(self->no);
#else
	PyObject *s = _PyUnicode_AsDefaultEncodedString(self->bank_code, NULL);
	bank_code = PyBytes_AS_STRING(s);
	s = _PyUnicode_AsDefaultEncodedString(self->no, NULL);
	account_no = PyBytes_AS_STRING(s);
#endif
	AB_ACCOUNT *a;
	AB_JOB *job = 0;
	AB_JOB_LIST2 *jl = 0;
	AB_IMEXPORTER_CONTEXT *ctx = 0;
	AB_IMEXPORTER_ACCOUNTINFO *ai;

	// Initialize aqbanking.
	rv = AB_create(self);
	if (rv > 0)
	{
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(self->ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account! ");
		return NULL;
	}

	// Create job and execute it.
	ctx = AB_ImExporterContext_new();
	jl = AB_Job_List2_new();
	job = AB_JobGetBalance_new(a);
	AB_Job_List2_PushBack(jl, job);
	rv = AB_Banking_ExecuteJobs(self->ab, jl, ctx);
	if (rv > 0)
	{
		PyErr_SetString(ExecutionFailed, "Could not get the balance!");
		return NULL;
	}

	// With success. No process the result.
	ai = AB_ImExporterContext_GetFirstAccountInfo(ctx);
	if (ai == NULL) {
		PyErr_SetString(ExecutionFailed, "Could not retrieve balance.");
		return NULL;		
	}
	status = AB_ImExporterAccountInfo_GetFirstAccountStatus(ai);
	bal = AB_AccountStatus_GetBookedBalance(status);
	v = AB_Balance_GetValue(bal);
	balance = AB_Value_GetValueAsDouble(v);
	currency = PyUnicode_FromString(AB_Value_GetCurrency(v));

	// Free jobs.
	AB_Job_List2_free(jl);
	AB_ImExporterContext_free(ctx);

	// Exit aqbanking.
	rv = AB_free(self);
	if (rv > 0)
	{
		PyErr_SetString(ExecutionFailed, "Could not free up aqbanking.");
		return NULL;
	}

	return Py_BuildValue("(d,O)", balance, currency);
}

static PyObject *aqbanking_Account_available_jobs(aqbanking_Account* self, PyObject *args, PyObject *keywds)
{
	int rv;
	AB_ACCOUNT *a;	
	const char *bank_code;
	const char *account_no;

	// Check if all necessary data in account object is given!
	if (self->no == NULL)
	{                       
		PyErr_SetString(PyExc_ValueError, "Account number not set.");
		return NULL;
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_ValueError, "Bank code not set.");
		return NULL;
	}

 #if PY_VERSION_HEX >= 0x03030000
	bank_code = PyUnicode_AsUTF8(self->bank_code);
	account_no = PyUnicode_AsUTF8(self->no);
#else
	PyObject *s = _PyUnicode_AsDefaultEncodedString(self->bank_code, NULL);
	bank_code = PyBytes_AS_STRING(s);
	s = _PyUnicode_AsDefaultEncodedString(self->no, NULL);
	account_no = PyBytes_AS_STRING(s);
#endif
	PyObject *featList = PyList_New(0);

	// Initialize aqbanking.
	rv = AB_create(self);
	if (rv > 0)
	{
		Py_DECREF(featList);
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(self->ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account! ");
		Py_DECREF(featList);
		return NULL;
	}


	// Check availableJobs
	// national transfer
	PyObject *feature = NULL;
	AB_JOB *abJob = AB_JobSingleTransfer_new(a);
	if (AB_Job_CheckAvailability(abJob) == 0) 
	{
		feature = PyUnicode_FromString("nationalTransfer");
		PyList_Append(featList, (PyObject *)feature);
		Py_DECREF(feature);
	}
	AB_Job_free(abJob);


	// sepa transfer
	abJob = AB_JobSepaTransfer_new(a);
	if (AB_Job_CheckAvailability(abJob) == 0) 
	{
		feature = PyUnicode_FromString("sepaTransfer");
		PyList_Append(featList, (PyObject *)feature);
		Py_DECREF(feature);
	}
	AB_Job_free(abJob);

	PyList_Append(featList, (PyObject *)feature);
	Py_DECREF(feature);

	// Exit aqbanking.
	rv = AB_free(self);
	if (rv > 0)
	{
		Py_DECREF(featList);
		return NULL;
	}

	return featList;
}

static PyObject *aqbanking_Account_transactions(aqbanking_Account* self, PyObject *args, PyObject *kwds)
{
	int rv;
	double tmpDateTime = 0;
	const char *bank_code;
	const char *account_no;

	// Check if all necessary data in account object is given!
	if (self->no == NULL)
	{
		PyErr_SetString(PyExc_ValueError, "Account number not set.");
		return NULL;
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_ValueError, "Bank code not set.");
		return NULL;
	}

#if PY_VERSION_HEX >= 0x03030000
	bank_code = PyUnicode_AsUTF8(self->bank_code);
	account_no = PyUnicode_AsUTF8(self->no);
#else
	PyObject *s = _PyUnicode_AsDefaultEncodedString(self->bank_code, NULL);
	bank_code = PyBytes_AS_STRING(s);
	s = _PyUnicode_AsDefaultEncodedString(self->no, NULL);
	account_no = PyBytes_AS_STRING(s);
#endif
	GWEN_TIME *gwTime;
	const char *dateFrom=NULL, *dateTo=NULL;
	static char *kwlist[] = {"dateFrom", "dateTo", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ss", kwlist, &dateFrom, &dateTo))
	{
		return NULL;
	}

	AB_ACCOUNT *a;
	AB_JOB *job = 0;
	AB_JOB_LIST2 *jl = 0;
	AB_IMEXPORTER_CONTEXT *ctx = 0;
	AB_IMEXPORTER_ACCOUNTINFO *ai;
	/*aqbanking_Transaction *trans = NULL;*/
	PyObject *transList = PyList_New(0);

	// Initialize aqbanking.
	rv = AB_create(self);
	if (rv > 0)
	{
		Py_DECREF(transList);
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(self->ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account! ");
		Py_DECREF(transList);
		return NULL;
	}

	// Create job and execute it.
	job = AB_JobGetTransactions_new(a);
	if (dateFrom != NULL)
	{
		gwTime = GWEN_Time_fromString(dateFrom, "YYYYMMDD");
		AB_JobGetTransactions_SetFromTime(job, gwTime);
	}
	if (dateTo != NULL)
	{
		gwTime = GWEN_Time_fromString(dateTo, "YYYYMMDD");
		AB_JobGetTransactions_SetToTime(job, gwTime);
	}
	// Check for availability
	rv = AB_Job_CheckAvailability(job);
	if (rv) {
		PyErr_SetString(ExecutionFailed, "Transaction retrieval is not supported!");
		Py_DECREF(transList);
		return NULL;
	}

	jl = AB_Job_List2_new();
	AB_Job_List2_PushBack(jl, job);
	ctx = AB_ImExporterContext_new();
	rv = AB_Banking_ExecuteJobs(self->ab, jl, ctx);

	if (rv)
	{
		PyErr_SetString(ExecutionFailed, "Could not retrieve transactions!");
		Py_DECREF(transList);
		return NULL;
	}

	// With success. No process the result.
	ai = AB_ImExporterContext_GetFirstAccountInfo (ctx);
	while(ai)
	{
		const AB_TRANSACTION *t;
		
		t = AB_ImExporterAccountInfo_GetFirstTransaction(ai);
		while(t) {
			const AB_VALUE *v;
			AB_TRANSACTION_STATUS state;

			v=AB_Transaction_GetValue(t);
			if (v) {
				const GWEN_STRINGLIST *sl;
				const GWEN_TIME *tdtime;
				const char *purpose;
				char* purposeBuffer = NULL;
				char* remoteNameBuffer = NULL;
				aqbanking_Transaction *trans = (aqbanking_Transaction*) PyObject_CallObject((PyObject *) &aqbanking_TransactionType, NULL);

				sl = AB_Transaction_GetPurpose(t);
				if (!sl) {
					purpose = "";
				} else {
					unsigned int count = GWEN_StringList_Count(sl);
					if (!count) {
						purpose = "";
					} else {
						unsigned int length = 0;
						for (unsigned int i = 0; i < count; i++) {
							length += strlen(GWEN_StringList_StringAt(sl, i));
						}

						if (!length) {
							purpose = "";
						} else {
							purposeBuffer = (char*)malloc(length + 1);
							if (!purposeBuffer) {
								purpose = "";
							} else {
								char* rover = purposeBuffer;
								for (unsigned int i = 0; i < count; i++) {
									unsigned int stringLength = strlen(GWEN_StringList_StringAt(sl, i));
									memcpy(rover, GWEN_StringList_StringAt(sl, i), stringLength);
									rover += stringLength;
								}
								*rover = '\0';
								purpose = purposeBuffer;
							}
						}
					}
				}

				#ifdef DEBUGSTDERR
				fprintf(stderr, "[%-10d]: [%-10s/%-10s][%-10s/%-10s] %-32s (%.2f %s)\n",
					AB_Transaction_GetUniqueId(t),
					AB_Transaction_GetRemoteIban(t),
					AB_Transaction_GetRemoteBic(t),
					AB_Transaction_GetRemoteAccountNumber(t),
					AB_Transaction_GetRemoteBankCode(t),
					purpose,
					AB_Value_GetValueAsDouble(v),
					AB_Value_GetCurrency(v)
					);
				#endif

				tdtime = AB_Transaction_GetDate(t);
				tmpDateTime = PyLong_AsDouble(PyLong_FromSize_t(GWEN_Time_Seconds(tdtime)));
				trans->date = PyDate_FromTimestamp(Py_BuildValue("(O)", PyFloat_FromDouble(tmpDateTime)));
				tdtime = AB_Transaction_GetValutaDate(t);
				tmpDateTime = PyLong_AsDouble(PyLong_FromSize_t(GWEN_Time_Seconds(tdtime)));
				trans->valutaDate = PyDate_FromTimestamp(Py_BuildValue("(O)", PyFloat_FromDouble(tmpDateTime)));
				trans->purpose = PyUnicode_FromString(purpose);

				if (purposeBuffer) {
					free(purposeBuffer);
					purposeBuffer = NULL;
				}

				// Local user
				if (AB_Transaction_GetLocalAccountNumber(t) == NULL) {
					trans->localAccount = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localAccount = PyUnicode_FromString(AB_Transaction_GetLocalAccountNumber(t));
				}
				if (AB_Transaction_GetLocalBankCode(t) == NULL) {
					trans->localBank = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localBank = PyUnicode_FromString(AB_Transaction_GetLocalBankCode(t));
				}
				if (AB_Transaction_GetLocalIban(t) == NULL) {
					trans->localIban = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localIban = PyUnicode_FromString(AB_Transaction_GetLocalIban(t));
				}
				if (AB_Transaction_GetLocalBic(t) == NULL) {
					trans->localBic = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localBic = PyUnicode_FromString(AB_Transaction_GetLocalBic(t));
				}
				if (AB_Transaction_GetLocalName(t) == NULL) {
					trans->localName = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localName = PyUnicode_FromString(AB_Transaction_GetLocalName(t));
				}

				// Remote user
				if (AB_Transaction_GetRemoteAccountNumber(t) == NULL) {
					trans->remoteAccount = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteAccount = PyUnicode_FromString(AB_Transaction_GetRemoteAccountNumber(t));
				}
				if (AB_Transaction_GetRemoteBankCode(t) == NULL) {
					trans->remoteBank = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteBank = PyUnicode_FromString(AB_Transaction_GetRemoteBankCode(t));
				}
				if (AB_Transaction_GetRemoteIban(t) == NULL) {
					trans->remoteIban = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteIban = PyUnicode_FromString(AB_Transaction_GetRemoteIban(t));
				}
				if (AB_Transaction_GetRemoteBic(t) == NULL) {
					trans->remoteBic = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteBic = PyUnicode_FromString(AB_Transaction_GetRemoteBic(t));
				}
				// Retrieve remote name of transaction
				if (AB_Transaction_GetRemoteName(t) == NULL) {
					trans->remoteName = Py_None;
					Py_INCREF(Py_None);
				} else {
					sl = AB_Transaction_GetRemoteName(t);
					unsigned int countRemoteName = GWEN_StringList_Count(sl);
					if (!countRemoteName) {
						trans->remoteName = Py_None;
						Py_INCREF(Py_None);
					} else {
						unsigned int lengthRN = 0;
						for (unsigned int i = 0; i < countRemoteName; i++) {
							lengthRN += strlen(GWEN_StringList_StringAt(sl, i));
						}
						if (!lengthRN) {
							trans->remoteName = Py_None;
							Py_INCREF(Py_None);
						} else {
							remoteNameBuffer = (char*)malloc(lengthRN + 1);
							if (!remoteNameBuffer) {
								trans->remoteName = Py_None;
								Py_INCREF(Py_None);
							} else {
								char* remoteNameHlp = remoteNameBuffer;
								for (unsigned int i = 0; i < countRemoteName; i++) {
									unsigned int stringLength = strlen(GWEN_StringList_StringAt(sl, i));
									memcpy(remoteNameHlp, GWEN_StringList_StringAt(sl, i), stringLength);
									remoteNameHlp += stringLength;
								}
								*remoteNameHlp = '\0';
								trans->remoteName = PyUnicode_FromString(remoteNameBuffer);
							}
						}
					}
				}

				// After filling up remote name, cleanup.
				if (remoteNameBuffer) {
					free(remoteNameBuffer);
					remoteNameBuffer = NULL;
				}

				trans->value = PyFloat_FromDouble(AB_Value_GetValueAsDouble(v));
				trans->currency = PyUnicode_FromString(AB_Value_GetCurrency(v));
				trans->uniqueId = PyLong_FromLong(AB_Transaction_GetUniqueId(t));
				if (AB_Transaction_GetTransactionText(t) == NULL) {
					trans->transactionText = PyUnicode_FromString("");
				} else {
					trans->transactionText = PyUnicode_FromString(AB_Transaction_GetTransactionText(t));
				}
				trans->transactionCode = PyLong_FromLong(AB_Transaction_GetTransactionCode(t));
				trans->textKey = PyLong_FromLong(AB_Transaction_GetTextKey(t));
				trans->textKeyExt = PyLong_FromLong(AB_Transaction_GetTextKeyExt(t));
				if (AB_Transaction_GetMandateId(t) == NULL) {
					trans->sepaMandateId = Py_None;
				} else {
					trans->sepaMandateId = PyUnicode_FromString(AB_Transaction_GetMandateId(t));
				}
				if (AB_Transaction_GetCustomerReference(t) == NULL) {
					trans->customerReference = PyUnicode_FromString("");
				} else {
					trans->customerReference = PyUnicode_FromString(AB_Transaction_GetCustomerReference(t));
				}
				if (AB_Transaction_GetBankReference(t) == NULL) {
					trans->bankReference = PyUnicode_FromString("");
				} else {
					trans->bankReference = PyUnicode_FromString(AB_Transaction_GetBankReference(t));
				}
				if (AB_Transaction_GetEndToEndReference(t) == NULL) {
					trans->endToEndReference = PyUnicode_FromString("");
				} else {
					trans->endToEndReference = PyUnicode_FromString(AB_Transaction_GetEndToEndReference(t));
				}
				if (AB_Transaction_GetFiId(t) == NULL) {
					trans->fiId = PyUnicode_FromString("");
				} else {
					trans->fiId = PyUnicode_FromString(AB_Transaction_GetFiId(t));
				}
				if (AB_Transaction_GetPrimanota(t) == NULL) {
					trans->primaNota = PyUnicode_FromString("");
				} else {
					trans->primaNota = PyUnicode_FromString(AB_Transaction_GetPrimanota(t));
				}
				trans->state = 0;
				state = AB_Transaction_GetStatus(t);
				switch(state)
				{
					case AB_Transaction_StatusUnknown:
					trans->state = -1;
					break;
					case AB_Transaction_StatusNone:
					trans->state = 0;
					break;
					case AB_Transaction_StatusAccepted:
					trans->state = 1;
					break;
					case AB_Transaction_StatusRejected:
					trans->state = 2;
					break;
					case AB_Transaction_StatusPending:
					trans->state = 4;
					break;
					case AB_Transaction_StatusSending:
					trans->state = 8;
					break;
					case AB_Transaction_StatusAutoReconciled:
					trans->state = 16;
					break;
					case AB_Transaction_StatusManuallyReconciled:
					trans->state = 32;
					break;
					case AB_Transaction_StatusRevoked:
					trans->state = 64;
					break;
					case AB_Transaction_StatusAborted:
					trans->state = 128;
					break;
				}

				PyList_Append(transList, (PyObject *)trans);
				Py_DECREF(trans);
			}
			t = AB_ImExporterAccountInfo_GetNextTransaction(ai);
		} 
		ai = AB_ImExporterContext_GetNextAccountInfo(ctx);
	}

	// Free jobs.
	AB_Job_free(job);
	AB_Job_List2_free(jl);
	AB_ImExporterContext_free(ctx);

	// Exit aqbanking.
	rv = AB_free(self);
	if (rv > 0)
	{
		//Py_XDECREF(trans);
		Py_DECREF(transList);
		return NULL;
	}

	return transList;
}

#ifdef FENQUEJOB
static PyObject *aqbanking_Account_enqueue_job(aqbanking_Account* self, PyObject *args, PyObject *kwds)
{
	int rv;
	AB_ACCOUNT *a;
	AB_JOB_LIST2 *jl;
	PyObject *result = NULL;
	const char *bank_code;
	const char *account_no;
#if PY_VERSION_HEX >= 0x03030000
	bank_code = PyUnicode_AsUTF8(self->bank_code);
	account_no = PyUnicode_AsUTF8(self->no);
#else
	PyObject *s = _PyUnicode_AsDefaultEncodedString(self->bank_code, NULL);
	bank_code = PyBytes_AS_STRING(s);
	s = _PyUnicode_AsDefaultEncodedString(self->no, NULL);
	account_no = PyBytes_AS_STRING(s);
#endif

	const char *remoteName=NULL, 
	*remoteIban=NULL,
	*remoteBic=NULL,
	*purpose=NULL,
	*endToEndReference=NULL,
	*textKey=NULL;
	double value = 0.00;
	const char *delim = "\n";

	static char *kwlist[] = {"remoteName", "remoteIban", "remoteBic", "purpose", "endToEndReference", "textKey", "value", NULL};
	if (! PyArg_ParseTupleAndKeywords(
		args, kwds, "|ssssssd", kwlist, &remoteName, &remoteIban, &remoteBic,
		&purpose, &endToEndReference, &textKey, &value))
	{
		return NULL;
	}

	// Valid data set?
	if (self->no == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "no");
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "bank_code");
	}

	// Initialize aqbanking.
	rv = AB_create(self);
	if (rv > 0)
	{
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(self->ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account! ");
		return NULL;
	}
	assert(a);

	// Validate remote data...
	rv = AB_Banking_CheckIban(remoteIban);
	if (rv > 0)
	{
		PyErr_SetString(InvalidIBAN, "Remote IBAN given for transfer is invalid.");
		return NULL;
	}

	// Check availableJobs
	// sepa transfer
	AB_JOB *abJob = AB_JobSepaTransfer_new(a);
	if (AB_Job_CheckAvailability(abJob) != 0) 
	{
		PyErr_SetString(JobNotAvailable, "SEPA transfer job not available!");
		AB_Job_free(abJob);
		return NULL;
	}

	AB_TRANSACTION *abTransaction = AB_Transaction_new();
	// Basic data
	AB_BANKING *abDetails = AB_Account_GetBanking(a);
	assert(abDetails);
	AB_Banking_FillGapsInTransaction(abDetails, a, abTransaction);

	// Recipient
	GWEN_STRINGLIST *remoteNameList = GWEN_StringList_fromString(remoteName, delim, 0);
	AB_Transaction_SetRemoteName(abTransaction, remoteNameList);
	GWEN_StringList_free(remoteNameList);
	AB_Transaction_SetRemoteIban(abTransaction, remoteIban);
	AB_Transaction_SetRemoteBic(abTransaction, remoteBic);

	// Origin
	//AB_Transaction_SetLocalAccount(abTransaction, a);
	
	// Purpose
	GWEN_STRINGLIST *purposeList = GWEN_StringList_fromString(purpose, delim, 0);
	AB_Transaction_SetPurpose(abTransaction, purposeList);
	GWEN_StringList_free(purposeList);

	// Reference
	//AB_Transaction_SetEndToEndReference(abTransaction, xxx);
	// Other fields
	// AB_Transaction_SetTextKey(abTransaction, xxx);
	AB_Transaction_SetValue(abTransaction, AB_Value_fromDouble(value));
	
	// Enque job.
	AB_Job_SetTransaction(abJob, abTransaction);
	AB_Transaction_free(abTransaction);
	jl = AB_Job_List2_new();
	AB_Job_List2_PushBack(jl, abJob);
	AB_IMEXPORTER_CONTEXT *ctx = AB_ImExporterContext_new();
	rv = AB_Banking_ExecuteJobs(self->ab, jl, ctx);
	if (rv) {
		PyErr_SetString(ExecutionFailed, "Could not execute SEPA transfer job.");
		AB_Job_free(abJob);
		AB_ImExporterContext_free(ctx);
		return NULL;
	}
	//AB_Job_free(abJob);

	// Free jobs.
	AB_Job_List2_FreeAll(jl);
	AB_ImExporterContext_free(ctx);
	AB_free(self);

	// Exit aqbanking.
	rv = AB_free(self);
	/*if (rv > 0)
	{
		return NULL;
	}*/

	Py_INCREF(Py_None);
	result = Py_None;
	return result;
}
#endif

static PyMemberDef aqbanking_Account_members[] = {
	{"no", T_OBJECT_EX, offsetof(aqbanking_Account, no), 0, "Account No."},
	{"name", T_OBJECT_EX, offsetof(aqbanking_Account, name), 0, "Name"},
	{"description", T_OBJECT_EX, offsetof(aqbanking_Account, description), 0, "Type of account (e.g. Kontokorrent)"},
	{"bank_code", T_OBJECT_EX, offsetof(aqbanking_Account, bank_code), 0, "Bank No."},
	{"bank_name", T_OBJECT_EX, offsetof(aqbanking_Account, bank_name), 0, "Name of Bank"},
	{NULL}
};

static PyMethodDef aqbanking_Account_methods[] = {
	/*{"name", (PyCFunction)aqbanking_Account_name, METH_NOARGS,
	 "Return the name, combining the first and last name"
	},*/
	{"balance", (PyCFunction)aqbanking_Account_balance, METH_VARARGS | METH_KEYWORDS, "Get the balance of the account."},
	{"transactions", (PyCFunction)aqbanking_Account_transactions, METH_VARARGS | METH_KEYWORDS, "Get the list of transactions of an account."},
	{"availableJobs", (PyCFunction)aqbanking_Account_available_jobs, METH_VARARGS | METH_KEYWORDS, "Get a list of available jobs."},
#ifdef FENQUEJOB
	{"enqueJob", (PyCFunction)aqbanking_Account_enqueue_job, METH_VARARGS | METH_KEYWORDS, "Make a transfer."},
#endif	
	{"set_callbackLog", (PyCFunction)aqbanking_Account_set_callbackLog, METH_VARARGS, "Adds a callback for the log output."},
	{"set_callbackPassword", (PyCFunction)aqbanking_Account_set_callbackPassword, METH_VARARGS, "Adds a callback to retrieve the password (pin)."},
	{"set_callbackPasswordStatus", (PyCFunction)aqbanking_Account_set_callbackPasswordStatus, METH_VARARGS, "Adds a callback to get feedback about pin status."},
	{"set_callbackCheckCert", (PyCFunction)aqbanking_Account_set_callbackCheckCert, METH_VARARGS, "Adds a callback to check the certificate."},
	{"cleanup", (PyCFunction)aqbanking_Account_cleanup, METH_VARARGS, "Cleanup and remove all callbacks."},
	{NULL}  /* Sentinel */
};

static PyTypeObject aqbanking_AccountType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"aqbanking.Account", /* tp_name */
	sizeof(aqbanking_Account), /* tp_basicsize */
	0, /* tp_itemsize */
	(destructor)aqbanking_Account_dealloc, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_reserved */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash  */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Account", /* tp_doc */
	0, /* tp_traverse */
	(inquiry)aqbanking_Account_clear, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	aqbanking_Account_methods, /* tp_methods */
	aqbanking_Account_members, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	(initproc)aqbanking_Account_init, /* tp_init */
	0, /* tp_alloc */
	aqbanking_Account_New, /* tp_new */
};

static PyObject * aqbanking_listacc(PyObject *self, PyObject *args)
{
	int rv;
	AB_ACCOUNT_LIST2 *accs;
	// List of accounts => to return.
	PyObject *accountList;
	aqbanking_Account *account = NULL;
	accountList = PyList_New(0);

	// Initialize aqbanking.
	rv = AB_create(NULL);
	if (rv > 0)
	{
		return NULL;
	}

	/* Get a list of accounts which are known to AqBanking.
	 * There are some pecularities about the list returned:
	 * The list itself is owned by the caller (who must call
	 * AB_Account_List2_free() as we do below), but the elements of that
	 * list (->the accounts) are still owned by AqBanking.
	 * Therefore you MUST NOT free any of the accounts within the list returned.
	 * This also rules out calling AB_Account_List2_freeAll() which not only
	 * frees the list itself but also frees all its elements.
	 *
	 * The rest of this tutorial shows how lists are generally used by
	 * AqBanking.
	 */
	accs = AB_Banking_GetAccounts(ab);
	if (accs) {
		AB_ACCOUNT_LIST2_ITERATOR *it;

		/* List2's are traversed using iterators. An iterator is an object
		 * which points to a single element of a list.
		 * If the list is empty NULL is returned.
		 */
		it=AB_Account_List2_First(accs);
		if (it) {
			AB_ACCOUNT *a;

			/* this function returns a pointer to the element of the list to
			 * which the iterator currently points to */
			a=AB_Account_List2Iterator_Data(it);
			while(a) {
				AB_PROVIDER *pro;
				account = (aqbanking_Account*) PyObject_CallObject((PyObject *) &aqbanking_AccountType, NULL);

				/* every account is assigned to a backend (sometimes called provider)
				 * which actually performs online banking tasks. We get a pointer
				 * to that provider/backend with this call to show its name in our
				 * example.*/
				pro = AB_Account_GetProvider(a);
				// Populate the object.
				account->no = PyUnicode_FromString(AB_Account_GetAccountNumber(a));
				account->name = PyUnicode_FromString(AB_Account_GetAccountName(a));
				account->description = PyUnicode_FromString(AB_Provider_GetName(pro));
				account->bank_code = PyUnicode_FromString(AB_Account_GetBankCode(a));
				account->bank_name = PyUnicode_FromString(AB_Account_GetBankName(a));
				PyList_Append(accountList, (PyObject *)account);
				Py_DECREF(account);

				/* this function lets the iterator advance to the next element in
				 * the list, so a following call to AB_Account_List2Iterator_Data()
				 * would return a pointer to the next element.
				 * This function also returns a pointer to the next element of the
				 * list. If there is no next element then NULL is returned. */
				a = AB_Account_List2Iterator_Next(it);
			}
			/* the iterator must be freed after using it */
			AB_Account_List2Iterator_free(it);
		}
		/* as discussed the list itself is only a container which has to be freed
		 * after use. This explicitly does not free any of the elements in that
		 * list, and it shouldn't because AqBanking still is the owner of the
		 * accounts */
		AB_Account_List2_free(accs);
	}

	// Exit aqbanking.
	rv = AB_free(NULL);
	if (rv > 0)
	{
		Py_DECREF(account);
		Py_DECREF(accountList);
		return NULL;
	}

	return accountList;
}

#ifdef SUPPORT_APPREGISTRATION
static PyObject *aqbanking_setRegistrationKey(PyObject *self, PyObject *args)
{
	int res;
	int rv;

	#ifdef DEBUGSTDERR
	if (fintsRegistrationKey == NULL) {
		fprintf(stderr, "aqbanking_setRegistrationKey: fintsRegistrationKey not set!\n");
	} else {
		fprintf(stderr, "aqbanking_setRegistrationKey: fintsRegistrationKey set: %s\n", fintsRegistrationKey);
	}
	#endif
	// List of accounts => to return.
	const char *registrationKey;

	if (!PyArg_ParseTuple(args, "s", &registrationKey))
		return NULL;

	fintsRegistrationKey = registrationKey;
	#ifdef DEBUGSTDERR
	if (fintsRegistrationKey == NULL) {
		fprintf(stderr, "aqbanking_setRegistrationKey: fintsRegistrationKey not set!\n");
	} else {
		fprintf(stderr, "aqbanking_setRegistrationKey: fintsRegistrationKey set: %s\n", fintsRegistrationKey);
	}
	#endif

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

static PyObject *aqbanking_chkiban(PyObject *self, PyObject *args)
{
	int res;
	int rv;
	// List of accounts => to return.
	const char *iban;

	if (!PyArg_ParseTuple(args, "s", &iban))
		return NULL;
	
	// Initialize aqbanking.
	rv = AB_create(NULL);
	if (rv > 0)
	{
		return NULL;
	}

	res = AB_Banking_CheckIban(iban);

	// Exit aqbanking.
	rv = AB_free(NULL);
	if (rv > 0)
	{
		return NULL;
	}

	if (res == 0) {
		return PyBool_FromLong(1);
	} else {
		return PyBool_FromLong(0);
	}
}

static PyMethodDef AqBankingMethods[] = {
	{"listacc", aqbanking_listacc, METH_VARARGS, "Get a list of accounts"},
	{"chkiban", aqbanking_chkiban, METH_VARARGS, "Validates an IBAN"},
	#ifdef SUPPORT_APPREGISTRATION
	{"setRegistrationKey", aqbanking_setRegistrationKey, METH_VARARGS, "Set the FinTS registration key"},
	#endif
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef aqbankingmodule = {
	PyModuleDef_HEAD_INIT,
	"aqbanking",
	"This is an API to the AqBanking software.",
	-1,
	AqBankingMethods
};

PyMODINIT_FUNC
PyInit_aqbanking(void)
{
	PyObject *m;

	// First initialize the types
	aqbanking_AccountType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&aqbanking_AccountType) < 0)
		return NULL;

	aqbanking_TransactionType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&aqbanking_TransactionType) < 0)
		return NULL;

	// And then the module. 
	m = PyModule_Create(&aqbankingmodule);
	if (m == NULL)
		return NULL;

	// Exceptions 
	AqBankingInitializeError = PyErr_NewException("aqbanking.AqBankingInitializeError", NULL, NULL);
	Py_INCREF(AqBankingInitializeError);
	PyModule_AddObject(m, "AqBankingInitializeError", AqBankingInitializeError);
	AqBankingDeInitializeError = PyErr_NewException("aqbanking.AqBankingDeInitializeError", NULL, NULL);
	Py_INCREF(AqBankingDeInitializeError);
	PyModule_AddObject(m, "AqBankingDeInitializeError", AqBankingDeInitializeError);
	AccountNotFound = PyErr_NewException("aqbanking.AccountNotFound", NULL, NULL);
	Py_INCREF(AccountNotFound);
	PyModule_AddObject(m, "AccountNotFound", AccountNotFound);
	ExecutionFailed = PyErr_NewException("aqbanking.ExecutionFailed", NULL, NULL);
	Py_INCREF(ExecutionFailed);
	PyModule_AddObject(m, "ExecutionFailed", ExecutionFailed);
#ifdef FENQUEJOB
	InvalidIBAN = PyErr_NewException("aqbanking.InvalidIBAN", NULL, NULL);
	Py_INCREF(InvalidIBAN);
	PyModule_AddObject(m, "InvalidIBAN", InvalidIBAN);
	JobNotAvailable = PyErr_NewException("aqbanking.JobNotAvailable", NULL, NULL);
	Py_INCREF(JobNotAvailable);
	PyModule_AddObject(m, "JobNotAvailable", JobNotAvailable);
#endif

	// Some types
	Py_INCREF(&aqbanking_AccountType);
	PyModule_AddObject(m, "Account", (PyObject *)&aqbanking_AccountType);
	Py_INCREF(&aqbanking_TransactionType);
	PyModule_AddObject(m, "Transaction", (PyObject *)&aqbanking_TransactionType);

	// Initialize PyDateTime
	PyDateTime_IMPORT;

	aqh = new PyAqHandler();

	return m;
}
