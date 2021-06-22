#include <QtCore/QString>
#undef slots
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/cast.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>
#include <pybind11/cast.h>


namespace pybind11 { namespace detail {
    template <> struct type_caster<QString>  : public type_caster_base<QString> {
        using base = type_caster_base<QString>;
    public:
        /**
         * This macro establishes the name 'inty' in
         * function signatures and declares a local variable
         * 'value' of type inty
         */
        PYBIND11_TYPE_CASTER(QString, _("QString"));

        /**
         * Conversion part 1 (Python->C++): convert a PyObject into a inty
         * instance or return false upon failure. The second argument
         * indicates whether implicit conversions should be applied.
         */
        bool load(handle src, bool) {
            /* Extract PyObject from handle */
            PyObject *source = src.ptr();
            if ( !PyUnicode_Check(source) ) 
                return false;
            ssize_t sz;
            const char *s = PyUnicode_AsUTF8AndSize(source, &sz);
            value = QString::fromUtf8(s, sz);
            /* Try converting into a Python integer value */
            /* Ensure return code was OK (to avoid out-of-range errors etc) */
            return true; //!(value.long_value == -1 && !PyErr_Occurred());
        }

        /**
         * Conversion part 2 (C++ -> Python): convert an inty instance into
         * a Python object. The second and third arguments are used to
         * indicate the return value policy and parent object (for
         * ``return_value_policy::reference_internal``) and are generally
         * ignored by implicit casters.
         */
        static handle cast(QString src, return_value_policy /* policy */, handle /* parent */) {
            QByteArray utf8 = src.toUtf8();
            return PyUnicode_FromStringAndSize(utf8.data(), utf8.size());
        }
    };
}} // namespace pybind11::detail