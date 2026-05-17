#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QWidget>
#include <QHBoxLayout>
#include <QMap>

#include "models/UserInfo.h"
#include "network/NetworkManager.h"
#include "pages/SignUpPage.h"
#include "pages/SignInPage.h"
#include "pages/HomePage.h"
#include "pages/ShareDialog.h"
#include "pages/TransferListPage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void connectSignals();
    void navigateToSignIn();
    void navigateToHome(const UserInfo &info);
    void doLogout();

    QStackedWidget *m_stack;
    QWidget *m_mainContainer;
    QListWidget *m_sidebar;
    QStackedWidget *m_contentStack;
    NetworkManager *m_network;
    SignUpPage *m_signUpPage;
    SignInPage *m_signInPage;
    HomePage *m_homePage;
    TransferListPage *m_transferPage;
    UserInfo m_currentUser;
    QMap<QString, QString> m_transferIds;
    bool m_loggedIn = false;
};
