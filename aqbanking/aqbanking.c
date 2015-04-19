
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Python.h>
#include "unicodeobject.h"
#include "structmember.h"
#include <aqbanking/banking.h>
#include <aqbanking/jobgetbalance.h>
#include <gwenhywfar/cgui.h>

/**
 * Here are some exceptions.
 */
static PyObject *AqBankingInitializeError;
static PyObject *AqBankingDeInitializeError;
static PyObject *AccountNotFound;
static PyObject *ExecutionFailed;

/**
 * Here are some callback variables.
 */
static PyObject *callbackLog = NULL;
static PyObject *callbackMsgBox = NULL;
static PyObject *callbackPassword = NULL;
static PyObject *userId = NULL;

/**
 * AqBanking
 */
AB_BANKING *ab;
GWEN_GUI *gui;

typedef struct {
	PyObject_HEAD
	PyObject *no;
	PyObject *name;
	PyObject *description;
	PyObject *bank_code;
	PyObject *bank_name;

} aqbanking_Account;

static void aqbanking_Account_dealloc(aqbanking_Account* self)
{
	Py_XDECREF(self->no);
	Py_XDECREF(self->name);
	Py_XDECREF(self->description);
	Py_XDECREF(self->bank_code);
	Py_XDECREF(self->bank_name);
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

	return 0;
}

static PyMemberDef aqbanking_Account_members[] = {
	{"no", T_OBJECT_EX, offsetof(aqbanking_Account, no), 0, "Account No."},
	{"name", T_OBJECT_EX, offsetof(aqbanking_Account, name), 0, "Name"},
	{"description", T_OBJECT_EX, offsetof(aqbanking_Account, description), 0, "Type of account (e.g. Kontokorrent)"},
	{"bank_code", T_OBJECT_EX, offsetof(aqbanking_Account, bank_code), 0, "Bank No."},
	{"bank_name", T_OBJECT_EX, offsetof(aqbanking_Account, bank_name), 0, "Name of Bank"},
	{NULL}
};

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
	/*if (self->description == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "description");
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "bank_code");
	}
	if (self->bank_name == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "bank_name");
	}*/

	return PyUnicode_FromFormat("%S: %S", self->no, self->name);
}

static PyMethodDef aqbanking_Account_methods[] = {
	/*{"name", (PyCFunction)aqbanking_Account_name, METH_NOARGS,
	 "Return the name, combining the first and last name"
	},*/
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
	0, /* tp_clear */
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
	aqbanking_Account *account;
	accountList = PyList_New(0);

	// Initialize aqbanking.
	rv = AB_create();
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
				account = (aqbanking_Account*) _PyObject_New(&aqbanking_AccountType);
				account = (aqbanking_Account*) PyObject_Init((PyObject *)account, &aqbanking_AccountType);

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
	rv = AB_free();
	if (rv > 0)
	{
		Py_DECREF(account);
		Py_DECREF(accountList);
		return NULL;
	}

	return accountList;
}

