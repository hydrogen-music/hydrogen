#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QCharRef
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringliteral.h> // QStringDataPtr
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qvector.h> // QVector
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

void bind_unknown_unknown_11(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B605_[QTextCodec] ";
	{ // QTextCodec file: line:56
		pybind11::class_<QTextCodec, QTextCodec*> cl(M(""), "QTextCodec", "");

		pybind11::enum_<QTextCodec::ConversionFlag>(cl, "ConversionFlag", pybind11::arithmetic(), "")
			.value("DefaultConversion", QTextCodec::DefaultConversion)
			.value("ConvertInvalidToNull", QTextCodec::ConvertInvalidToNull)
			.value("IgnoreHeader", QTextCodec::IgnoreHeader)
			.value("FreeFunction", QTextCodec::FreeFunction)
			.export_values();

		cl.def_static("codecForName", (class QTextCodec * (*)(const char *)) &QTextCodec::codecForName, "C++: QTextCodec::codecForName(const char *) --> class QTextCodec *", pybind11::return_value_policy::automatic, pybind11::arg("name"));
		cl.def_static("codecForMib", (class QTextCodec * (*)(int)) &QTextCodec::codecForMib, "C++: QTextCodec::codecForMib(int) --> class QTextCodec *", pybind11::return_value_policy::automatic, pybind11::arg("mib"));
		cl.def_static("codecForLocale", (class QTextCodec * (*)()) &QTextCodec::codecForLocale, "C++: QTextCodec::codecForLocale() --> class QTextCodec *", pybind11::return_value_policy::automatic);
		cl.def_static("setCodecForLocale", (void (*)(class QTextCodec *)) &QTextCodec::setCodecForLocale, "C++: QTextCodec::setCodecForLocale(class QTextCodec *) --> void", pybind11::arg("c"));
		cl.def("canEncode", (bool (QTextCodec::*)(class QChar) const) &QTextCodec::canEncode, "C++: QTextCodec::canEncode(class QChar) const --> bool", pybind11::arg(""));
		cl.def("canEncode", (bool (QTextCodec::*)(const class QString &) const) &QTextCodec::canEncode, "C++: QTextCodec::canEncode(const class QString &) const --> bool", pybind11::arg(""));
		cl.def("canEncode", (bool (QTextCodec::*)(class QStringView) const) &QTextCodec::canEncode, "C++: QTextCodec::canEncode(class QStringView) const --> bool", pybind11::arg(""));
		cl.def("toUnicode", (class QString (QTextCodec::*)(const char *) const) &QTextCodec::toUnicode, "C++: QTextCodec::toUnicode(const char *) const --> class QString", pybind11::arg("chars"));
		cl.def("toUnicode", [](QTextCodec const &o, const char * a0, int const & a1) -> QString { return o.toUnicode(a0, a1); }, "", pybind11::arg("in"), pybind11::arg("length"));
		cl.def("toUnicode", (class QString (QTextCodec::*)(const char *, int, struct QTextCodec::ConverterState *) const) &QTextCodec::toUnicode, "C++: QTextCodec::toUnicode(const char *, int, struct QTextCodec::ConverterState *) const --> class QString", pybind11::arg("in"), pybind11::arg("length"), pybind11::arg("state"));
		cl.def("mibEnum", (int (QTextCodec::*)() const) &QTextCodec::mibEnum, "C++: QTextCodec::mibEnum() const --> int");

		{ // QTextCodec::ConverterState file: line:100
			auto & enclosing_class = cl;
			pybind11::class_<QTextCodec::ConverterState, std::shared_ptr<QTextCodec::ConverterState>> cl(enclosing_class, "ConverterState", "");
			cl.def_readwrite("flags", &QTextCodec::ConverterState::flags);
			cl.def_readwrite("remainingChars", &QTextCodec::ConverterState::remainingChars);
			cl.def_readwrite("invalidChars", &QTextCodec::ConverterState::invalidChars);
		}

	}
	std::cout << "B606_[QTextEncoder] ";
	std::cout << "B607_[QTextDecoder] ";
}
