#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::VariantData
#include <iostream> // --trace
#include <sstream> // __str__
#include <qtreset.h>


#include <pybind11/pybind11.h>
#include <functional>
#include <string>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qline.h>
#include <QtCore/qmargins.h>
#include <QtCore/qurl.h>
#include <QtCore/qurlquery.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qrect.h>
#include <QtCore/qtextcodec.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qeasingcurve.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qxmlstream.h>
#include <QtGui/qcolor.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qabstractnativeeventfilter.h>
#include <QtCore/qcalendar.h>
#include <QtXml/qdom.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qtimezone.h>
#include <setjmp.h>
#include <core/Logger.h>
#include <custom_qt_casters.h>


#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>)
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*)
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>)
#endif

void bind_QtCore_qmetatype_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B348_[QtMetaTypePrivate::QPairVariantInterfaceImpl] ";
	{ // QtMetaTypePrivate::QPairVariantInterfaceImpl file:QtCore/qmetatype.h line:1414
		pybind11::class_<QtMetaTypePrivate::QPairVariantInterfaceImpl, std::shared_ptr<QtMetaTypePrivate::QPairVariantInterfaceImpl>> cl(M("QtMetaTypePrivate"), "QPairVariantInterfaceImpl", "");
		cl.def( pybind11::init( [](){ return new QtMetaTypePrivate::QPairVariantInterfaceImpl(); } ) );
		cl.def( pybind11::init( [](QtMetaTypePrivate::QPairVariantInterfaceImpl const &o){ return new QtMetaTypePrivate::QPairVariantInterfaceImpl(o); } ) );
		cl.def("first", (struct QtMetaTypePrivate::VariantData (QtMetaTypePrivate::QPairVariantInterfaceImpl::*)() const) &QtMetaTypePrivate::QPairVariantInterfaceImpl::first, "C++: QtMetaTypePrivate::QPairVariantInterfaceImpl::first() const --> struct QtMetaTypePrivate::VariantData");
		cl.def("second", (struct QtMetaTypePrivate::VariantData (QtMetaTypePrivate::QPairVariantInterfaceImpl::*)() const) &QtMetaTypePrivate::QPairVariantInterfaceImpl::second, "C++: QtMetaTypePrivate::QPairVariantInterfaceImpl::second() const --> struct QtMetaTypePrivate::VariantData");
		cl.def("assign", (class QtMetaTypePrivate::QPairVariantInterfaceImpl & (QtMetaTypePrivate::QPairVariantInterfaceImpl::*)(const class QtMetaTypePrivate::QPairVariantInterfaceImpl &)) &QtMetaTypePrivate::QPairVariantInterfaceImpl::operator=, "C++: QtMetaTypePrivate::QPairVariantInterfaceImpl::operator=(const class QtMetaTypePrivate::QPairVariantInterfaceImpl &) --> class QtMetaTypePrivate::QPairVariantInterfaceImpl &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
