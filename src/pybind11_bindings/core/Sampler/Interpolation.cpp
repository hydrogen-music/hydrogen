#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::InterpolateMode
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::cosine_Interpolate
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::cubic_Interpolate
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::hermite_Interpolate
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::linear_Interpolate
#include <core/Sampler/Interpolation.h> // H2Core::Interpolation::third_Interpolate
#include <iostream> // --trace
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

void bind_core_Sampler_Interpolation(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B640_[H2Core::Interpolation::InterpolateMode] ";
	// H2Core::Interpolation::InterpolateMode file:core/Sampler/Interpolation.h line:33
	pybind11::enum_<H2Core::Interpolation::InterpolateMode>(M("H2Core::Interpolation"), "InterpolateMode", "")
		.value("Linear", H2Core::Interpolation::InterpolateMode::Linear)
		.value("Cosine", H2Core::Interpolation::InterpolateMode::Cosine)
		.value("Third", H2Core::Interpolation::InterpolateMode::Third)
		.value("Cubic", H2Core::Interpolation::InterpolateMode::Cubic)
		.value("Hermite", H2Core::Interpolation::InterpolateMode::Hermite);

;

	std::cout << "B641_[float H2Core::Interpolation::linear_Interpolate(float, float, float)] ";
	// H2Core::Interpolation::linear_Interpolate(float, float, float) file:core/Sampler/Interpolation.h line:39
	M("H2Core::Interpolation").def("linear_Interpolate", (float (*)(float, float, float)) &H2Core::Interpolation::linear_Interpolate, "C++: H2Core::Interpolation::linear_Interpolate(float, float, float) --> float", pybind11::arg("y1"), pybind11::arg("y2"), pybind11::arg("mu"));

	std::cout << "B642_[float H2Core::Interpolation::cosine_Interpolate(float, float, double)] ";
	// H2Core::Interpolation::cosine_Interpolate(float, float, double) file:core/Sampler/Interpolation.h line:49
	M("H2Core::Interpolation").def("cosine_Interpolate", (float (*)(float, float, double)) &H2Core::Interpolation::cosine_Interpolate, "C++: H2Core::Interpolation::cosine_Interpolate(float, float, double) --> float", pybind11::arg("y1"), pybind11::arg("y2"), pybind11::arg("mu"));

	std::cout << "B643_[float H2Core::Interpolation::third_Interpolate(float, float, float, float, double)] ";
	// H2Core::Interpolation::third_Interpolate(float, float, float, float, double) file:core/Sampler/Interpolation.h line:62
	M("H2Core::Interpolation").def("third_Interpolate", (float (*)(float, float, float, float, double)) &H2Core::Interpolation::third_Interpolate, "C++: H2Core::Interpolation::third_Interpolate(float, float, float, float, double) --> float", pybind11::arg("y0"), pybind11::arg("y1"), pybind11::arg("y2"), pybind11::arg("y3"), pybind11::arg("mu"));

	std::cout << "B644_[float H2Core::Interpolation::cubic_Interpolate(float, float, float, float, double)] ";
	// H2Core::Interpolation::cubic_Interpolate(float, float, float, float, double) file:core/Sampler/Interpolation.h line:79
	M("H2Core::Interpolation").def("cubic_Interpolate", (float (*)(float, float, float, float, double)) &H2Core::Interpolation::cubic_Interpolate, "C++: H2Core::Interpolation::cubic_Interpolate(float, float, float, float, double) --> float", pybind11::arg("y0"), pybind11::arg("y1"), pybind11::arg("y2"), pybind11::arg("y3"), pybind11::arg("mu"));

	std::cout << "B645_[float H2Core::Interpolation::hermite_Interpolate(float, float, float, float, double)] ";
	// H2Core::Interpolation::hermite_Interpolate(float, float, float, float, double) file:core/Sampler/Interpolation.h line:100
	M("H2Core::Interpolation").def("hermite_Interpolate", (float (*)(float, float, float, float, double)) &H2Core::Interpolation::hermite_Interpolate, "C++: H2Core::Interpolation::hermite_Interpolate(float, float, float, float, double) --> float", pybind11::arg("y0"), pybind11::arg("y1"), pybind11::arg("y2"), pybind11::arg("y3"), pybind11::arg("mu"));

}
