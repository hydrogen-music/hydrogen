#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qcoreevent.h> // 
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qfile.h> // QFile
#include <QtCore/qfiledevice.h> // 
#include <QtCore/qfiledevice.h> // QFileDevice
#include <QtCore/qfileinfo.h> // QFileInfo
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qfloat16.h> // qfloat16
#include <QtCore/qiodevice.h> // 
#include <QtCore/qlist.h> // QList
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimeSpec
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qpair.h> // QPair
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
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
#include <QtCore/qvariant.h> // 
#include <QtCore/qvariant.h> // QVariant
#include <QtCore/qvector.h> // QVector
#include <chrono> // std::chrono::duration
#include <cstdio> // _IO_FILE
#include <iostream> // --trace
#include <iterator> // std::reverse_iterator
#include <memory> // std::allocator
#include <ratio> // std::ratio
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

void bind_unknown_unknown_6(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B451_[QDeadlineTimer] ";
	{ // QDeadlineTimer file: line:61
		pybind11::class_<QDeadlineTimer, std::shared_ptr<QDeadlineTimer>> cl(M(""), "QDeadlineTimer", "");
		cl.def( pybind11::init( [](){ return new QDeadlineTimer(); } ), "doc" );
		cl.def( pybind11::init<enum Qt::TimerType>(), pybind11::arg("type_") );

		cl.def( pybind11::init( [](enum QDeadlineTimer::ForeverConstant const & a0){ return new QDeadlineTimer(a0); } ), "doc" , pybind11::arg(""));
		cl.def( pybind11::init<enum QDeadlineTimer::ForeverConstant, enum Qt::TimerType>(), pybind11::arg(""), pybind11::arg("type_") );

		cl.def( pybind11::init( [](long long const & a0){ return new QDeadlineTimer(a0); } ), "doc" , pybind11::arg("msecs"));
		cl.def( pybind11::init<long long, enum Qt::TimerType>(), pybind11::arg("msecs"), pybind11::arg("type") );

		cl.def( pybind11::init( [](QDeadlineTimer const &o){ return new QDeadlineTimer(o); } ) );

		pybind11::enum_<QDeadlineTimer::ForeverConstant>(cl, "ForeverConstant", pybind11::arithmetic(), "")
			.value("Forever", QDeadlineTimer::Forever)
			.export_values();

		cl.def("swap", (void (QDeadlineTimer::*)(class QDeadlineTimer &)) &QDeadlineTimer::swap, "C++: QDeadlineTimer::swap(class QDeadlineTimer &) --> void", pybind11::arg("other"));
		cl.def("isForever", (bool (QDeadlineTimer::*)() const) &QDeadlineTimer::isForever, "C++: QDeadlineTimer::isForever() const --> bool");
		cl.def("hasExpired", (bool (QDeadlineTimer::*)() const) &QDeadlineTimer::hasExpired, "C++: QDeadlineTimer::hasExpired() const --> bool");
		cl.def("timerType", (enum Qt::TimerType (QDeadlineTimer::*)() const) &QDeadlineTimer::timerType, "C++: QDeadlineTimer::timerType() const --> enum Qt::TimerType");
		cl.def("setTimerType", (void (QDeadlineTimer::*)(enum Qt::TimerType)) &QDeadlineTimer::setTimerType, "C++: QDeadlineTimer::setTimerType(enum Qt::TimerType) --> void", pybind11::arg("type"));
		cl.def("remainingTime", (long long (QDeadlineTimer::*)() const) &QDeadlineTimer::remainingTime, "C++: QDeadlineTimer::remainingTime() const --> long long");
		cl.def("remainingTimeNSecs", (long long (QDeadlineTimer::*)() const) &QDeadlineTimer::remainingTimeNSecs, "C++: QDeadlineTimer::remainingTimeNSecs() const --> long long");
		cl.def("setRemainingTime", [](QDeadlineTimer &o, long long const & a0) -> void { return o.setRemainingTime(a0); }, "", pybind11::arg("msecs"));
		cl.def("setRemainingTime", (void (QDeadlineTimer::*)(long long, enum Qt::TimerType)) &QDeadlineTimer::setRemainingTime, "C++: QDeadlineTimer::setRemainingTime(long long, enum Qt::TimerType) --> void", pybind11::arg("msecs"), pybind11::arg("type"));
		cl.def("setPreciseRemainingTime", [](QDeadlineTimer &o, long long const & a0) -> void { return o.setPreciseRemainingTime(a0); }, "", pybind11::arg("secs"));
		cl.def("setPreciseRemainingTime", [](QDeadlineTimer &o, long long const & a0, long long const & a1) -> void { return o.setPreciseRemainingTime(a0, a1); }, "", pybind11::arg("secs"), pybind11::arg("nsecs"));
		cl.def("setPreciseRemainingTime", (void (QDeadlineTimer::*)(long long, long long, enum Qt::TimerType)) &QDeadlineTimer::setPreciseRemainingTime, "C++: QDeadlineTimer::setPreciseRemainingTime(long long, long long, enum Qt::TimerType) --> void", pybind11::arg("secs"), pybind11::arg("nsecs"), pybind11::arg("type"));
		cl.def("deadline", (long long (QDeadlineTimer::*)() const) &QDeadlineTimer::deadline, "C++: QDeadlineTimer::deadline() const --> long long");
		cl.def("deadlineNSecs", (long long (QDeadlineTimer::*)() const) &QDeadlineTimer::deadlineNSecs, "C++: QDeadlineTimer::deadlineNSecs() const --> long long");
		cl.def("setDeadline", [](QDeadlineTimer &o, long long const & a0) -> void { return o.setDeadline(a0); }, "", pybind11::arg("msecs"));
		cl.def("setDeadline", (void (QDeadlineTimer::*)(long long, enum Qt::TimerType)) &QDeadlineTimer::setDeadline, "C++: QDeadlineTimer::setDeadline(long long, enum Qt::TimerType) --> void", pybind11::arg("msecs"), pybind11::arg("timerType"));
		cl.def("setPreciseDeadline", [](QDeadlineTimer &o, long long const & a0) -> void { return o.setPreciseDeadline(a0); }, "", pybind11::arg("secs"));
		cl.def("setPreciseDeadline", [](QDeadlineTimer &o, long long const & a0, long long const & a1) -> void { return o.setPreciseDeadline(a0, a1); }, "", pybind11::arg("secs"), pybind11::arg("nsecs"));
		cl.def("setPreciseDeadline", (void (QDeadlineTimer::*)(long long, long long, enum Qt::TimerType)) &QDeadlineTimer::setPreciseDeadline, "C++: QDeadlineTimer::setPreciseDeadline(long long, long long, enum Qt::TimerType) --> void", pybind11::arg("secs"), pybind11::arg("nsecs"), pybind11::arg("type"));
		cl.def_static("addNSecs", (class QDeadlineTimer (*)(class QDeadlineTimer, long long)) &QDeadlineTimer::addNSecs, "C++: QDeadlineTimer::addNSecs(class QDeadlineTimer, long long) --> class QDeadlineTimer", pybind11::arg("dt"), pybind11::arg("nsecs"));
		cl.def_static("current", []() -> QDeadlineTimer { return QDeadlineTimer::current(); }, "");
		cl.def_static("current", (class QDeadlineTimer (*)(enum Qt::TimerType)) &QDeadlineTimer::current, "C++: QDeadlineTimer::current(enum Qt::TimerType) --> class QDeadlineTimer", pybind11::arg("timerType"));
		cl.def("__iadd__", (class QDeadlineTimer & (QDeadlineTimer::*)(long long)) &QDeadlineTimer::operator+=, "C++: QDeadlineTimer::operator+=(long long) --> class QDeadlineTimer &", pybind11::return_value_policy::automatic, pybind11::arg("msecs"));
		cl.def("__isub__", (class QDeadlineTimer & (QDeadlineTimer::*)(long long)) &QDeadlineTimer::operator-=, "C++: QDeadlineTimer::operator-=(long long) --> class QDeadlineTimer &", pybind11::return_value_policy::automatic, pybind11::arg("msecs"));
		cl.def("_q_data", (struct QPair<long long, unsigned int> (QDeadlineTimer::*)() const) &QDeadlineTimer::_q_data, "C++: QDeadlineTimer::_q_data() const --> struct QPair<long long, unsigned int>");
		cl.def("assign", (class QDeadlineTimer & (QDeadlineTimer::*)(const class QDeadlineTimer &)) &QDeadlineTimer::operator=, "C++: QDeadlineTimer::operator=(const class QDeadlineTimer &) --> class QDeadlineTimer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B452_[QTypeInfo<QDeadlineTimer>] ";
	std::cout << "B453_[void swap(class QDeadlineTimer &, class QDeadlineTimer &)] ";
	std::cout << "B454_[QMetaTypeId<QDeadlineTimer>] ";
	std::cout << "B455_[QFileDevice] ";
	std::cout << "B456_[QFile] ";
	std::cout << "B457_[QFileInfo] ";
	std::cout << "B458_[QTypeInfo<QFileInfo>] ";
	std::cout << "B459_[void swap(class QFileInfo &, class QFileInfo &)] ";
	std::cout << "B460_[QMetaTypeId<QFileInfo>] ";
	std::cout << "B461_[QDir] ";
	std::cout << "B462_[QTypeInfo<QDir>] ";
	std::cout << "B463_[void swap(class QDir &, class QDir &)] ";
	std::cout << "B464_[QDirIterator] ";
	std::cout << "B465_[QEasingCurve] ";
	{ // QEasingCurve file: line:59
		pybind11::class_<QEasingCurve, std::shared_ptr<QEasingCurve>> cl(M(""), "QEasingCurve", "");
		cl.def( pybind11::init( [](){ return new QEasingCurve(); } ), "doc" );
		cl.def( pybind11::init<enum QEasingCurve::Type>(), pybind11::arg("type") );

		cl.def( pybind11::init( [](QEasingCurve const &o){ return new QEasingCurve(o); } ) );

		pybind11::enum_<QEasingCurve::Type>(cl, "Type", pybind11::arithmetic(), "")
			.value("Linear", QEasingCurve::Linear)
			.value("InQuad", QEasingCurve::InQuad)
			.value("OutQuad", QEasingCurve::OutQuad)
			.value("InOutQuad", QEasingCurve::InOutQuad)
			.value("OutInQuad", QEasingCurve::OutInQuad)
			.value("InCubic", QEasingCurve::InCubic)
			.value("OutCubic", QEasingCurve::OutCubic)
			.value("InOutCubic", QEasingCurve::InOutCubic)
			.value("OutInCubic", QEasingCurve::OutInCubic)
			.value("InQuart", QEasingCurve::InQuart)
			.value("OutQuart", QEasingCurve::OutQuart)
			.value("InOutQuart", QEasingCurve::InOutQuart)
			.value("OutInQuart", QEasingCurve::OutInQuart)
			.value("InQuint", QEasingCurve::InQuint)
			.value("OutQuint", QEasingCurve::OutQuint)
			.value("InOutQuint", QEasingCurve::InOutQuint)
			.value("OutInQuint", QEasingCurve::OutInQuint)
			.value("InSine", QEasingCurve::InSine)
			.value("OutSine", QEasingCurve::OutSine)
			.value("InOutSine", QEasingCurve::InOutSine)
			.value("OutInSine", QEasingCurve::OutInSine)
			.value("InExpo", QEasingCurve::InExpo)
			.value("OutExpo", QEasingCurve::OutExpo)
			.value("InOutExpo", QEasingCurve::InOutExpo)
			.value("OutInExpo", QEasingCurve::OutInExpo)
			.value("InCirc", QEasingCurve::InCirc)
			.value("OutCirc", QEasingCurve::OutCirc)
			.value("InOutCirc", QEasingCurve::InOutCirc)
			.value("OutInCirc", QEasingCurve::OutInCirc)
			.value("InElastic", QEasingCurve::InElastic)
			.value("OutElastic", QEasingCurve::OutElastic)
			.value("InOutElastic", QEasingCurve::InOutElastic)
			.value("OutInElastic", QEasingCurve::OutInElastic)
			.value("InBack", QEasingCurve::InBack)
			.value("OutBack", QEasingCurve::OutBack)
			.value("InOutBack", QEasingCurve::InOutBack)
			.value("OutInBack", QEasingCurve::OutInBack)
			.value("InBounce", QEasingCurve::InBounce)
			.value("OutBounce", QEasingCurve::OutBounce)
			.value("InOutBounce", QEasingCurve::InOutBounce)
			.value("OutInBounce", QEasingCurve::OutInBounce)
			.value("InCurve", QEasingCurve::InCurve)
			.value("OutCurve", QEasingCurve::OutCurve)
			.value("SineCurve", QEasingCurve::SineCurve)
			.value("CosineCurve", QEasingCurve::CosineCurve)
			.value("BezierSpline", QEasingCurve::BezierSpline)
			.value("TCBSpline", QEasingCurve::TCBSpline)
			.value("Custom", QEasingCurve::Custom)
			.value("NCurveTypes", QEasingCurve::NCurveTypes)
			.export_values();

		cl.def("assign", (class QEasingCurve & (QEasingCurve::*)(const class QEasingCurve &)) &QEasingCurve::operator=, "C++: QEasingCurve::operator=(const class QEasingCurve &) --> class QEasingCurve &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QEasingCurve::*)(class QEasingCurve &)) &QEasingCurve::swap, "C++: QEasingCurve::swap(class QEasingCurve &) --> void", pybind11::arg("other"));
		cl.def("__eq__", (bool (QEasingCurve::*)(const class QEasingCurve &) const) &QEasingCurve::operator==, "C++: QEasingCurve::operator==(const class QEasingCurve &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QEasingCurve::*)(const class QEasingCurve &) const) &QEasingCurve::operator!=, "C++: QEasingCurve::operator!=(const class QEasingCurve &) const --> bool", pybind11::arg("other"));
		cl.def("amplitude", (double (QEasingCurve::*)() const) &QEasingCurve::amplitude, "C++: QEasingCurve::amplitude() const --> double");
		cl.def("setAmplitude", (void (QEasingCurve::*)(double)) &QEasingCurve::setAmplitude, "C++: QEasingCurve::setAmplitude(double) --> void", pybind11::arg("amplitude"));
		cl.def("period", (double (QEasingCurve::*)() const) &QEasingCurve::period, "C++: QEasingCurve::period() const --> double");
		cl.def("setPeriod", (void (QEasingCurve::*)(double)) &QEasingCurve::setPeriod, "C++: QEasingCurve::setPeriod(double) --> void", pybind11::arg("period"));
		cl.def("overshoot", (double (QEasingCurve::*)() const) &QEasingCurve::overshoot, "C++: QEasingCurve::overshoot() const --> double");
		cl.def("setOvershoot", (void (QEasingCurve::*)(double)) &QEasingCurve::setOvershoot, "C++: QEasingCurve::setOvershoot(double) --> void", pybind11::arg("overshoot"));
		cl.def("addCubicBezierSegment", (void (QEasingCurve::*)(const class QPointF &, const class QPointF &, const class QPointF &)) &QEasingCurve::addCubicBezierSegment, "C++: QEasingCurve::addCubicBezierSegment(const class QPointF &, const class QPointF &, const class QPointF &) --> void", pybind11::arg("c1"), pybind11::arg("c2"), pybind11::arg("endPoint"));
		cl.def("addTCBSegment", (void (QEasingCurve::*)(const class QPointF &, double, double, double)) &QEasingCurve::addTCBSegment, "C++: QEasingCurve::addTCBSegment(const class QPointF &, double, double, double) --> void", pybind11::arg("nextPoint"), pybind11::arg("t"), pybind11::arg("c"), pybind11::arg("b"));
		cl.def("type", (enum QEasingCurve::Type (QEasingCurve::*)() const) &QEasingCurve::type, "C++: QEasingCurve::type() const --> enum QEasingCurve::Type");
		cl.def("setType", (void (QEasingCurve::*)(enum QEasingCurve::Type)) &QEasingCurve::setType, "C++: QEasingCurve::setType(enum QEasingCurve::Type) --> void", pybind11::arg("type"));
		cl.def("valueForProgress", (double (QEasingCurve::*)(double) const) &QEasingCurve::valueForProgress, "C++: QEasingCurve::valueForProgress(double) const --> double", pybind11::arg("progress"));
	}
	std::cout << "B466_[const struct QMetaObject * qt_getEnumMetaObject(enum QEasingCurve::Type)] ";
	std::cout << "B467_[const char * qt_getEnumName(enum QEasingCurve::Type)] ";
	std::cout << "B468_[QTypeInfo<QEasingCurve>] ";
	std::cout << "B469_[void swap(class QEasingCurve &, class QEasingCurve &)] ";
	std::cout << "B470_[unsigned short qFromUnaligned<unsigned short>(const void *)] ";
	std::cout << "B471_[class qfloat16 qFromUnaligned<qfloat16>(const void *)] ";
	std::cout << "B472_[unsigned int qFromUnaligned<unsigned int>(const void *)] ";
	std::cout << "B473_[float qFromUnaligned<float>(const void *)] ";
	std::cout << "B474_[unsigned long long qFromUnaligned<unsigned long long>(const void *)] ";
	std::cout << "B475_[double qFromUnaligned<double>(const void *)] ";
	std::cout << "B476_[unsigned long long qbswap<unsigned long long>(unsigned long long)] ";
	std::cout << "B477_[unsigned int qbswap<unsigned int>(unsigned int)] ";
	std::cout << "B478_[unsigned short qbswap<unsigned short>(unsigned short)] ";
	std::cout << "B479_[unsigned char qbswap<unsigned char>(unsigned char)] ";
	std::cout << "B480_[long long qbswap<long long>(long long)] ";
	std::cout << "B481_[int qbswap<int>(int)] ";
	std::cout << "B482_[short qbswap<short>(short)] ";
	std::cout << "B483_[signed char qbswap<signed char>(signed char)] ";
	std::cout << "B484_[class qfloat16 qbswapFloatHelper<qfloat16>(class qfloat16)] ";
	std::cout << "B485_[float qbswapFloatHelper<float>(float)] ";
	std::cout << "B486_[double qbswapFloatHelper<double>(double)] ";
	std::cout << "B487_[class qfloat16 qbswap(class qfloat16)] ";
	std::cout << "B488_[float qbswap(float)] ";
	std::cout << "B489_[double qbswap(double)] ";
	std::cout << "B490_[void * qbswap<1>(const void *, long long, void *)] ";
	std::cout << "B491_[void * qbswap<2>(const void *, long long, void *)] ";
	std::cout << "B492_[void * qbswap<4>(const void *, long long, void *)] ";
	std::cout << "B493_[void * qbswap<8>(const void *, long long, void *)] ";
	std::cout << "B494_[unsigned char qFromLittleEndian<unsigned char>(const void *)] ";
	std::cout << "B495_[signed char qFromLittleEndian<signed char>(const void *)] ";
	std::cout << "B496_[unsigned char qFromBigEndian<unsigned char>(const void *)] ";
	std::cout << "B497_[signed char qFromBigEndian<signed char>(const void *)] ";
	std::cout << "B498_[QEventTransition] ";
}
