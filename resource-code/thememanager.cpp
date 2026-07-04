#include "thememanager.h"
#include <QApplication>
#include <QStyle>
#include <QWidget>
#include <QSettings>
#include <QStyleHints>
#include <QGuiApplication>

// -------------------- 浅色主题  --------------------
static QString lightStyleSheet()
{
    return QStringLiteral(R"(
        /*mainwindow*/
        QMainWindow {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #f8fbff, stop:0.45 #f2fff8, stop:1 #fff8f5);
        }

        QFrame#sideCard {
            background: rgba(255,255,255,235);
            border: 1px solid rgba(219,231,236,180);
            border-radius: 28px;
        }

        QFrame#mainCard {
            background: rgba(255,255,255,242);
            border: 1px solid rgba(219,231,236,180);
            border-radius: 32px;
        }

        QFrame#heroCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #e7f7ef, stop:0.55 #f8f1ff, stop:1 #fff3e7);
            border-radius: 26px;
        }

        QLabel#appIconLabel {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #d8fff0, stop:1 #dfe8ff);
            border-radius: 22px;
        }

        QLabel#titleLabel, QLabel#headlineLabel { color: #283044; }
        QLabel#headlineLabel {
            padding-top: 8px;
            padding-bottom: 8px;
            min-height: 44px;
        }
        QLabel#subtitleLabel, QLabel#hintLabel { color: #6b7280; }
        QLabel#hintLabel {
            padding-top: 2px;
            padding-bottom: 4px;
            min-height: 44px;
        }

        QFrame#plantCard {
            background: rgba(255,255,255,236);
            border: 1px solid rgba(198,229,215,190);
            border-radius: 28px;
        }

        QLabel#plantTitleLabel {
            color: #284139;
            font-size: 17px;
            font-weight: 800;
        }

        QLabel#plantHintLabel { color: #60736b; }

        QLabel#sideIllustrationLabel {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #f3fff8, stop:1 #edf3ff);
            border-radius: 22px;
            padding: 8px;
        }

        QLabel#privacyTipLabel, QLabel#tipsLabel {
            color: #5f6c72;
            background: rgba(245,250,252,235);
            border-radius: 18px;
            padding: 12px;
        }

        QTextEdit#inputEdit {
            border: 1px solid #e1eceb;
            border-radius: 22px;
            padding: 14px;
            background: #ffffff;
            color: #2b2d42;
        }

        QLineEdit {
            border: 1px solid #dfe9ec;
            border-radius: 16px;
            padding: 9px 12px;
            background: #fbfdff;
        }

        QPushButton {
            border: 1px solid #dce8ef;
            border-radius: 18px;
            padding: 9px 14px;
            background: #ffffff;
            color: #334155;
        }

        QPushButton:hover { background: #f1fbf7; border-color: #bfe4d2; }

        QPushButton:checked {
            background: #b8eadb;
            color: #263238;
            border-color: #8ed9c0;
            font-weight: 700;
        }

        QPushButton#generateButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #85dcb8, stop:1 #9eb7ff);
            color: white;
            border: none;
            border-radius: 22px;
            padding: 12px 26px;
            font-weight: 800;
        }

        QPushButton#generateButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #70d1aa, stop:1 #8aa5f2);
        }

        QStatusBar {
            background: rgba(255,255,255,210);
            color: #5f6c72;
            border-top: 1px solid #e6eef2;
            padding-left: 12px;
        }

        /*SettingsDialog*/

        QScrollArea {
            background: transparent;
        }
        QTextEdit {
            background: white;
            border: 1px solid #e6eef2;
            border-radius: 20px;
            padding: 14px;
            color: #2b2d42;
        }
        QLineEdit {
            border: 1px solid #dfe9ec;
            border-radius: 16px;
            padding: 9px 12px;
            background: #fbfdff;
            color: #2b2d42;
        }
        QLabel {
            color: #283044;
        }
        QLabel#settingsTitleLabel {
            color: #283044;
            font-weight: bold;
        }
        QLabel#llmHintLabel {
            color: #6b7c85;
            font-size: 12px;
        }
        QLabel#privacyTextLabel {
            background: white;
            border: 1px solid #e6eef2;
            border-radius: 20px;
            padding: 18px;
            color: #283044;
            font-size: 14px;
        }
        QPushButton#saveLlmBtn {
            background: #85dcb8;
            color: white;
            border: none;
            font-weight: bold;
        }
        QLabel#themeLabel {
            color: #283044;
            font-weight: bold;
            font-size: 14px;
        }
        QDialog {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #f8fbff, stop:1 #fff8f2);
        }
        QFrame#card {
            background: rgba(255,255,255,242);
            border: 1px solid #e6eef2;
            border-radius: 24px;
        }
        QWidget#settingsContentWidget {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #f8fbff, stop:1 #fff8f2);
        }
        QComboBox {
            border: 1px solid #ccd4db;
            border-radius: 14px;
            padding: 6px 12px;
            background: #ffffff;
            color: #283044;
            min-height: 20px;
        }
        QComboBox:hover {
            border-color: #a0b4c0;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 24px;
            border-left: 1px solid #ccd4db;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #ccd4db;
            border-radius: 8px;
            color: #283044;
            selection-background-color: #e0f0f0;
        }

        /* ResultDialog */

        QFrame#comfortCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #f0faf5, stop:1 #fff8f0);
            border: 1px solid #d8ebe3;
            border-radius: 18px;
        }
        QFrame#infoCard {
            background: rgba(255,255,255,242);
            border: 1px solid #e6eef2;
            border-radius: 20px;
        }
        QFrame#footerBar {
            background: rgba(255,255,255,248);
            border-top: 1px solid #e6eef2;
        }
        QLabel#comfortTitleLabel {
            color: #2d5a4a;
            font-weight: bold;
            font-size: 14px;
        }
        QLabel#comfortBodyLabel {
            color: #3d4f56;
            font-size: 13px;
        }
        QLabel#rawTextLabel {
            border: 1px solid #e1eceb;
            border-radius: 14px;
            padding: 10px 12px;
            background: white;
            color: #52626b;
            font-size: 13px;
        }
        QLabel#cbtHintLabel {
            color: #6b7c85;
            font-size: 12px;
        }
        QPushButton#saveButton {
            background: #85dcb8;
            color: white;
            border: none;
            font-weight: bold;
            min-width: 148px;
        }
        QPushButton#saveButton:hover {
            background: #75d1aa;
        }
        QWidget#resultScrollContent {
            background: transparent;
        }

        /* TimelineWindow */
        QDialog#timelineDialog {
            /* 背景由代码动态设置，这里可留空或作为 fallback */
        }
        QLabel#monthTitleLabel {
            color: #283044;
            font-weight: bold;
        }
        QLabel#sceneLabel {
            border-radius: 24px;
            padding: 14px;
            color: #34444c;
            font-weight: bold;
        }
        QLabel#detailHintLabel {
            color: #7c8b94;
            font-size: 13px;
            padding: 8px 4px;
        }
        QLabel#entryMetaLabel {
            color: #56616a;
            font-size: 12px;
        }
        QLabel#entryEmotionLabel {
            color: #2f3a44;
            font-size: 15px;
            font-weight: 600;
        }
        QLabel#entryBodyLabel {
            color: #52626b;
            font-size: 13px;
        }
        QToolButton {
            border: none;
            border-radius: 18px;
            padding: 8px;
            background: white;
            color: #334155;
        }
        QToolButton:hover {
            background: #eefaf5;
        }
        QPushButton#deleteEntryButton {
            background: #fff5f5;
            border: 1px solid #f0d4d4;
            color: #b85c5c;
            padding: 6px 14px;
            font-size: 12px;
        }
        QPushButton#deleteEntryButton:hover {
            background: #ffecec;
        }
        QFrame#detailPanel {
            border: 1px solid #e6eef2;
            border-radius: 22px;
            background: white; /* fallback */
        }
        QFrame#entryCard {
            border: 1px solid #e8eef2;
            border-radius: 18px;
            background: rgba(255,255,255,235);
        }

        /* WeeklyReportWindow */
        QDialog {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #e7f3f8, stop:0.45 #edf8f2, stop:1 #fdf0e8);
        }
        QWidget#reportScrollContent {
            background: transparent;
        }
        QFrame#headerCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #e8faf3, stop:1 #efe9ff);
            border: 1px solid rgba(205,224,218,180);
            border-radius: 22px;
        }
        QFrame#insightCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #f8f0ff, stop:1 #fff6f0);
            border: 1px solid rgba(220,208,232,180);
            border-radius: 22px;
        }
        QFrame#adviceCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #edf9f2, stop:1 #f4fff8);
            border: 1px solid rgba(196,224,210,180);
            border-radius: 22px;
        }
        QFrame#sceneryCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #edf5fb, stop:1 #f7f2ea);
            border: 1px solid rgba(205,220,232,180);
            border-radius: 22px;
        }
        QFrame#footerBar {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #eef7fb, stop:1 #f7fbf4);
            border-top: 1px solid #dce8e4;
        }
        QLabel#reportTitle {
            color: #24303a;
            font-size: 28px;
            font-weight: 600;
        }
        QLabel#reportEpithet {
            color: #8a7a92;
            font-size: 15px;
        }
        QLabel#reportPeriod {
            color: #6f7d86;
            font-size: 13px;
        }
        QLabel#reportSource {
            color: #93a0a8;
            font-size: 12px;
        }
        QLabel#sectionTitle {
            color: #7a8790;
            font-size: 12px;
            font-weight: bold;
        }
        QLabel#insightBody, QLabel#adviceBody {
            color: #3f4b54;
            font-size: 15px;
            line-height: 1.85;
        }
        QPushButton {
            border-radius: 16px;
            padding: 10px 20px;
            border: 1px solid #cfe0db;
            background: rgba(255,255,255,210);
            color: #334155;
        }
        QPushButton:hover {
            background: #f0faf5;
        }
        /* --- LoginDialog --- */
        QDialog#loginDialog {
            background: transparent;
        }
        QFrame#loginMainCard {
            background: rgba(255,255,255,248);
            border: 1px solid rgba(219,231,236,190);
            border-radius: 32px;
        }
        QFrame#loginHeroCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #e7f7ef, stop:0.55 #f8f1ff, stop:1 #fff3e7);
            border-radius: 24px;
        }
        QLabel#loginAppIcon {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #d8fff0, stop:1 #dfe8ff);
            border-radius: 20px;
            font-size: 28px;
        }
        QLabel#loginTagline {
            color: #566878;
            font-size: 14px;
            font-weight: 600;
        }
        QLabel#loginSubtitle {
            color: #6b7280;
            font-size: 13px;
            line-height: 1.5;
        }
        QLabel#loginFieldLabel {
            color: #475569;
            font-size: 13px;
            font-weight: 600;
        }
        QFrame#loginTabBar {
            background: rgba(245,250,252,235);
            border: 1px solid #e1eceb;
            border-radius: 18px;
        }
        QPushButton#loginTabButton {
            background: transparent;
            border: none;
            border-radius: 14px;
            color: #64748b;
            font-weight: 600;
            padding: 8px 16px;
        }
        QPushButton#loginTabButton:hover {
            background: rgba(255,255,255,180);
            color: #334155;
        }
        QPushButton#loginTabButton:checked {
            background: #ffffff;
            color: #283044;
            border: 1px solid #d7e8df;
            font-weight: 700;
        }
        QFrame#loginFormFrame {
            background: rgba(255,255,255,242);
            border: 1px solid #e1eee8;
            border-radius: 22px;
        }
        QLineEdit#loginLineEdit {
            border: 1px solid #dfe9ec;
            border-radius: 16px;
            padding: 10px 14px;
            background: #fbfdff;
            color: #2b2d42;
        }
        QLineEdit#loginLineEdit:focus {
            border-color: #85dcb8;
            background: #ffffff;
        }
        QPushButton#loginPrimaryButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #85dcb8, stop:1 #9eb7ff);
            color: white;
            border: none;
            font-weight: bold;
            font-size: 15px;
            border-radius: 18px;
        }
        QPushButton#loginPrimaryButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #75d1aa, stop:1 #8ea8f0);
        }
        QPushButton#loginSwitchLink {
            color: #64748b;
            border: none;
            font-size: 13px;
        }
        QPushButton#loginSwitchLink:hover {
            color: #334155;
            background: transparent;
        }
        /* --- ToolboxDialog --- */
        QLabel#toolboxTitle {
            color: #283044;
            font-size: 22px;
            font-weight: bold;
        }
        QLabel#toolboxSubtitle {
            color: #64748b;
        }
        QTabWidget#treeHoleTabs::pane {
            border: none;
        }
        QTabWidget#treeHoleTabs QTabBar::tab {
            padding: 10px 18px;
            border-radius: 16px;
            background: #ffffff;
            margin-right: 6px;
            color: #3a4a4f;
        }
        QTabWidget#treeHoleTabs QTabBar::tab:selected {
            background: #bdebd9;
            font-weight: bold;
        }
        QWidget#page {
            background: rgba(255,255,255,242);
            border: 1px solid #e1eee8;
            border-radius: 26px;
        }
        QLabel#previewInfoLabel {
            color: #52626b;
            background: #f7fbff;
            border-radius: 16px;
            padding: 10px;
        }
        QLabel#treeHoleListTitle {
            color: #34444c;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton#treeHolePostButton {
            background: #85dcb8;
            color: white;
            border: none;
            font-weight: bold;
        }
        QPushButton#treeHolePostButton:hover {
            background: #75d1aa;
        }
    )");
}

