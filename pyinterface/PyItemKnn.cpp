#include "PyItemKnn.h"
#include "PyCommon.h"
#include "SigHandler.h"
#include "DataWriter.h"
#include "MAP.h"
#include "NDCG.h"

#include <iostream>
#include <string>

using namespace std;

static
PyMethodDef ItemKnn_methods[] =
{
   { "train",     (PyCFunction)ItemKnn_train,          METH_VARARGS|METH_KEYWORDS, "train model" },
   { "test",      (PyCFunction)PyTest<PyItemKnn>,      METH_VARARGS|METH_KEYWORDS, "test prediction model" },
   { "testrec",   (PyCFunction)PyTestrec<PyItemKnn>,   METH_VARARGS|METH_KEYWORDS, "test recommendation model" },
   { "predict",   (PyCFunction)PyPredict<PyItemKnn>,   METH_VARARGS,               "predict user's rating for an item" },
   { "recommend", (PyCFunction)PyRecommend<PyItemKnn>, METH_VARARGS|METH_KEYWORDS, "recommend ranked items to a user" },
   { "precision", (PyCFunction)PyPrecision<PyItemKnn>, METH_VARARGS|METH_KEYWORDS, "calculate Precision for a user" },
   { "recall",    (PyCFunction)PyRecall<PyItemKnn>,    METH_VARARGS|METH_KEYWORDS, "calculate Recall for a user" },
   { "AUC",       (PyCFunction)PyAUC<PyItemKnn>,       METH_VARARGS|METH_KEYWORDS, "calculate Area Under the ROC Curve for a user" },
   { "MAP",       (PyCFunction)PynDCG<PyItemKnn>,      METH_VARARGS|METH_KEYWORDS, "calculate Mean Average Precision for a user" },
   { "nDCG",      (PyCFunction)PyMAP<PyItemKnn>,       METH_VARARGS|METH_KEYWORDS, "calculate Normalized Discounted Cumulative Gain for a user" },
   { NULL, NULL, 0, NULL }
};

static PyTypeObject ItemKnnType =
{
#if PY_MAJOR_VERSION >= 3
   PyVarObject_HEAD_INIT(NULL, 0)
#else
   PyObject_HEAD_INIT( NULL )
   0,                                        // ob_size
#endif
   "libpyreclab.ItemKnn",                    // tp_name
   sizeof(PyItemKnn),                        // tp_basicsize
   0,                                        // tp_itemsize
   (destructor)PyDealloc<PyItemKnn>,         // tp_dealloc
   0,                                        // tp_print
   0,                                        // tp_getattr
   0,                                        // tp_setattr
   0,                                        // tp_compare
   0,                                        // tp_repr
   0,                                        // tp_as_number
   0,                                        // tp_as_sequence
   0,                                        // tp_as_mapping
   0,                                        // tp_hash
   0,                                        // tp_call
   0,                                        // tp_str
   0,                                        // tp_getattro
   0,                                        // tp_setattro
   0,                                        // tp_as_buffer
   Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
   "ItemKnn objects",                        // tp_doc
   0,                                        // tp_traverse
   0,                                        // tp_clear
   0,                                        // tp_richcompare
   0,                                        // tp_weaklistoffset
   0,                                        // tp_iter
   0,                                        // tp_iternext
   ItemKnn_methods,                          // tp_methods
   0,                                        // tp_members
   0,                                        // tp_getset
   0,                                        // tp_base
   0,                                        // tp_dict
   0,                                        // tp_descr_get
   0,                                        // tp_descr_set
   0,                                        // tp_dictoffset
   0,                                        // tp_init
   0,                                        // tp_alloc
   PyNew<PyItemKnn, AlgItemBasedKnn>,        // tp_new
};


PyTypeObject* ItemKnnGetType()
{
   return &ItemKnnType;
}

PyObject* ItemKnn_train( PyItemKnn* self, PyObject* args, PyObject* kwdict )
{
   int k = 10;
   const char* simType = NULL;
   int progress = 0;

   static char* kwlist[] = { const_cast<char*>( "k" ),
                             const_cast<char*>( "similarity" ),
                             const_cast<char*>( "progress" ),
                             NULL };

   if( !PyArg_ParseTupleAndKeywords( args, kwdict, "|isi", kwlist, &k, &simType, &progress ) )
   {
      return NULL;
   }

   string strSimType = NULL != simType ? simType : "pearson";
   transform( strSimType.begin(), strSimType.end(), strSimType.begin(), ::tolower );
   if( !strSimType.empty() && strSimType != "pearson" && strSimType != "cosine" )
   {
      PyGILState_STATE gstate = PyGILState_Ensure();
      string eMsg = "Unknown similarity metric '";
      eMsg += strSimType + "'";
      PyErr_SetString( PyExc_ValueError, eMsg.c_str() );
      PyGILState_Release( gstate );
      return NULL;
   }

   SigHandler sigHandler( SIGINT );
   int cause = -1;
   string eMsg;
   try
   {
      cause = dynamic_cast<AlgItemBasedKnn*>( self->m_recAlgorithm )->train( k, strSimType, sigHandler, progress );
   }
   catch( const char* e )
   {
      eMsg = e;
   }

   if( AlgItemBasedKnn::STOPPED == cause )
   {
      PyGILState_STATE gstate = PyGILState_Ensure();
      PyErr_SetString( PyExc_KeyboardInterrupt, "SIGINT received" );
      PyGILState_Release( gstate );
      return NULL;
   }
   else if( 0 > cause )
   {
      PyGILState_STATE gstate = PyGILState_Ensure();
      PyErr_SetString( PyExc_RuntimeError, eMsg.c_str() );
      PyGILState_Release( gstate );
      return NULL;
   }

   Py_INCREF( Py_None );
   return Py_None;
}