static PyObject * aqbanking_chkiban(PyObject *self, PyObject *args)
{
	AB_BANKINFO_CHECKRESULT res;
	int rv;
	// List of accounts => to return.
	const char *iban;

	if (!PyArg_ParseTuple(args, "s", &iban))
		return NULL;
	
	// Initialize aqbanking.
	rv = AB_create();
	if (rv > 0)
	{
		return NULL;
	}

	res = AB_Banking_CheckIban(iban);

	// Exit aqbanking.
	rv = AB_free();
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

static PyObject *aqbanking_balance(PyObject *self, PyObject *args, PyObject *keywds)
{
	const AB_ACCOUNT_STATUS * status;
	const AB_BALANCE * bal;
	const AB_VALUE *v = 0;
	int rv;
	double balance;
	//const char *currency;
	const char *bank_code;
	const char *account_no;
	AB_ACCOUNT *a;
	AB_USER *u = 0;
	AB_JOB *job = 0;
	AB_JOB_LIST2 *jl = 0;
	AB_IMEXPORTER_CONTEXT *ctx = 0;
	AB_IMEXPORTER_ACCOUNTINFO *ai;
	// Return:
	PyObject *result;

	static char *kwlist[] = {"account", "bank", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &account_no, &bank_code))
		return NULL;

	// Initialize aqbanking.
	rv = AB_create();
	if (rv > 0)
	{
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account!");
		return NULL;
	}

	u = AB_Account_GetFirstUser(a);

	// Create job and execute it.
	ctx = AB_ImExporterContext_new();
	jl = AB_Job_List2_new();
	job = AB_JobGetBalance_new(a);
	AB_Job_List2_PushBack(jl, job);
	rv = AB_Banking_ExecuteJobs(ab, jl, ctx);

	if (rv)
	{
		PyErr_SetString(ExecutionFailed, "Could not get the balance!");
		return NULL;
	}

	// With success. No process the result.
	ai = AB_ImExporterContext_GetFirstAccountInfo (ctx);
	status = AB_ImExporterAccountInfo_GetFirstAccountStatus (ai);
	bal = AB_AccountStatus_GetBookedBalance (status);
	v = AB_Balance_GetValue (bal);
	balance = AB_Value_GetValueAsDouble(v);
	//currency = AB_Value_GetCurrency(v);

	// Free jobs.
	AB_Job_List2_free(jl);
	AB_ImExporterContext_free(ctx);

	// Exit aqbanking.
	rv = AB_free();
	if (rv > 0)
	{
		return NULL;
	}

	return Py_BuildValue("(d,s)", balance, "EUR");
}

/**
 * Gwen Gui progress log callback (which will then send to python callback!)
 */
int zProgressLog(GWEN_GUI *gui, uint32_t id, GWEN_LOGGER_LEVEL level, const char *text) 
{
	PyObject *arglist = Py_BuildValue("ss", userId, text);
	PyObject_CallObject(callbackLog, arglist);
	Py_DECREF(arglist);

	return 0;
}

/**
 * Gwen Gui message box callback (which will then send to python callback!)
 */
int zMessageBox(GWEN_GUI *gui, uint32_t flags,  const char *title,  const char *text,  const char *b1,  const char *b2,  const char *b3,  uint32_t guiid)
{
	long retCode;
	PyObject *result;
	PyObject *arglist = Py_BuildValue("ssssss", userId, title, text, b1, b2, b3);
	result = PyObject_CallObject(callbackMsgBox, arglist);
	Py_DECREF(arglist);

	// So we need a result! So now lets check it.
	if (result == NULL)
		return 1;

	// Now convert the object to int.
	retCode = PyLong_AsLong(result);
	if (retCode == -1 && PyErr_Occurred())
	{
		return -1;
	}

	return (int)retCode;
}

/**
 * Gwen GUI passwort callback.
 * @param  gui    [description]
 * @param  flags  [description]
 * @param  token  [description]
 * @param  title  [description]
 * @param  text   [description]
 * @param  buffer [description]
 * @param  minLen [description]
 * @param  maxLen [description]
 * @param  guiid  [description]
 * @return        [description]
 */
int zPasswordFn(GWEN_GUI *gui, uint32_t flags, const char *token, const char *title, const char *text, char *buffer, int minLen, int maxLen, uint32_t guiid)
{
	const char *passwordPy;
	PyObject *result;
	PyObject *arglist = Py_BuildValue("sIsssii", userId, flags, token, title, text, minLen, maxLen);
	result = PyObject_CallObject(callbackPassword, arglist);
	Py_DECREF(arglist);

	// So we need a result! So now lets check it.
	if (result == NULL) {
		fprintf(stderr, "%s", "No password returned!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		PyErr_Print(); 
		return 1;
	} else {
		// Now convert the object to int.
		passwordPy = PyUnicode_AsUTF8(result);
		strcpy(buffer, passwordPy);
		Py_DECREF(result);
	}

	return 0;
}

/**
 * Return always: yes mam!
 */
int zCheckCert(GWEN_GUI *gui, const GWEN_SSLCERTDESCR *cd, GWEN_SYNCIO *sio, uint32_t guiid) 
{
	// FIXME: implement at least some basic checks => or ask user via callback!
	return 0;
}

static PyObject *aqbanking_set_callbackLog(PyObject *dummy, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "O:set_callbackLog", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(callbackLog);  /* Dispose of previous callback */
        callbackLog = temp;       /* Remember new callback */
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }
    return result;
}

