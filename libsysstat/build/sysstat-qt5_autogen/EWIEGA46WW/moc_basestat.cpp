/****************************************************************************
** Meta object code from reading C++ file 'basestat.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../basestat.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'basestat.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SysStat__BaseStat_t {
    QByteArrayData data[12];
    char stringdata0[178];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SysStat__BaseStat_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SysStat__BaseStat_t qt_meta_stringdata_SysStat__BaseStat = {
    {
QT_MOC_LITERAL(0, 0, 17), // "SysStat::BaseStat"
QT_MOC_LITERAL(1, 18, 21), // "updateIntervalChanged"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 22), // "monitoredSourceChanged"
QT_MOC_LITERAL(4, 64, 14), // "updateInterval"
QT_MOC_LITERAL(5, 79, 17), // "setUpdateInterval"
QT_MOC_LITERAL(6, 97, 4), // "msec"
QT_MOC_LITERAL(7, 102, 12), // "stopUpdating"
QT_MOC_LITERAL(8, 115, 15), // "monitoredSource"
QT_MOC_LITERAL(9, 131, 18), // "setMonitoredSource"
QT_MOC_LITERAL(10, 150, 6), // "Source"
QT_MOC_LITERAL(11, 157, 20) // "monitorDefaultSource"

    },
    "SysStat::BaseStat\0updateIntervalChanged\0"
    "\0monitoredSourceChanged\0updateInterval\0"
    "setUpdateInterval\0msec\0stopUpdating\0"
    "monitoredSource\0setMonitoredSource\0"
    "Source\0monitorDefaultSource"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SysStat__BaseStat[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       2,   70, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       3,    1,   57,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   60,    2, 0x0a /* Public */,
       5,    1,   61,    2, 0x0a /* Public */,
       7,    0,   64,    2, 0x0a /* Public */,
       8,    0,   65,    2, 0x0a /* Public */,
       9,    1,   66,    2, 0x0a /* Public */,
      11,    0,   69,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::QString,    2,

 // slots: parameters
    QMetaType::Int,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,
    QMetaType::QString,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,

 // properties: name, type, flags
       4, QMetaType::Int, 0x00495107,
       8, QMetaType::QString, 0x00495107,

 // properties: notify_signal_id
       0,
       1,

       0        // eod
};

void SysStat::BaseStat::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BaseStat *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->updateIntervalChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->monitoredSourceChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: { int _r = _t->updateInterval();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->setUpdateInterval((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->stopUpdating(); break;
        case 5: { QString _r = _t->monitoredSource();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->setMonitoredSource((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->monitorDefaultSource(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BaseStat::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseStat::updateIntervalChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BaseStat::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseStat::monitoredSourceChanged)) {
                *result = 1;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<BaseStat *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = _t->updateInterval(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->monitoredSource(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<BaseStat *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setUpdateInterval(*reinterpret_cast< int*>(_v)); break;
        case 1: _t->setMonitoredSource(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
        BaseStat *_t = static_cast<BaseStat *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->stopUpdating(); break;
        case 1: _t->monitorDefaultSource(); break;
        default: break;
        }
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject SysStat::BaseStat::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SysStat__BaseStat.data,
    qt_meta_data_SysStat__BaseStat,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SysStat::BaseStat::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SysStat::BaseStat::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SysStat__BaseStat.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SysStat::BaseStat::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void SysStat::BaseStat::updateIntervalChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SysStat::BaseStat::monitoredSourceChanged(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
