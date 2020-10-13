/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Palo Kisa <palo.kisa@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#if !defined(SNIASYNC_H)
#define SNIASYNC_H

#include <functional>
#include "statusnotifieriteminterface.h"

template<typename>
struct remove_class_type { using type = void; }; // bluff
template<typename C, typename R, typename... ArgTypes>
struct  remove_class_type<R (C::*)(ArgTypes...)> { using type = R(ArgTypes...); };
template<typename C, typename R, typename... ArgTypes>
struct remove_class_type<R (C::*)(ArgTypes...) const> { using type = R(ArgTypes...); };

template <typename L>
class call_sig_helper
{
    template <typename L1>
        static decltype(&L1::operator()) test(int);
    template <typename L1>
        static void test(...); //bluff
public:
    using type = decltype(test<L>(0));
};
template <typename L>
struct call_signature : public remove_class_type<typename call_sig_helper<L>::type> {};
template <typename R, typename... ArgTypes>
struct call_signature<R (ArgTypes...)> { using type = R (ArgTypes...); };
template <typename R, typename... ArgTypes>
struct call_signature<R (*)(ArgTypes...)> { using type = R (ArgTypes...); };
template <typename C, typename R, typename... ArgTypes>
struct call_signature<R (C::*)(ArgTypes...)> { using type = R (ArgTypes...); };
template<typename C, typename R, typename... ArgTypes>
struct call_signature<R (C::*)(ArgTypes...) const>  { using type = R(ArgTypes...); };

template <typename> struct is_valid_signature : public std::false_type {};
template <typename Arg>
struct is_valid_signature<void (Arg)> : public std::true_type {};

class SniAsync : public QObject
{
    Q_OBJECT
public:
    SniAsync(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    template <typename F>
    inline void propertyGetAsync(QString const &name, F finished)
    {
        static_assert(is_valid_signature<typename call_signature<F>::type>::value, "need callable (lambda, *function, callable obj) (Arg) -> void");
        connect(new QDBusPendingCallWatcher{asyncPropGet(name), this},
                &QDBusPendingCallWatcher::finished,
                [this, finished, name] (QDBusPendingCallWatcher * call)
                {
                    QDBusPendingReply<QVariant> reply = *call;
                    if (reply.isError())
                        qDebug().noquote().nospace() << "Error on DBus request(" << mSni.service() << ',' << mSni.path() << "): " << reply.error();
                    finished(qdbus_cast<typename std::function<typename call_signature<F>::type>::argument_type>(reply.value()));
                    call->deleteLater();
                }
        );
    }

    //exposed methods from org::kde::StatusNotifierItem
    inline QString service() const { return mSni.service(); }

public slots:
    //Forwarded slots from org::kde::StatusNotifierItem
    inline QDBusPendingReply<> Activate(int x, int y) { return mSni.Activate(x, y); }
    inline QDBusPendingReply<> ContextMenu(int x, int y) { return mSni.ContextMenu(x, y); }
    inline QDBusPendingReply<> Scroll(int delta, const QString &orientation) { return mSni.Scroll(delta, orientation); }
    inline QDBusPendingReply<> SecondaryActivate(int x, int y) { return mSni.SecondaryActivate(x, y); }

signals:
    //Forwarded signals from org::kde::StatusNotifierItem
    void NewAttentionIcon();
    void NewIcon();
    void NewOverlayIcon();
    void NewStatus(const QString &status);
    void NewTitle();
    void NewToolTip();

private:
    QDBusPendingReply<QDBusVariant> asyncPropGet(QString const & property);

private:
    org::kde::StatusNotifierItem mSni;

};

#endif