static PyObject *aqbanking_set_callbackMsgBox(PyObject *dummy, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "O:set_callbackMsgBox", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(callbackMsgBox);  /* Dispose of previous callback */
        callbackMsgBox = temp;       /* Remember new callback */
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }
    return result;
}

static PyObject *aqbanking_set_callbackPassword(PyObject *dummy, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "O:set_callbackPassword", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(callbackPassword);  /* Dispose of previous callback */
        callbackPassword = temp;       /* Remember new callback */
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }
    return result;
}

static PyObject *aqbanking_set_user_id(PyObject *dummy, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "O:set_user_id", &temp)) {
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(userId);  /* Dispose of previous callback */
        userId = temp;       /* Remember new callback */
        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }
    return result;
}

/**
 * TODO: some kind of testing!!!!
 */
static PyObject *aqbanking_hello(PyObject *self, PyObject *args)
{
	// Time to call our callback!
	PyObject *result;
	PyObject *arglist = Py_BuildValue("(s)", "Hello World!!!!");
	result = PyObject_CallObject(callbackLog, arglist);
	Py_DECREF(arglist);

	// So we need a result! So now lets check it.
	if (result == NULL)
		return NULL;

	return result;
}

static PyMethodDef AqBankingMethods[] = {
	{"listacc", aqbanking_listacc, METH_VARARGS, "Get a list of accounts"},
	{"balance", (PyCFunction)aqbanking_balance, METH_VARARGS | METH_KEYWORDS, "Get the balance of a specific account."},
	{"chkiban", aqbanking_chkiban, METH_VARARGS, "Validates an IBAN"},
	{"set_callbackLog", aqbanking_set_callbackLog, METH_VARARGS, "Adds a callback for the log output."},
	{"set_callbackMsgBox", aqbanking_set_callbackMsgBox, METH_VARARGS, "Adds a callback for the log output."},
	{"set_callbackPassword", aqbanking_set_callbackPassword, METH_VARARGS, "Adds a callback to retrieve the password (pin)."},
	{"set_user_id", aqbanking_set_user_id, METH_VARARGS, "Set the user id of the session."},
	{"hello", aqbanking_hello, METH_VARARGS, "Triggers a hello world to callback."},

	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef aqbankingmodule = {
	PyModuleDef_HEAD_INIT,
	"aqbanking",
	"This is an API to the AqBanking software.",
	-1,
	AqBankingMethods
};

int AB_create() {

        int rv = 0;

        //Initialisierungen GWEN
        //GWEN_Logger_SetLevel(0, GWEN_LoggerLevel_Verbous);
        gui = GWEN_Gui_CGui_new();
        GWEN_Gui_SetGui(gui);

        // Initialisierungen AB
        ab = AB_Banking_new("python-aqbanking", 0, AB_BANKING_EXTENSION_NONE);
        rv = AB_Banking_Init(ab);
        if (rv) {
                PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not initialize (%d).", rv));
                return 2;
        }
        rv = AB_Banking_OnlineInit(ab);
        if (rv) {
        		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online initialize (%d).", rv));
                return 2;
        }
        
        //Setzen der Call-backs
        GWEN_Gui_SetProgressLogFn(gui, zProgressLog);
        GWEN_Gui_SetCheckCertFn(gui, zCheckCert);
        GWEN_Gui_SetMessageBoxFn(gui, zMessageBox);
        GWEN_Gui_SetGetPasswordFn(gui, zPasswordFn);

        return 0;

}

int AB_free() {
        int rv = 0;

        rv = AB_Banking_OnlineFini(ab);
        if (rv) 
        {
                PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online deinit. (%d).", rv));
                return 3;
        }

        rv = AB_Banking_Fini(ab);
        if (rv) 
        {
                PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do deinit. (%d).", rv));
                return 3;
        }

        AB_Banking_free(ab);
        return 0;
}

PyMODINIT_FUNC
PyInit_aqbanking(void)
{
	PyObject *m;

	// First initialize the types
	aqbanking_AccountType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&aqbanking_AccountType) < 0)
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

	// Some types
	Py_INCREF(&aqbanking_AccountType);
	PyModule_AddObject(m, "Account", (PyObject *)&aqbanking_AccountType);

	return m;
}
 