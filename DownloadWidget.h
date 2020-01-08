#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QFile>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include "downloadcontrol.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class DownloadWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadWidget(QWidget *parent = nullptr);
    ~DownloadWidget() override;

    void savePosInfo(QVector<QVector<qint64>>& breakTable);

    bool getPosInfo();

    void changeStatus();

    void close();

    void deleteBreakXml();

private slots:
    void on_downBtn_clicked();

    void on_pathBtn_clicked();

    void on_breakBtn_clicked();

    void startDownLoad();

    void updateProgress(qint64 bytesRead, qint64 totalBytes);

    void finishedDownload();

    void setProgress();

    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Widget *ui;
    QFile*                  m_preFile;
    QFile*                  m_file;
    QUrl                    m_url;

    QNetworkAccessManager*  m_netManager;
    QNetworkReply*          m_preReply;
    qint64                  m_progressSize;

    QVector<DownloadControl* >    m_vecDownload;

    int                     m_tCount;
    QTimer                  m_timer;
    qint64                  m_sizeMS;
    QString                 m_downFilePath;
    QVector<QVector<qint64>>    m_breakTable;
};
#endif // WIDGET_H
