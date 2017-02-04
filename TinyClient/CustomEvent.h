﻿#ifndef CUSTOMEVENT_H
#define CUSTOMEVENT_H
#include <QEvent>
#include <QString>

namespace Events
{
    class CustomEvent : public QEvent
    {
    public:
        CustomEvent(int arg1 = 0, const QString &arg2 = QString());
        ~CustomEvent() = default;
    
        static Type eventType();
    
        int m_arg1;
        QString m_arg2;
    
    private:
        static Type m_evType;
    };
}

#endif // CUSTOMEVENT_H
