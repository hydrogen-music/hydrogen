#include <QtCore/qarraydata.h> // QArrayDataPointerRef
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qvector.h> // QVector
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

void bind_QtCore_qvector_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B113_[QVector<int>] ";
	{ // QVector file:QtCore/qvector.h line:63
		pybind11::class_<QVector<int>, std::shared_ptr<QVector<int>>> cl(M(""), "QVector_int_t", "");
		cl.def( pybind11::init( [](){ return new QVector<int>(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("size") );

		cl.def( pybind11::init<int, const int &>(), pybind11::arg("size"), pybind11::arg("t") );

		cl.def( pybind11::init( [](QVector<int> const &o){ return new QVector<int>(o); } ) );
		cl.def( pybind11::init<struct QArrayDataPointerRef<int>>(), pybind11::arg("ref") );

		cl.def("assign", (class QVector<int> & (QVector<int>::*)(const class QVector<int> &)) &QVector<int>::operator=, "C++: QVector<int>::operator=(const class QVector<int> &) --> class QVector<int> &", pybind11::return_value_policy::automatic, pybind11::arg("v"));
		cl.def("swap", (void (QVector<int>::*)(class QVector<int> &)) &QVector<int>::swap, "C++: QVector<int>::swap(class QVector<int> &) --> void", pybind11::arg("other"));
		cl.def("__eq__", (bool (QVector<int>::*)(const class QVector<int> &) const) &QVector<int>::operator==, "C++: QVector<int>::operator==(const class QVector<int> &) const --> bool", pybind11::arg("v"));
		cl.def("__ne__", (bool (QVector<int>::*)(const class QVector<int> &) const) &QVector<int>::operator!=, "C++: QVector<int>::operator!=(const class QVector<int> &) const --> bool", pybind11::arg("v"));
		cl.def("size", (int (QVector<int>::*)() const) &QVector<int>::size, "C++: QVector<int>::size() const --> int");
		cl.def("isEmpty", (bool (QVector<int>::*)() const) &QVector<int>::isEmpty, "C++: QVector<int>::isEmpty() const --> bool");
		cl.def("resize", (void (QVector<int>::*)(int)) &QVector<int>::resize, "C++: QVector<int>::resize(int) --> void", pybind11::arg("asize"));
		cl.def("capacity", (int (QVector<int>::*)() const) &QVector<int>::capacity, "C++: QVector<int>::capacity() const --> int");
		cl.def("reserve", (void (QVector<int>::*)(int)) &QVector<int>::reserve, "C++: QVector<int>::reserve(int) --> void", pybind11::arg("size"));
		cl.def("squeeze", (void (QVector<int>::*)()) &QVector<int>::squeeze, "C++: QVector<int>::squeeze() --> void");
		cl.def("detach", (void (QVector<int>::*)()) &QVector<int>::detach, "C++: QVector<int>::detach() --> void");
		cl.def("isDetached", (bool (QVector<int>::*)() const) &QVector<int>::isDetached, "C++: QVector<int>::isDetached() const --> bool");
		cl.def("setSharable", (void (QVector<int>::*)(bool)) &QVector<int>::setSharable, "C++: QVector<int>::setSharable(bool) --> void", pybind11::arg("sharable"));
		cl.def("isSharedWith", (bool (QVector<int>::*)(const class QVector<int> &) const) &QVector<int>::isSharedWith, "C++: QVector<int>::isSharedWith(const class QVector<int> &) const --> bool", pybind11::arg("other"));
		cl.def("data", (int * (QVector<int>::*)()) &QVector<int>::data, "C++: QVector<int>::data() --> int *", pybind11::return_value_policy::automatic);
		cl.def("constData", (const int * (QVector<int>::*)() const) &QVector<int>::constData, "C++: QVector<int>::constData() const --> const int *", pybind11::return_value_policy::automatic);
		cl.def("clear", (void (QVector<int>::*)()) &QVector<int>::clear, "C++: QVector<int>::clear() --> void");
		cl.def("at", (const int & (QVector<int>::*)(int) const) &QVector<int>::at, "C++: QVector<int>::at(int) const --> const int &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__getitem__", (int & (QVector<int>::*)(int)) &QVector<int>::operator[], "C++: QVector<int>::operator[](int) --> int &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("append", (void (QVector<int>::*)(const int &)) &QVector<int>::append, "C++: QVector<int>::append(const int &) --> void", pybind11::arg("t"));
		cl.def("append", (void (QVector<int>::*)(const class QVector<int> &)) &QVector<int>::append, "C++: QVector<int>::append(const class QVector<int> &) --> void", pybind11::arg("l"));
		cl.def("prepend", (void (QVector<int>::*)(const int &)) &QVector<int>::prepend, "C++: QVector<int>::prepend(const int &) --> void", pybind11::arg("t"));
		cl.def("insert", (void (QVector<int>::*)(int, const int &)) &QVector<int>::insert, "C++: QVector<int>::insert(int, const int &) --> void", pybind11::arg("i"), pybind11::arg("t"));
		cl.def("insert", (void (QVector<int>::*)(int, int, const int &)) &QVector<int>::insert, "C++: QVector<int>::insert(int, int, const int &) --> void", pybind11::arg("i"), pybind11::arg("n"), pybind11::arg("t"));
		cl.def("replace", (void (QVector<int>::*)(int, const int &)) &QVector<int>::replace, "C++: QVector<int>::replace(int, const int &) --> void", pybind11::arg("i"), pybind11::arg("t"));
		cl.def("remove", (void (QVector<int>::*)(int)) &QVector<int>::remove, "C++: QVector<int>::remove(int) --> void", pybind11::arg("i"));
		cl.def("remove", (void (QVector<int>::*)(int, int)) &QVector<int>::remove, "C++: QVector<int>::remove(int, int) --> void", pybind11::arg("i"), pybind11::arg("n"));
		cl.def("removeFirst", (void (QVector<int>::*)()) &QVector<int>::removeFirst, "C++: QVector<int>::removeFirst() --> void");
		cl.def("removeLast", (void (QVector<int>::*)()) &QVector<int>::removeLast, "C++: QVector<int>::removeLast() --> void");
		cl.def("takeFirst", (int (QVector<int>::*)()) &QVector<int>::takeFirst, "C++: QVector<int>::takeFirst() --> int");
		cl.def("takeLast", (int (QVector<int>::*)()) &QVector<int>::takeLast, "C++: QVector<int>::takeLast() --> int");
		cl.def("fill", [](QVector<int> &o, const int & a0) -> QVector<int> & { return o.fill(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("t"));
		cl.def("fill", (class QVector<int> & (QVector<int>::*)(const int &, int)) &QVector<int>::fill, "C++: QVector<int>::fill(const int &, int) --> class QVector<int> &", pybind11::return_value_policy::automatic, pybind11::arg("t"), pybind11::arg("size"));
		cl.def("indexOf", [](QVector<int> const &o, const int & a0) -> int { return o.indexOf(a0); }, "", pybind11::arg("t"));
		cl.def("indexOf", (int (QVector<int>::*)(const int &, int) const) &QVector<int>::indexOf, "C++: QVector<int>::indexOf(const int &, int) const --> int", pybind11::arg("t"), pybind11::arg("from"));
		cl.def("lastIndexOf", [](QVector<int> const &o, const int & a0) -> int { return o.lastIndexOf(a0); }, "", pybind11::arg("t"));
		cl.def("lastIndexOf", (int (QVector<int>::*)(const int &, int) const) &QVector<int>::lastIndexOf, "C++: QVector<int>::lastIndexOf(const int &, int) const --> int", pybind11::arg("t"), pybind11::arg("from"));
		cl.def("contains", (bool (QVector<int>::*)(const int &) const) &QVector<int>::contains, "C++: QVector<int>::contains(const int &) const --> bool", pybind11::arg("t"));
		cl.def("count", (int (QVector<int>::*)(const int &) const) &QVector<int>::count, "C++: QVector<int>::count(const int &) const --> int", pybind11::arg("t"));
		cl.def("removeAt", (void (QVector<int>::*)(int)) &QVector<int>::removeAt, "C++: QVector<int>::removeAt(int) --> void", pybind11::arg("i"));
		cl.def("removeAll", (int (QVector<int>::*)(const int &)) &QVector<int>::removeAll, "C++: QVector<int>::removeAll(const int &) --> int", pybind11::arg("t"));
		cl.def("removeOne", (bool (QVector<int>::*)(const int &)) &QVector<int>::removeOne, "C++: QVector<int>::removeOne(const int &) --> bool", pybind11::arg("t"));
		cl.def("length", (int (QVector<int>::*)() const) &QVector<int>::length, "C++: QVector<int>::length() const --> int");
		cl.def("takeAt", (int (QVector<int>::*)(int)) &QVector<int>::takeAt, "C++: QVector<int>::takeAt(int) --> int", pybind11::arg("i"));
		cl.def("move", (void (QVector<int>::*)(int, int)) &QVector<int>::move, "C++: QVector<int>::move(int, int) --> void", pybind11::arg("from"), pybind11::arg("to"));
		cl.def("begin", (int * (QVector<int>::*)()) &QVector<int>::begin, "C++: QVector<int>::begin() --> int *", pybind11::return_value_policy::automatic);
		cl.def("cbegin", (const int * (QVector<int>::*)() const) &QVector<int>::cbegin, "C++: QVector<int>::cbegin() const --> const int *", pybind11::return_value_policy::automatic);
		cl.def("constBegin", (const int * (QVector<int>::*)() const) &QVector<int>::constBegin, "C++: QVector<int>::constBegin() const --> const int *", pybind11::return_value_policy::automatic);
		cl.def("end", (int * (QVector<int>::*)()) &QVector<int>::end, "C++: QVector<int>::end() --> int *", pybind11::return_value_policy::automatic);
		cl.def("cend", (const int * (QVector<int>::*)() const) &QVector<int>::cend, "C++: QVector<int>::cend() const --> const int *", pybind11::return_value_policy::automatic);
		cl.def("constEnd", (const int * (QVector<int>::*)() const) &QVector<int>::constEnd, "C++: QVector<int>::constEnd() const --> const int *", pybind11::return_value_policy::automatic);
		cl.def("insert", (int * (QVector<int>::*)(int *, int, const int &)) &QVector<int>::insert, "C++: QVector<int>::insert(int *, int, const int &) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("before"), pybind11::arg("n"), pybind11::arg("x"));
		cl.def("insert", (int * (QVector<int>::*)(int *, const int &)) &QVector<int>::insert, "C++: QVector<int>::insert(int *, const int &) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("before"), pybind11::arg("x"));
		cl.def("erase", (int * (QVector<int>::*)(int *, int *)) &QVector<int>::erase, "C++: QVector<int>::erase(int *, int *) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("begin"), pybind11::arg("end"));
		cl.def("erase", (int * (QVector<int>::*)(int *)) &QVector<int>::erase, "C++: QVector<int>::erase(int *) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("pos"));
		cl.def("count", (int (QVector<int>::*)() const) &QVector<int>::count, "C++: QVector<int>::count() const --> int");
		cl.def("first", (int & (QVector<int>::*)()) &QVector<int>::first, "C++: QVector<int>::first() --> int &", pybind11::return_value_policy::automatic);
		cl.def("constFirst", (const int & (QVector<int>::*)() const) &QVector<int>::constFirst, "C++: QVector<int>::constFirst() const --> const int &", pybind11::return_value_policy::automatic);
		cl.def("last", (int & (QVector<int>::*)()) &QVector<int>::last, "C++: QVector<int>::last() --> int &", pybind11::return_value_policy::automatic);
		cl.def("constLast", (const int & (QVector<int>::*)() const) &QVector<int>::constLast, "C++: QVector<int>::constLast() const --> const int &", pybind11::return_value_policy::automatic);
		cl.def("startsWith", (bool (QVector<int>::*)(const int &) const) &QVector<int>::startsWith, "C++: QVector<int>::startsWith(const int &) const --> bool", pybind11::arg("t"));
		cl.def("endsWith", (bool (QVector<int>::*)(const int &) const) &QVector<int>::endsWith, "C++: QVector<int>::endsWith(const int &) const --> bool", pybind11::arg("t"));
		cl.def("mid", [](QVector<int> const &o, int const & a0) -> QVector<int> { return o.mid(a0); }, "", pybind11::arg("pos"));
		cl.def("mid", (class QVector<int> (QVector<int>::*)(int, int) const) &QVector<int>::mid, "C++: QVector<int>::mid(int, int) const --> class QVector<int>", pybind11::arg("pos"), pybind11::arg("len"));
		cl.def("value", (int (QVector<int>::*)(int) const) &QVector<int>::value, "C++: QVector<int>::value(int) const --> int", pybind11::arg("i"));
		cl.def("value", (int (QVector<int>::*)(int, const int &) const) &QVector<int>::value, "C++: QVector<int>::value(int, const int &) const --> int", pybind11::arg("i"), pybind11::arg("defaultValue"));
		cl.def("swapItemsAt", (void (QVector<int>::*)(int, int)) &QVector<int>::swapItemsAt, "C++: QVector<int>::swapItemsAt(int, int) --> void", pybind11::arg("i"), pybind11::arg("j"));
		cl.def("push_back", (void (QVector<int>::*)(const int &)) &QVector<int>::push_back, "C++: QVector<int>::push_back(const int &) --> void", pybind11::arg("t"));
		cl.def("push_front", (void (QVector<int>::*)(const int &)) &QVector<int>::push_front, "C++: QVector<int>::push_front(const int &) --> void", pybind11::arg("t"));
		cl.def("pop_back", (void (QVector<int>::*)()) &QVector<int>::pop_back, "C++: QVector<int>::pop_back() --> void");
		cl.def("pop_front", (void (QVector<int>::*)()) &QVector<int>::pop_front, "C++: QVector<int>::pop_front() --> void");
		cl.def("empty", (bool (QVector<int>::*)() const) &QVector<int>::empty, "C++: QVector<int>::empty() const --> bool");
		cl.def("front", (int & (QVector<int>::*)()) &QVector<int>::front, "C++: QVector<int>::front() --> int &", pybind11::return_value_policy::automatic);
		cl.def("back", (int & (QVector<int>::*)()) &QVector<int>::back, "C++: QVector<int>::back() --> int &", pybind11::return_value_policy::automatic);
		cl.def("shrink_to_fit", (void (QVector<int>::*)()) &QVector<int>::shrink_to_fit, "C++: QVector<int>::shrink_to_fit() --> void");
		cl.def("__iadd__", (class QVector<int> & (QVector<int>::*)(const class QVector<int> &)) &QVector<int>::operator+=, "C++: QVector<int>::operator+=(const class QVector<int> &) --> class QVector<int> &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__add__", (class QVector<int> (QVector<int>::*)(const class QVector<int> &) const) &QVector<int>::operator+, "C++: QVector<int>::operator+(const class QVector<int> &) const --> class QVector<int>", pybind11::arg("l"));
		cl.def("__iadd__", (class QVector<int> & (QVector<int>::*)(const int &)) &QVector<int>::operator+=, "C++: QVector<int>::operator+=(const int &) --> class QVector<int> &", pybind11::return_value_policy::automatic, pybind11::arg("t"));
	}
	std::cout << "B114_[QVector<QXmlStreamAttribute>] ";
	{ // QVector file:QtCore/qvector.h line:63
		pybind11::class_<QVector<QXmlStreamAttribute>, std::shared_ptr<QVector<QXmlStreamAttribute>>> cl(M(""), "QVector_QXmlStreamAttribute_t", "");
		cl.def( pybind11::init( [](){ return new QVector<QXmlStreamAttribute>(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("size") );

		cl.def( pybind11::init<int, const class QXmlStreamAttribute &>(), pybind11::arg("size"), pybind11::arg("t") );

		cl.def( pybind11::init( [](QVector<QXmlStreamAttribute> const &o){ return new QVector<QXmlStreamAttribute>(o); } ) );
		cl.def("assign", (class QVector<class QXmlStreamAttribute> & (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &)) &QVector<QXmlStreamAttribute>::operator=, "C++: QVector<QXmlStreamAttribute>::operator=(const class QVector<class QXmlStreamAttribute> &) --> class QVector<class QXmlStreamAttribute> &", pybind11::return_value_policy::automatic, pybind11::arg("v"));
		cl.def("swap", (void (QVector<QXmlStreamAttribute>::*)(class QVector<class QXmlStreamAttribute> &)) &QVector<QXmlStreamAttribute>::swap, "C++: QVector<QXmlStreamAttribute>::swap(class QVector<class QXmlStreamAttribute> &) --> void", pybind11::arg("other"));
		cl.def("__eq__", (bool (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &) const) &QVector<QXmlStreamAttribute>::operator==, "C++: QVector<QXmlStreamAttribute>::operator==(const class QVector<class QXmlStreamAttribute> &) const --> bool", pybind11::arg("v"));
		cl.def("__ne__", (bool (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &) const) &QVector<QXmlStreamAttribute>::operator!=, "C++: QVector<QXmlStreamAttribute>::operator!=(const class QVector<class QXmlStreamAttribute> &) const --> bool", pybind11::arg("v"));
		cl.def("size", (int (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::size, "C++: QVector<QXmlStreamAttribute>::size() const --> int");
		cl.def("isEmpty", (bool (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::isEmpty, "C++: QVector<QXmlStreamAttribute>::isEmpty() const --> bool");
		cl.def("resize", (void (QVector<QXmlStreamAttribute>::*)(int)) &QVector<QXmlStreamAttribute>::resize, "C++: QVector<QXmlStreamAttribute>::resize(int) --> void", pybind11::arg("size"));
		cl.def("capacity", (int (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::capacity, "C++: QVector<QXmlStreamAttribute>::capacity() const --> int");
		cl.def("reserve", (void (QVector<QXmlStreamAttribute>::*)(int)) &QVector<QXmlStreamAttribute>::reserve, "C++: QVector<QXmlStreamAttribute>::reserve(int) --> void", pybind11::arg("size"));
		cl.def("squeeze", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::squeeze, "C++: QVector<QXmlStreamAttribute>::squeeze() --> void");
		cl.def("detach", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::detach, "C++: QVector<QXmlStreamAttribute>::detach() --> void");
		cl.def("isDetached", (bool (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::isDetached, "C++: QVector<QXmlStreamAttribute>::isDetached() const --> bool");
		cl.def("setSharable", (void (QVector<QXmlStreamAttribute>::*)(bool)) &QVector<QXmlStreamAttribute>::setSharable, "C++: QVector<QXmlStreamAttribute>::setSharable(bool) --> void", pybind11::arg("sharable"));
		cl.def("isSharedWith", (bool (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &) const) &QVector<QXmlStreamAttribute>::isSharedWith, "C++: QVector<QXmlStreamAttribute>::isSharedWith(const class QVector<class QXmlStreamAttribute> &) const --> bool", pybind11::arg("other"));
		cl.def("data", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::data, "C++: QVector<QXmlStreamAttribute>::data() --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("constData", (const class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::constData, "C++: QVector<QXmlStreamAttribute>::constData() const --> const class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("clear", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::clear, "C++: QVector<QXmlStreamAttribute>::clear() --> void");
		cl.def("at", (const class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)(int) const) &QVector<QXmlStreamAttribute>::at, "C++: QVector<QXmlStreamAttribute>::at(int) const --> const class QXmlStreamAttribute &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__getitem__", (class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)(int)) &QVector<QXmlStreamAttribute>::operator[], "C++: QVector<QXmlStreamAttribute>::operator[](int) --> class QXmlStreamAttribute &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("append", (void (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::append, "C++: QVector<QXmlStreamAttribute>::append(const class QXmlStreamAttribute &) --> void", pybind11::arg("t"));
		cl.def("append", (void (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &)) &QVector<QXmlStreamAttribute>::append, "C++: QVector<QXmlStreamAttribute>::append(const class QVector<class QXmlStreamAttribute> &) --> void", pybind11::arg("l"));
		cl.def("prepend", (void (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::prepend, "C++: QVector<QXmlStreamAttribute>::prepend(const class QXmlStreamAttribute &) --> void", pybind11::arg("t"));
		cl.def("insert", (void (QVector<QXmlStreamAttribute>::*)(int, const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::insert, "C++: QVector<QXmlStreamAttribute>::insert(int, const class QXmlStreamAttribute &) --> void", pybind11::arg("i"), pybind11::arg("t"));
		cl.def("insert", (void (QVector<QXmlStreamAttribute>::*)(int, int, const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::insert, "C++: QVector<QXmlStreamAttribute>::insert(int, int, const class QXmlStreamAttribute &) --> void", pybind11::arg("i"), pybind11::arg("n"), pybind11::arg("t"));
		cl.def("replace", (void (QVector<QXmlStreamAttribute>::*)(int, const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::replace, "C++: QVector<QXmlStreamAttribute>::replace(int, const class QXmlStreamAttribute &) --> void", pybind11::arg("i"), pybind11::arg("t"));
		cl.def("remove", (void (QVector<QXmlStreamAttribute>::*)(int)) &QVector<QXmlStreamAttribute>::remove, "C++: QVector<QXmlStreamAttribute>::remove(int) --> void", pybind11::arg("i"));
		cl.def("remove", (void (QVector<QXmlStreamAttribute>::*)(int, int)) &QVector<QXmlStreamAttribute>::remove, "C++: QVector<QXmlStreamAttribute>::remove(int, int) --> void", pybind11::arg("i"), pybind11::arg("n"));
		cl.def("removeFirst", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::removeFirst, "C++: QVector<QXmlStreamAttribute>::removeFirst() --> void");
		cl.def("removeLast", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::removeLast, "C++: QVector<QXmlStreamAttribute>::removeLast() --> void");
		cl.def("takeFirst", (class QXmlStreamAttribute (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::takeFirst, "C++: QVector<QXmlStreamAttribute>::takeFirst() --> class QXmlStreamAttribute");
		cl.def("takeLast", (class QXmlStreamAttribute (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::takeLast, "C++: QVector<QXmlStreamAttribute>::takeLast() --> class QXmlStreamAttribute");
		cl.def("fill", [](QVector<QXmlStreamAttribute> &o, const class QXmlStreamAttribute & a0) -> QVector<class QXmlStreamAttribute> & { return o.fill(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("t"));
		cl.def("fill", (class QVector<class QXmlStreamAttribute> & (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &, int)) &QVector<QXmlStreamAttribute>::fill, "C++: QVector<QXmlStreamAttribute>::fill(const class QXmlStreamAttribute &, int) --> class QVector<class QXmlStreamAttribute> &", pybind11::return_value_policy::automatic, pybind11::arg("t"), pybind11::arg("size"));
		cl.def("indexOf", [](QVector<QXmlStreamAttribute> const &o, const class QXmlStreamAttribute & a0) -> int { return o.indexOf(a0); }, "", pybind11::arg("t"));
		cl.def("indexOf", (int (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &, int) const) &QVector<QXmlStreamAttribute>::indexOf, "C++: QVector<QXmlStreamAttribute>::indexOf(const class QXmlStreamAttribute &, int) const --> int", pybind11::arg("t"), pybind11::arg("from"));
		cl.def("lastIndexOf", [](QVector<QXmlStreamAttribute> const &o, const class QXmlStreamAttribute & a0) -> int { return o.lastIndexOf(a0); }, "", pybind11::arg("t"));
		cl.def("lastIndexOf", (int (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &, int) const) &QVector<QXmlStreamAttribute>::lastIndexOf, "C++: QVector<QXmlStreamAttribute>::lastIndexOf(const class QXmlStreamAttribute &, int) const --> int", pybind11::arg("t"), pybind11::arg("from"));
		cl.def("contains", (bool (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &) const) &QVector<QXmlStreamAttribute>::contains, "C++: QVector<QXmlStreamAttribute>::contains(const class QXmlStreamAttribute &) const --> bool", pybind11::arg("t"));
		cl.def("count", (int (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &) const) &QVector<QXmlStreamAttribute>::count, "C++: QVector<QXmlStreamAttribute>::count(const class QXmlStreamAttribute &) const --> int", pybind11::arg("t"));
		cl.def("removeAt", (void (QVector<QXmlStreamAttribute>::*)(int)) &QVector<QXmlStreamAttribute>::removeAt, "C++: QVector<QXmlStreamAttribute>::removeAt(int) --> void", pybind11::arg("i"));
		cl.def("removeAll", (int (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::removeAll, "C++: QVector<QXmlStreamAttribute>::removeAll(const class QXmlStreamAttribute &) --> int", pybind11::arg("t"));
		cl.def("removeOne", (bool (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::removeOne, "C++: QVector<QXmlStreamAttribute>::removeOne(const class QXmlStreamAttribute &) --> bool", pybind11::arg("t"));
		cl.def("length", (int (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::length, "C++: QVector<QXmlStreamAttribute>::length() const --> int");
		cl.def("takeAt", (class QXmlStreamAttribute (QVector<QXmlStreamAttribute>::*)(int)) &QVector<QXmlStreamAttribute>::takeAt, "C++: QVector<QXmlStreamAttribute>::takeAt(int) --> class QXmlStreamAttribute", pybind11::arg("i"));
		cl.def("move", (void (QVector<QXmlStreamAttribute>::*)(int, int)) &QVector<QXmlStreamAttribute>::move, "C++: QVector<QXmlStreamAttribute>::move(int, int) --> void", pybind11::arg("from"), pybind11::arg("to"));
		cl.def("begin", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::begin, "C++: QVector<QXmlStreamAttribute>::begin() --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("cbegin", (const class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::cbegin, "C++: QVector<QXmlStreamAttribute>::cbegin() const --> const class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("constBegin", (const class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::constBegin, "C++: QVector<QXmlStreamAttribute>::constBegin() const --> const class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("end", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::end, "C++: QVector<QXmlStreamAttribute>::end() --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("cend", (const class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::cend, "C++: QVector<QXmlStreamAttribute>::cend() const --> const class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("constEnd", (const class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::constEnd, "C++: QVector<QXmlStreamAttribute>::constEnd() const --> const class QXmlStreamAttribute *", pybind11::return_value_policy::automatic);
		cl.def("insert", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)(class QXmlStreamAttribute *, int, const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::insert, "C++: QVector<QXmlStreamAttribute>::insert(class QXmlStreamAttribute *, int, const class QXmlStreamAttribute &) --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic, pybind11::arg("before"), pybind11::arg("n"), pybind11::arg("x"));
		cl.def("insert", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)(class QXmlStreamAttribute *, const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::insert, "C++: QVector<QXmlStreamAttribute>::insert(class QXmlStreamAttribute *, const class QXmlStreamAttribute &) --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic, pybind11::arg("before"), pybind11::arg("x"));
		cl.def("erase", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)(class QXmlStreamAttribute *, class QXmlStreamAttribute *)) &QVector<QXmlStreamAttribute>::erase, "C++: QVector<QXmlStreamAttribute>::erase(class QXmlStreamAttribute *, class QXmlStreamAttribute *) --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic, pybind11::arg("begin"), pybind11::arg("end"));
		cl.def("erase", (class QXmlStreamAttribute * (QVector<QXmlStreamAttribute>::*)(class QXmlStreamAttribute *)) &QVector<QXmlStreamAttribute>::erase, "C++: QVector<QXmlStreamAttribute>::erase(class QXmlStreamAttribute *) --> class QXmlStreamAttribute *", pybind11::return_value_policy::automatic, pybind11::arg("pos"));
		cl.def("count", (int (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::count, "C++: QVector<QXmlStreamAttribute>::count() const --> int");
		cl.def("first", (class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::first, "C++: QVector<QXmlStreamAttribute>::first() --> class QXmlStreamAttribute &", pybind11::return_value_policy::automatic);
		cl.def("constFirst", (const class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::constFirst, "C++: QVector<QXmlStreamAttribute>::constFirst() const --> const class QXmlStreamAttribute &", pybind11::return_value_policy::automatic);
		cl.def("last", (class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::last, "C++: QVector<QXmlStreamAttribute>::last() --> class QXmlStreamAttribute &", pybind11::return_value_policy::automatic);
		cl.def("constLast", (const class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::constLast, "C++: QVector<QXmlStreamAttribute>::constLast() const --> const class QXmlStreamAttribute &", pybind11::return_value_policy::automatic);
		cl.def("startsWith", (bool (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &) const) &QVector<QXmlStreamAttribute>::startsWith, "C++: QVector<QXmlStreamAttribute>::startsWith(const class QXmlStreamAttribute &) const --> bool", pybind11::arg("t"));
		cl.def("endsWith", (bool (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &) const) &QVector<QXmlStreamAttribute>::endsWith, "C++: QVector<QXmlStreamAttribute>::endsWith(const class QXmlStreamAttribute &) const --> bool", pybind11::arg("t"));
		cl.def("mid", [](QVector<QXmlStreamAttribute> const &o, int const & a0) -> QVector<class QXmlStreamAttribute> { return o.mid(a0); }, "", pybind11::arg("pos"));
		cl.def("mid", (class QVector<class QXmlStreamAttribute> (QVector<QXmlStreamAttribute>::*)(int, int) const) &QVector<QXmlStreamAttribute>::mid, "C++: QVector<QXmlStreamAttribute>::mid(int, int) const --> class QVector<class QXmlStreamAttribute>", pybind11::arg("pos"), pybind11::arg("len"));
		cl.def("value", (class QXmlStreamAttribute (QVector<QXmlStreamAttribute>::*)(int) const) &QVector<QXmlStreamAttribute>::value, "C++: QVector<QXmlStreamAttribute>::value(int) const --> class QXmlStreamAttribute", pybind11::arg("i"));
		cl.def("value", (class QXmlStreamAttribute (QVector<QXmlStreamAttribute>::*)(int, const class QXmlStreamAttribute &) const) &QVector<QXmlStreamAttribute>::value, "C++: QVector<QXmlStreamAttribute>::value(int, const class QXmlStreamAttribute &) const --> class QXmlStreamAttribute", pybind11::arg("i"), pybind11::arg("defaultValue"));
		cl.def("swapItemsAt", (void (QVector<QXmlStreamAttribute>::*)(int, int)) &QVector<QXmlStreamAttribute>::swapItemsAt, "C++: QVector<QXmlStreamAttribute>::swapItemsAt(int, int) --> void", pybind11::arg("i"), pybind11::arg("j"));
		cl.def("push_back", (void (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::push_back, "C++: QVector<QXmlStreamAttribute>::push_back(const class QXmlStreamAttribute &) --> void", pybind11::arg("t"));
		cl.def("push_front", (void (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::push_front, "C++: QVector<QXmlStreamAttribute>::push_front(const class QXmlStreamAttribute &) --> void", pybind11::arg("t"));
		cl.def("pop_back", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::pop_back, "C++: QVector<QXmlStreamAttribute>::pop_back() --> void");
		cl.def("pop_front", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::pop_front, "C++: QVector<QXmlStreamAttribute>::pop_front() --> void");
		cl.def("empty", (bool (QVector<QXmlStreamAttribute>::*)() const) &QVector<QXmlStreamAttribute>::empty, "C++: QVector<QXmlStreamAttribute>::empty() const --> bool");
		cl.def("front", (class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::front, "C++: QVector<QXmlStreamAttribute>::front() --> class QXmlStreamAttribute &", pybind11::return_value_policy::automatic);
		cl.def("back", (class QXmlStreamAttribute & (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::back, "C++: QVector<QXmlStreamAttribute>::back() --> class QXmlStreamAttribute &", pybind11::return_value_policy::automatic);
		cl.def("shrink_to_fit", (void (QVector<QXmlStreamAttribute>::*)()) &QVector<QXmlStreamAttribute>::shrink_to_fit, "C++: QVector<QXmlStreamAttribute>::shrink_to_fit() --> void");
		cl.def("__iadd__", (class QVector<class QXmlStreamAttribute> & (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &)) &QVector<QXmlStreamAttribute>::operator+=, "C++: QVector<QXmlStreamAttribute>::operator+=(const class QVector<class QXmlStreamAttribute> &) --> class QVector<class QXmlStreamAttribute> &", pybind11::return_value_policy::automatic, pybind11::arg("l"));
		cl.def("__add__", (class QVector<class QXmlStreamAttribute> (QVector<QXmlStreamAttribute>::*)(const class QVector<class QXmlStreamAttribute> &) const) &QVector<QXmlStreamAttribute>::operator+, "C++: QVector<QXmlStreamAttribute>::operator+(const class QVector<class QXmlStreamAttribute> &) const --> class QVector<class QXmlStreamAttribute>", pybind11::arg("l"));
		cl.def("__iadd__", (class QVector<class QXmlStreamAttribute> & (QVector<QXmlStreamAttribute>::*)(const class QXmlStreamAttribute &)) &QVector<QXmlStreamAttribute>::operator+=, "C++: QVector<QXmlStreamAttribute>::operator+=(const class QXmlStreamAttribute &) --> class QVector<class QXmlStreamAttribute> &", pybind11::return_value_policy::automatic, pybind11::arg("t"));
	}
}
