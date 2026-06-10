#pragma once
#include <QDialog>
class QLineEdit;
class QLabel;
class QPushButton;
class AppCore;

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(AppCore* core, QWidget* parent = nullptr);
private:
    void buildUi();
    void wire();
    void setRegisterMode(bool enabled);

    AppCore* m_core;
    bool m_registerMode = false;
    QLineEdit* m_nameEdit;
    QLineEdit* m_emailEdit;
    QLineEdit* m_passwordEdit;
    QLabel* m_title;
    QLabel* m_subtitle;
    QPushButton* m_submit;
    QPushButton* m_switch;
};
