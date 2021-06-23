#include <QtCore/qarraydata.h> // 
#include <QtCore/qarraydata.h> // QArrayData
#include <QtCore/qarraydata.h> // QArrayDataPointerRef
#include <QtCore/qarraydata.h> // QTypedArrayData
#include <QtCore/qarraydata.h> // QTypedArrayData<QStringRef>::AlignmentDummy
#include <QtCore/qarraydata.h> // QTypedArrayData<QXmlStreamAttribute>::AlignmentDummy
#include <QtCore/qarraydata.h> // QTypedArrayData<char>::AlignmentDummy
#include <QtCore/qarraydata.h> // QTypedArrayData<int>::AlignmentDummy
#include <QtCore/qarraydata.h> // QTypedArrayData<unsigned int>::AlignmentDummy
#include <QtCore/qarraydata.h> // QTypedArrayData<unsigned short>::AlignmentDummy
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qvector.h> // QVector
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
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

void bind_QtCore_qarraydata(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B300_[QTypedArrayData<char>] ";
	{ // QTypedArrayData file:QtCore/qarraydata.h line:129
		pybind11::class_<QTypedArrayData<char>, std::shared_ptr<QTypedArrayData<char>>> cl(M(""), "QTypedArrayData_char_t", "");
		cl.def( pybind11::init( [](){ return new QTypedArrayData<char>(); } ) );
		cl.def("data", (char * (QTypedArrayData<char>::*)()) &QTypedArrayData<char>::data, "C++: QTypedArrayData<char>::data() --> char *", pybind11::return_value_policy::automatic);
		cl.def("begin", [](QTypedArrayData<char> &o) -> char * { return o.begin(); }, "", pybind11::return_value_policy::automatic);
		cl.def("begin", (char * (QTypedArrayData<char>::*)(char *)) &QTypedArrayData<char>::begin, "C++: QTypedArrayData<char>::begin(char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("end", [](QTypedArrayData<char> &o) -> char * { return o.end(); }, "", pybind11::return_value_policy::automatic);
		cl.def("end", (char * (QTypedArrayData<char>::*)(char *)) &QTypedArrayData<char>::end, "C++: QTypedArrayData<char>::end(char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("begin", [](QTypedArrayData<char> const &o) -> const char * { return o.begin(); }, "", pybind11::return_value_policy::automatic);
		cl.def("begin", (const char * (QTypedArrayData<char>::*)(const char *) const) &QTypedArrayData<char>::begin, "C++: QTypedArrayData<char>::begin(const char *) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("end", [](QTypedArrayData<char> const &o) -> const char * { return o.end(); }, "", pybind11::return_value_policy::automatic);
		cl.def("end", (const char * (QTypedArrayData<char>::*)(const char *) const) &QTypedArrayData<char>::end, "C++: QTypedArrayData<char>::end(const char *) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("constBegin", [](QTypedArrayData<char> const &o) -> const char * { return o.constBegin(); }, "", pybind11::return_value_policy::automatic);
		cl.def("constBegin", (const char * (QTypedArrayData<char>::*)(const char *) const) &QTypedArrayData<char>::constBegin, "C++: QTypedArrayData<char>::constBegin(const char *) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("constEnd", [](QTypedArrayData<char> const &o) -> const char * { return o.constEnd(); }, "", pybind11::return_value_policy::automatic);
		cl.def("constEnd", (const char * (QTypedArrayData<char>::*)(const char *) const) &QTypedArrayData<char>::constEnd, "C++: QTypedArrayData<char>::constEnd(const char *) const --> const char *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("sharedNull", (struct QTypedArrayData<char> * (*)()) &QTypedArrayData<char>::sharedNull, "C++: QTypedArrayData<char>::sharedNull() --> struct QTypedArrayData<char> *", pybind11::return_value_policy::automatic);
		cl.def_static("sharedEmpty", (struct QTypedArrayData<char> * (*)()) &QTypedArrayData<char>::sharedEmpty, "C++: QTypedArrayData<char>::sharedEmpty() --> struct QTypedArrayData<char> *", pybind11::return_value_policy::automatic);
		cl.def_static("unsharableEmpty", (struct QTypedArrayData<char> * (*)()) &QTypedArrayData<char>::unsharableEmpty, "C++: QTypedArrayData<char>::unsharableEmpty() --> struct QTypedArrayData<char> *", pybind11::return_value_policy::automatic);

		{ // QTypedArrayData<char>::AlignmentDummy file:QtCore/qarraydata.h line:218
			auto & enclosing_class = cl;
			pybind11::class_<QTypedArrayData<char>::AlignmentDummy, std::shared_ptr<QTypedArrayData<char>::AlignmentDummy>> cl(enclosing_class, "AlignmentDummy", "");
			cl.def( pybind11::init( [](){ return new QTypedArrayData<char>::AlignmentDummy(); } ) );
		}

	}
	std::cout << "B301_[QTypedArrayData<unsigned short>] ";
	std::cout << "B302_[QTypedArrayData<unsigned int>] ";
	std::cout << "B303_[QTypedArrayData<QStringRef>] ";
	std::cout << "B304_[QTypedArrayData<int>] ";
	std::cout << "B305_[QTypedArrayData<QXmlStreamAttribute>] ";
	std::cout << "B306_[QArrayDataPointerRef<int>] ";
	{ // QArrayDataPointerRef file:QtCore/qarraydata.h line:287
		pybind11::class_<QArrayDataPointerRef<int>, std::shared_ptr<QArrayDataPointerRef<int>>> cl(M(""), "QArrayDataPointerRef_int_t", "");
		cl.def( pybind11::init( [](){ return new QArrayDataPointerRef<int>(); } ) );
		cl.def( pybind11::init( [](QArrayDataPointerRef<int> const &o){ return new QArrayDataPointerRef<int>(o); } ) );
	}
}
