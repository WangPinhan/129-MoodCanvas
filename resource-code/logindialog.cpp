#include "logindialog.h"

#include "arttitlelabel.h"
#include "loginbackdropwidget.h"
#include "thememanager.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

QWidget *buildFieldBlock(const QString &labelText, QLineEdit *edit, QWidget *parent)
{
    auto *block = new QWidget(parent);
    auto *layout = new QVBoxLayout(block);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *label = new QLabel(labelText, block);
    label->setObjectName(QStringLiteral("loginFieldLabel"));
    layout->addWidget(label);
    layout->addWidget(edit);
    return block;
}

} // namespace

LoginDialog::LoginDialog(UserService *userService, QWidget *parent)
    : QDialog(parent),
      m_userService(userService)
{
    setWindowTitle(QStringLiteral("登录 · MoodCanvas"));
    setModal(true);
    resize(560, 640);
    setMinimumSize(520, 600);
    setObjectName(QStringLiteral("loginDialog"));
    setAutoFillBackground(false);

    auto *root = new QGridLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->setRowStretch(0, 1);
    root->setColumnStretch(0, 1);

    auto *backdrop = new LoginBackdropWidget(this);
    root->addWidget(backdrop, 0, 0);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("loginMainCard"));
    card->setMinimumWidth(420);
    card->setMaximumWidth(460);

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 32, 28, 24);
    cardLayout->setSpacing(16);

    auto *hero = new QFrame(card);
    hero->setObjectName(QStringLiteral("loginHeroCard"));
    auto *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(20, 20, 20, 18);
    heroLayout->setSpacing(12);

    auto *heroTop = new QHBoxLayout();
    heroTop->setSpacing(14);

    auto *icon = new QLabel(QStringLiteral("🌙"), hero);
    icon->setObjectName(QStringLiteral("loginAppIcon"));
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(64, 64);
    heroTop->addWidget(icon, 0, Qt::AlignTop);

    auto *titleCol = new QVBoxLayout();
    titleCol->setContentsMargins(0, 6, 0, 0);
    titleCol->setSpacing(6);

    auto *title = new ArtTitleLabel(QStringLiteral("MoodCanvas"), hero);
    title->setObjectName(QStringLiteral("loginArtTitle"));
    titleCol->addWidget(title);

    auto *tagline = new QLabel(QStringLiteral("把心情化成温柔色彩"), hero);
    tagline->setObjectName(QStringLiteral("loginTagline"));
    titleCol->addWidget(tagline);

    heroTop->addLayout(titleCol, 1);
    heroLayout->addLayout(heroTop);

    auto *subtitle = new QLabel(QStringLiteral("登录后可保存日记，并在树洞与其他用户实时互动。"), hero);
    subtitle->setWordWrap(true);
    subtitle->setObjectName(QStringLiteral("loginSubtitle"));
    heroLayout->addWidget(subtitle);
    cardLayout->addWidget(hero);

    auto *tabBar = new QFrame(card);
    tabBar->setObjectName(QStringLiteral("loginTabBar"));
    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(4, 4, 4, 4);
    tabLayout->setSpacing(6);

    m_loginTabButton = new QPushButton(QStringLiteral("登录"), tabBar);
    m_loginTabButton->setObjectName(QStringLiteral("loginTabButton"));
    m_loginTabButton->setCheckable(true);
    m_loginTabButton->setChecked(true);
    m_loginTabButton->setMinimumHeight(40);
    m_loginTabButton->setCursor(Qt::PointingHandCursor);

    m_registerTabButton = new QPushButton(QStringLiteral("注册"), tabBar);
    m_registerTabButton->setObjectName(QStringLiteral("loginTabButton"));
    m_registerTabButton->setCheckable(true);
    m_registerTabButton->setMinimumHeight(40);
    m_registerTabButton->setCursor(Qt::PointingHandCursor);

    tabLayout->addWidget(m_loginTabButton);
    tabLayout->addWidget(m_registerTabButton);
    cardLayout->addWidget(tabBar);

    m_stack = new QStackedWidget(card);
    m_stack->addWidget(buildLoginPage());
    m_stack->addWidget(buildRegisterPage());
    cardLayout->addWidget(m_stack, 1);

    root->addWidget(card, 0, 0, Qt::AlignCenter);

    connect(m_loginTabButton, &QPushButton::clicked, this, &LoginDialog::switchToLoginTab);
    connect(m_registerTabButton, &QPushButton::clicked, this, &LoginDialog::switchToRegisterTab);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, backdrop, [backdrop](bool) {
        backdrop->update();
    });

    if (m_userService && !m_userService->lastUsername().isEmpty()) {
        m_loginUserEdit->setText(m_userService->lastUsername());
    }
}

UserProfile LoginDialog::profile() const
{
    return m_profile;
}

QFrame *LoginDialog::buildFormCard(QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("loginFormFrame"));
    return frame;
}

