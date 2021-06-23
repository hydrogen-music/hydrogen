#include <QtCore/qabstractitemmodel.h> // +include_for_class
#include <QtCore/qarraydata.h> // QArrayDataPointerRef
#include <QtCore/qbytearray.h> // 
#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qbytearray.h> // QByteArrayDataPtr
#include <QtCore/qbytearray.h> // QByteRef
#include <QtCore/qchar.h> // 
#include <QtCore/qchar.h> // QChar
#include <QtCore/qchar.h> // QLatin1Char
#include <QtCore/qcoreevent.h> // 
#include <QtCore/qcoreevent.h> // QChildEvent
#include <QtCore/qcoreevent.h> // QEvent
#include <QtCore/qcoreevent.h> // QTimerEvent
#include <QtCore/qdatetime.h> // 
#include <QtCore/qdatetime.h> // QDate
#include <QtCore/qdatetime.h> // QDateTime
#include <QtCore/qdatetime.h> // QTime
#include <QtCore/qflags.h> // QFlag
#include <QtCore/qflags.h> // QFlags
#include <QtCore/qhash.h> // QHash
#include <QtCore/qiodevice.h> // 
#include <QtCore/qiodevice.h> // QIODevice
#include <QtCore/qjsonvalue.h> // 
#include <QtCore/qjsonvalue.h> // QJsonValue
#include <QtCore/qjsonvalue.h> // QJsonValueRef
#include <QtCore/qlist.h> // QList
#include <QtCore/qlocale.h> // 
#include <QtCore/qlocale.h> // QLocale
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::VariantData
#include <QtCore/qnamespace.h> // Qt::AspectRatioMode
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qnamespace.h> // Qt::ConnectionType
#include <QtCore/qnamespace.h> // Qt::DateFormat
#include <QtCore/qnamespace.h> // Qt::DayOfWeek
#include <QtCore/qnamespace.h> // Qt::DropAction
#include <QtCore/qnamespace.h> // Qt::Initialization
#include <QtCore/qnamespace.h> // Qt::ItemFlag
#include <QtCore/qnamespace.h> // Qt::LayoutDirection
#include <QtCore/qnamespace.h> // Qt::MatchFlag
#include <QtCore/qnamespace.h> // Qt::Orientation
#include <QtCore/qnamespace.h> // Qt::SortOrder
#include <QtCore/qnamespace.h> // Qt::SplitBehaviorFlags
#include <QtCore/qnamespace.h> // Qt::TimeSpec
#include <QtCore/qnamespace.h> // Qt::TimerType
#include <QtCore/qobject.h> // QObject
#include <QtCore/qobject.h> // QObjectUserData
#include <QtCore/qobjectdefs.h> // QGenericArgument
#include <QtCore/qobjectdefs.h> // QGenericReturnArgument
#include <QtCore/qobjectdefs.h> // QMetaObject
#include <QtCore/qpoint.h> // QPoint
#include <QtCore/qpoint.h> // QPointF
#include <QtCore/qregexp.h> // 
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // 
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qregularexpression.h> // QRegularExpressionMatch
#include <QtCore/qregularexpression.h> // QRegularExpressionMatchIterator
#include <QtCore/qsize.h> // QSize
#include <QtCore/qsize.h> // QSizeF
#include <QtCore/qstring.h> // 
#include <QtCore/qstring.h> // QCharRef
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstring.h> // QStringRef
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringliteral.h> // QStringDataPtr
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qurl.h> // 
#include <QtCore/qurl.h> // QUrl
#include <QtCore/qurl.h> // QUrlTwoFlags
#include <QtCore/quuid.h> // 
#include <QtCore/quuid.h> // QUuid
#include <QtCore/qvariant.h> // 
#include <QtCore/qvariant.h> // QVariant
#include <QtCore/qvector.h> // QVector
#include <chrono> // std::chrono::duration
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

// QAbstractItemModel file: line:168
struct PyCallBack_QAbstractItemModel : public QAbstractItemModel {
	using QAbstractItemModel::QAbstractItemModel;

