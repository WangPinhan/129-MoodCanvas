#include "reportartutils.h"

#include <QFontDatabase>
#include <QRegularExpression>

namespace {
QString firstAvailableFamily(const QStringList &candidates)
{
    const QFontDatabase db;
    for (const QString &family : candidates) {
        if (db.hasFamily(family)) {
            return family;
        }
    }
    return QString();
}
QString escapeHtml(const QString &text)
{
    QString out = text.toHtmlEscaped();
    out.replace('\n', QStringLiteral("<br/>"));
    return out;
}
QString firstSentence(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return trimmed;
    }
    static const QRegularExpression splitter(
        QStringLiteral(R"([。！？!?])"));
    const QRegularExpressionMatch match = splitter.match(trimmed);
    if (match.hasMatch()) {
        int end = match.capturedEnd();
        const QString trailingClosers = QStringLiteral("\"''」』》）) ");
        while (end < trimmed.length() && trailingClosers.contains(trimmed.at(end))) {
            ++end;
        }
        return trimmed.left(end).trimmed();
    }
    return trimmed.length() > 36 ? trimmed.left(36).trimmed() + QStringLiteral("…") : trimmed;
}
QString bodyAfterFirstSentence(const QString &text)
{
    const QString lead = firstSentence(text);
    const QString trimmed = text.trimmed();
    if (lead.isEmpty() || lead == trimmed) {
        return QString();
    }
    return trimmed.mid(lead.length()).trimmed();
}
QString richBlock(const QString &lead, const QString &body, const QString &leadColor)
{
    const QString scriptFamily = ReportArtUtils::scriptFontFamily();
    const QString bodyFamily = ReportArtUtils::displayFontFamily();
    QString html;
    html += QStringLiteral("<p style=\"margin:0 0 14px 0;"
                           "font-family:'%1';"
                           "font-size:18px;"
                           "line-height:1.75;"
                           "color:%2;"
                           "letter-spacing:1px;\">%3</p>")
                .arg(scriptFamily, leadColor, escapeHtml(lead));

    if (!body.isEmpty()) {
        html += QStringLiteral("<p style=\"margin:0;"
                               "font-family:'%1';"
                               "font-size:15px;"
                               "line-height:1.85;"
                               "color:#3f4b54;\">%2</p>")
                    .arg(bodyFamily, escapeHtml(body));
    }
    return html;
}
}
QString ReportArtUtils::displayFontFamily()
{
    static const QString family = firstAvailableFamily(
        {QStringLiteral("Microsoft YaHei UI"),
         QStringLiteral("Microsoft YaHei"),
         QStringLiteral("PingFang SC"),
         QStringLiteral("Segoe UI")});
    return family.isEmpty() ? QStringLiteral("Sans Serif") : family;
}
QString ReportArtUtils::scriptFontFamily()
{
    static const QString family = firstAvailableFamily({QStringLiteral("STXingkai"),QStringLiteral("KaiTi"),QStringLiteral("STKaiti"),QStringLiteral("FangSong"),QStringLiteral("SimSun"),QStringLiteral("Microsoft YaHei UI")});
    return family.isEmpty()?displayFontFamily():family;
}
QFont ReportArtUtils::displayFont(int pointSize)
{
    QFont font(displayFontFamily(), pointSize);
    font.setWeight(QFont::DemiBold);
    return font;
}
QFont ReportArtUtils::scriptFont(int pointSize)
{
    QFont font(scriptFontFamily(), pointSize);
    return font;
}
QString ReportArtUtils::weekEpithet(double avgValence)
{
    if (avgValence >= 0.35) {
        return QStringLiteral("向光而行的这一周");
    }
    if (avgValence >= 0.05) {
        return QStringLiteral("温软起伏的一周");
    }
    if (avgValence >= -0.25) {
        return QStringLiteral("与风雨共处的一周");
    }
    return QStringLiteral("允许悲伤存在的一周");
}
QString formatNarrativeHtml(const QString &text, const QString &leadColor)
{
    return richBlock(firstSentence(text),bodyAfterFirstSentence(text),leadColor);
}
QString ReportArtUtils::formatInsightHtml(const QString &text)
{
    return formatNarrativeHtml(text, QStringLiteral("#6a5878"));
}
QString ReportArtUtils::formatAdviceHtml(const QString &text)
{
    return formatNarrativeHtml(text, QStringLiteral("#4f7568"));
}