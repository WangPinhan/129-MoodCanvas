#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "datastore.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;

/**
 * SettingsDialog 对应“设置 / 隐私”：配置大模型接口、说明本地数据路径、导出日记 JSON。
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(DataStore *store, QWidget *parent = nullptr);

private:
    DataStore *m_store = nullptr;

    QCheckBox *m_llmEnabledCheck = nullptr;
    QLineEdit *m_llmApiKeyEdit = nullptr;
    QLineEdit *m_llmBaseUrlEdit = nullptr;
    QLineEdit *m_llmModelEdit = nullptr;
    QLineEdit *m_cloudServerEdit = nullptr;

    void saveLlmSettings();
    void saveCloudSettings();
    void testCloudConnection();
    void testLlmConnection();
    void exportData();
};

#endif // SETTINGSDIALOG_H
