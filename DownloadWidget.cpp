#include "DownloadWidget.h"
#include "ui_widget.h"

#include <QStandardPaths>
#include <QMessageBox>
#include <QDateTime>
#include <QFileInfo>
#include <QDesktopServices>
#include <QTimer>
#include <QFileDialog>
#include <QDomDocument>
#include <QDebug>

#define MUL 1

DownloadWidget::DownloadWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_netManager(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("52Hz Download Manager");
    setFixedSize(QSize(620, 140));

    QString downUrl = "http://xz5.jb51.net:81/201604/books/cprimer5_jb51.rar";
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    qDebug() << desktopPath;


    ui->urlEdit->setText(downUrl);
    ui->pathEdit->setText(desktopPath);

    m_preFile = new QFile;
    m_file = new QFile;
    m_netManager = new QNetworkAccessManager;

    m_tCount = 0;

    m_sizeMS = 0;
    connect(&m_timer, &QTimer::timeout, this, &DownloadWidget::setProgress);
}

DownloadWidget::~DownloadWidget()
{
    delete ui;
}


void DownloadWidget::savePosInfo(QVector<QVector<qint64>> &breakTable)
{
    //创建xml文件
    QString fileName = qApp->applicationDirPath() + "/break.xml";
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    QTextStream out(&file);
    QDomDocument doc;
    QDomText text;
    QDomElement element;
    QDomAttr attr;
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction( "xml", "version = \'1.0\' encoding=\'UTF-8\'" );
    doc.appendChild( instruction );

    QDomElement root = doc.createElement( "BreakInfo" );
    doc.appendChild(root);

    element = doc.createElement( "Path" );
    element.setAttribute("V", m_downFilePath);

    root.appendChild(element);

    element = doc.createElement( "TCount" );
    element.setAttribute("V", ui->threadSpinBox->text());
    root.appendChild(element);

    element = doc.createElement( "Breaks" );
    root.appendChild(element);

    QVector<qint64> breakInfo;
    for (int i = 0; i < breakTable.size(); ++i)
    {
        breakInfo = breakTable.at(i);
        QDomElement breakElement = doc.createElement( "Break" );

        attr = doc.createAttribute( "StartPos" );
        attr.setValue(QString::number(breakInfo.at(0)));
        breakElement.setAttributeNode(attr);


        attr = doc.createAttribute( "EndPos" );
        attr.setValue(QString::number(breakInfo.at(1)));
        breakElement.setAttributeNode(attr);

        attr = doc.createAttribute( "SeekPos" );
        attr.setValue(QString::number(breakInfo.at(2)));
        breakElement.setAttributeNode(attr);

        element.appendChild(breakElement);
    }

    doc.save(out, 4); //输出时，缩进4格
}


// 获取暂停下载保存的信息
bool DownloadWidget::getPosInfo()
{
    QString fileName = qApp->applicationDirPath() + "/break.xml";
    QFile file(fileName);//打开或创建文件

    if (!file.exists()) {
        qDebug() << "file not exists!";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "file open failed!";
        return false;
    }

    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();

        qDebug() << "doc open .xml error!";
        return false;
    }
    file.close();

    QDomElement root = doc.documentElement(); //返回根节点
    qDebug()<<root.nodeName();

    m_breakTable.clear();
    for (QDomNode node = root.firstChild(); !node.isNull(); node = node.nextSibling())
    {
        qDebug() << node.nodeName();
        if (node.nodeName() == "Path")
        {
            QString path = node.toElement().attribute("V") + ".52Hz";
            file.setFileName(path);
            if (!file.exists()) {
                return false;
            }
        }
        else if (node.nodeName() == "TCount")
        {
            QString count = node.toElement().attribute("V");
            if (count != ui->threadSpinBox->text()) {
                qDebug() << "tCount differ";
                return false;
            }

        }
        else if (node.nodeName() == "Breaks")
        {
            for (QDomNode tnode = node.firstChild(); !tnode.isNull(); tnode = tnode.nextSibling())
            {
                QVector<qint64> breaks;
                breaks.append(tnode.toElement().attribute("StartPos").toInt());
                breaks.append(tnode.toElement().attribute("EndPos").toInt());
                breaks.append(tnode.toElement().attribute("SeekPos").toInt());
                qDebug() << tnode.toElement().attribute("StartPos")
                         << tnode.toElement().attribute("EndPos")
                         << tnode.toElement().attribute("SeekPos");
                m_breakTable.append(breaks);
            }
        }
    }
    return true;
}

