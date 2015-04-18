
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Python.h>
#include "unicodeobject.h"
#include "structmember.h"
#include <aqbanking/banking.h>
#include <gwenhywfar/cgui.h>

static PyObject *AqBankingInitializeError;
static PyObject *AqBankingDeInitializeError;

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
	AB_BANKING *ab;
	AB_ACCOUNT_LIST2 *accs;
	int rv;
	GWEN_GUI *gui;
	// List of accounts => to return.
	PyObject *accountList;
	aqbanking_Account *account;
	accountList = PyList_New(0);

	gui=GWEN_Gui_CGui_new();
	GWEN_Gui_SetGui(gui);

	ab = AB_Banking_new("python-aqbanking", 0, 0);
	rv = AB_Banking_Init(ab);
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not initialize (%d).", rv));
		return NULL;
	}

	rv = AB_Banking_OnlineInit(ab);
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do onlineinit. (%d).", rv));
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


	rv=AB_Banking_OnlineFini(ab);
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online deinit. (%d).", rv));
		Py_DECREF(account);
		Py_DECREF(accountList);
		return NULL;
	}

	rv=AB_Banking_Fini(ab);
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do deinit. (%d).", rv));
		Py_DECREF(account);
		Py_DECREF(accountList);
		return NULL;
	}
	AB_Banking_free(ab);
	return accountList;
}

static PyObject * aqbanking_chkiban(PyObject *self, PyObject *args)
{
	AB_BANKING *ab;
	AB_BANKINFO_CHECKRESULT res;
	int rv;
	GWEN_GUI *gui;
	// List of accounts => to return.
	const char *iban;

	if (!PyArg_ParseTuple(args, "s", &iban))
		return NULL;

	gui=GWEN_Gui_CGui_new();
	GWEN_Gui_SetGui(gui);

	ab = AB_Banking_new("python-aqbanking", 0, 0);
	rv = AB_Banking_Init(ab);
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not initialize (%d).", rv));
		return NULL;
	}

	res = AB_Banking_CheckIban(iban);

	rv = AB_Banking_Fini(ab);
	if (rv) {
		PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do deinit. (%d).", rv));
		return NULL;
	}
	AB_Banking_free(ab);
	if (res == 0) {
		return PyBool_FromLong(1);
	} else {
		return PyBool_FromLong(0);
	}
}

static PyMethodDef AqBankingMethods[] = {
	{"listacc", aqbanking_listacc, METH_VARARGS, "Get a list of accounts"},
	{"chkiban", aqbanking_chkiban, METH_VARARGS, "Validates an IBAN"},

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

	/* First initialize the types */
	aqbanking_AccountType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&aqbanking_AccountType) < 0)
		return NULL;

	/* And then the module. */
	m = PyModule_Create(&aqbankingmodule);
	if (m == NULL)
		return NULL;

	/* Exceptions */
	AqBankingInitializeError = PyErr_NewException("aqbanking.AqBankingInitializeError", NULL, NULL);
	Py_INCREF(AqBankingInitializeError);
	PyModule_AddObject(m, "AqBankingInitializeError", AqBankingInitializeError);
	AqBankingDeInitializeError = PyErr_NewException("aqbanking.AqBankingDeInitializeError", NULL, NULL);
	Py_INCREF(AqBankingDeInitializeError);
	PyModule_AddObject(m, "AqBankingDeInitializeError", AqBankingDeInitializeError);

	/* Some types */
	Py_INCREF(&aqbanking_AccountType);
	PyModule_AddObject(m, "Account", (PyObject *)&aqbanking_AccountType);

	return m;
}
 