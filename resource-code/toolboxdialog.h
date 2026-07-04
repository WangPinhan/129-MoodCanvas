#ifndef TOOLBOXDIALOG_H
#define TOOLBOXDIALOG_H

#include "cloudapiclient.h"
#include "cloudtreehole.h"
#include "datastore.h"
#include "treeholeemotion.h"
#include "userprofile.h"

#include <QDialog>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>

class MoodCanvasWidget;

class ToolboxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToolboxDialog(DataStore *store, const UserProfile &user, QWidget *parent = nullptr);
    ~ToolboxDialog() override;

private:
    DataStore *m_store = nullptr;
    UserProfile m_user;
    CloudApiClient m_api;
    CloudTreehole m_cloud;
    TreeholeEmotionAnalyzer m_emotionAnalyzer;

    MoodCanvasWidget *m_previewCanvas = nullptr;
    QLabel *m_previewInfoLabel = nullptr;
    QLabel *m_networkStatusLabel = nullptr;
    QListWidget *m_treeList = nullptr;
    QPlainTextEdit *m_replyEdit = nullptr;
    QPushButton *m_replyButton = nullptr;

    QWidget *createTreeHolePage();

    MoodEntry latestEntry() const;
    void refreshTreeList();
    void requestEmotionsForPosts(const QVector<QJsonObject> &posts);
    void updatePostItemWidget(QListWidgetItem *item, const QJsonObject &post);
    void submitTreeHolePost(const QString &moodLine);
    void submitReply();
    void updateNetworkUi();
    QString selectedPostId() const;
    QJsonObject buildPostPayload(const QString &moodLine) const;
};

#endif // TOOLBOXDIALOG_H
