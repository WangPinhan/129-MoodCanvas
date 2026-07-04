#ifndef TIMELINEWINDOW_H
#define TIMELINEWINDOW_H

#include "datastore.h"
#include "moodentry.h"

#include <QDate>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

/**
 * TimelineWindow 对应“时间长廊 / 情绪日历”。
 *
 * 新版支持同一天多条记录：
 * - 日历色块使用当天最后一条记录的主色；
 * - 点击日期时，右侧会列出这一天的全部记录，并可删除单条心象日记。
 */
class TimelineWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TimelineWindow(DataStore *store, QWidget *parent = nullptr);

private:
    DataStore *m_store = nullptr;
    QDate m_currentMonth;
    QDate m_selectedDate;
    QGridLayout *m_calendarGrid = nullptr;
    QLabel *m_monthLabel = nullptr;
    QLabel *m_sceneLabel = nullptr;
    QFrame *m_detailPanel = nullptr;
    QWidget *m_detailContainer = nullptr;
    QVBoxLayout *m_detailLayout = nullptr;

    void refreshCalendar();
    void showEntryDetail(const QDate &date);
    void clearDetailPanel();
    void appendEntryCard(const MoodEntry &entry, int index, const QDate &date);
    void confirmDeleteEntry(const MoodEntry &entry);
    QColor colorForDate(const QDate &date, const QVector<MoodEntry> &entries) const;
    int countForDate(const QDate &date, const QVector<MoodEntry> &entries) const;
    MoodEntry latestEntryForDate(const QDate &date) const;
    QString monthSceneStyle() const;
    QString monthSceneText() const;
    QString detailPanelStyleForDate(const QDate &date) const;
    QString entryCardStyle(const MoodEntry &entry, const QDate &date) const;


//add by HYH
    struct MonthSceneColors {
        QString dialogGradient;      // 例如 "qlineargradient(...)"
        QString sceneBackground;     // 例如 "rgba(255,255,255,205)"
    };
    MonthSceneColors monthSceneColors() const;
//end by HYH
};
#endif // TIMELINEWINDOW_H
