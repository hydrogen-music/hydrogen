#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qflags.h> // QFlags
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
#include <core/Basics/AutomationPath.h> // H2Core::AutomationPath
#include <core/Basics/DrumkitComponent.h> // H2Core::DrumkitComponent
#include <core/Basics/Instrument.h> // H2Core::Instrument
#include <core/Basics/InstrumentList.h> // H2Core::InstrumentList
#include <core/Basics/PatternList.h> // H2Core::PatternList
#include <core/Basics/Song.h> // 
#include <core/Basics/Song.h> // H2Core::Song
#include <core/Lilipond/Lilypond.h> // H2Core::LilyPond
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <memory> // std::shared_ptr
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

void bind_core_Lilipond_Lilypond(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B718_[H2Core::LilyPond] ";
	{ // H2Core::LilyPond file:core/Lilipond/Lilypond.h line:39
		pybind11::class_<H2Core::LilyPond, std::shared_ptr<H2Core::LilyPond>> cl(M("H2Core"), "LilyPond", "A class to convert a Hydrogen song to LilyPond format");
		cl.def( pybind11::init( [](){ return new H2Core::LilyPond(); } ) );
		cl.def( pybind11::init( [](H2Core::LilyPond const &o){ return new H2Core::LilyPond(o); } ) );
		cl.def("extractData", (void (H2Core::LilyPond::*)(const class H2Core::Song &)) &H2Core::LilyPond::extractData, "C++: H2Core::LilyPond::extractData(const class H2Core::Song &) --> void", pybind11::arg("song"));
		cl.def("write", (void (H2Core::LilyPond::*)(const class QString &) const) &H2Core::LilyPond::write, "C++: H2Core::LilyPond::write(const class QString &) const --> void", pybind11::arg("sFilename"));
		cl.def("assign", (class H2Core::LilyPond & (H2Core::LilyPond::*)(const class H2Core::LilyPond &)) &H2Core::LilyPond::operator=, "C++: H2Core::LilyPond::operator=(const class H2Core::LilyPond &) --> class H2Core::LilyPond &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
