#include "downloadcontrol.h"

#include <QFileInfo>

DownloadControl::DownloadControl(QObject *parent) :
    QObject(parent)
{
    m_file = new QFile;
    m_netManager = new QNetworkAccessManager;
    m_seekOffset = 0;
    m_doneSize = 0;
}

void DownloadControl::init(const QUrl &url, QFile* file, qint64 startBytes, qint64 endBytes)
{
    m_url = url;
    //m_file->setFileName(filePath);
    m_file = file;

    m_startPos = startBytes;
    m_endPos = endBytes;

    QNetworkRequest request(m_url);

    // 断点续传 分段下载
    m_seekOffset = startBytes;
    qDebug() << m_seekOffset;
    qDebug() << QString("bytes=%1-%2").arg(startBytes).arg(endBytes).toLocal8Bit();
    request.setRawHeader(QByteArray("Range"),
                         QString("bytes=%1-%2").arg(startBytes).arg(endBytes).toLocal8Bit());


    m_reply = m_netManager->get(request);

    connect(m_reply, &QNetworkReply::finished, this, &DownloadControl::httpFinished);
    connect(m_reply, &QNetworkReply::readyRead, this, &DownloadControl::httpReadyRead);
}

void DownloadControl::breakDown()
{
    if (m_reply && m_reply->isRunning())
    {
        m_reply->abort();
        qDebug() << "StratPos: " << m_startPos << "EndPos: " << m_endPos << "seekPos: " << m_seekOffset;
    }
}

void DownloadControl::httpReadyRead()
{
    if (m_file)
    {
        if (m_file->seek(m_seekOffset))
        {
            qint64 offset = m_file->write(m_reply->readAll());
            m_seekOffset += offset;
            emit updateProgres_ssignal(offset, 0);
        }
    }
}

void DownloadControl::httpFinished()
{
    qDebug() << "httpFinished";

    const QVariant redirectionTarget = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    m_reply->deleteLater();
    m_reply = nullptr;

    if (!redirectionTarget.isNull()) {
        const QUrl redirectedUrl = m_url.resolved(redirectionTarget.toUrl());
        return;
    }

    qDebug() << m_startPos << m_endPos;
    emit finishDownload();

}
