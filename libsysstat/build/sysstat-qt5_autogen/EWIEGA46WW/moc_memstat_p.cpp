/****************************************************************************
** Meta object code from reading C++ file 'memstat_p.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../memstat_p.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'memstat_p.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SysStat__MemStatPrivate_t {
    QByteArrayData data[9];
    char stringdata0[82];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SysStat__MemStatPrivate_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SysStat__MemStatPrivate_t qt_meta_stringdata_SysStat__MemStatPrivate = {
    {
QT_MOC_LITERAL(0, 0, 23), // "SysStat::MemStatPrivate"
QT_MOC_LITERAL(1, 24, 12), // "memoryUpdate"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 4), // "apps"
QT_MOC_LITERAL(4, 43, 7), // "buffers"
QT_MOC_LITERAL(5, 51, 6), // "cached"
QT_MOC_LITERAL(6, 58, 10), // "swapUpdate"
QT_MOC_LITERAL(7, 69, 4), // "used"
QT_MOC_LITERAL(8, 74, 7) // "timeout"

    },
    "SysStat::MemStatPrivate\0memoryUpdate\0"
    "\0apps\0buffers\0cached\0swapUpdate\0used\0"
    "timeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SysStat__MemStatPrivate[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   29,    2, 0x06 /* Public */,
       6,    1,   36,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,   39,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,    3,    4,    5,
    QMetaType::Void, QMetaType::Float,    7,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void SysStat::MemStatPrivate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MemStatPrivate *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->memoryUpdate((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 1: _t->swapUpdate((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 2: _t->timeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MemStatPrivate::*)(float , float , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MemStatPrivate::memoryUpdate)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MemStatPrivate::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MemStatPrivate::swapUpdate)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SysStat::MemStatPrivate::staticMetaObject = { {
    QMetaObject::SuperData::link<BaseStatPrivate::staticMetaObject>(),
    qt_meta_stringdata_SysStat__MemStatPrivate.data,
    qt_meta_data_SysStat__MemStatPrivate,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SysStat::MemStatPrivate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SysStat::MemStatPrivate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SysStat__MemStatPrivate.stringdata0))
        return static_cast<void*>(this);
    return BaseStatPrivate::qt_metacast(_clname);
}

int SysStat::MemStatPrivate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseStatPrivate::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void SysStat::MemStatPrivate::memoryUpdate(float _t1, float _t2, float _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SysStat::MemStatPrivate::swapUpdate(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
