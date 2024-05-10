#ifndef SESSION_H
#define SESSION_H

#include "cppREST_global.h"
#include <QDateTime>

struct CPPRESTSHARED_EXPORT Session
{
    QString string_id;
    int user_id;
    QString user_login;
    QString user_name;
    QDateTime login_time;
    bool is_for_db_only;

    Session()
        : string_id()
        , user_id()
        , user_login()
        , user_name()
        , login_time()
        , is_for_db_only()
    {
    }

    Session(const QString string_id_in, const int user_id_in, const QString user_login_in, const QString user_name_in, const QDateTime login_time_in, const bool is_for_db_only_in = false)
        : string_id(string_id_in)
        , user_id(user_id_in)
        , user_login(user_login_in)
        , user_name(user_name_in)
        , login_time(login_time_in)
        , is_for_db_only(is_for_db_only_in)
    {
    }

    bool isEmpty()
    {
        return string_id.isEmpty() && user_id==0 && login_time.isNull() && !is_for_db_only;
    }
};

#endif // SESSION_H
