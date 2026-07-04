#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "datastore.h"
#include "plantwidget.h"

#include "userprofile.h"

#include <QMainWindow>

class QFrame;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * MainWindow 是应用主界面，即“首页 / 今日心象”。
 *
 * 最终版主流程：用户输入日记文本，程序调用大模型 API 完成情绪分析，
 * 再生成心象图并允许保存到本地时间长廊。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DataStore *store, const UserProfile &user, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void generateMoodCanvas();
    void openTimeline();
    void openWeeklyReport();
    void openToolbox();
    void openSettings();

private:
    Ui::MainWindow *ui = nullptr;

    DataStore *m_store = nullptr;
    UserProfile m_user;

    QFrame *m_plantCard = nullptr;
    PlantWidget *m_plantWidget = nullptr;
    QLabel *m_plantTitleLabel = nullptr;
    QLabel *m_plantHintLabel = nullptr;

    void setupLogic();
    void setupStyle();
    void fixResponsiveLayout();
    void setupPlantCard();
    void refreshPlantState();
    PlantWidget::PlantState plantStateForEntry(const MoodEntry &entry) const;
};

#endif // MAINWINDOW_H
