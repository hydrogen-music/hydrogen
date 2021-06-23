#include <QtCore/qbytearray.h> // QByteArray
#include <QtCore/qhash.h> // QHash
#include <QtCore/qlist.h> // QList
#include <QtCore/qmap.h> // QMap
#include <QtCore/qmap.h> // QMapNode
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::CapabilitiesImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::ContainerAPI
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::ContainerCapabilitiesImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::ContainerCapability
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::IteratorCapability
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::IteratorOwner
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::IteratorOwnerCommon
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QPairVariantInterfaceImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::QSequentialIterableImpl
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::VariantData
#include <QtCore/qmetatype.h> // QtMetaTypePrivate::VectorBoolElements
#include <QtCore/qnamespace.h> // Qt::CaseSensitivity
#include <QtCore/qregexp.h> // QRegExp
#include <QtCore/qregularexpression.h> // QRegularExpression
#include <QtCore/qstring.h> // QLatin1String
#include <QtCore/qstring.h> // QString
#include <QtCore/qstringlist.h> // QStringList
#include <QtCore/qstringview.h> // QStringView
#include <QtCore/qvariant.h> // QVariant
#include <iostream> // --trace
#include <iterator> // std::random_access_iterator_tag
#include <sstream> // __str__
#include <vector> // std::_Bit_const_iterator
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

void bind_QtCore_qmetatype(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	std::cout << "B317_[QtMetaTypePrivate::VariantData] ";
	{ // QtMetaTypePrivate::VariantData file:QtCore/qmetatype.h line:855
		pybind11::class_<QtMetaTypePrivate::VariantData, std::shared_ptr<QtMetaTypePrivate::VariantData>> cl(M("QtMetaTypePrivate"), "VariantData", "");
		cl.def( pybind11::init<const int, const void *, const unsigned int>(), pybind11::arg("metaTypeId_"), pybind11::arg("data_"), pybind11::arg("flags_") );

		cl.def( pybind11::init( [](QtMetaTypePrivate::VariantData const &o){ return new QtMetaTypePrivate::VariantData(o); } ) );
		cl.def_readonly("metaTypeId", &QtMetaTypePrivate::VariantData::metaTypeId);
		cl.def_readonly("flags", &QtMetaTypePrivate::VariantData::flags);
	}
	std::cout << "B318_[QtMetaTypePrivate::IteratorOwnerCommon<std::_Bit_const_iterator>] ";
	std::cout << "B319_[QtMetaTypePrivate::IteratorOwnerCommon<QList<QVariant>::const_iterator>] ";
	std::cout << "B320_[QtMetaTypePrivate::IteratorOwnerCommon<QList<QString>::const_iterator>] ";
	std::cout << "B321_[QtMetaTypePrivate::IteratorOwnerCommon<QList<QByteArray>::const_iterator>] ";
	std::cout << "B322_[QtMetaTypePrivate::IteratorOwnerCommon<QMap<QString, QVariant>::const_iterator>] ";
	std::cout << "B323_[QtMetaTypePrivate::IteratorOwnerCommon<QHash<QString, QVariant>::const_iterator>] ";
	std::cout << "B324_[QtMetaTypePrivate::IteratorOwnerCommon<QList<QModelIndex>::const_iterator>] ";
	std::cout << "B325_[QtMetaTypePrivate::IteratorOwner<QList<QVariant>::const_iterator>] ";
	std::cout << "B326_[QtMetaTypePrivate::IteratorOwner<QList<QString>::const_iterator>] ";
	std::cout << "B327_[QtMetaTypePrivate::IteratorOwner<QList<QByteArray>::const_iterator>] ";
	std::cout << "B328_[QtMetaTypePrivate::IteratorOwner<QMap<QString, QVariant>::const_iterator>] ";
	std::cout << "B329_[QtMetaTypePrivate::IteratorOwner<QHash<QString, QVariant>::const_iterator>] ";
	std::cout << "B330_[QtMetaTypePrivate::IteratorOwner<QList<QModelIndex>::const_iterator>] ";
	std::cout << "B331_[QtMetaTypePrivate::VectorBoolElements] ";
	std::cout << "B332_[QtMetaTypePrivate::IteratorOwner<std::_Bit_const_iterator>] ";
	std::cout << "B333_[QtMetaTypePrivate::IteratorCapability] ";
	std::cout << "B334_[QtMetaTypePrivate::ContainerCapability] ";
	std::cout << "B335_[QtMetaTypePrivate::ContainerCapabilitiesImpl<QList<QVariant>,void>] ";
	std::cout << "B336_[QtMetaTypePrivate::ContainerCapabilitiesImpl<QStringList,void>] ";
	std::cout << "B337_[QtMetaTypePrivate::ContainerCapabilitiesImpl<QList<QByteArray>,void>] ";
	std::cout << "B338_[QtMetaTypePrivate::ContainerCapabilitiesImpl<QList<QModelIndex>,void>] ";
	std::cout << "B339_[QtMetaTypePrivate::CapabilitiesImpl<QList<QVariant>,std::random_access_iterator_tag>] ";
	std::cout << "B340_[QtMetaTypePrivate::CapabilitiesImpl<QStringList,std::random_access_iterator_tag>] ";
	std::cout << "B341_[QtMetaTypePrivate::CapabilitiesImpl<QList<QByteArray>,std::random_access_iterator_tag>] ";
	std::cout << "B342_[QtMetaTypePrivate::CapabilitiesImpl<QList<QModelIndex>,std::random_access_iterator_tag>] ";
	std::cout << "B343_[QtMetaTypePrivate::ContainerAPI<QList<QVariant>>] ";
	std::cout << "B344_[QtMetaTypePrivate::ContainerAPI<QStringList>] ";
	std::cout << "B345_[QtMetaTypePrivate::ContainerAPI<QList<QByteArray>>] ";
	std::cout << "B346_[QtMetaTypePrivate::ContainerAPI<QList<QModelIndex>>] ";
	std::cout << "B347_[QtMetaTypePrivate::QSequentialIterableImpl] ";
}
