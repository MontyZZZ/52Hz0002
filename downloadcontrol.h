#ifndef DOWNLOADCONTROL_H
#define DOWNLOADCONTROL_H

#include <QObject>
#include <QNetworkReply>
#include <QFile>

class DownloadControl : public QObject
{
    Q_OBJECT
public:
    explicit DownloadControl(QObject *parent = nullptr);

    void init(const QUrl& url, QFile* file, qint64 startBytes, qint64 endBytes = 0);

    void breakDown();

    qint64 getStartPos() const  { return m_startPos; }

    qint64 getEndPos() const  { return m_endPos; }

    qint64 getSeekOffset() const  { return m_seekOffset; }



signals:
    void updateProgres_ssignal(qint64 bytesRead, qint64 totalBytes);
    void finishDownload();

public slots:
    void httpReadyRead();

    void httpFinished();
private:
    QUrl                    m_url;
    QNetworkAccessManager*  m_netManager;
    QNetworkReply*          m_reply;
    QFile*                  m_file;
    qint64                  m_seekOffset;
    qint64                  m_doneSize;

    qint64                  m_startPos;
    qint64                  m_endPos;

};

#endif // DOWNLOADCONTROL_H
