#include "terminal.h"

#include <QCoreApplication>
#include <QDebug>
#include <iostream>

static Terminal* terminal_instance = 0;

Terminal* Terminal::instance()
{
    if (!terminal_instance) {
        terminal_instance = new Terminal();
    }

    return terminal_instance;
}

Terminal::Terminal()
    : QObject(QCoreApplication::instance())
{
}

QString Terminal::getEncoding() const
{
    return m_encoding.getName();
}

bool Terminal::setEncoding(const QString& encoding)
{
    // Since there can be multiple names for the same codec (i.e., "utf8" and
    // "utf-8"), we need to get the codec in the system first and use its
    // canonical name
    QTextCodec* codec = QTextCodec::codecForName(encoding.toLatin1());
    if (!codec) {
        return false;
    }

    // Check whether encoding actually needs to be changed
    const QString encodingBeforeUpdate(m_encoding.getName());
    if (0 == encodingBeforeUpdate.compare(QString(codec->name()), Qt::CaseInsensitive)) {
        return false;
    }

    m_encoding.setEncoding(encoding);

    // Emit the signal only if the encoding actually was changed
    const QString encodingAfterUpdate(m_encoding.getName());
    if (0 == encodingBeforeUpdate.compare(encodingAfterUpdate, Qt::CaseInsensitive)) {
        return false;
    }

    emit encodingChanged(encoding);

    return true;
}

void Terminal::cout(const QString& string, const bool newline) const
{
    output(std::cout, string, newline);
}

void Terminal::cerr(const QString& string, const bool newline) const
{
    output(std::cerr, string, newline);
}

// private
void Terminal::output(std::ostream& out, const QString& string, const bool newline) const
{
    out << m_encoding.encode(string).constData();
    if (newline) {
        out << std::endl;
    }
    out << std::flush;
}
