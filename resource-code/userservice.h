#ifndef USERSERVICE_H
#define USERSERVICE_H

#include "cloudapiclient.h"
#include "userprofile.h"

#include <QObject>

class DataStore;

class UserService : public QObject
{
    Q_OBJECT

public:
    explicit UserService(DataStore *store, QObject *parent = nullptr);

    UserProfile currentUser() const;
    QString lastUsername() const;
    CloudApiClient *apiClient();
    void logout();

    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &nickname,
                      QString *errorOut = nullptr);

    bool login(const QString &username,
               const QString &password,
               UserProfile *profileOut = nullptr,
               QString *errorOut = nullptr);

private:
    DataStore *m_store = nullptr;
    CloudApiClient m_api;
    UserProfile m_currentUser;

    static bool isValidUsername(const QString &username, QString *errorOut);
    void saveSession(const UserProfile &profile);
    void clearSession();
};

#endif // USERSERVICE_H