QWidget *LoginDialog::buildLoginPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto *formFrame = buildFormCard(page);
    auto *form = new QVBoxLayout(formFrame);
    form->setContentsMargins(18, 18, 18, 18);
    form->setSpacing(12);

    m_loginUserEdit = new QLineEdit(formFrame);
    m_loginUserEdit->setPlaceholderText(QStringLiteral("输入用户名"));
    m_loginUserEdit->setMinimumHeight(42);
    m_loginUserEdit->setObjectName(QStringLiteral("loginLineEdit"));

    m_loginPasswordEdit = new QLineEdit(formFrame);
    m_loginPasswordEdit->setEchoMode(QLineEdit::Password);
    m_loginPasswordEdit->setPlaceholderText(QStringLiteral("输入密码"));
    m_loginPasswordEdit->setMinimumHeight(42);
    m_loginPasswordEdit->setObjectName(QStringLiteral("loginLineEdit"));

    form->addWidget(buildFieldBlock(QStringLiteral("用户名"), m_loginUserEdit, formFrame));
    form->addWidget(buildFieldBlock(QStringLiteral("密码"), m_loginPasswordEdit, formFrame));
    layout->addWidget(formFrame);

    auto *loginButton = new QPushButton(QStringLiteral("进入心象日记 ✨"), page);
    loginButton->setObjectName(QStringLiteral("loginPrimaryButton"));
    loginButton->setMinimumHeight(46);
    loginButton->setCursor(Qt::PointingHandCursor);
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    layout->addWidget(loginButton);

    auto *switchRow = new QHBoxLayout();
    switchRow->addStretch();
    auto *switchButton = new QPushButton(QStringLiteral("没有账号？去注册"), page);
    switchButton->setObjectName(QStringLiteral("loginSwitchLink"));
    switchButton->setFlat(true);
    switchButton->setCursor(Qt::PointingHandCursor);
    connect(switchButton, &QPushButton::clicked, this, &LoginDialog::switchToRegisterTab);
    switchRow->addWidget(switchButton);
    layout->addLayout(switchRow);

    return page;
}

QWidget *LoginDialog::buildRegisterPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    auto *formFrame = buildFormCard(page);
    auto *form = new QVBoxLayout(formFrame);
    form->setContentsMargins(18, 18, 18, 18);
    form->setSpacing(12);

    m_registerUserEdit = new QLineEdit(formFrame);
    m_registerUserEdit->setPlaceholderText(QStringLiteral("3-20 个字符"));
    m_registerUserEdit->setMinimumHeight(42);
    m_registerUserEdit->setObjectName(QStringLiteral("loginLineEdit"));

    m_registerPasswordEdit = new QLineEdit(formFrame);
    m_registerPasswordEdit->setEchoMode(QLineEdit::Password);
    m_registerPasswordEdit->setPlaceholderText(QStringLiteral("至少 6 位"));
    m_registerPasswordEdit->setMinimumHeight(42);
    m_registerPasswordEdit->setObjectName(QStringLiteral("loginLineEdit"));

    m_registerNicknameEdit = new QLineEdit(formFrame);
    m_registerNicknameEdit->setPlaceholderText(QStringLiteral("树洞中显示的昵称，可留空"));
    m_registerNicknameEdit->setMinimumHeight(42);
    m_registerNicknameEdit->setObjectName(QStringLiteral("loginLineEdit"));

    form->addWidget(buildFieldBlock(QStringLiteral("用户名"), m_registerUserEdit, formFrame));
    form->addWidget(buildFieldBlock(QStringLiteral("密码"), m_registerPasswordEdit, formFrame));
    form->addWidget(buildFieldBlock(QStringLiteral("昵称"), m_registerNicknameEdit, formFrame));
    layout->addWidget(formFrame);

    auto *registerButton = new QPushButton(QStringLiteral("注册并进入 ✨"), page);
    registerButton->setObjectName(QStringLiteral("loginPrimaryButton"));
    registerButton->setMinimumHeight(46);
    registerButton->setCursor(Qt::PointingHandCursor);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::attemptRegister);
    layout->addWidget(registerButton);

    auto *switchRow = new QHBoxLayout();
    switchRow->addStretch();
    auto *switchButton = new QPushButton(QStringLiteral("已有账号？去登录"), page);
    switchButton->setObjectName(QStringLiteral("loginSwitchLink"));
    switchButton->setFlat(true);
    switchButton->setCursor(Qt::PointingHandCursor);
    connect(switchButton, &QPushButton::clicked, this, &LoginDialog::switchToLoginTab);
    switchRow->addWidget(switchButton);
    layout->addLayout(switchRow);

    return page;
}

void LoginDialog::switchToLoginTab()
{
    m_loginTabButton->setChecked(true);
    m_registerTabButton->setChecked(false);
    m_stack->setCurrentIndex(0);
}

void LoginDialog::switchToRegisterTab()
{
    m_loginTabButton->setChecked(false);
    m_registerTabButton->setChecked(true);
    m_stack->setCurrentIndex(1);
}

void LoginDialog::attemptLogin()
{
    if (!m_userService) {
        return;
    }

    QString error;
    UserProfile profile;
    if (!m_userService->login(m_loginUserEdit->text(), m_loginPasswordEdit->text(), &profile, &error)) {
        QMessageBox::warning(this, QStringLiteral("登录失败"), error);
        return;
    }

    m_profile = profile;
    accept();
}

void LoginDialog::attemptRegister()
{
    if (!m_userService) {
        return;
    }

    QString error;
    if (!m_userService->registerUser(m_registerUserEdit->text(),
                                     m_registerPasswordEdit->text(),
                                     m_registerNicknameEdit->text(),
                                     &error)) {
        QMessageBox::warning(this, QStringLiteral("注册失败"), error);
        return;
    }

    m_profile = m_userService->currentUser();
    accept();
}