	const struct QMetaObject * metaObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "metaObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct QMetaObject *>::value) {
				static pybind11::detail::override_caster_t<const struct QMetaObject *> caster;
				return pybind11::detail::cast_ref<const struct QMetaObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct QMetaObject *>(std::move(o));
		}
		return QAbstractItemModel::metaObject();
	}
	void * qt_metacast(const char * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "qt_metacast");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return QAbstractItemModel::qt_metacast(a0);
	}
	class QModelIndex index(int a0, int a1, const class QModelIndex & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "index");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<class QModelIndex>::value) {
				static pybind11::detail::override_caster_t<class QModelIndex> caster;
				return pybind11::detail::cast_ref<class QModelIndex>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QModelIndex>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QAbstractItemModel::index\"");
	}
	class QModelIndex parent(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "parent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class QModelIndex>::value) {
				static pybind11::detail::override_caster_t<class QModelIndex> caster;
				return pybind11::detail::cast_ref<class QModelIndex>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QModelIndex>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QAbstractItemModel::parent\"");
	}
	class QModelIndex sibling(int a0, int a1, const class QModelIndex & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "sibling");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<class QModelIndex>::value) {
				static pybind11::detail::override_caster_t<class QModelIndex> caster;
				return pybind11::detail::cast_ref<class QModelIndex>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QModelIndex>(std::move(o));
		}
		return QAbstractItemModel::sibling(a0, a1, a2);
	}
	int rowCount(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "rowCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QAbstractItemModel::rowCount\"");
	}
	int columnCount(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "columnCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QAbstractItemModel::columnCount\"");
	}
	bool hasChildren(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "hasChildren");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::hasChildren(a0);
	}
	class QVariant data(const class QModelIndex & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "data");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class QVariant>::value) {
				static pybind11::detail::override_caster_t<class QVariant> caster;
				return pybind11::detail::cast_ref<class QVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QVariant>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"QAbstractItemModel::data\"");
	}
	bool setData(const class QModelIndex & a0, const class QVariant & a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "setData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::setData(a0, a1, a2);
	}
	class QVariant headerData(int a0, enum Qt::Orientation a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "headerData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<class QVariant>::value) {
				static pybind11::detail::override_caster_t<class QVariant> caster;
				return pybind11::detail::cast_ref<class QVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QVariant>(std::move(o));
		}
		return QAbstractItemModel::headerData(a0, a1, a2);
	}
	bool setHeaderData(int a0, enum Qt::Orientation a1, const class QVariant & a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "setHeaderData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::setHeaderData(a0, a1, a2, a3);
	}
	bool canDropMimeData(const class QMimeData * a0, enum Qt::DropAction a1, int a2, int a3, const class QModelIndex & a4) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "canDropMimeData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::canDropMimeData(a0, a1, a2, a3, a4);
	}
	bool dropMimeData(const class QMimeData * a0, enum Qt::DropAction a1, int a2, int a3, const class QModelIndex & a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "dropMimeData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::dropMimeData(a0, a1, a2, a3, a4);
	}
	bool insertRows(int a0, int a1, const class QModelIndex & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "insertRows");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::insertRows(a0, a1, a2);
	}
	bool insertColumns(int a0, int a1, const class QModelIndex & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "insertColumns");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::insertColumns(a0, a1, a2);
	}
	bool removeRows(int a0, int a1, const class QModelIndex & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "removeRows");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::removeRows(a0, a1, a2);
	}
	bool removeColumns(int a0, int a1, const class QModelIndex & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "removeColumns");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::removeColumns(a0, a1, a2);
	}
	bool moveRows(const class QModelIndex & a0, int a1, int a2, const class QModelIndex & a3, int a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "moveRows");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::moveRows(a0, a1, a2, a3, a4);
	}
	bool moveColumns(const class QModelIndex & a0, int a1, int a2, const class QModelIndex & a3, int a4) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "moveColumns");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3, a4);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::moveColumns(a0, a1, a2, a3, a4);
	}
	void fetchMore(const class QModelIndex & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "fetchMore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QAbstractItemModel::fetchMore(a0);
	}
	bool canFetchMore(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "canFetchMore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::canFetchMore(a0);
	}
	void sort(int a0, enum Qt::SortOrder a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "sort");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QAbstractItemModel::sort(a0, a1);
	}
	class QModelIndex buddy(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "buddy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class QModelIndex>::value) {
				static pybind11::detail::override_caster_t<class QModelIndex> caster;
				return pybind11::detail::cast_ref<class QModelIndex>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QModelIndex>(std::move(o));
		}
		return QAbstractItemModel::buddy(a0);
	}
	class QSize span(const class QModelIndex & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "span");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class QSize>::value) {
				static pybind11::detail::override_caster_t<class QSize> caster;
				return pybind11::detail::cast_ref<class QSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class QSize>(std::move(o));
		}
		return QAbstractItemModel::span(a0);
	}
	bool submit() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "submit");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return QAbstractItemModel::submit();
	}
	void revert() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "revert");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QAbstractItemModel::revert();
	}
	void timerEvent(class QTimerEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "timerEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::timerEvent(a0);
	}
	void childEvent(class QChildEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "childEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::childEvent(a0);
	}
	void connectNotify(const class QMetaMethod & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "connectNotify");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::connectNotify(a0);
	}
	void disconnectNotify(const class QMetaMethod & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const QAbstractItemModel *>(this), "disconnectNotify");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return QObject::disconnectNotify(a0);
	}
};

