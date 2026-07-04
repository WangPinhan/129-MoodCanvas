#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include "userservice.h"

#include <QDialog>

class QFrame;
class QLineEdit;
class QPushButton;
class QStackedWidget;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(UserService *userService, QWidget *parent = nullptr);

    UserProfile profile() const;

private:
    UserService *m_userService = nullptr;
    UserProfile m_profile;

    QStackedWidget *m_stack = nullptr;
    QPushButton *m_loginTabButton = nullptr;
    QPushButton *m_registerTabButton = nullptr;

    QLineEdit *m_loginUserEdit = nullptr;
    QLineEdit *m_loginPasswordEdit = nullptr;
    QLineEdit *m_registerUserEdit = nullptr;
    QLineEdit *m_registerPasswordEdit = nullptr;
    QLineEdit *m_registerNicknameEdit = nullptr;

    QWidget *buildLoginPage();
    QWidget *buildRegisterPage();
    QFrame *buildFormCard(QWidget *parent);
    void switchToLoginTab();
    void switchToRegisterTab();
    void attemptLogin();
    void attemptRegister();
};

#endif // LOGINDIALOG_H
