/****************************************************************************
** Meta object code from reading C++ file 'cpustat_p.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../cpustat_p.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cpustat_p.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SysStat__CpuStatPrivate_t {
    QByteArrayData data[10];
    char stringdata0[87];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SysStat__CpuStatPrivate_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SysStat__CpuStatPrivate_t qt_meta_stringdata_SysStat__CpuStatPrivate = {
    {
QT_MOC_LITERAL(0, 0, 23), // "SysStat::CpuStatPrivate"
QT_MOC_LITERAL(1, 24, 6), // "update"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 4), // "user"
QT_MOC_LITERAL(4, 37, 4), // "nice"
QT_MOC_LITERAL(5, 42, 6), // "system"
QT_MOC_LITERAL(6, 49, 5), // "other"
QT_MOC_LITERAL(7, 55, 9), // "frequency"
QT_MOC_LITERAL(8, 65, 13), // "frequencyRate"
QT_MOC_LITERAL(9, 79, 7) // "timeout"

    },
    "SysStat::CpuStatPrivate\0update\0\0user\0"
    "nice\0system\0other\0frequency\0frequencyRate\0"
    "timeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SysStat__CpuStatPrivate[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    4,   34,    2, 0x06 /* Public */,
       1,    1,   43,    2, 0x06 /* Public */,
       1,    6,   46,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    0,   59,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float,    3,    4,    5,    6,
    QMetaType::Void, QMetaType::UInt,    7,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::UInt,    3,    4,    5,    6,    8,    7,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void SysStat::CpuStatPrivate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CpuStatPrivate *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->update((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4]))); break;
        case 1: _t->update((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 2: _t->update((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4])),(*reinterpret_cast< float(*)>(_a[5])),(*reinterpret_cast< uint(*)>(_a[6]))); break;
        case 3: _t->timeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CpuStatPrivate::*)(float , float , float , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStatPrivate::update)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CpuStatPrivate::*)(uint );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStatPrivate::update)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CpuStatPrivate::*)(float , float , float , float , float , uint );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStatPrivate::update)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SysStat::CpuStatPrivate::staticMetaObject = { {
    QMetaObject::SuperData::link<BaseStatPrivate::staticMetaObject>(),
    qt_meta_stringdata_SysStat__CpuStatPrivate.data,
    qt_meta_data_SysStat__CpuStatPrivate,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SysStat::CpuStatPrivate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SysStat::CpuStatPrivate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SysStat__CpuStatPrivate.stringdata0))
        return static_cast<void*>(this);
    return BaseStatPrivate::qt_metacast(_clname);
}

int SysStat::CpuStatPrivate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseStatPrivate::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void SysStat::CpuStatPrivate::update(float _t1, float _t2, float _t3, float _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SysStat::CpuStatPrivate::update(uint _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SysStat::CpuStatPrivate::update(float _t1, float _t2, float _t3, float _t4, float _t5, uint _t6)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t6))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
