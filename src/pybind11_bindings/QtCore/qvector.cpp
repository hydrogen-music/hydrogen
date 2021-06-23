#include <QtCore/qvector.h> // QVector
#include <iostream> // --trace
#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <vector> // std::vector
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

void bind_QtCore_qvector(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B112_[QVector<unsigned int>] ";
	{ // QVector file:QtCore/qvector.h line:63
		pybind11::class_<QVector<unsigned int>, std::shared_ptr<QVector<unsigned int>>> cl(M(""), "QVector_unsigned_int_t", "");
		cl.def( pybind11::init( [](){ return new QVector<unsigned int>(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("size") );

		cl.def( pybind11::init<int, const unsigned int &>(), pybind11::arg("size"), pybind11::arg("t") );

		cl.def( pybind11::init( [](QVector<unsigned int> const &o){ return new QVector<unsigned int>(o); } ) );
		cl.def("assign", (class QVector<unsigned int> & (QVector<unsigned int>::*)(const class QVector<unsigned int> &)) &QVector<unsigned int>::operator=, "C++: QVector<unsigned int>::operator=(const class QVector<unsigned int> &) --> class QVector<unsigned int> &", pybind11::return_value_policy::automatic, pybind11::arg("v"));
		cl.def("swap", (void (QVector<unsigned int>::*)(class QVector<unsigned int> &)) &QVector<unsigned int>::swap, "C++: QVector<unsigned int>::swap(class QVector<unsigned int> &) --> void", pybind11::arg("other"));
		cl.def("__eq__", (bool (QVector<unsigned int>::*)(const class QVector<unsigned int> &) const) &QVector<unsigned int>::operator==, "C++: QVector<unsigned int>::operator==(const class QVector<unsigned int> &) const --> bool", pybind11::arg("v"));
		cl.def("__ne__", (bool (QVector<unsigned int>::*)(const class QVector<unsigned int> &) const) &QVector<unsigned int>::operator!=, "C++: QVector<unsigned int>::operator!=(const class QVector<unsigned int> &) const --> bool", pybind11::arg("v"));
		cl.def("size", (int (QVector<unsigned int>::*)() const) &QVector<unsigned int>::size, "C++: QVector<unsigned int>::size() const --> int");
		cl.def("isEmpty", (bool (QVector<unsigned int>::*)() const) &QVector<unsigned int>::isEmpty, "C++: QVector<unsigned int>::isEmpty() const --> bool");
		cl.def("resize", (void (QVector<unsigned int>::*)(int)) &QVector<unsigned int>::resize, "C++: QVector<unsigned int>::resize(int) --> void", pybind11::arg("size"));
		cl.def("capacity", (int (QVector<unsigned int>::*)() const) &QVector<unsigned int>::capacity, "C++: QVector<unsigned int>::capacity() const --> int");
		cl.def("reserve", (void (QVector<unsigned int>::*)(int)) &QVector<unsigned int>::reserve, "C++: QVector<unsigned int>::reserve(int) --> void", pybind11::arg("size"));
		cl.def("squeeze", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::squeeze, "C++: QVector<unsigned int>::squeeze() --> void");
		cl.def("detach", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::detach, "C++: QVector<unsigned int>::detach() --> void");
		cl.def("isDetached", (bool (QVector<unsigned int>::*)() const) &QVector<unsigned int>::isDetached, "C++: QVector<unsigned int>::isDetached() const --> bool");
		cl.def("setSharable", (void (QVector<unsigned int>::*)(bool)) &QVector<unsigned int>::setSharable, "C++: QVector<unsigned int>::setSharable(bool) --> void", pybind11::arg("sharable"));
		cl.def("isSharedWith", (bool (QVector<unsigned int>::*)(const class QVector<unsigned int> &) const) &QVector<unsigned int>::isSharedWith, "C++: QVector<unsigned int>::isSharedWith(const class QVector<unsigned int> &) const --> bool", pybind11::arg("other"));
		cl.def("data", (unsigned int * (QVector<unsigned int>::*)()) &QVector<unsigned int>::data, "C++: QVector<unsigned int>::data() --> unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("constData", (const unsigned int * (QVector<unsigned int>::*)() const) &QVector<unsigned int>::constData, "C++: QVector<unsigned int>::constData() const --> const unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("clear", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::clear, "C++: QVector<unsigned int>::clear() --> void");
		cl.def("at", (const unsigned int & (QVector<unsigned int>::*)(int) const) &QVector<unsigned int>::at, "C++: QVector<unsigned int>::at(int) const --> const unsigned int &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__getitem__", (unsigned int & (QVector<unsigned int>::*)(int)) &QVector<unsigned int>::operator[], "C++: QVector<unsigned int>::operator[](int) --> unsigned int &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("append", (void (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::append, "C++: QVector<unsigned int>::append(const unsigned int &) --> void", pybind11::arg("t"));
		cl.def("append", (void (QVector<unsigned int>::*)(const class QVector<unsigned int> &)) &QVector<unsigned int>::append, "C++: QVector<unsigned int>::append(const class QVector<unsigned int> &) --> void", pybind11::arg("l"));
		cl.def("prepend", (void (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::prepend, "C++: QVector<unsigned int>::prepend(const unsigned int &) --> void", pybind11::arg("t"));
		cl.def("insert", (void (QVector<unsigned int>::*)(int, const unsigned int &)) &QVector<unsigned int>::insert, "C++: QVector<unsigned int>::insert(int, const unsigned int &) --> void", pybind11::arg("i"), pybind11::arg("t"));
		cl.def("insert", (void (QVector<unsigned int>::*)(int, int, const unsigned int &)) &QVector<unsigned int>::insert, "C++: QVector<unsigned int>::insert(int, int, const unsigned int &) --> void", pybind11::arg("i"), pybind11::arg("n"), pybind11::arg("t"));
		cl.def("replace", (void (QVector<unsigned int>::*)(int, const unsigned int &)) &QVector<unsigned int>::replace, "C++: QVector<unsigned int>::replace(int, const unsigned int &) --> void", pybind11::arg("i"), pybind11::arg("t"));
		cl.def("remove", (void (QVector<unsigned int>::*)(int)) &QVector<unsigned int>::remove, "C++: QVector<unsigned int>::remove(int) --> void", pybind11::arg("i"));
		cl.def("remove", (void (QVector<unsigned int>::*)(int, int)) &QVector<unsigned int>::remove, "C++: QVector<unsigned int>::remove(int, int) --> void", pybind11::arg("i"), pybind11::arg("n"));
		cl.def("removeFirst", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::removeFirst, "C++: QVector<unsigned int>::removeFirst() --> void");
		cl.def("removeLast", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::removeLast, "C++: QVector<unsigned int>::removeLast() --> void");
		cl.def("takeFirst", (unsigned int (QVector<unsigned int>::*)()) &QVector<unsigned int>::takeFirst, "C++: QVector<unsigned int>::takeFirst() --> unsigned int");
		cl.def("takeLast", (unsigned int (QVector<unsigned int>::*)()) &QVector<unsigned int>::takeLast, "C++: QVector<unsigned int>::takeLast() --> unsigned int");
		cl.def("fill", [](QVector<unsigned int> &o, const unsigned int & a0) -> QVector<unsigned int> & { return o.fill(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("t"));
		cl.def("fill", (class QVector<unsigned int> & (QVector<unsigned int>::*)(const unsigned int &, int)) &QVector<unsigned int>::fill, "C++: QVector<unsigned int>::fill(const unsigned int &, int) --> class QVector<unsigned int> &", pybind11::return_value_policy::automatic, pybind11::arg("t"), pybind11::arg("size"));
		cl.def("indexOf", [](QVector<unsigned int> const &o, const unsigned int & a0) -> int { return o.indexOf(a0); }, "", pybind11::arg("t"));
		cl.def("indexOf", (int (QVector<unsigned int>::*)(const unsigned int &, int) const) &QVector<unsigned int>::indexOf, "C++: QVector<unsigned int>::indexOf(const unsigned int &, int) const --> int", pybind11::arg("t"), pybind11::arg("from"));
		cl.def("lastIndexOf", [](QVector<unsigned int> const &o, const unsigned int & a0) -> int { return o.lastIndexOf(a0); }, "", pybind11::arg("t"));
		cl.def("lastIndexOf", (int (QVector<unsigned int>::*)(const unsigned int &, int) const) &QVector<unsigned int>::lastIndexOf, "C++: QVector<unsigned int>::lastIndexOf(const unsigned int &, int) const --> int", pybind11::arg("t"), pybind11::arg("from"));
		cl.def("contains", (bool (QVector<unsigned int>::*)(const unsigned int &) const) &QVector<unsigned int>::contains, "C++: QVector<unsigned int>::contains(const unsigned int &) const --> bool", pybind11::arg("t"));
		cl.def("count", (int (QVector<unsigned int>::*)(const unsigned int &) const) &QVector<unsigned int>::count, "C++: QVector<unsigned int>::count(const unsigned int &) const --> int", pybind11::arg("t"));
		cl.def("removeAt", (void (QVector<unsigned int>::*)(int)) &QVector<unsigned int>::removeAt, "C++: QVector<unsigned int>::removeAt(int) --> void", pybind11::arg("i"));
		cl.def("removeAll", (int (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::removeAll, "C++: QVector<unsigned int>::removeAll(const unsigned int &) --> int", pybind11::arg("t"));
		cl.def("removeOne", (bool (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::removeOne, "C++: QVector<unsigned int>::removeOne(const unsigned int &) --> bool", pybind11::arg("t"));
		cl.def("length", (int (QVector<unsigned int>::*)() const) &QVector<unsigned int>::length, "C++: QVector<unsigned int>::length() const --> int");
		cl.def("takeAt", (unsigned int (QVector<unsigned int>::*)(int)) &QVector<unsigned int>::takeAt, "C++: QVector<unsigned int>::takeAt(int) --> unsigned int", pybind11::arg("i"));
		cl.def("move", (void (QVector<unsigned int>::*)(int, int)) &QVector<unsigned int>::move, "C++: QVector<unsigned int>::move(int, int) --> void", pybind11::arg("from"), pybind11::arg("to"));
		cl.def("begin", (unsigned int * (QVector<unsigned int>::*)()) &QVector<unsigned int>::begin, "C++: QVector<unsigned int>::begin() --> unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("cbegin", (const unsigned int * (QVector<unsigned int>::*)() const) &QVector<unsigned int>::cbegin, "C++: QVector<unsigned int>::cbegin() const --> const unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("constBegin", (const unsigned int * (QVector<unsigned int>::*)() const) &QVector<unsigned int>::constBegin, "C++: QVector<unsigned int>::constBegin() const --> const unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("end", (unsigned int * (QVector<unsigned int>::*)()) &QVector<unsigned int>::end, "C++: QVector<unsigned int>::end() --> unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("cend", (const unsigned int * (QVector<unsigned int>::*)() const) &QVector<unsigned int>::cend, "C++: QVector<unsigned int>::cend() const --> const unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("constEnd", (const unsigned int * (QVector<unsigned int>::*)() const) &QVector<unsigned int>::constEnd, "C++: QVector<unsigned int>::constEnd() const --> const unsigned int *", pybind11::return_value_policy::automatic);
		cl.def("insert", (unsigned int * (QVector<unsigned int>::*)(unsigned int *, int, const unsigned int &)) &QVector<unsigned int>::insert, "C++: QVector<unsigned int>::insert(unsigned int *, int, const unsigned int &) --> unsigned int *", pybind11::return_value_policy::automatic, pybind11::arg("before"), pybind11::arg("n"), pybind11::arg("x"));
		cl.def("insert", (unsigned int * (QVector<unsigned int>::*)(unsigned int *, const unsigned int &)) &QVector<unsigned int>::insert, "C++: QVector<unsigned int>::insert(unsigned int *, const unsigned int &) --> unsigned int *", pybind11::return_value_policy::automatic, pybind11::arg("before"), pybind11::arg("x"));
		cl.def("erase", (unsigned int * (QVector<unsigned int>::*)(unsigned int *, unsigned int *)) &QVector<unsigned int>::erase, "C++: QVector<unsigned int>::erase(unsigned int *, unsigned int *) --> unsigned int *", pybind11::return_value_policy::automatic, pybind11::arg("begin"), pybind11::arg("end"));
		cl.def("erase", (unsigned int * (QVector<unsigned int>::*)(unsigned int *)) &QVector<unsigned int>::erase, "C++: QVector<unsigned int>::erase(unsigned int *) --> unsigned int *", pybind11::return_value_policy::automatic, pybind11::arg("pos"));
		cl.def("count", (int (QVector<unsigned int>::*)() const) &QVector<unsigned int>::count, "C++: QVector<unsigned int>::count() const --> int");
		cl.def("first", (unsigned int & (QVector<unsigned int>::*)()) &QVector<unsigned int>::first, "C++: QVector<unsigned int>::first() --> unsigned int &", pybind11::return_value_policy::automatic);
		cl.def("constFirst", (const unsigned int & (QVector<unsigned int>::*)() const) &QVector<unsigned int>::constFirst, "C++: QVector<unsigned int>::constFirst() const --> const unsigned int &", pybind11::return_value_policy::automatic);
		cl.def("last", (unsigned int & (QVector<unsigned int>::*)()) &QVector<unsigned int>::last, "C++: QVector<unsigned int>::last() --> unsigned int &", pybind11::return_value_policy::automatic);
		cl.def("constLast", (const unsigned int & (QVector<unsigned int>::*)() const) &QVector<unsigned int>::constLast, "C++: QVector<unsigned int>::constLast() const --> const unsigned int &", pybind11::return_value_policy::automatic);
		cl.def("startsWith", (bool (QVector<unsigned int>::*)(const unsigned int &) const) &QVector<unsigned int>::startsWith, "C++: QVector<unsigned int>::startsWith(const unsigned int &) const --> bool", pybind11::arg("t"));
		cl.def("endsWith", (bool (QVector<unsigned int>::*)(const unsigned int &) const) &QVector<unsigned int>::endsWith, "C++: QVector<unsigned int>::endsWith(const unsigned int &) const --> bool", pybind11::arg("t"));
		cl.def("mid", [](QVector<unsigned int> const &o, int const & a0) -> QVector<unsigned int> { return o.mid(a0); }, "", pybind11::arg("pos"));
		cl.def("mid", (class QVector<unsigned int> (QVector<unsigned int>::*)(int, int) const) &QVector<unsigned int>::mid, "C++: QVector<unsigned int>::mid(int, int) const --> class QVector<unsigned int>", pybind11::arg("pos"), pybind11::arg("len"));
		cl.def("value", (unsigned int (QVector<unsigned int>::*)(int) const) &QVector<unsigned int>::value, "C++: QVector<unsigned int>::value(int) const --> unsigned int", pybind11::arg("i"));
		cl.def("value", (unsigned int (QVector<unsigned int>::*)(int, const unsigned int &) const) &QVector<unsigned int>::value, "C++: QVector<unsigned int>::value(int, const unsigned int &) const --> unsigned int", pybind11::arg("i"), pybind11::arg("defaultValue"));
		cl.def("swapItemsAt", (void (QVector<unsigned int>::*)(int, int)) &QVector<unsigned int>::swapItemsAt, "C++: QVector<unsigned int>::swapItemsAt(int, int) --> void", pybind11::arg("i"), pybind11::arg("j"));
		cl.def("push_back", (void (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::push_back, "C++: QVector<unsigned int>::push_back(const unsigned int &) --> void", pybind11::arg("t"));
		cl.def("push_front", (void (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::push_front, "C++: QVector<unsigned int>::push_front(const unsigned int &) --> void", pybind11::arg("t"));
		cl.def("pop_back", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::pop_back, "C++: QVector<unsigned int>::pop_back() --> void");
		cl.def("pop_front", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::pop_front, "C++: QVector<unsigned int>::pop_front() --> void");
		cl.def("empty", (bool (QVector<unsigned int>::*)() const) &QVector<unsigned int>::empty, "C++: QVector<unsigned int>::empty() const --> bool");
		cl.def("front", (unsigned int & (QVector<unsigned int>::*)()) &QVector<unsigned int>::front, "C++: QVector<unsigned int>::front() --> unsigned int &", pybind11::return_value_policy::automatic);
		cl.def("back", (unsigned int & (QVector<unsigned int>::*)()) &QVector<unsigned int>::back, "C++: QVector<unsigned int>::back() --> unsigned int &", pybind11::return_value_policy::automatic);
		cl.def("shrink_to_fit", (void (QVector<unsigned int>::*)()) &QVector<unsigned int>::shrink_to_fit, "C++: QVector<unsigned int>::shrink_to_fit() --> void");
		cl.def("__iadd__", (class QVector<unsigned int> & (QVector<unsigned int>::*)(const class QVector<unsigned int> &)) &QVector<unsigned int>::operator+=, "C++: QVector<unsigned int>::operator+=(const class QVector<unsigned int> &) --> class QVector<unsigned int> &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__add__", (class QVector<unsigned int> (QVector<unsigned int>::*)(const class QVector<unsigned int> &) const) &QVector<unsigned int>::operator+, "C++: QVector<unsigned int>::operator+(const class QVector<unsigned int> &) const --> class QVector<unsigned int>", pybind11::arg("l"));
		cl.def("__iadd__", (class QVector<unsigned int> & (QVector<unsigned int>::*)(const unsigned int &)) &QVector<unsigned int>::operator+=, "C++: QVector<unsigned int>::operator+=(const unsigned int &) --> class QVector<unsigned int> &", pybind11::return_value_policy::automatic, pybind11::arg("t"));
	}
}