void DownloadWidget::changeStatus()
{
    if (ui->downBtn->isEnabled())
    {
        ui->downBtn->setEnabled(false);
        ui->urlEdit->setEnabled(false);
        ui->pathBtn->setEnabled(false);
        ui->pathEdit->setEnabled(false);
        ui->threadSpinBox->setEnabled(false);
    }
    else
    {
        ui->downBtn->setEnabled(true);
        ui->urlEdit->setEnabled(true);
        ui->pathBtn->setEnabled(true);
        ui->pathEdit->setEnabled(true);
        ui->threadSpinBox->setEnabled(true);
    }
}

void DownloadWidget::deleteBreakXml()
{
    QString fileName = qApp->applicationDirPath() + "/break.xml";
    QFile file;
    file.remove(fileName);
}

// download  and propare get file size
void DownloadWidget::on_downBtn_clicked()
{
    qDebug() << "prepare start download!~~";

    QString urlString = ui->urlEdit->text().trimmed();
    if (urlString.isEmpty()) {
        //QString errorStr = "Address Blank!~~";
        QMessageBox::warning(this, "错误", "下载地址为空！");
        return;
    }

    const QUrl newUrl = QUrl::fromUserInput(urlString);
    if (!newUrl.isValid()) {
        //QString errorStr = QString("Invalid URL: %1: %2").arg(urlString, newUrl.errorString());
        QMessageBox::warning(this, "错误", "不可用地址！");
        return;
    }

    QString fileName = newUrl.fileName();
    if (fileName.isEmpty()) {
        fileName = QDateTime::currentDateTime().toString("yyyy.MM.dd.hh.mm.ss") + ".html";
    }

    QString downDir = ui->pathEdit->text().trimmed();
    QString downloadPath = downDir + "/" + fileName;
    qDebug() << "fileName: " << fileName;
    qDebug() << "download full path:  " << downloadPath;

    m_downFilePath = downloadPath;
    downloadPath += ".52Hz";

    m_preFile->setFileName(downloadPath);
    if (!m_preFile->open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "错误", "无法保存文件！");
        m_preFile->close();
        return;
    }

    m_url = newUrl;

    m_preReply = m_netManager->head(QNetworkRequest(newUrl));
    connect(m_preReply, &QNetworkReply::finished, this, &DownloadWidget::startDownLoad);

}

// open file
void DownloadWidget::on_pathBtn_clicked()
{
    // set path
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    // qDebug() << dir;
    ui->pathEdit->setText(dir);
}

// 断点续传
void DownloadWidget::on_breakBtn_clicked()
{
    m_timer.stop();
    m_breakTable.clear();
    ui->dsLabel->clear();
    if (ui->breakBtn->text() == "暂停")
    {
        ui->breakBtn->setText("开始");
        DownloadControl* control = nullptr;
        //QVector<QVector<qint64>> breakTable;
        for (int i = 0; i < m_vecDownload.size(); i++)
        {
            control = m_vecDownload.at(i);
            control->breakDown();
            QVector<qint64> vecPos;
            vecPos.append(control->getStartPos());
            vecPos.append(control->getEndPos());
            vecPos.append(control->getSeekOffset());

            m_breakTable.append(vecPos);
            //qDebug() << control->getStartPos() << control->getEndPos() << control->getSeekOffset();
        }

        savePosInfo(m_breakTable);

        m_breakTable.clear();
        m_vecDownload.clear();
        m_sizeMS = 0;

        changeStatus();
    }
    else
    {
        startDownLoad();
    }
}

