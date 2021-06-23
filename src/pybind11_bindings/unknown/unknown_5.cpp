#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qstring.h> // QString
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
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

void bind_unknown_unknown_5(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B450_[QDataStream] ";
	{ // QDataStream file: line:68
		pybind11::class_<QDataStream, std::shared_ptr<QDataStream>> cl(M(""), "QDataStream", "");
		cl.def( pybind11::init( [](){ return new QDataStream(); } ) );
		cl.def( pybind11::init<class QIODevice *>(), pybind11::arg("") );


		pybind11::enum_<QDataStream::Version>(cl, "Version", pybind11::arithmetic(), "")
			.value("Qt_1_0", QDataStream::Qt_1_0)
			.value("Qt_2_0", QDataStream::Qt_2_0)
			.value("Qt_2_1", QDataStream::Qt_2_1)
			.value("Qt_3_0", QDataStream::Qt_3_0)
			.value("Qt_3_1", QDataStream::Qt_3_1)
			.value("Qt_3_3", QDataStream::Qt_3_3)
			.value("Qt_4_0", QDataStream::Qt_4_0)
			.value("Qt_4_1", QDataStream::Qt_4_1)
			.value("Qt_4_2", QDataStream::Qt_4_2)
			.value("Qt_4_3", QDataStream::Qt_4_3)
			.value("Qt_4_4", QDataStream::Qt_4_4)
			.value("Qt_4_5", QDataStream::Qt_4_5)
			.value("Qt_4_6", QDataStream::Qt_4_6)
			.value("Qt_4_7", QDataStream::Qt_4_7)
			.value("Qt_4_8", QDataStream::Qt_4_8)
			.value("Qt_4_9", QDataStream::Qt_4_9)
			.value("Qt_5_0", QDataStream::Qt_5_0)
			.value("Qt_5_1", QDataStream::Qt_5_1)
			.value("Qt_5_2", QDataStream::Qt_5_2)
			.value("Qt_5_3", QDataStream::Qt_5_3)
			.value("Qt_5_4", QDataStream::Qt_5_4)
			.value("Qt_5_5", QDataStream::Qt_5_5)
			.value("Qt_5_6", QDataStream::Qt_5_6)
			.value("Qt_5_7", QDataStream::Qt_5_7)
			.value("Qt_5_8", QDataStream::Qt_5_8)
			.value("Qt_5_9", QDataStream::Qt_5_9)
			.value("Qt_5_10", QDataStream::Qt_5_10)
			.value("Qt_5_11", QDataStream::Qt_5_11)
			.value("Qt_5_12", QDataStream::Qt_5_12)
			.value("Qt_5_13", QDataStream::Qt_5_13)
			.value("Qt_5_14", QDataStream::Qt_5_14)
			.value("Qt_5_15", QDataStream::Qt_5_15)
			.value("Qt_DefaultCompiledVersion", QDataStream::Qt_DefaultCompiledVersion)
			.export_values();


		pybind11::enum_<QDataStream::ByteOrder>(cl, "ByteOrder", pybind11::arithmetic(), "")
			.value("BigEndian", QDataStream::BigEndian)
			.value("LittleEndian", QDataStream::LittleEndian)
			.export_values();


		pybind11::enum_<QDataStream::Status>(cl, "Status", pybind11::arithmetic(), "")
			.value("Ok", QDataStream::Ok)
			.value("ReadPastEnd", QDataStream::ReadPastEnd)
			.value("ReadCorruptData", QDataStream::ReadCorruptData)
			.value("WriteFailed", QDataStream::WriteFailed)
			.export_values();


		pybind11::enum_<QDataStream::FloatingPointPrecision>(cl, "FloatingPointPrecision", pybind11::arithmetic(), "")
			.value("SinglePrecision", QDataStream::SinglePrecision)
			.value("DoublePrecision", QDataStream::DoublePrecision)
			.export_values();

		cl.def("device", (class QIODevice * (QDataStream::*)() const) &QDataStream::device, "C++: QDataStream::device() const --> class QIODevice *", pybind11::return_value_policy::automatic);
		cl.def("setDevice", (void (QDataStream::*)(class QIODevice *)) &QDataStream::setDevice, "C++: QDataStream::setDevice(class QIODevice *) --> void", pybind11::arg(""));
		cl.def("unsetDevice", (void (QDataStream::*)()) &QDataStream::unsetDevice, "C++: QDataStream::unsetDevice() --> void");
		cl.def("atEnd", (bool (QDataStream::*)() const) &QDataStream::atEnd, "C++: QDataStream::atEnd() const --> bool");
		cl.def("status", (enum QDataStream::Status (QDataStream::*)() const) &QDataStream::status, "C++: QDataStream::status() const --> enum QDataStream::Status");
		cl.def("setStatus", (void (QDataStream::*)(enum QDataStream::Status)) &QDataStream::setStatus, "C++: QDataStream::setStatus(enum QDataStream::Status) --> void", pybind11::arg("status"));
		cl.def("resetStatus", (void (QDataStream::*)()) &QDataStream::resetStatus, "C++: QDataStream::resetStatus() --> void");
		cl.def("floatingPointPrecision", (enum QDataStream::FloatingPointPrecision (QDataStream::*)() const) &QDataStream::floatingPointPrecision, "C++: QDataStream::floatingPointPrecision() const --> enum QDataStream::FloatingPointPrecision");
		cl.def("setFloatingPointPrecision", (void (QDataStream::*)(enum QDataStream::FloatingPointPrecision)) &QDataStream::setFloatingPointPrecision, "C++: QDataStream::setFloatingPointPrecision(enum QDataStream::FloatingPointPrecision) --> void", pybind11::arg("precision"));
		cl.def("byteOrder", (enum QDataStream::ByteOrder (QDataStream::*)() const) &QDataStream::byteOrder, "C++: QDataStream::byteOrder() const --> enum QDataStream::ByteOrder");
		cl.def("setByteOrder", (void (QDataStream::*)(enum QDataStream::ByteOrder)) &QDataStream::setByteOrder, "C++: QDataStream::setByteOrder(enum QDataStream::ByteOrder) --> void", pybind11::arg(""));
		cl.def("version", (int (QDataStream::*)() const) &QDataStream::version, "C++: QDataStream::version() const --> int");
		cl.def("setVersion", (void (QDataStream::*)(int)) &QDataStream::setVersion, "C++: QDataStream::setVersion(int) --> void", pybind11::arg(""));
		cl.def("readRawData", (int (QDataStream::*)(char *, int)) &QDataStream::readRawData, "C++: QDataStream::readRawData(char *, int) --> int", pybind11::arg(""), pybind11::arg("len"));
		cl.def("writeBytes", (class QDataStream & (QDataStream::*)(const char *, unsigned int)) &QDataStream::writeBytes, "C++: QDataStream::writeBytes(const char *, unsigned int) --> class QDataStream &", pybind11::return_value_policy::automatic, pybind11::arg(""), pybind11::arg("len"));
		cl.def("writeRawData", (int (QDataStream::*)(const char *, int)) &QDataStream::writeRawData, "C++: QDataStream::writeRawData(const char *, int) --> int", pybind11::arg(""), pybind11::arg("len"));
		cl.def("skipRawData", (int (QDataStream::*)(int)) &QDataStream::skipRawData, "C++: QDataStream::skipRawData(int) --> int", pybind11::arg("len"));
		cl.def("startTransaction", (void (QDataStream::*)()) &QDataStream::startTransaction, "C++: QDataStream::startTransaction() --> void");
		cl.def("commitTransaction", (bool (QDataStream::*)()) &QDataStream::commitTransaction, "C++: QDataStream::commitTransaction() --> bool");
		cl.def("rollbackTransaction", (void (QDataStream::*)()) &QDataStream::rollbackTransaction, "C++: QDataStream::rollbackTransaction() --> void");
		cl.def("abortTransaction", (void (QDataStream::*)()) &QDataStream::abortTransaction, "C++: QDataStream::abortTransaction() --> void");
	}
}
