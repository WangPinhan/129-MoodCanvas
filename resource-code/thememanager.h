#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum ThemeMode {
        Auto,   // 跟随系统
        Light,  // 始终浅色
        Dark    // 始终深色
    };
    Q_ENUM(ThemeMode)

    static ThemeManager &instance();

    ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);

    // 获取当前实际生效的是否深色（根据 mode 和系统状态计算）
    bool isDarkMode() const;

signals:
    void themeChanged(bool isDark);

private:
    ThemeManager();
    void applyEffectiveTheme();
    void onSystemColorSchemeChanged();

    ThemeMode m_mode = Auto;
    bool m_systemIsDark = false;   // 缓存系统当前颜色方案
};

#endif // THEMEMANAGER_H