void DownloadWidget::startDownLoad()
{
    qDebug() << "start download!~~";

    qint64 totalBytes = m_preReply->rawHeader("Content-Length").toLongLong();
    qDebug() << "total size: " << totalBytes;

    if (totalBytes > 0) {
        m_preReply->abort();
        m_preFile->close();
    } else {
        m_preFile->close();
        return;
    }

    m_preFile->close();
    m_file->setFileName(m_downFilePath + ".52Hz");

    qint64 downloadedBytes = m_file->size();
    qDebug() << "down size: " << downloadedBytes;

    if (downloadedBytes == totalBytes) {
        QMessageBox::warning(this, "错误", "文件已经完整存在！");
        return;
    }

    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
        QMessageBox::warning(this, "错误", "无法保存文件！");
        return;
    }

    m_tCount = 0;

    bool isBreakPoint = getPosInfo();

    ui->breakBtn->setText("暂停");
    QString tStr = QString::number(totalBytes / 1024.00 / 1024.00)  + " M";
    ui->tsLabel->setText(tStr);
    ui->downloadProgressBar->setMaximum(static_cast<int>(totalBytes));

    changeStatus();
    //qDebug() << "mul download";
    int tNum = ui->threadSpinBox->text().toInt();


    QVector<QPair<qlonglong, qlonglong>> vec(tNum);
    m_progressSize = 0;
    if (isBreakPoint)
    {
        for (int i = 0; i < tNum; ++i) {
            vec[i].first = m_breakTable.at(i).at(2);
            vec[i].second = m_breakTable.at(i).at(1);
            m_progressSize += m_breakTable.at(i).at(2) - m_breakTable.at(i).at(0);
        }

        deleteBreakXml();
    }
    else
    {
        qlonglong segmentSize = totalBytes / tNum;
        for (int i = 0; i < tNum; ++i) {
            vec[i].first = i * segmentSize;
            vec[i].second = i * segmentSize + segmentSize - 1;
        }

        //余数部分加入最后一个
        vec[tNum -1].second = totalBytes;
    }

    qDebug() << vec;

    for (int i = 0; i < tNum; ++i) {
        qint64 bBytes = vec.at(i).first;
        qint64 eBytes = vec.at(i).second;

        qDebug() << i << bBytes << eBytes;
        DownloadControl* down = new DownloadControl;
        down->init(m_url, m_file, bBytes, eBytes);
        connect(down, &DownloadControl::updateProgres_ssignal, this, &DownloadWidget::updateProgress);
        connect(down, &DownloadControl::finishDownload, this, &DownloadWidget::finishedDownload);
        m_vecDownload.append(down);
    }

    m_timer.start(1000);
}

void DownloadWidget::updateProgress(qint64 bytesRead, qint64 totalBytes)
{
    m_progressSize += bytesRead;
    m_sizeMS += bytesRead;
}

void DownloadWidget::finishedDownload()
{
    qDebug() << "finished download";
    m_tCount++;
    if (m_tCount == ui->threadSpinBox->text().toInt()) {
        m_file->close();
        m_file->rename(m_downFilePath);

        m_timer.stop();
        ui->downloadProgressBar->reset();
        ui->dsLabel->clear();
        ui->tsLabel->clear();
        changeStatus();

        // open download
        if (ui->openCheckBox->checkState())
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_downFilePath));
        }
    }
}

void DownloadWidget::setProgress()
{
    //qDebug() << "setProgress" << m_progressSize << ui->downloadProgressBar->maximum();
    ui->downloadProgressBar->setValue(static_cast<int>(m_progressSize));
    QString tStr = QString::number(m_sizeMS / 1024.00 / 1024.00)  + " M/S";
    ui->dsLabel->setText(tStr);

    m_sizeMS = 0;
}

void DownloadWidget::closeEvent(QCloseEvent *event)
{
    deleteBreakXml();
    m_file->close();
}