// -------------------- 深色主题 --------------------
static QString darkStyleSheet()
{
    return QStringLiteral(R"(

        /*mainwindow*/

        QMainWindow {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #1a1e24, stop:0.45 #1e2328, stop:1 #1f2327);
        }

        QFrame#sideCard {
            background: rgba(30,34,42,220);
            border: 1px solid rgba(70,78,92,180);
            border-radius: 28px;
        }

        QFrame#mainCard {
            background: rgba(34,39,48,235);
            border: 1px solid rgba(70,78,92,180);
            border-radius: 32px;
        }

        QFrame#heroCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #2a3439, stop:0.55 #2f353c, stop:1 #2f332e);
            border-radius: 26px;
        }

        QLabel#appIconLabel {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #2a3a3f, stop:1 #2f3845);
            border-radius: 22px;
        }

        QLabel#titleLabel, QLabel#headlineLabel { color: #d6dbe0; }
        QLabel#headlineLabel {
            padding-top: 8px;
            padding-bottom: 8px;
            min-height: 44px;
        }
        QLabel#subtitleLabel, QLabel#hintLabel { color: #9aa1ac; }
        QLabel#hintLabel {
            padding-top: 2px;
            padding-bottom: 4px;
            min-height: 44px;
        }

        QFrame#plantCard {
            background: rgba(40,46,55,230);
            border: 1px solid rgba(90,100,115,180);
            border-radius: 28px;
        }

        QLabel#plantTitleLabel {
            color: #b5c7c2;
            font-size: 17px;
            font-weight: 800;
        }

        QLabel#plantHintLabel { color: #889490; }

        QLabel#sideIllustrationLabel {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #2b3333, stop:1 #2f3642);
            border-radius: 22px;
            padding: 8px;
        }

        QLabel#privacyTipLabel, QLabel#tipsLabel {
            color: #95a0a8;
            background: rgba(45,52,60,220);
            border-radius: 18px;
            padding: 12px;
        }

        QTextEdit#inputEdit {
            border: 1px solid #45505a;
            border-radius: 22px;
            padding: 14px;
            background: #252b32;
            color: #d0d6dd;
        }

        QLineEdit {
            border: 1px solid #3f4752;
            border-radius: 16px;
            padding: 9px 12px;
            background: #252b32;
            color: #d0d6dd;
        }

        QPushButton {
            border: 1px solid #3c4450;
            border-radius: 18px;
            padding: 9px 14px;
            background: #2d333b;
            color: #cbd0d8;
        }

        QPushButton:hover { background: #38424e; border-color: #5c7b6e; }

        QPushButton:checked {
            background: #496655;
            color: #d4e0da;
            border-color: #5e8b74;
            font-weight: 700;
        }

        QPushButton#generateButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #4d7c65, stop:1 #596da8);
            color: white;
            border: none;
            border-radius: 22px;
            padding: 12px 26px;
            font-weight: 800;
        }

        QPushButton#generateButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #3f6a56, stop:1 #4c5f94);
        }

        QStatusBar {
            background: rgba(30,34,42,210);
            color: #95a0a8;
            border-top: 1px solid #3b434f;
            padding-left: 12px;
        }

        /*SettingsDialog*/

        QDialog {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #1a1e24, stop:1 #1f2327);
        }
        QScrollArea {
            background: transparent;
        }
        QTextEdit {
            background: #252b32;
            border: 1px solid #45505a;
            border-radius: 20px;
            padding: 14px;
            color: #d0d6dd;
        }
        QLineEdit {
            border: 1px solid #3f4752;
            border-radius: 16px;
            padding: 9px 12px;
            background: #252b32;
            color: #d0d6dd;
        }
        QLabel {
            color: #d6dbe0;
        }
        QLabel#settingsTitleLabel {
            color: #d6dbe0;
            font-weight: bold;
        }
        QLabel#llmHintLabel {
            color: #9aa1ac;
            font-size: 12px;
        }
        QLabel#privacyTextLabel {
            background: #252b32;
            border: 1px solid #3b434f;
            border-radius: 20px;
            padding: 18px;
            color: #d0d6dd;
            font-size: 14px;
        }
        QPushButton#saveLlmBtn {
            background: #4d7c65;
            color: white;
            border: none;
            font-weight: bold;
        }
        QLabel#themeLabel {
            color: #d6dbe0;
            font-weight: bold;
            font-size: 14px;
        }
        QFrame#card {
            background: rgba(34,39,48,235);
            border: 1px solid rgba(70,78,92,180);
            border-radius: 24px;
        }
        QWidget#settingsContentWidget {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #1a1e24, stop:1 #1f2327);
        }
        QComboBox {
            border: 1px solid #4a5560;
            border-radius: 14px;
            padding: 6px 12px;
            background: #2d333b;
            color: #d0d6dd;
            min-height: 20px;
        }
        QComboBox:hover {
            border-color: #6b8290;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 24px;
            border-left: 1px solid #4a5560;
        }
        QComboBox QAbstractItemView {
            background: #2d333b;
            border: 1px solid #4a5560;
            border-radius: 8px;
            color: #d0d6dd;
            selection-background-color: #3e4a55;
        }
        /* ResultDialog */
        QFrame#comfortCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #2a3330, stop:1 #2f3229);
            border: 1px solid #3d5149;
            border-radius: 18px;
        }
        QFrame#infoCard {
            background: rgba(34,39,48,235);
            border: 1px solid rgba(70,78,92,180);
            border-radius: 20px;
        }
        QFrame#footerBar {
            background: rgba(30,34,42,235);
            border-top: 1px solid #3b434f;
        }
        QLabel#comfortTitleLabel {
            color: #8ab4a2;
            font-weight: bold;
            font-size: 14px;
        }
        QLabel#comfortBodyLabel {
            color: #b8c5c2;
            font-size: 13px;
        }
        QLabel#rawTextLabel {
            border: 1px solid #45505a;
            border-radius: 14px;
            padding: 10px 12px;
            background: #252b32;
            color: #a0aab4;
            font-size: 13px;
        }
        QLabel#cbtHintLabel {
            color: #8a95a0;
            font-size: 12px;
        }
        QPushButton#saveButton {
            background: #4d7c65;
            color: white;
            border: none;
            font-weight: bold;
            min-width: 148px;
        }
        QPushButton#saveButton:hover {
            background: #3f6a56;
        }
        QWidget#resultScrollContent {
            background: transparent;
        }

        /* TimelineWindow */
        QLabel#monthTitleLabel {
            color: #d6dbe0;
            font-weight: bold;
        }
        QLabel#sceneLabel {
            border-radius: 24px;
            padding: 14px;
            color: #cbd0d8;
            font-weight: bold;
        }
        QLabel#detailHintLabel {
            color: #88939b;
            font-size: 13px;
            padding: 8px 4px;
        }
        QLabel#entryMetaLabel {
            color: #8a939c;
            font-size: 12px;
        }
        QLabel#entryEmotionLabel {
            color: #cbd0d8;
            font-size: 15px;
            font-weight: 600;
        }
        QLabel#entryBodyLabel {
            color: #a0a8b0;
            font-size: 13px;
        }
        QToolButton {
            border: none;
            border-radius: 18px;
            padding: 8px;
            background: #2d333b;
            color: #cbd0d8;
        }
        QToolButton:hover {
            background: #38424e;
        }
        QPushButton#deleteEntryButton {
            background: #3a2525;
            border: 1px solid #5c3a3a;
            color: #d48c8c;
            padding: 6px 14px;
            font-size: 12px;
        }
        QPushButton#deleteEntryButton:hover {
            background: #4d3232;
        }
        QFrame#detailPanel {
            border: 1px solid #3b434f;
            border-radius: 22px;
            background: #252b32; /* fallback */
        }
        QFrame#entryCard {
            border: 1px solid #3b434f;
            border-radius: 18px;
            background: rgba(34,39,48,235);
        }

        /* WeeklyReportWindow */
        QDialog {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #1b2226, stop:0.45 #1e2627, stop:1 #222322);
        }
        QWidget#reportScrollContent {
            background: transparent;
        }
        QFrame#headerCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #26312f, stop:1 #2a2b38);
            border: 1px solid rgba(70,78,92,180);
            border-radius: 22px;
        }
        QFrame#insightCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #2e293b, stop:1 #2b2a2c);
            border: 1px solid rgba(90,82,102,180);
            border-radius: 22px;
        }
        QFrame#adviceCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #25322d, stop:1 #273028);
            border: 1px solid rgba(80,95,85,180);
            border-radius: 22px;
        }
        QFrame#sceneryCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #252f36, stop:1 #2a2b28);
            border: 1px solid rgba(70,80,92,180);
            border-radius: 22px;
        }
        QFrame#footerBar {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #242c30, stop:1 #282c28);
            border-top: 1px solid #3b434f;
        }
        QLabel#reportTitle {
            color: #d6dbe0;
            font-size: 28px;
            font-weight: 600;
        }
        QLabel#reportEpithet {
            color: #b0a0b8;
            font-size: 15px;
        }
        QLabel#reportPeriod {
            color: #9aa1a8;
            font-size: 13px;
        }
        QLabel#reportSource {
            color: #8a929c;
            font-size: 12px;
        }
        QLabel#sectionTitle {
            color: #8a929c;
            font-size: 12px;
            font-weight: bold;
        }
        QLabel#insightBody, QLabel#adviceBody {
            color: #c0c7d0;
            font-size: 15px;
            line-height: 1.85;
        }
        QPushButton {
            border-radius: 16px;
            padding: 10px 20px;
            border: 1px solid #3c4450;
            background: #2d333b;
            color: #cbd0d8;
        }
        QPushButton:hover {
            background: #38424e;
        }
        /* --- LoginDialog --- */
        QDialog#loginDialog {
            background: transparent;
        }
        QFrame#loginMainCard {
            background: rgba(30,34,42,248);
            border: 1px solid rgba(70,78,92,190);
            border-radius: 32px;
        }
        QFrame#loginHeroCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #24352f, stop:0.55 #2a3040, stop:1 #3a3028);
            border-radius: 24px;
        }
        QLabel#loginAppIcon {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #2f4a42, stop:1 #334058);
            border-radius: 20px;
            font-size: 28px;
        }
        QLabel#loginTagline {
            color: #a8b4c0;
            font-size: 14px;
            font-weight: 600;
        }
        QLabel#loginSubtitle {
            color: #8b95a1;
            font-size: 13px;
            line-height: 1.5;
        }
        QLabel#loginFieldLabel {
            color: #a0a8b0;
            font-size: 13px;
            font-weight: 600;
        }
        QFrame#loginTabBar {
            background: rgba(36,40,48,235);
            border: 1px solid #3b434f;
            border-radius: 18px;
        }
        QPushButton#loginTabButton {
            background: transparent;
            border: none;
            border-radius: 14px;
            color: #8b95a1;
            font-weight: 600;
            padding: 8px 16px;
        }
        QPushButton#loginTabButton:hover {
            background: rgba(56,66,78,180);
            color: #cbd0d8;
        }
        QPushButton#loginTabButton:checked {
            background: #2d333b;
            color: #e8ecf1;
            border: 1px solid #4d5868;
            font-weight: 700;
        }
        QFrame#loginFormFrame {
            background: rgba(36,40,48,242);
            border: 1px solid #3b434f;
            border-radius: 22px;
        }
        QLineEdit#loginLineEdit {
            border: 1px solid #3b434f;
            border-radius: 16px;
            padding: 10px 14px;
            background: #252a32;
            color: #e8ecf1;
        }
        QLineEdit#loginLineEdit:focus {
            border-color: #4d7c65;
            background: #2b3238;
        }
        QPushButton#loginPrimaryButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #4d7c65, stop:1 #5a6f9a);
            color: white;
            border: none;
            font-weight: bold;
            font-size: 15px;
            border-radius: 18px;
        }
        QPushButton#loginPrimaryButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #3f6a56, stop:1 #4d6088);
        }
        QPushButton#loginSwitchLink {
            color: #8b95a1;
            border: none;
            font-size: 13px;
        }
        QPushButton#loginSwitchLink:hover {
            color: #cbd0d8;
            background: transparent;
        }
        /* --- ToolboxDialog --- */
        QLabel#toolboxTitle {
            color: #d6dbe0;
            font-size: 22px;
            font-weight: bold;
        }
        QLabel#toolboxSubtitle {
            color: #9aa1a8;
        }
        QTabWidget#treeHoleTabs::pane {
            border: none;
        }
        QTabWidget#treeHoleTabs QTabBar::tab {
            padding: 10px 18px;
            border-radius: 16px;
            background: #2d333b;
            margin-right: 6px;
            color: #cbd0d8;
        }
        QTabWidget#treeHoleTabs QTabBar::tab:selected {
            background: #496655;
            font-weight: bold;
            color: #ffffff;
        }
        QWidget#page {
            background: rgba(34,39,48,235);
            border: 1px solid #3b434f;
            border-radius: 26px;
        }
        QLabel#previewInfoLabel {
            color: #a0a8b0;
            background: #2b3238;
            border-radius: 16px;
            padding: 10px;
        }
        QLabel#treeHoleListTitle {
            color: #d0d6dd;
            font-size: 15px;
            font-weight: bold;
        }
        QPushButton#treeHolePostButton {
            background: #4d7c65;
            color: white;
            border: none;
            font-weight: bold;
        }
        QPushButton#treeHolePostButton:hover {
            background: #3f6a56;
        }
    )");
}
ThemeManager &ThemeManager::instance()
{
    static ThemeManager s;
    return s;
}

