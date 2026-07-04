#ifndef RESULTDIALOG_H
#define RESULTDIALOG_H

#include "moodcanvaswidget.h"
#include "moodentry.h"

#include <QDialog>
#include <QLabel>
#include <QTextEdit>

/**
 * ResultDialog 展示“生成今日心象”的结果。
 * 用户可以在此确认并保存到本地时间长廊。
 */
class ResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResultDialog(const MoodEntry &entry, QWidget *parent = nullptr);

signals:
    void saveRequested(const MoodEntry &entry);

private:
    MoodEntry m_entry;
    MoodCanvasWidget *m_canvas = nullptr;

};

#endif // RESULTDIALOG_H
