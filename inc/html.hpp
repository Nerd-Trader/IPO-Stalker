#pragma once

#include <QList>
#include <QString>
#include <QStringList>

// Minimal, dependency-free helpers for scraping server-rendered HTML pages
namespace Html {
    // Decode character references (&amp;, &#39;, &#x2F;, &nbsp;, etc)
    QString decodeEntities(const QString& input);

    // Strip all tags, decode entities and collapse whitespace
    QString toText(const QString& html);

    // Return inner HTML of every <tag>...</tag> element found within html (non-nested)
    QStringList elements(const QString& html, const QString& tag);

    // Return all rows of all tables found in html, each row being a list of cell texts (<td> and <th>)
    QList<QStringList> tableRows(const QString& html);
}
