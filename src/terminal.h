#ifndef TERMINAL_H
#define TERMINAL_H

#include <QObject>
#include <QString>
#include <ostream>

#include "encoding.h"

#ifdef Q_OS_WIN32
    #define EMPTY                ""
    #define NONE                 EMPTY
    #define BLACK                EMPTY
    #define L_BLACK              EMPTY
    #define RED                  EMPTY
    #define L_RED                EMPTY
    #define GREEN                EMPTY
    #define L_GREEN              EMPTY
    #define BROWN                EMPTY
    #define YELLOW               EMPTY
    #define BLUE                 EMPTY
    #define L_BLUE               EMPTY
    #define PURPLE               EMPTY
    #define L_PURPLE             EMPTY
    #define CYAN                 EMPTY
    #define L_CYAN               EMPTY
    #define GRAY                 EMPTY
    #define WHITE                EMPTY

    #define BOLD                 EMPTY
    #define UNDERLINE            EMPTY
    #define BLINK                EMPTY
    #define REVERSE              EMPTY
    #define HIDE                 EMPTY
    #define CLEAR                EMPTY
    #define CLRLINE              EMPTY
#else
    #define NONE                 "\e[0m"
    #define BLACK                "\e[0;30m"
    #define L_BLACK              "\e[1;30m"
    #define RED                  "\e[0;31m"
    #define L_RED                "\e[1;31m"
    #define GREEN                "\e[0;32m"
    #define L_GREEN              "\e[1;32m"
    #define BROWN                "\e[0;33m"
    #define YELLOW               "\e[1;33m"
    #define BLUE                 "\e[0;34m"
    #define L_BLUE               "\e[1;34m"
    #define PURPLE               "\e[0;35m"
    #define L_PURPLE             "\e[1;35m"
    #define CYAN                 "\e[0;36m"
    #define L_CYAN               "\e[1;36m"
    #define GRAY                 "\e[0;37m"
    #define WHITE                "\e[1;37m"

    #define BOLD                 "\e[1m"
    #define UNDERLINE            "\e[4m"
    #define BLINK                "\e[5m"
    #define REVERSE              "\e[7m"
    #define HIDE                 "\e[8m"
    #define CLEAR                "\e[2J"
    #define CLRLINE              "\r\e[K" //or "\e[1K\r"
#endif


class Terminal: public QObject
{
    Q_OBJECT

public:
    static Terminal* instance();

    QString getEncoding() const;
    bool setEncoding(const QString& encoding);

    void cout(const QString& string, const bool newline = true) const;
    void cerr(const QString& string, const bool newline = true) const;

signals:
    void encodingChanged(const QString& encoding);

private:
    void output(std::ostream& out, const QString& string, const bool newline) const;

private:
    Terminal();
    Encoding m_encoding;
};

#endif // TERMINAL_H
