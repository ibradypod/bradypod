#ifndef ENCODING_H
#define ENCODING_H

#include <QTextCodec>

class Encoding
{
public:
    Encoding();
    Encoding(const QString& encoding);
    ~Encoding();

    QString decode(const QByteArray& bytes) const;
    QByteArray encode(const QString& string) const;

    QString getName() const;
    void setEncoding(const QString& encoding);

    QTextCodec* getCodec() const;

    static const Encoding UTF8;

private:
    QTextCodec* m_codec;
    static const QByteArray DEFAULT_CODEC_NAME;
};

#endif // ENCODING_H