ThemeManager::ThemeManager()
{
    // 从配置读取上次选择的模式，默认 Auto
    QSettings settings;
    const int savedMode = settings.value("theme/mode", static_cast<int>(Auto)).toInt();
    m_mode = static_cast<ThemeMode>(qBound(0, savedMode, 2)); // 安全转换

    // 获取当前系统颜色方案并监听变化（Auto 模式需要）
    if (auto *styleHints = QGuiApplication::styleHints()) {
        m_systemIsDark = (styleHints->colorScheme() == Qt::ColorScheme::Dark);
        connect(styleHints, &QStyleHints::colorSchemeChanged,
                this, &ThemeManager::onSystemColorSchemeChanged);
    }

    // 初始化时应用一次主题
    applyEffectiveTheme();
}

ThemeManager::ThemeMode ThemeManager::themeMode() const
{
    return m_mode;
}

void ThemeManager::setThemeMode(ThemeMode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;

    // 保存到设置
    QSettings settings;
    settings.setValue("theme/mode", static_cast<int>(m_mode));

    // 应用新主题
    applyEffectiveTheme();
}

bool ThemeManager::isDarkMode() const
{
    return (m_mode == Dark) || (m_mode == Auto && m_systemIsDark);
}

void ThemeManager::onSystemColorSchemeChanged()
{
    if (auto *styleHints = QGuiApplication::styleHints()) {
        const bool newSystemDark = (styleHints->colorScheme() == Qt::ColorScheme::Dark);
        if (m_systemIsDark != newSystemDark) {
            m_systemIsDark = newSystemDark;
            // 仅在 Auto 模式下才需要响应系统变化
            if (m_mode == Auto) {
                applyEffectiveTheme();
            }
        }
    }
}

void ThemeManager::applyEffectiveTheme()
{
    if (!qApp) {
        return;
    }

    const bool dark = isDarkMode();
    const QString style = dark ? darkStyleSheet() : lightStyleSheet();

    qApp->setStyleSheet(QString());
    qApp->setStyleSheet(style);

    const auto widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets) {
        if (!widget) {
            continue;
        }

        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }

    emit themeChanged(dark);
}