void bind_unknown_unknown_2(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B367_[QModelIndex] ";
	{ // QModelIndex file: line:56
		pybind11::class_<QModelIndex, std::shared_ptr<QModelIndex>> cl(M(""), "QModelIndex", "");
		cl.def( pybind11::init( [](){ return new QModelIndex(); } ) );
		cl.def( pybind11::init( [](QModelIndex const &o){ return new QModelIndex(o); } ) );
		cl.def("row", (int (QModelIndex::*)() const) &QModelIndex::row, "C++: QModelIndex::row() const --> int");
		cl.def("column", (int (QModelIndex::*)() const) &QModelIndex::column, "C++: QModelIndex::column() const --> int");
		cl.def("internalId", (unsigned long long (QModelIndex::*)() const) &QModelIndex::internalId, "C++: QModelIndex::internalId() const --> unsigned long long");
		cl.def("internalPointer", (void * (QModelIndex::*)() const) &QModelIndex::internalPointer, "C++: QModelIndex::internalPointer() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("parent", (class QModelIndex (QModelIndex::*)() const) &QModelIndex::parent, "C++: QModelIndex::parent() const --> class QModelIndex");
		cl.def("sibling", (class QModelIndex (QModelIndex::*)(int, int) const) &QModelIndex::sibling, "C++: QModelIndex::sibling(int, int) const --> class QModelIndex", pybind11::arg("row"), pybind11::arg("column"));
		cl.def("siblingAtColumn", (class QModelIndex (QModelIndex::*)(int) const) &QModelIndex::siblingAtColumn, "C++: QModelIndex::siblingAtColumn(int) const --> class QModelIndex", pybind11::arg("column"));
		cl.def("siblingAtRow", (class QModelIndex (QModelIndex::*)(int) const) &QModelIndex::siblingAtRow, "C++: QModelIndex::siblingAtRow(int) const --> class QModelIndex", pybind11::arg("row"));
		cl.def("child", (class QModelIndex (QModelIndex::*)(int, int) const) &QModelIndex::child, "C++: QModelIndex::child(int, int) const --> class QModelIndex", pybind11::arg("row"), pybind11::arg("column"));
		cl.def("data", [](QModelIndex const &o) -> QVariant { return o.data(); }, "");
		cl.def("data", (class QVariant (QModelIndex::*)(int) const) &QModelIndex::data, "C++: QModelIndex::data(int) const --> class QVariant", pybind11::arg("role"));
		cl.def("model", (const class QAbstractItemModel * (QModelIndex::*)() const) &QModelIndex::model, "C++: QModelIndex::model() const --> const class QAbstractItemModel *", pybind11::return_value_policy::automatic);
		cl.def("isValid", (bool (QModelIndex::*)() const) &QModelIndex::isValid, "C++: QModelIndex::isValid() const --> bool");
		cl.def("__eq__", (bool (QModelIndex::*)(const class QModelIndex &) const) &QModelIndex::operator==, "C++: QModelIndex::operator==(const class QModelIndex &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QModelIndex::*)(const class QModelIndex &) const) &QModelIndex::operator!=, "C++: QModelIndex::operator!=(const class QModelIndex &) const --> bool", pybind11::arg("other"));
		cl.def("assign", (class QModelIndex & (QModelIndex::*)(const class QModelIndex &)) &QModelIndex::operator=, "C++: QModelIndex::operator=(const class QModelIndex &) --> class QModelIndex &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	std::cout << "B368_[QTypeInfo<QModelIndex>] ";
	std::cout << "B369_[unsigned int qHash(const class QPersistentModelIndex &, unsigned int)] ";
	std::cout << "B370_[QPersistentModelIndex] ";
	{ // QPersistentModelIndex file: line:108
		pybind11::class_<QPersistentModelIndex, std::shared_ptr<QPersistentModelIndex>> cl(M(""), "QPersistentModelIndex", "");
		cl.def( pybind11::init( [](){ return new QPersistentModelIndex(); } ) );
		cl.def( pybind11::init<const class QModelIndex &>(), pybind11::arg("index") );

		cl.def( pybind11::init( [](QPersistentModelIndex const &o){ return new QPersistentModelIndex(o); } ) );
		cl.def("__eq__", (bool (QPersistentModelIndex::*)(const class QPersistentModelIndex &) const) &QPersistentModelIndex::operator==, "C++: QPersistentModelIndex::operator==(const class QPersistentModelIndex &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QPersistentModelIndex::*)(const class QPersistentModelIndex &) const) &QPersistentModelIndex::operator!=, "C++: QPersistentModelIndex::operator!=(const class QPersistentModelIndex &) const --> bool", pybind11::arg("other"));
		cl.def("assign", (class QPersistentModelIndex & (QPersistentModelIndex::*)(const class QPersistentModelIndex &)) &QPersistentModelIndex::operator=, "C++: QPersistentModelIndex::operator=(const class QPersistentModelIndex &) --> class QPersistentModelIndex &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("swap", (void (QPersistentModelIndex::*)(class QPersistentModelIndex &)) &QPersistentModelIndex::swap, "C++: QPersistentModelIndex::swap(class QPersistentModelIndex &) --> void", pybind11::arg("other"));
		cl.def("__eq__", (bool (QPersistentModelIndex::*)(const class QModelIndex &) const) &QPersistentModelIndex::operator==, "C++: QPersistentModelIndex::operator==(const class QModelIndex &) const --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (QPersistentModelIndex::*)(const class QModelIndex &) const) &QPersistentModelIndex::operator!=, "C++: QPersistentModelIndex::operator!=(const class QModelIndex &) const --> bool", pybind11::arg("other"));
		cl.def("assign", (class QPersistentModelIndex & (QPersistentModelIndex::*)(const class QModelIndex &)) &QPersistentModelIndex::operator=, "C++: QPersistentModelIndex::operator=(const class QModelIndex &) --> class QPersistentModelIndex &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("row", (int (QPersistentModelIndex::*)() const) &QPersistentModelIndex::row, "C++: QPersistentModelIndex::row() const --> int");
		cl.def("column", (int (QPersistentModelIndex::*)() const) &QPersistentModelIndex::column, "C++: QPersistentModelIndex::column() const --> int");
		cl.def("internalPointer", (void * (QPersistentModelIndex::*)() const) &QPersistentModelIndex::internalPointer, "C++: QPersistentModelIndex::internalPointer() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("internalId", (unsigned long long (QPersistentModelIndex::*)() const) &QPersistentModelIndex::internalId, "C++: QPersistentModelIndex::internalId() const --> unsigned long long");
		cl.def("parent", (class QModelIndex (QPersistentModelIndex::*)() const) &QPersistentModelIndex::parent, "C++: QPersistentModelIndex::parent() const --> class QModelIndex");
		cl.def("sibling", (class QModelIndex (QPersistentModelIndex::*)(int, int) const) &QPersistentModelIndex::sibling, "C++: QPersistentModelIndex::sibling(int, int) const --> class QModelIndex", pybind11::arg("row"), pybind11::arg("column"));
		cl.def("child", (class QModelIndex (QPersistentModelIndex::*)(int, int) const) &QPersistentModelIndex::child, "C++: QPersistentModelIndex::child(int, int) const --> class QModelIndex", pybind11::arg("row"), pybind11::arg("column"));
		cl.def("data", [](QPersistentModelIndex const &o) -> QVariant { return o.data(); }, "");
		cl.def("data", (class QVariant (QPersistentModelIndex::*)(int) const) &QPersistentModelIndex::data, "C++: QPersistentModelIndex::data(int) const --> class QVariant", pybind11::arg("role"));
		cl.def("model", (const class QAbstractItemModel * (QPersistentModelIndex::*)() const) &QPersistentModelIndex::model, "C++: QPersistentModelIndex::model() const --> const class QAbstractItemModel *", pybind11::return_value_policy::automatic);
		cl.def("isValid", (bool (QPersistentModelIndex::*)() const) &QPersistentModelIndex::isValid, "C++: QPersistentModelIndex::isValid() const --> bool");
	}
	std::cout << "B371_[QTypeInfo<QPersistentModelIndex>] ";
	std::cout << "B372_[void swap(class QPersistentModelIndex &, class QPersistentModelIndex &)] ";
	std::cout << "B373_[QAbstractItemModel] ";
	{ // QAbstractItemModel file: line:168
		pybind11::class_<QAbstractItemModel, std::shared_ptr<QAbstractItemModel>, PyCallBack_QAbstractItemModel, QObject> cl(M(""), "QAbstractItemModel", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_QAbstractItemModel(); } ), "doc");
		cl.def( pybind11::init<class QObject *>(), pybind11::arg("parent") );


		pybind11::enum_<QAbstractItemModel::LayoutChangeHint>(cl, "LayoutChangeHint", pybind11::arithmetic(), "")
			.value("NoLayoutChangeHint", QAbstractItemModel::NoLayoutChangeHint)
			.value("VerticalSortHint", QAbstractItemModel::VerticalSortHint)
			.value("HorizontalSortHint", QAbstractItemModel::HorizontalSortHint)
			.export_values();


		pybind11::enum_<QAbstractItemModel::CheckIndexOption>(cl, "CheckIndexOption", "")
			.value("NoOption", QAbstractItemModel::CheckIndexOption::NoOption)
			.value("IndexIsValid", QAbstractItemModel::CheckIndexOption::IndexIsValid)
			.value("DoNotUseParent", QAbstractItemModel::CheckIndexOption::DoNotUseParent)
			.value("ParentIsInvalid", QAbstractItemModel::CheckIndexOption::ParentIsInvalid);

		cl.def("parent", [](QAbstractItemModel const &o) -> QObject * { return o.parent(); }, "", pybind11::return_value_policy::automatic);
		cl.def("metaObject", (const struct QMetaObject * (QAbstractItemModel::*)() const) &QAbstractItemModel::metaObject, "C++: QAbstractItemModel::metaObject() const --> const struct QMetaObject *", pybind11::return_value_policy::automatic);
		cl.def("qt_metacast", (void * (QAbstractItemModel::*)(const char *)) &QAbstractItemModel::qt_metacast, "C++: QAbstractItemModel::qt_metacast(const char *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg(""));
		cl.def_static("tr", [](const char * a0) -> QString { return QAbstractItemModel::tr(a0); }, "", pybind11::arg("s"));
		cl.def_static("tr", [](const char * a0, const char * a1) -> QString { return QAbstractItemModel::tr(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("tr", (class QString (*)(const char *, const char *, int)) &QAbstractItemModel::tr, "C++: QAbstractItemModel::tr(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def_static("trUtf8", [](const char * a0) -> QString { return QAbstractItemModel::trUtf8(a0); }, "", pybind11::arg("s"));
		cl.def_static("trUtf8", [](const char * a0, const char * a1) -> QString { return QAbstractItemModel::trUtf8(a0, a1); }, "", pybind11::arg("s"), pybind11::arg("c"));
		cl.def_static("trUtf8", (class QString (*)(const char *, const char *, int)) &QAbstractItemModel::trUtf8, "C++: QAbstractItemModel::trUtf8(const char *, const char *, int) --> class QString", pybind11::arg("s"), pybind11::arg("c"), pybind11::arg("n"));
		cl.def("hasIndex", [](QAbstractItemModel const &o, int const & a0, int const & a1) -> bool { return o.hasIndex(a0, a1); }, "", pybind11::arg("row"), pybind11::arg("column"));
		cl.def("hasIndex", (bool (QAbstractItemModel::*)(int, int, const class QModelIndex &) const) &QAbstractItemModel::hasIndex, "C++: QAbstractItemModel::hasIndex(int, int, const class QModelIndex &) const --> bool", pybind11::arg("row"), pybind11::arg("column"), pybind11::arg("parent"));
		cl.def("index", [](QAbstractItemModel const &o, int const & a0, int const & a1) -> QModelIndex { return o.index(a0, a1); }, "", pybind11::arg("row"), pybind11::arg("column"));
		cl.def("index", (class QModelIndex (QAbstractItemModel::*)(int, int, const class QModelIndex &) const) &QAbstractItemModel::index, "C++: QAbstractItemModel::index(int, int, const class QModelIndex &) const --> class QModelIndex", pybind11::arg("row"), pybind11::arg("column"), pybind11::arg("parent"));
		cl.def("parent", (class QModelIndex (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::parent, "C++: QAbstractItemModel::parent(const class QModelIndex &) const --> class QModelIndex", pybind11::arg("child"));
		cl.def("sibling", (class QModelIndex (QAbstractItemModel::*)(int, int, const class QModelIndex &) const) &QAbstractItemModel::sibling, "C++: QAbstractItemModel::sibling(int, int, const class QModelIndex &) const --> class QModelIndex", pybind11::arg("row"), pybind11::arg("column"), pybind11::arg("idx"));
		cl.def("rowCount", [](QAbstractItemModel const &o) -> int { return o.rowCount(); }, "");
		cl.def("rowCount", (int (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::rowCount, "C++: QAbstractItemModel::rowCount(const class QModelIndex &) const --> int", pybind11::arg("parent"));
		cl.def("columnCount", [](QAbstractItemModel const &o) -> int { return o.columnCount(); }, "");
		cl.def("columnCount", (int (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::columnCount, "C++: QAbstractItemModel::columnCount(const class QModelIndex &) const --> int", pybind11::arg("parent"));
		cl.def("hasChildren", [](QAbstractItemModel const &o) -> bool { return o.hasChildren(); }, "");
		cl.def("hasChildren", (bool (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::hasChildren, "C++: QAbstractItemModel::hasChildren(const class QModelIndex &) const --> bool", pybind11::arg("parent"));
		cl.def("data", [](QAbstractItemModel const &o, const class QModelIndex & a0) -> QVariant { return o.data(a0); }, "", pybind11::arg("index"));
		cl.def("data", (class QVariant (QAbstractItemModel::*)(const class QModelIndex &, int) const) &QAbstractItemModel::data, "C++: QAbstractItemModel::data(const class QModelIndex &, int) const --> class QVariant", pybind11::arg("index"), pybind11::arg("role"));
		cl.def("setData", [](QAbstractItemModel &o, const class QModelIndex & a0, const class QVariant & a1) -> bool { return o.setData(a0, a1); }, "", pybind11::arg("index"), pybind11::arg("value"));
		cl.def("setData", (bool (QAbstractItemModel::*)(const class QModelIndex &, const class QVariant &, int)) &QAbstractItemModel::setData, "C++: QAbstractItemModel::setData(const class QModelIndex &, const class QVariant &, int) --> bool", pybind11::arg("index"), pybind11::arg("value"), pybind11::arg("role"));
		cl.def("headerData", [](QAbstractItemModel const &o, int const & a0, enum Qt::Orientation const & a1) -> QVariant { return o.headerData(a0, a1); }, "", pybind11::arg("section"), pybind11::arg("orientation"));
		cl.def("headerData", (class QVariant (QAbstractItemModel::*)(int, enum Qt::Orientation, int) const) &QAbstractItemModel::headerData, "C++: QAbstractItemModel::headerData(int, enum Qt::Orientation, int) const --> class QVariant", pybind11::arg("section"), pybind11::arg("orientation"), pybind11::arg("role"));
		cl.def("setHeaderData", [](QAbstractItemModel &o, int const & a0, enum Qt::Orientation const & a1, const class QVariant & a2) -> bool { return o.setHeaderData(a0, a1, a2); }, "", pybind11::arg("section"), pybind11::arg("orientation"), pybind11::arg("value"));
		cl.def("setHeaderData", (bool (QAbstractItemModel::*)(int, enum Qt::Orientation, const class QVariant &, int)) &QAbstractItemModel::setHeaderData, "C++: QAbstractItemModel::setHeaderData(int, enum Qt::Orientation, const class QVariant &, int) --> bool", pybind11::arg("section"), pybind11::arg("orientation"), pybind11::arg("value"), pybind11::arg("role"));
		cl.def("canDropMimeData", (bool (QAbstractItemModel::*)(const class QMimeData *, enum Qt::DropAction, int, int, const class QModelIndex &) const) &QAbstractItemModel::canDropMimeData, "C++: QAbstractItemModel::canDropMimeData(const class QMimeData *, enum Qt::DropAction, int, int, const class QModelIndex &) const --> bool", pybind11::arg("data"), pybind11::arg("action"), pybind11::arg("row"), pybind11::arg("column"), pybind11::arg("parent"));
		cl.def("dropMimeData", (bool (QAbstractItemModel::*)(const class QMimeData *, enum Qt::DropAction, int, int, const class QModelIndex &)) &QAbstractItemModel::dropMimeData, "C++: QAbstractItemModel::dropMimeData(const class QMimeData *, enum Qt::DropAction, int, int, const class QModelIndex &) --> bool", pybind11::arg("data"), pybind11::arg("action"), pybind11::arg("row"), pybind11::arg("column"), pybind11::arg("parent"));
		cl.def("insertRows", [](QAbstractItemModel &o, int const & a0, int const & a1) -> bool { return o.insertRows(a0, a1); }, "", pybind11::arg("row"), pybind11::arg("count"));
		cl.def("insertRows", (bool (QAbstractItemModel::*)(int, int, const class QModelIndex &)) &QAbstractItemModel::insertRows, "C++: QAbstractItemModel::insertRows(int, int, const class QModelIndex &) --> bool", pybind11::arg("row"), pybind11::arg("count"), pybind11::arg("parent"));
		cl.def("insertColumns", [](QAbstractItemModel &o, int const & a0, int const & a1) -> bool { return o.insertColumns(a0, a1); }, "", pybind11::arg("column"), pybind11::arg("count"));
		cl.def("insertColumns", (bool (QAbstractItemModel::*)(int, int, const class QModelIndex &)) &QAbstractItemModel::insertColumns, "C++: QAbstractItemModel::insertColumns(int, int, const class QModelIndex &) --> bool", pybind11::arg("column"), pybind11::arg("count"), pybind11::arg("parent"));
		cl.def("removeRows", [](QAbstractItemModel &o, int const & a0, int const & a1) -> bool { return o.removeRows(a0, a1); }, "", pybind11::arg("row"), pybind11::arg("count"));
		cl.def("removeRows", (bool (QAbstractItemModel::*)(int, int, const class QModelIndex &)) &QAbstractItemModel::removeRows, "C++: QAbstractItemModel::removeRows(int, int, const class QModelIndex &) --> bool", pybind11::arg("row"), pybind11::arg("count"), pybind11::arg("parent"));
		cl.def("removeColumns", [](QAbstractItemModel &o, int const & a0, int const & a1) -> bool { return o.removeColumns(a0, a1); }, "", pybind11::arg("column"), pybind11::arg("count"));
		cl.def("removeColumns", (bool (QAbstractItemModel::*)(int, int, const class QModelIndex &)) &QAbstractItemModel::removeColumns, "C++: QAbstractItemModel::removeColumns(int, int, const class QModelIndex &) --> bool", pybind11::arg("column"), pybind11::arg("count"), pybind11::arg("parent"));
		cl.def("moveRows", (bool (QAbstractItemModel::*)(const class QModelIndex &, int, int, const class QModelIndex &, int)) &QAbstractItemModel::moveRows, "C++: QAbstractItemModel::moveRows(const class QModelIndex &, int, int, const class QModelIndex &, int) --> bool", pybind11::arg("sourceParent"), pybind11::arg("sourceRow"), pybind11::arg("count"), pybind11::arg("destinationParent"), pybind11::arg("destinationChild"));
		cl.def("moveColumns", (bool (QAbstractItemModel::*)(const class QModelIndex &, int, int, const class QModelIndex &, int)) &QAbstractItemModel::moveColumns, "C++: QAbstractItemModel::moveColumns(const class QModelIndex &, int, int, const class QModelIndex &, int) --> bool", pybind11::arg("sourceParent"), pybind11::arg("sourceColumn"), pybind11::arg("count"), pybind11::arg("destinationParent"), pybind11::arg("destinationChild"));
		cl.def("insertRow", [](QAbstractItemModel &o, int const & a0) -> bool { return o.insertRow(a0); }, "", pybind11::arg("row"));
		cl.def("insertRow", (bool (QAbstractItemModel::*)(int, const class QModelIndex &)) &QAbstractItemModel::insertRow, "C++: QAbstractItemModel::insertRow(int, const class QModelIndex &) --> bool", pybind11::arg("row"), pybind11::arg("parent"));
		cl.def("insertColumn", [](QAbstractItemModel &o, int const & a0) -> bool { return o.insertColumn(a0); }, "", pybind11::arg("column"));
		cl.def("insertColumn", (bool (QAbstractItemModel::*)(int, const class QModelIndex &)) &QAbstractItemModel::insertColumn, "C++: QAbstractItemModel::insertColumn(int, const class QModelIndex &) --> bool", pybind11::arg("column"), pybind11::arg("parent"));
		cl.def("removeRow", [](QAbstractItemModel &o, int const & a0) -> bool { return o.removeRow(a0); }, "", pybind11::arg("row"));
		cl.def("removeRow", (bool (QAbstractItemModel::*)(int, const class QModelIndex &)) &QAbstractItemModel::removeRow, "C++: QAbstractItemModel::removeRow(int, const class QModelIndex &) --> bool", pybind11::arg("row"), pybind11::arg("parent"));
		cl.def("removeColumn", [](QAbstractItemModel &o, int const & a0) -> bool { return o.removeColumn(a0); }, "", pybind11::arg("column"));
		cl.def("removeColumn", (bool (QAbstractItemModel::*)(int, const class QModelIndex &)) &QAbstractItemModel::removeColumn, "C++: QAbstractItemModel::removeColumn(int, const class QModelIndex &) --> bool", pybind11::arg("column"), pybind11::arg("parent"));
		cl.def("moveRow", (bool (QAbstractItemModel::*)(const class QModelIndex &, int, const class QModelIndex &, int)) &QAbstractItemModel::moveRow, "C++: QAbstractItemModel::moveRow(const class QModelIndex &, int, const class QModelIndex &, int) --> bool", pybind11::arg("sourceParent"), pybind11::arg("sourceRow"), pybind11::arg("destinationParent"), pybind11::arg("destinationChild"));
		cl.def("moveColumn", (bool (QAbstractItemModel::*)(const class QModelIndex &, int, const class QModelIndex &, int)) &QAbstractItemModel::moveColumn, "C++: QAbstractItemModel::moveColumn(const class QModelIndex &, int, const class QModelIndex &, int) --> bool", pybind11::arg("sourceParent"), pybind11::arg("sourceColumn"), pybind11::arg("destinationParent"), pybind11::arg("destinationChild"));
		cl.def("fetchMore", (void (QAbstractItemModel::*)(const class QModelIndex &)) &QAbstractItemModel::fetchMore, "C++: QAbstractItemModel::fetchMore(const class QModelIndex &) --> void", pybind11::arg("parent"));
		cl.def("canFetchMore", (bool (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::canFetchMore, "C++: QAbstractItemModel::canFetchMore(const class QModelIndex &) const --> bool", pybind11::arg("parent"));
		cl.def("sort", [](QAbstractItemModel &o, int const & a0) -> void { return o.sort(a0); }, "", pybind11::arg("column"));
		cl.def("sort", (void (QAbstractItemModel::*)(int, enum Qt::SortOrder)) &QAbstractItemModel::sort, "C++: QAbstractItemModel::sort(int, enum Qt::SortOrder) --> void", pybind11::arg("column"), pybind11::arg("order"));
		cl.def("buddy", (class QModelIndex (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::buddy, "C++: QAbstractItemModel::buddy(const class QModelIndex &) const --> class QModelIndex", pybind11::arg("index"));
		cl.def("span", (class QSize (QAbstractItemModel::*)(const class QModelIndex &) const) &QAbstractItemModel::span, "C++: QAbstractItemModel::span(const class QModelIndex &) const --> class QSize", pybind11::arg("index"));
		cl.def("dataChanged", [](QAbstractItemModel &o, const class QModelIndex & a0, const class QModelIndex & a1) -> void { return o.dataChanged(a0, a1); }, "", pybind11::arg("topLeft"), pybind11::arg("bottomRight"));
		cl.def("dataChanged", (void (QAbstractItemModel::*)(const class QModelIndex &, const class QModelIndex &, const class QVector<int> &)) &QAbstractItemModel::dataChanged, "C++: QAbstractItemModel::dataChanged(const class QModelIndex &, const class QModelIndex &, const class QVector<int> &) --> void", pybind11::arg("topLeft"), pybind11::arg("bottomRight"), pybind11::arg("roles"));
		cl.def("headerDataChanged", (void (QAbstractItemModel::*)(enum Qt::Orientation, int, int)) &QAbstractItemModel::headerDataChanged, "C++: QAbstractItemModel::headerDataChanged(enum Qt::Orientation, int, int) --> void", pybind11::arg("orientation"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("submit", (bool (QAbstractItemModel::*)()) &QAbstractItemModel::submit, "C++: QAbstractItemModel::submit() --> bool");
		cl.def("revert", (void (QAbstractItemModel::*)()) &QAbstractItemModel::revert, "C++: QAbstractItemModel::revert() --> void");
	}
}
