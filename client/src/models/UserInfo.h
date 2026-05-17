#pragma once
#include <QString>

struct UserInfo {
    QString username;
    QString token;
    QString signupAt;

    bool isValid() const { return !username.isEmpty() && !token.isEmpty(); }
};
