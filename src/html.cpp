#include <QRegularExpression>

#include "html.hpp"

namespace Html {

QString decodeEntities(const QString& input)
{
    static const QRegularExpression entityRe("&(#[0-9]+|#[xX][0-9a-fA-F]+|[a-zA-Z]+);");
    static const struct { const char* name; const char* value; } named[] = {
        { "amp", "&" }, { "lt", "<" }, { "gt", ">" }, { "quot", "\"" }, { "apos", "'" },
        { "nbsp", " " }, { "ndash", "–" }, { "mdash", "—" }, { "hellip", "…" },
        { "rsquo", "’" }, { "lsquo", "‘" }, { "rdquo", "”" }, { "ldquo", "“" },
        { "eacute", "é" }, { "egrave", "è" }, { "aacute", "á" }, { "agrave", "à" },
        { "oacute", "ó" }, { "ouml", "ö" }, { "uuml", "ü" }, { "auml", "ä" },
        { "oslash", "ø" }, { "aring", "å" }, { "aelig", "æ" }, { "ccedil", "ç" },
    };

    QString result;
    result.reserve(input.size());
    int last = 0;

    QRegularExpressionMatchIterator it = entityRe.globalMatch(input);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        result += input.mid(last, m.capturedStart() - last);
        const QString entity = m.captured(1);
        QString replacement = m.captured(0);

        if (entity.startsWith("#x") || entity.startsWith("#X")) {
            bool ok;
            const uint code = entity.mid(2).toUInt(&ok, 16);
            if (ok) {
                const char32_t c = code;
                replacement = QString::fromUcs4(&c, 1);
            }
        } else if (entity.startsWith('#')) {
            bool ok;
            const uint code = entity.mid(1).toUInt(&ok, 10);
            if (ok) {
                const char32_t c = code;
                replacement = QString::fromUcs4(&c, 1);
            }
        } else {
            for (const auto& n : named) {
                if (entity == QLatin1String(n.name)) {
                    replacement = QString::fromUtf8(n.value);
                    break;
                }
            }
        }

        result += replacement;
        last = m.capturedEnd();
    }
    result += input.mid(last);

    return result;
}

QString toText(const QString& html)
{
    static const QRegularExpression scriptRe("<(script|style)\\b.*?</\\1\\s*>",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression brRe("<br\\s*/?>", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tagRe("<[^>]*>");
    static const QRegularExpression wsRe("\\s+");

    QString text = html;
    text.remove(scriptRe);
    text.replace(brRe, " ");
    text.remove(tagRe);
    text = decodeEntities(text);
    text.replace(QChar(0x00A0), ' ');

    return text.replace(wsRe, " ").trimmed();
}

QStringList elements(const QString& html, const QString& tag)
{
    const QRegularExpression re(
        QString("<%1\\b[^>]*>(.*?)</%1\\s*>").arg(QRegularExpression::escape(tag)),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
    );

    QStringList result;
    QRegularExpressionMatchIterator it = re.globalMatch(html);
    while (it.hasNext()) {
        result << it.next().captured(1);
    }

    return result;
}

QList<QStringList> tableRows(const QString& html)
{
    static const QRegularExpression cellRe("<t([dh])\\b[^>]*>(.*?)(?=<t[dh]\\b|</tr\\s*>|$)",
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);

    QList<QStringList> rows;

    for (const QString& rowHtml : elements(html, "tr")) {
        QStringList cells;
        QRegularExpressionMatchIterator it = cellRe.globalMatch(rowHtml);
        while (it.hasNext()) {
            cells << toText(it.next().captured(2));
        }
        if (!cells.isEmpty()) {
            rows << cells;
        }
    }

    return rows;
}

}
