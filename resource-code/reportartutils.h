#ifndef REPORTARTUTILS_H
#define REPORTARTUTILS_H

#include <QFont>
#include <QString>

class ReportArtUtils
{
public:
    static QString displayFontFamily();
    static QString scriptFontFamily();
    static QFont displayFont(int pointSize);
    static QFont scriptFont(int pointSize);
    static QString weekEpithet(double avgValence);
    static QString formatInsightHtml(const QString &text);
    static QString formatAdviceHtml(const QString &text);
};

#endif // REPORTARTUTILS_H
