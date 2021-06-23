#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qpair.h> // QPair
#include <QtCore/qpair.h> // qMakePair
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

void bind_QtCore_qpair(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B308_[QPair<long long,unsigned int>] ";
	{ // QPair file:QtCore/qpair.h line:49
		pybind11::class_<QPair<long long,unsigned int>, std::shared_ptr<QPair<long long,unsigned int>>> cl(M(""), "QPair_long_long_unsigned_int_t", "");
		cl.def( pybind11::init( [](){ return new QPair<long long,unsigned int>(); } ) );
		cl.def( pybind11::init<const long long &, const unsigned int &>(), pybind11::arg("t1"), pybind11::arg("t2") );

		cl.def( pybind11::init( [](QPair<long long,unsigned int> const &o){ return new QPair<long long,unsigned int>(o); } ) );
		cl.def_readwrite("first", &QPair<long long,unsigned int>::first);
		cl.def_readwrite("second", &QPair<long long,unsigned int>::second);
		cl.def("swap", (void (QPair<long long,unsigned int>::*)(struct QPair<long long, unsigned int> &)) &QPair<long long, unsigned int>::swap, "C++: QPair<long long, unsigned int>::swap(struct QPair<long long, unsigned int> &) --> void", pybind11::arg("other"));
	}
	std::cout << "B309_[QPair<QString,QString>] ";
	std::cout << "B310_[struct QPair<long long, unsigned int> qMakePair<long long,unsigned int>(const long long &, const unsigned int &)] ";
}
