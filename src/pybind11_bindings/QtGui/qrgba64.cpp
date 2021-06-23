#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qlist.h> // QList
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::GlobalColor
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
#include <QtGui/qrgba64.h> // QRgba64
#include <QtGui/qrgba64.h> // qAlpha
#include <QtGui/qrgba64.h> // qBlue
#include <QtGui/qrgba64.h> // qGreen
#include <QtGui/qrgba64.h> // qPremultiply
#include <QtGui/qrgba64.h> // qRed
#include <QtGui/qrgba64.h> // qRgba64
#include <QtGui/qrgba64.h> // qUnpremultiply
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

void bind_QtGui_qrgba64(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B719_[QRgba64] ";
	{ // QRgba64 file:QtGui/qrgba64.h line:48
		pybind11::class_<QRgba64, std::shared_ptr<QRgba64>> cl(M(""), "QRgba64", "");
		cl.def( pybind11::init( [](){ return new QRgba64(); } ) );
		cl.def( pybind11::init( [](QRgba64 const &o){ return new QRgba64(o); } ) );
		cl.def_static("fromRgba64", (class QRgba64 (*)(unsigned long long)) &QRgba64::fromRgba64, "C++: QRgba64::fromRgba64(unsigned long long) --> class QRgba64", pybind11::arg("c"));
		cl.def_static("fromRgba64", (class QRgba64 (*)(unsigned short, unsigned short, unsigned short, unsigned short)) &QRgba64::fromRgba64, "C++: QRgba64::fromRgba64(unsigned short, unsigned short, unsigned short, unsigned short) --> class QRgba64", pybind11::arg("red"), pybind11::arg("green"), pybind11::arg("blue"), pybind11::arg("alpha"));
		cl.def_static("fromRgba", (class QRgba64 (*)(unsigned char, unsigned char, unsigned char, unsigned char)) &QRgba64::fromRgba, "C++: QRgba64::fromRgba(unsigned char, unsigned char, unsigned char, unsigned char) --> class QRgba64", pybind11::arg("red"), pybind11::arg("green"), pybind11::arg("blue"), pybind11::arg("alpha"));
		cl.def_static("fromArgb32", (class QRgba64 (*)(unsigned int)) &QRgba64::fromArgb32, "C++: QRgba64::fromArgb32(unsigned int) --> class QRgba64", pybind11::arg("rgb"));
		cl.def("isOpaque", (bool (QRgba64::*)() const) &QRgba64::isOpaque, "C++: QRgba64::isOpaque() const --> bool");
		cl.def("isTransparent", (bool (QRgba64::*)() const) &QRgba64::isTransparent, "C++: QRgba64::isTransparent() const --> bool");
		cl.def("red", (unsigned short (QRgba64::*)() const) &QRgba64::red, "C++: QRgba64::red() const --> unsigned short");
		cl.def("green", (unsigned short (QRgba64::*)() const) &QRgba64::green, "C++: QRgba64::green() const --> unsigned short");
		cl.def("blue", (unsigned short (QRgba64::*)() const) &QRgba64::blue, "C++: QRgba64::blue() const --> unsigned short");
		cl.def("alpha", (unsigned short (QRgba64::*)() const) &QRgba64::alpha, "C++: QRgba64::alpha() const --> unsigned short");
		cl.def("setRed", (void (QRgba64::*)(unsigned short)) &QRgba64::setRed, "C++: QRgba64::setRed(unsigned short) --> void", pybind11::arg("_red"));
		cl.def("setGreen", (void (QRgba64::*)(unsigned short)) &QRgba64::setGreen, "C++: QRgba64::setGreen(unsigned short) --> void", pybind11::arg("_green"));
		cl.def("setBlue", (void (QRgba64::*)(unsigned short)) &QRgba64::setBlue, "C++: QRgba64::setBlue(unsigned short) --> void", pybind11::arg("_blue"));
		cl.def("setAlpha", (void (QRgba64::*)(unsigned short)) &QRgba64::setAlpha, "C++: QRgba64::setAlpha(unsigned short) --> void", pybind11::arg("_alpha"));
		cl.def("red8", (unsigned char (QRgba64::*)() const) &QRgba64::red8, "C++: QRgba64::red8() const --> unsigned char");
		cl.def("green8", (unsigned char (QRgba64::*)() const) &QRgba64::green8, "C++: QRgba64::green8() const --> unsigned char");
		cl.def("blue8", (unsigned char (QRgba64::*)() const) &QRgba64::blue8, "C++: QRgba64::blue8() const --> unsigned char");
		cl.def("alpha8", (unsigned char (QRgba64::*)() const) &QRgba64::alpha8, "C++: QRgba64::alpha8() const --> unsigned char");
		cl.def("toArgb32", (unsigned int (QRgba64::*)() const) &QRgba64::toArgb32, "C++: QRgba64::toArgb32() const --> unsigned int");
		cl.def("toRgb16", (unsigned short (QRgba64::*)() const) &QRgba64::toRgb16, "C++: QRgba64::toRgb16() const --> unsigned short");
		cl.def("premultiplied", (class QRgba64 (QRgba64::*)() const) &QRgba64::premultiplied, "C++: QRgba64::premultiplied() const --> class QRgba64");
		cl.def("unpremultiplied", (class QRgba64 (QRgba64::*)() const) &QRgba64::unpremultiplied, "C++: QRgba64::unpremultiplied() const --> class QRgba64");
		cl.def("assign", (class QRgba64 (QRgba64::*)(unsigned long long)) &QRgba64::operator=, "C++: QRgba64::operator=(unsigned long long) --> class QRgba64", pybind11::arg("_rgba"));
	}
	std::cout << "B720_[QTypeInfo<QRgba64>] ";
	std::cout << "B721_[class QRgba64 qRgba64(unsigned short, unsigned short, unsigned short, unsigned short)] ";
	std::cout << "B722_[class QRgba64 qRgba64(unsigned long long)] ";
	std::cout << "B723_[class QRgba64 qPremultiply(class QRgba64)] ";
	std::cout << "B724_[class QRgba64 qUnpremultiply(class QRgba64)] ";
	std::cout << "B725_[unsigned int qRed(class QRgba64)] ";
	std::cout << "B726_[unsigned int qGreen(class QRgba64)] ";
	std::cout << "B727_[unsigned int qBlue(class QRgba64)] ";
	std::cout << "B728_[unsigned int qAlpha(class QRgba64)] ";
	std::cout << "B729_[QColor] ";
	{ // QColor file: line:64
		pybind11::class_<QColor, std::shared_ptr<QColor>> cl(M(""), "QColor", "");
		cl.def( pybind11::init( [](){ return new QColor(); } ) );
		cl.def( pybind11::init<enum Qt::GlobalColor>(), pybind11::arg("color") );

		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2){ return new QColor(a0, a1, a2); } ), "doc" , pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a") );

		cl.def( pybind11::init<unsigned int>(), pybind11::arg("rgb") );

		cl.def( pybind11::init<class QRgba64>(), pybind11::arg("rgba64") );

		cl.def( pybind11::init<const class QString &>(), pybind11::arg("name") );

		cl.def( pybind11::init<class QStringView>(), pybind11::arg("name") );

		cl.def( pybind11::init<const char *>(), pybind11::arg("aname") );

		cl.def( pybind11::init<class QLatin1String>(), pybind11::arg("name") );

		cl.def( pybind11::init<enum QColor::Spec>(), pybind11::arg("spec") );

		cl.def( pybind11::init( [](QColor const &o){ return new QColor(o); } ) );
		cl.def( pybind11::init( [](enum QColor::Spec const & a0, unsigned short const & a1, unsigned short const & a2, unsigned short const & a3, unsigned short const & a4){ return new QColor(a0, a1, a2, a3, a4); } ), "doc" , pybind11::arg("spec"), pybind11::arg("a1"), pybind11::arg("a2"), pybind11::arg("a3"), pybind11::arg("a4"));
		cl.def( pybind11::init<enum QColor::Spec, unsigned short, unsigned short, unsigned short, unsigned short, unsigned short>(), pybind11::arg("spec"), pybind11::arg("a1"), pybind11::arg("a2"), pybind11::arg("a3"), pybind11::arg("a4"), pybind11::arg("a5") );


		pybind11::enum_<QColor::Spec>(cl, "Spec", pybind11::arithmetic(), "")
			.value("Invalid", QColor::Invalid)
			.value("Rgb", QColor::Rgb)
			.value("Hsv", QColor::Hsv)
			.value("Cmyk", QColor::Cmyk)
			.value("Hsl", QColor::Hsl)
			.value("ExtendedRgb", QColor::ExtendedRgb)
			.export_values();


		pybind11::enum_<QColor::NameFormat>(cl, "NameFormat", pybind11::arithmetic(), "")
			.value("HexRgb", QColor::HexRgb)
			.value("HexArgb", QColor::HexArgb)
			.export_values();

		cl.def("assign", (class QColor & (QColor::*)(const class QColor &)) &QColor::operator=, "C++: QColor::operator=(const class QColor &) --> class QColor &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def("assign", (class QColor & (QColor::*)(enum Qt::GlobalColor)) &QColor::operator=, "C++: QColor::operator=(enum Qt::GlobalColor) --> class QColor &", pybind11::return_value_policy::automatic, pybind11::arg("color"));
		cl.def("isValid", (bool (QColor::*)() const) &QColor::isValid, "C++: QColor::isValid() const --> bool");
		cl.def("name", (class QString (QColor::*)() const) &QColor::name, "C++: QColor::name() const --> class QString");
		cl.def("name", (class QString (QColor::*)(enum QColor::NameFormat) const) &QColor::name, "C++: QColor::name(enum QColor::NameFormat) const --> class QString", pybind11::arg("format"));
		cl.def("setNamedColor", (void (QColor::*)(const class QString &)) &QColor::setNamedColor, "C++: QColor::setNamedColor(const class QString &) --> void", pybind11::arg("name"));
		cl.def("setNamedColor", (void (QColor::*)(class QStringView)) &QColor::setNamedColor, "C++: QColor::setNamedColor(class QStringView) --> void", pybind11::arg("name"));
		cl.def("setNamedColor", (void (QColor::*)(class QLatin1String)) &QColor::setNamedColor, "C++: QColor::setNamedColor(class QLatin1String) --> void", pybind11::arg("name"));
		cl.def("spec", (enum QColor::Spec (QColor::*)() const) &QColor::spec, "C++: QColor::spec() const --> enum QColor::Spec");
		cl.def("alpha", (int (QColor::*)() const) &QColor::alpha, "C++: QColor::alpha() const --> int");
		cl.def("setAlpha", (void (QColor::*)(int)) &QColor::setAlpha, "C++: QColor::setAlpha(int) --> void", pybind11::arg("alpha"));
		cl.def("alphaF", (double (QColor::*)() const) &QColor::alphaF, "C++: QColor::alphaF() const --> double");
		cl.def("setAlphaF", (void (QColor::*)(double)) &QColor::setAlphaF, "C++: QColor::setAlphaF(double) --> void", pybind11::arg("alpha"));
		cl.def("red", (int (QColor::*)() const) &QColor::red, "C++: QColor::red() const --> int");
		cl.def("green", (int (QColor::*)() const) &QColor::green, "C++: QColor::green() const --> int");
		cl.def("blue", (int (QColor::*)() const) &QColor::blue, "C++: QColor::blue() const --> int");
		cl.def("setRed", (void (QColor::*)(int)) &QColor::setRed, "C++: QColor::setRed(int) --> void", pybind11::arg("red"));
		cl.def("setGreen", (void (QColor::*)(int)) &QColor::setGreen, "C++: QColor::setGreen(int) --> void", pybind11::arg("green"));
		cl.def("setBlue", (void (QColor::*)(int)) &QColor::setBlue, "C++: QColor::setBlue(int) --> void", pybind11::arg("blue"));
		cl.def("redF", (double (QColor::*)() const) &QColor::redF, "C++: QColor::redF() const --> double");
		cl.def("greenF", (double (QColor::*)() const) &QColor::greenF, "C++: QColor::greenF() const --> double");
		cl.def("blueF", (double (QColor::*)() const) &QColor::blueF, "C++: QColor::blueF() const --> double");
		cl.def("setRedF", (void (QColor::*)(double)) &QColor::setRedF, "C++: QColor::setRedF(double) --> void", pybind11::arg("red"));
		cl.def("setGreenF", (void (QColor::*)(double)) &QColor::setGreenF, "C++: QColor::setGreenF(double) --> void", pybind11::arg("green"));
		cl.def("setBlueF", (void (QColor::*)(double)) &QColor::setBlueF, "C++: QColor::setBlueF(double) --> void", pybind11::arg("blue"));
		cl.def("getRgb", [](QColor const &o, int * a0, int * a1, int * a2) -> void { return o.getRgb(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("getRgb", (void (QColor::*)(int *, int *, int *, int *) const) &QColor::getRgb, "C++: QColor::getRgb(int *, int *, int *, int *) const --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def("setRgb", [](QColor &o, int const & a0, int const & a1, int const & a2) -> void { return o.setRgb(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("setRgb", (void (QColor::*)(int, int, int, int)) &QColor::setRgb, "C++: QColor::setRgb(int, int, int, int) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def("getRgbF", [](QColor const &o, double * a0, double * a1, double * a2) -> void { return o.getRgbF(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("getRgbF", (void (QColor::*)(double *, double *, double *, double *) const) &QColor::getRgbF, "C++: QColor::getRgbF(double *, double *, double *, double *) const --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def("setRgbF", [](QColor &o, double const & a0, double const & a1, double const & a2) -> void { return o.setRgbF(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("setRgbF", (void (QColor::*)(double, double, double, double)) &QColor::setRgbF, "C++: QColor::setRgbF(double, double, double, double) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def("rgba64", (class QRgba64 (QColor::*)() const) &QColor::rgba64, "C++: QColor::rgba64() const --> class QRgba64");
		cl.def("setRgba64", (void (QColor::*)(class QRgba64)) &QColor::setRgba64, "C++: QColor::setRgba64(class QRgba64) --> void", pybind11::arg("rgba"));
		cl.def("rgba", (unsigned int (QColor::*)() const) &QColor::rgba, "C++: QColor::rgba() const --> unsigned int");
		cl.def("setRgba", (void (QColor::*)(unsigned int)) &QColor::setRgba, "C++: QColor::setRgba(unsigned int) --> void", pybind11::arg("rgba"));
		cl.def("rgb", (unsigned int (QColor::*)() const) &QColor::rgb, "C++: QColor::rgb() const --> unsigned int");
		cl.def("setRgb", (void (QColor::*)(unsigned int)) &QColor::setRgb, "C++: QColor::setRgb(unsigned int) --> void", pybind11::arg("rgb"));
		cl.def("hue", (int (QColor::*)() const) &QColor::hue, "C++: QColor::hue() const --> int");
		cl.def("saturation", (int (QColor::*)() const) &QColor::saturation, "C++: QColor::saturation() const --> int");
		cl.def("hsvHue", (int (QColor::*)() const) &QColor::hsvHue, "C++: QColor::hsvHue() const --> int");
		cl.def("hsvSaturation", (int (QColor::*)() const) &QColor::hsvSaturation, "C++: QColor::hsvSaturation() const --> int");
		cl.def("value", (int (QColor::*)() const) &QColor::value, "C++: QColor::value() const --> int");
		cl.def("hueF", (double (QColor::*)() const) &QColor::hueF, "C++: QColor::hueF() const --> double");
		cl.def("saturationF", (double (QColor::*)() const) &QColor::saturationF, "C++: QColor::saturationF() const --> double");
		cl.def("hsvHueF", (double (QColor::*)() const) &QColor::hsvHueF, "C++: QColor::hsvHueF() const --> double");
		cl.def("hsvSaturationF", (double (QColor::*)() const) &QColor::hsvSaturationF, "C++: QColor::hsvSaturationF() const --> double");
		cl.def("valueF", (double (QColor::*)() const) &QColor::valueF, "C++: QColor::valueF() const --> double");
		cl.def("getHsv", [](QColor const &o, int * a0, int * a1, int * a2) -> void { return o.getHsv(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"));
		cl.def("getHsv", (void (QColor::*)(int *, int *, int *, int *) const) &QColor::getHsv, "C++: QColor::getHsv(int *, int *, int *, int *) const --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"), pybind11::arg("a"));
		cl.def("setHsv", [](QColor &o, int const & a0, int const & a1, int const & a2) -> void { return o.setHsv(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"));
		cl.def("setHsv", (void (QColor::*)(int, int, int, int)) &QColor::setHsv, "C++: QColor::setHsv(int, int, int, int) --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"), pybind11::arg("a"));
		cl.def("getHsvF", [](QColor const &o, double * a0, double * a1, double * a2) -> void { return o.getHsvF(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"));
		cl.def("getHsvF", (void (QColor::*)(double *, double *, double *, double *) const) &QColor::getHsvF, "C++: QColor::getHsvF(double *, double *, double *, double *) const --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"), pybind11::arg("a"));
		cl.def("setHsvF", [](QColor &o, double const & a0, double const & a1, double const & a2) -> void { return o.setHsvF(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"));
		cl.def("setHsvF", (void (QColor::*)(double, double, double, double)) &QColor::setHsvF, "C++: QColor::setHsvF(double, double, double, double) --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"), pybind11::arg("a"));
		cl.def("cyan", (int (QColor::*)() const) &QColor::cyan, "C++: QColor::cyan() const --> int");
		cl.def("magenta", (int (QColor::*)() const) &QColor::magenta, "C++: QColor::magenta() const --> int");
		cl.def("yellow", (int (QColor::*)() const) &QColor::yellow, "C++: QColor::yellow() const --> int");
		cl.def("black", (int (QColor::*)() const) &QColor::black, "C++: QColor::black() const --> int");
		cl.def("cyanF", (double (QColor::*)() const) &QColor::cyanF, "C++: QColor::cyanF() const --> double");
		cl.def("magentaF", (double (QColor::*)() const) &QColor::magentaF, "C++: QColor::magentaF() const --> double");
		cl.def("yellowF", (double (QColor::*)() const) &QColor::yellowF, "C++: QColor::yellowF() const --> double");
		cl.def("blackF", (double (QColor::*)() const) &QColor::blackF, "C++: QColor::blackF() const --> double");
		cl.def("getCmyk", [](QColor &o, int * a0, int * a1, int * a2, int * a3) -> void { return o.getCmyk(a0, a1, a2, a3); }, "", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"));
		cl.def("getCmyk", (void (QColor::*)(int *, int *, int *, int *, int *)) &QColor::getCmyk, "C++: QColor::getCmyk(int *, int *, int *, int *, int *) --> void", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"), pybind11::arg("a"));
		cl.def("setCmyk", [](QColor &o, int const & a0, int const & a1, int const & a2, int const & a3) -> void { return o.setCmyk(a0, a1, a2, a3); }, "", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"));
		cl.def("setCmyk", (void (QColor::*)(int, int, int, int, int)) &QColor::setCmyk, "C++: QColor::setCmyk(int, int, int, int, int) --> void", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"), pybind11::arg("a"));
		cl.def("getCmykF", [](QColor &o, double * a0, double * a1, double * a2, double * a3) -> void { return o.getCmykF(a0, a1, a2, a3); }, "", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"));
		cl.def("getCmykF", (void (QColor::*)(double *, double *, double *, double *, double *)) &QColor::getCmykF, "C++: QColor::getCmykF(double *, double *, double *, double *, double *) --> void", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"), pybind11::arg("a"));
		cl.def("setCmykF", [](QColor &o, double const & a0, double const & a1, double const & a2, double const & a3) -> void { return o.setCmykF(a0, a1, a2, a3); }, "", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"));
		cl.def("setCmykF", (void (QColor::*)(double, double, double, double, double)) &QColor::setCmykF, "C++: QColor::setCmykF(double, double, double, double, double) --> void", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"), pybind11::arg("a"));
		cl.def("hslHue", (int (QColor::*)() const) &QColor::hslHue, "C++: QColor::hslHue() const --> int");
		cl.def("hslSaturation", (int (QColor::*)() const) &QColor::hslSaturation, "C++: QColor::hslSaturation() const --> int");
		cl.def("lightness", (int (QColor::*)() const) &QColor::lightness, "C++: QColor::lightness() const --> int");
		cl.def("hslHueF", (double (QColor::*)() const) &QColor::hslHueF, "C++: QColor::hslHueF() const --> double");
		cl.def("hslSaturationF", (double (QColor::*)() const) &QColor::hslSaturationF, "C++: QColor::hslSaturationF() const --> double");
		cl.def("lightnessF", (double (QColor::*)() const) &QColor::lightnessF, "C++: QColor::lightnessF() const --> double");
		cl.def("getHsl", [](QColor const &o, int * a0, int * a1, int * a2) -> void { return o.getHsl(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"));
		cl.def("getHsl", (void (QColor::*)(int *, int *, int *, int *) const) &QColor::getHsl, "C++: QColor::getHsl(int *, int *, int *, int *) const --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"), pybind11::arg("a"));
		cl.def("setHsl", [](QColor &o, int const & a0, int const & a1, int const & a2) -> void { return o.setHsl(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"));
		cl.def("setHsl", (void (QColor::*)(int, int, int, int)) &QColor::setHsl, "C++: QColor::setHsl(int, int, int, int) --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"), pybind11::arg("a"));
		cl.def("getHslF", [](QColor const &o, double * a0, double * a1, double * a2) -> void { return o.getHslF(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"));
		cl.def("getHslF", (void (QColor::*)(double *, double *, double *, double *) const) &QColor::getHslF, "C++: QColor::getHslF(double *, double *, double *, double *) const --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"), pybind11::arg("a"));
		cl.def("setHslF", [](QColor &o, double const & a0, double const & a1, double const & a2) -> void { return o.setHslF(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"));
		cl.def("setHslF", (void (QColor::*)(double, double, double, double)) &QColor::setHslF, "C++: QColor::setHslF(double, double, double, double) --> void", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"), pybind11::arg("a"));
		cl.def("toRgb", (class QColor (QColor::*)() const) &QColor::toRgb, "C++: QColor::toRgb() const --> class QColor");
		cl.def("toHsv", (class QColor (QColor::*)() const) &QColor::toHsv, "C++: QColor::toHsv() const --> class QColor");
		cl.def("toCmyk", (class QColor (QColor::*)() const) &QColor::toCmyk, "C++: QColor::toCmyk() const --> class QColor");
		cl.def("toHsl", (class QColor (QColor::*)() const) &QColor::toHsl, "C++: QColor::toHsl() const --> class QColor");
		cl.def("toExtendedRgb", (class QColor (QColor::*)() const) &QColor::toExtendedRgb, "C++: QColor::toExtendedRgb() const --> class QColor");
		cl.def("convertTo", (class QColor (QColor::*)(enum QColor::Spec) const) &QColor::convertTo, "C++: QColor::convertTo(enum QColor::Spec) const --> class QColor", pybind11::arg("colorSpec"));
		cl.def_static("fromRgb", (class QColor (*)(unsigned int)) &QColor::fromRgb, "C++: QColor::fromRgb(unsigned int) --> class QColor", pybind11::arg("rgb"));
		cl.def_static("fromRgba", (class QColor (*)(unsigned int)) &QColor::fromRgba, "C++: QColor::fromRgba(unsigned int) --> class QColor", pybind11::arg("rgba"));
		cl.def_static("fromRgb", [](int const & a0, int const & a1, int const & a2) -> QColor { return QColor::fromRgb(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def_static("fromRgb", (class QColor (*)(int, int, int, int)) &QColor::fromRgb, "C++: QColor::fromRgb(int, int, int, int) --> class QColor", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def_static("fromRgbF", [](double const & a0, double const & a1, double const & a2) -> QColor { return QColor::fromRgbF(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def_static("fromRgbF", (class QColor (*)(double, double, double, double)) &QColor::fromRgbF, "C++: QColor::fromRgbF(double, double, double, double) --> class QColor", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def_static("fromRgba64", [](unsigned short const & a0, unsigned short const & a1, unsigned short const & a2) -> QColor { return QColor::fromRgba64(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def_static("fromRgba64", (class QColor (*)(unsigned short, unsigned short, unsigned short, unsigned short)) &QColor::fromRgba64, "C++: QColor::fromRgba64(unsigned short, unsigned short, unsigned short, unsigned short) --> class QColor", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"));
		cl.def_static("fromRgba64", (class QColor (*)(class QRgba64)) &QColor::fromRgba64, "C++: QColor::fromRgba64(class QRgba64) --> class QColor", pybind11::arg("rgba"));
		cl.def_static("fromHsv", [](int const & a0, int const & a1, int const & a2) -> QColor { return QColor::fromHsv(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"));
		cl.def_static("fromHsv", (class QColor (*)(int, int, int, int)) &QColor::fromHsv, "C++: QColor::fromHsv(int, int, int, int) --> class QColor", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"), pybind11::arg("a"));
		cl.def_static("fromHsvF", [](double const & a0, double const & a1, double const & a2) -> QColor { return QColor::fromHsvF(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"));
		cl.def_static("fromHsvF", (class QColor (*)(double, double, double, double)) &QColor::fromHsvF, "C++: QColor::fromHsvF(double, double, double, double) --> class QColor", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v"), pybind11::arg("a"));
		cl.def_static("fromCmyk", [](int const & a0, int const & a1, int const & a2, int const & a3) -> QColor { return QColor::fromCmyk(a0, a1, a2, a3); }, "", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"));
		cl.def_static("fromCmyk", (class QColor (*)(int, int, int, int, int)) &QColor::fromCmyk, "C++: QColor::fromCmyk(int, int, int, int, int) --> class QColor", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"), pybind11::arg("a"));
		cl.def_static("fromCmykF", [](double const & a0, double const & a1, double const & a2, double const & a3) -> QColor { return QColor::fromCmykF(a0, a1, a2, a3); }, "", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"));
		cl.def_static("fromCmykF", (class QColor (*)(double, double, double, double, double)) &QColor::fromCmykF, "C++: QColor::fromCmykF(double, double, double, double, double) --> class QColor", pybind11::arg("c"), pybind11::arg("m"), pybind11::arg("y"), pybind11::arg("k"), pybind11::arg("a"));
		cl.def_static("fromHsl", [](int const & a0, int const & a1, int const & a2) -> QColor { return QColor::fromHsl(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"));
		cl.def_static("fromHsl", (class QColor (*)(int, int, int, int)) &QColor::fromHsl, "C++: QColor::fromHsl(int, int, int, int) --> class QColor", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"), pybind11::arg("a"));
		cl.def_static("fromHslF", [](double const & a0, double const & a1, double const & a2) -> QColor { return QColor::fromHslF(a0, a1, a2); }, "", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"));
		cl.def_static("fromHslF", (class QColor (*)(double, double, double, double)) &QColor::fromHslF, "C++: QColor::fromHslF(double, double, double, double) --> class QColor", pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("l"), pybind11::arg("a"));
		cl.def("light", [](QColor const &o) -> QColor { return o.light(); }, "");
		cl.def("light", (class QColor (QColor::*)(int) const) &QColor::light, "C++: QColor::light(int) const --> class QColor", pybind11::arg("f"));
		cl.def("dark", [](QColor const &o) -> QColor { return o.dark(); }, "");
		cl.def("dark", (class QColor (QColor::*)(int) const) &QColor::dark, "C++: QColor::dark(int) const --> class QColor", pybind11::arg("f"));
		cl.def("lighter", [](QColor const &o) -> QColor { return o.lighter(); }, "");
		cl.def("lighter", (class QColor (QColor::*)(int) const) &QColor::lighter, "C++: QColor::lighter(int) const --> class QColor", pybind11::arg("f"));
		cl.def("darker", [](QColor const &o) -> QColor { return o.darker(); }, "");
		cl.def("darker", (class QColor (QColor::*)(int) const) &QColor::darker, "C++: QColor::darker(int) const --> class QColor", pybind11::arg("f"));
		cl.def("__eq__", (bool (QColor::*)(const class QColor &) const) &QColor::operator==, "C++: QColor::operator==(const class QColor &) const --> bool", pybind11::arg("c"));
		cl.def("__ne__", (bool (QColor::*)(const class QColor &) const) &QColor::operator!=, "C++: QColor::operator!=(const class QColor &) const --> bool", pybind11::arg("c"));
		cl.def_static("isValidColor", (bool (*)(const class QString &)) &QColor::isValidColor, "C++: QColor::isValidColor(const class QString &) --> bool", pybind11::arg("name"));
		cl.def_static("isValidColor", (bool (*)(class QStringView)) &QColor::isValidColor, "C++: QColor::isValidColor(class QStringView) --> bool", pybind11::arg(""));
		cl.def_static("isValidColor", (bool (*)(class QLatin1String)) &QColor::isValidColor, "C++: QColor::isValidColor(class QLatin1String) --> bool", pybind11::arg(""));
	}
}
