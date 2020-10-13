/****************************************************************************
** Meta object code from reading C++ file 'cpustat.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../cpustat.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cpustat.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SysStat__CpuStat_t {
    QByteArrayData data[17];
    char stringdata0[172];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SysStat__CpuStat_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SysStat__CpuStat_t qt_meta_stringdata_SysStat__CpuStat = {
    {
QT_MOC_LITERAL(0, 0, 16), // "SysStat::CpuStat"
QT_MOC_LITERAL(1, 17, 6), // "update"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 4), // "user"
QT_MOC_LITERAL(4, 30, 4), // "nice"
QT_MOC_LITERAL(5, 35, 6), // "system"
QT_MOC_LITERAL(6, 42, 5), // "other"
QT_MOC_LITERAL(7, 48, 13), // "frequencyRate"
QT_MOC_LITERAL(8, 62, 9), // "frequency"
QT_MOC_LITERAL(9, 72, 17), // "monitoringChanged"
QT_MOC_LITERAL(10, 90, 10), // "Monitoring"
QT_MOC_LITERAL(11, 101, 10), // "monitoring"
QT_MOC_LITERAL(12, 112, 13), // "setMonitoring"
QT_MOC_LITERAL(13, 126, 5), // "value"
QT_MOC_LITERAL(14, 132, 16), // "LoadAndFrequency"
QT_MOC_LITERAL(15, 149, 8), // "LoadOnly"
QT_MOC_LITERAL(16, 158, 13) // "FrequencyOnly"

    },
    "SysStat::CpuStat\0update\0\0user\0nice\0"
    "system\0other\0frequencyRate\0frequency\0"
    "monitoringChanged\0Monitoring\0monitoring\0"
    "setMonitoring\0value\0LoadAndFrequency\0"
    "LoadOnly\0FrequencyOnly"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SysStat__CpuStat[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       1,   76, // properties
       1,   80, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    6,   44,    2, 0x06 /* Public */,
       1,    4,   57,    2, 0x06 /* Public */,
       1,    1,   66,    2, 0x06 /* Public */,
       9,    1,   69,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,   72,    2, 0x0a /* Public */,
      12,    1,   73,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::UInt,    3,    4,    5,    6,    7,    8,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float,    3,    4,    5,    6,
    QMetaType::Void, QMetaType::UInt,    8,
    QMetaType::Void, 0x80000000 | 10,    2,

 // slots: parameters
    0x80000000 | 10,
    QMetaType::Void, 0x80000000 | 10,   13,

 // properties: name, type, flags
      11, 0x80000000 | 10, 0x0049510b,

 // properties: notify_signal_id
       3,

 // enums: name, alias, flags, count, data
      10,   10, 0x0,    3,   85,

 // enum data: key, value
      14, uint(SysStat::CpuStat::LoadAndFrequency),
      15, uint(SysStat::CpuStat::LoadOnly),
      16, uint(SysStat::CpuStat::FrequencyOnly),

       0        // eod
};

void SysStat::CpuStat::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CpuStat *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->update((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4])),(*reinterpret_cast< float(*)>(_a[5])),(*reinterpret_cast< uint(*)>(_a[6]))); break;
        case 1: _t->update((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4]))); break;
        case 2: _t->update((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 3: _t->monitoringChanged((*reinterpret_cast< Monitoring(*)>(_a[1]))); break;
        case 4: { Monitoring _r = _t->monitoring();
            if (_a[0]) *reinterpret_cast< Monitoring*>(_a[0]) = std::move(_r); }  break;
        case 5: _t->setMonitoring((*reinterpret_cast< Monitoring(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CpuStat::*)(float , float , float , float , float , uint );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStat::update)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CpuStat::*)(float , float , float , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStat::update)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CpuStat::*)(uint );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStat::update)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CpuStat::*)(Monitoring );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CpuStat::monitoringChanged)) {
                *result = 3;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<CpuStat *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< Monitoring*>(_v) = _t->monitoring(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<CpuStat *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setMonitoring(*reinterpret_cast< Monitoring*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject SysStat::CpuStat::staticMetaObject = { {
    QMetaObject::SuperData::link<BaseStat::staticMetaObject>(),
    qt_meta_stringdata_SysStat__CpuStat.data,
    qt_meta_data_SysStat__CpuStat,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SysStat::CpuStat::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SysStat::CpuStat::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SysStat__CpuStat.stringdata0))
        return static_cast<void*>(this);
    return BaseStat::qt_metacast(_clname);
}

int SysStat::CpuStat::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseStat::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void SysStat::CpuStat::update(float _t1, float _t2, float _t3, float _t4, float _t5, uint _t6)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t6))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SysStat::CpuStat::update(float _t1, float _t2, float _t3, float _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SysStat::CpuStat::update(uint _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SysStat::CpuStat::monitoringChanged(Monitoring _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
