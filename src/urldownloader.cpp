/*
    This file is part of DocScan.

    DocScan is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    DocScan is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DocScan.  If not, see <https://www.gnu.org/licenses/>.


    Source code written by Thomas Fischer <thomas.fischer@his.se>

 */


#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegExp>
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSignalMapper>
#include <QMutex>

#include "urldownloader.h"
#include "watchdog.h"
#include "general.h"

UrlDownloader::UrlDownloader(QNetworkAccessManager *networkAccessManager, const QString &filePattern, QObject *parent)
    : Downloader(parent), m_networkAccessManager(networkAccessManager), m_filePattern(filePattern), m_runningDownloads(0)
{
    m_countSuccessfulDownloads = m_countFaileDownloads = 0;
    m_signalMapperTimeout = new QSignalMapper(this);
    connect(m_signalMapperTimeout, SIGNAL(mapped(QObject *)), this, SLOT(timeout(QObject *)));
    m_setRunningJobs = new QSet<QNetworkReply *>();
    m_mutexRunningJobs = new QMutex();
}

UrlDownloader::~UrlDownloader()
{
    delete m_signalMapperTimeout;
    delete m_setRunningJobs;
    delete m_mutexRunningJobs;
}

bool UrlDownloader::isAlive()
{
    return m_runningDownloads > 0;
}

void UrlDownloader::download(QUrl url)
{
    /// avoid duplicate URLs
    QString urlString = url.toString();
    if (m_knownUrls.contains(urlString))
        return;
    else
        m_knownUrls.insert(urlString);

    ++m_runningDownloads;

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));

    m_mutexRunningJobs->lock();
    m_setRunningJobs->insert(reply);
    m_mutexRunningJobs->unlock();
    QTimer *timer = new QTimer(reply);
    connect(timer, SIGNAL(timeout()), m_signalMapperTimeout, SLOT(map()));
    m_signalMapperTimeout->setMapping(timer, reply);
    timer->start(10000 + m_runningDownloads * 200);
}

void UrlDownloader::finalReport()
{
    QString logText = QString("<download count-success=\"%1\" count-fail=\"%2\" />\n").arg(m_countSuccessfulDownloads).arg(m_countFaileDownloads);
    emit downloadReport(logText);
}

void UrlDownloader::finished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    m_mutexRunningJobs->lock();
    m_setRunningJobs->remove(reply);
    m_mutexRunningJobs->unlock();

    bool succeeded = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data(reply->readAll());
        QString filename = m_filePattern;

        QString host = reply->url().host().replace(QRegExp("[^.a-z-]", Qt::CaseInsensitive), "X");
        if (domainRegExp.indexIn(host) >= 0)
            filename = filename.replace("%{d}", domainRegExp.cap(0));
        else
            filename = filename.replace("%{d}", host);

        QString md5sum = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
        QRegExp md5sumRegExp("%\\{h(:(\\d+))?\\}");
        int p = -1;
        while ((p = md5sumRegExp.indexIn(filename)) >= 0) {
            if (md5sumRegExp.cap(1).isEmpty())
                filename = filename.replace(md5sumRegExp.cap(0), md5sum);
            else {
                bool ok = false;
                int left = md5sumRegExp.cap(2).toInt(&ok);
                if (ok && left > 0 && left <= md5sum.length())
                    filename = filename.replace(md5sumRegExp.cap(0), md5sum.left(left));
            }
        }

        QString urlString = reply->url().toString().replace(QRegExp("\\?.*$"), "").replace(QRegExp("[^a-z0-9]", Qt::CaseInsensitive), "_").replace(QRegExp("_([a-z0-9]{1,4})$", Qt::CaseInsensitive), ".\\1");
        filename = filename.replace("%{s}", urlString);

        if ((p = filename.indexOf("%{")) >= 0)
            qDebug() << "gap was not filled:" << filename.mid(p);

        QFileInfo fi(filename);
        if (!fi.absoluteDir().mkpath(fi.absolutePath())) {
            qCritical() << "Cannot create directory" << fi.absolutePath();
        } else {
            QFile output(filename);
            if (output.open(QIODevice::WriteOnly)) {
                output.write(data);
                output.close();

                QString logText = QString("<download url=\"%1\" filename=\"%2\" status=\"success\"/>\n").arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(filename));
                emit downloadReport(logText);
                succeeded = true;

                emit downloaded(reply->url(), filename);
                emit downloaded(filename);

                qDebug() << "Downloaded URL " << reply->url().toString() << " to " << filename;
            }
        }
    }

    if (!succeeded) {
        QString logText = QString("<download url=\"%1\" message=\"download-failed\" detailed=\"%2\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString())).arg(DocScan::xmlify(reply->errorString()));
        emit downloadReport(logText);
        ++m_countFaileDownloads;
    } else
        ++m_countSuccessfulDownloads;

    QCoreApplication::instance()->processEvents();

    reply->deleteLater();

    --m_runningDownloads;
}

void UrlDownloader::timeout(QObject *object)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(object);
    m_mutexRunningJobs->lock();
    if (m_setRunningJobs->contains(reply)) {
        m_setRunningJobs->remove(reply);
        m_mutexRunningJobs->unlock();
        reply->close();
        QString logText = QString("<download url=\"%1\" message=\"timeout\" status=\"error\"/>\n").arg(DocScan::xmlify(reply->url().toString()));
        emit downloadReport(logText);
    } else
        m_mutexRunningJobs->unlock();
}

const QRegExp UrlDownloader::domainRegExp = QRegExp("[^.]+\\.(ac|com\\.ac|edu\\.ac|gov\\.ac|net\\.ac|mil\\.ac|org\\.ac|ad|nom\\.ad|ae|net\\.ae|gov\\.ae|org\\.ae|mil\\.ae|sch\\.ae|ac\\.ae|pro\\.ae|name\\.ae|aero|af|gov\\.af|edu\\.af|net\\.af|com\\.af|ag|com\\.ag|org\\.ag|net\\.ag|co\\.ag|nom\\.ag|ai|off\\.ai|com\\.ai|net\\.ai|org\\.ai|al|gov\\.al|edu\\.al|org\\.al|com\\.al|net\\.al|uniti\\.al|tirana\\.al|soros\\.al|upt\\.al|inima\\.al|am|an|com\\.an|net\\.an|org\\.an|edu\\.an|ao|co\\.ao|ed\\.ao|gv\\.ao|it\\.ao|og\\.ao|pb\\.ao|ar|com\\.ar|gov\\.ar|int\\.ar|mil\\.ar|net\\.ar|org\\.ar|arpa|addr\\.arpa|iris\\.arpa|uri\\.arpa|urn\\.arpa|as|at|gv\\.at|ac\\.at|co\\.at|or\\.at|priv\\.at|au|asn\\.au|com\\.au|net\\.au|id\\.au|org\\.au|csiro\\.au|oz\\.au|info\\.au|conf\\.au|act\\.au|nsw\\.au|nt\\.au|qld\\.au|sa\\.au|tas\\.au|vic\\.au|wa\\.au|gov\\.au|edu\\.au|aw|com\\.aw|ax|az|com\\.az|net\\.az|int\\.az|gov\\.az|biz\\.az|org\\.az|edu\\.az|mil\\.az|pp\\.az|name\\.az|info\\.az|ba|bb|com\\.bb|edu\\.bb|gov\\.bb|net\\.bb|org\\.bb|bd|com\\.bd|edu\\.bd|net\\.bd|gov\\.bd|org\\.bd|mil\\.bd|be|ac\\.be|bf|gov\\.bf|bg|bh|bi|biz|bj|bm|com\\.bm|edu\\.bm|org\\.bm|gov\\.bm|net\\.bm|bn|com\\.bn|edu\\.bn|org\\.bn|net\\.bn|bo|com\\.bo|org\\.bo|net\\.bo|gov\\.bo|gob\\.bo|edu\\.bo|tv\\.bo|mil\\.bo|int\\.bo|br|agr\\.br|am\\.br|art\\.br|edu\\.br|com\\.br|coop\\.br|esp\\.br|far\\.br|fm\\.br|gov\\.br|imb\\.br|ind\\.br|inf\\.br|mil\\.br|net\\.br|org\\.br|psi\\.br|rec\\.br|srv\\.br|tmp\\.br|tur\\.br|tv\\.br|etc\\.br|adm\\.br|adv\\.br|arq\\.br|ato\\.br|bio\\.br|bmd\\.br|cim\\.br|cng\\.br|cnt\\.br|ecn\\.br|eng\\.br|eti\\.br|fnd\\.br|fot\\.br|fst\\.br|ggf\\.br|jor\\.br|lel\\.br|mat\\.br|med\\.br|mus\\.br|not\\.br|ntr\\.br|odo\\.br|ppg\\.br|pro\\.br|psc\\.br|qsl\\.br|slg\\.br|trd\\.br|vet\\.br|zlg\\.br|dpn\\.br|nom\\.br|bs|com\\.bs|net\\.bs|org\\.bs|bt|com\\.bt|edu\\.bt|gov\\.bt|net\\.bt|org\\.bt|bw|co\\.bw|org\\.bw|by|gov\\.by|mil\\.by|bz|ca|ab\\.ca|bc\\.ca|mb\\.ca|nb\\.ca|nf\\.ca|nl\\.ca|ns\\.ca|nt\\.ca|nu\\.ca|on\\.ca|pe\\.ca|qc\\.ca|sk\\.ca|yk\\.ca|cat|cc|co\\.cc|cd|com\\.cd|net\\.cd|org\\.cd|cf|ch|com\\.ch|net\\.ch|org\\.ch|gov\\.ch|ck|co\\.ck|cl|cn|ac\\.cn|com\\.cn|edu\\.cn|gov\\.cn|net\\.cn|org\\.cn|ah\\.cn|bj\\.cn|cq\\.cn|fj\\.cn|gd\\.cn|gs\\.cn|gz\\.cn|gx\\.cn|ha\\.cn|hb\\.cn|he\\.cn|hi\\.cn|hl\\.cn|hn\\.cn|jl\\.cn|js\\.cn|jx\\.cn|ln\\.cn|nm\\.cn|nx\\.cn|qh\\.cn|sc\\.cn|sd\\.cn|sh\\.cn|sn\\.cn|sx\\.cn|tj\\.cn|xj\\.cn|xz\\.cn|yn\\.cn|zj\\.cn|co|com\\.co|edu\\.co|org\\.co|gov\\.co|mil\\.co|net\\.co|nom\\.co|com|coop|cr|ac\\.cr|co\\.cr|ed\\.cr|fi\\.cr|go\\.cr|or\\.cr|sa\\.cr|cu|com\\.cu|edu\\.cu|org\\.cu|net\\.cu|gov\\.cu|inf\\.cu|cx|gov\\.cx|cy|com\\.cy|biz\\.cy|info\\.cy|ltd\\.cy|pro\\.cy|net\\.cy|org\\.cy|name\\.cy|tm\\.cy|ac\\.cy|ekloges\\.cy|press\\.cy|parliament\\.cy|cz|dj|dk|dm|com\\.dm|net\\.dm|org\\.dm|edu\\.dm|gov\\.dm|do|edu\\.do|gov\\.do|gob\\.do|com\\.do|org\\.do|sld\\.do|web\\.do|net\\.do|mil\\.do|art\\.do|dz|com\\.dz|org\\.dz|net\\.dz|gov\\.dz|edu\\.dz|asso\\.dz|pol\\.dz|art\\.dz|ec|com\\.ec|info\\.ec|net\\.ec|fin\\.ec|med\\.ec|pro\\.ec|org\\.ec|edu\\.ec|gov\\.ec|mil\\.ec|edu|ee|com\\.ee|org\\.ee|fie\\.ee|pri\\.ee|eg|eun\\.eg|edu\\.eg|sci\\.eg|gov\\.eg|com\\.eg|org\\.eg|net\\.eg|mil\\.eg|es|com\\.es|nom\\.es|org\\.es|gob\\.es|edu\\.es|et|com\\.et|gov\\.et|org\\.et|edu\\.et|net\\.et|biz\\.et|name\\.et|info\\.et|eu|fi|aland\\.fi|fj|biz\\.fj|com\\.fj|info\\.fj|name\\.fj|net\\.fj|org\\.fj|pro\\.fj|ac\\.fj|gov\\.fj|mil\\.fj|school\\.fj|fk|co\\.fk|org\\.fk|gov\\.fk|ac\\.fk|nom\\.fk|net\\.fk|fm|fo|fr|tm\\.fr|asso\\.fr|nom\\.fr|prd\\.fr|presse\\.fr|com\\.fr|gouv\\.fr|ga|gd|ge|com\\.ge|edu\\.ge|gov\\.ge|org\\.ge|mil\\.ge|net\\.ge|pvt\\.ge|gf|gg|co\\.gg|net\\.gg|org\\.gg|gh|com\\.gh|edu\\.gh|gov\\.gh|org\\.gh|mil\\.gh|gi|com\\.gi|ltd\\.gi|gov\\.gi|mod\\.gi|edu\\.gi|org\\.gi|gl|gm|gn|com\\.gn|ac\\.gn|gov\\.gn|org\\.gn|net\\.gn|gov|gp|com\\.gp|net\\.gp|edu\\.gp|asso\\.gp|org\\.gp|gq|gr|com\\.gr|edu\\.gr|net\\.gr|org\\.gr|gov\\.gr|gs|gt|gu|gw|gy|hk|com\\.hk|edu\\.hk|gov\\.hk|idv\\.hk|net\\.hk|org\\.hk|hm|iz\\.hr|from\\.hr|name\\.hr|com\\.hr|hn|com\\.hn|edu\\.hn|org\\.hn|net\\.hn|mil\\.hn|gob\\.hn|ht|com\\.ht|net\\.ht|firm\\.ht|shop\\.ht|info\\.ht|pro\\.ht|adult\\.ht|org\\.ht|art\\.ht|pol\\.ht|rel\\.ht|asso\\.ht|perso\\.ht|coop\\.ht|med\\.ht|edu\\.ht|gouv\\.ht|hu|co\\.hu|info\\.hu|org\\.hu|priv\\.hu|sport\\.hu|tm\\.hu|agrar\\.hu|bolt\\.hu|casino\\.hu|city\\.hu|erotica\\.hu|erotika\\.hu|film\\.hu|forum\\.hu|games\\.hu|hotel\\.hu|ingatlan\\.hu|jogasz\\.hu|konyvelo\\.hu|lakas\\.hu|media\\.hu|news\\.hu|reklam\\.hu|sex\\.hu|shop\\.hu|suli\\.hu|szex\\.hu|tozsde\\.hu|utazas\\.hu|video\\.hu|id|ac\\.id|co\\.id|or\\.id|go\\.id|ie|gov\\.ie|il|ac\\.il|co\\.il|org\\.il|net\\.il|gov\\.il|muni\\.il|idf\\.il|im|co\\.im|ltd\\.co|plc\\.co|net\\.im|gov\\.im|org\\.im|nic\\.im|ac\\.im|in|co\\.in|firm\\.in|net\\.in|org\\.in|gen\\.in|ind\\.in|nic\\.in|ac\\.in|edu\\.in|res\\.in|gov\\.in|mil\\.in|info|int|io|ir|ac\\.ir|co\\.ir|gov\\.ir|net\\.ir|org\\.ir|sch\\.ir|is|it|gov\\.it|je|co\\.je|net\\.je|org\\.je|jm|edu\\.jm|gov\\.jm|com\\.jm|net\\.jm|org\\.jm|jo|com\\.jo|org\\.jo|net\\.jo|edu\\.jo|gov\\.jo|mil\\.jo|jobs|jp|ac\\.jp|ad\\.jp|co\\.jp|ed\\.jp|go\\.jp|gr\\.jp|lg\\.jp|ne\\.jp|or\\.jp|hokkaido\\.jp|aomori\\.jp|iwate\\.jp|miyagi\\.jp|akita\\.jp|yamagata\\.jp|fukushima\\.jp|ibaraki\\.jp|tochigi\\.jp|gunma\\.jp|saitama\\.jp|chiba\\.jp|tokyo\\.jp|kanagawa\\.jp|niigata\\.jp|toyama\\.jp|ishikawa\\.jp|fukui\\.jp|yamanashi\\.jp|nagano\\.jp|gifu\\.jp|shizuoka\\.jp|aichi\\.jp|mie\\.jp|shiga\\.jp|kyoto\\.jp|osaka\\.jp|hyogo\\.jp|nara\\.jp|wakayama\\.jp|tottori\\.jp|shimane\\.jp|okayama\\.jp|hiroshima\\.jp|yamaguchi\\.jp|tokushima\\.jp|kagawa\\.jp|ehime\\.jp|kochi\\.jp|fukuoka\\.jp|saga\\.jp|nagasaki\\.jp|kumamoto\\.jp|oita\\.jp|miyazaki\\.jp|kagoshima\\.jp|okinawa\\.jp|sapporo\\.jp|sendai\\.jp|yokohama\\.jp|kawasaki\\.jp|nagoya\\.jp|kobe\\.jp|kitakyushu\\.jp|ke|kg|kh|per\\.kh|com\\.kh|edu\\.kh|gov\\.kh|mil\\.kh|net\\.kh|org\\.kh|ki|km|kn|kr|co\\.kr|or\\.kr|kw|com\\.kw|edu\\.kw|gov\\.kw|net\\.kw|org\\.kw|mil\\.kw|ky|edu\\.ky|gov\\.ky|com\\.ky|org\\.ky|net\\.ky|kz|org\\.kz|edu\\.kz|net\\.kz|gov\\.kz|mil\\.kz|com\\.kz|la|lb|net\\.lb|org\\.lb|gov\\.lb|edu\\.lb|com\\.lb|lc|com\\.lc|org\\.lc|edu\\.lc|gov\\.lc|li|com\\.li|net\\.li|org\\.li|gov\\.li|lk|gov\\.lk|sch\\.lk|net\\.lk|int\\.lk|com\\.lk|org\\.lk|edu\\.lk|ngo\\.lk|soc\\.lk|web\\.lk|ltd\\.lk|assn\\.lk|grp\\.lk|hotel\\.lk|lr|com\\.lr|edu\\.lr|gov\\.lr|org\\.lr|net\\.lr|ls|org\\.ls|co\\.ls|lt|gov\\.lt|mil\\.lt|lu|gov\\.lu|mil\\.lu|org\\.lu|net\\.lu|lv|com\\.lv|edu\\.lv|gov\\.lv|org\\.lv|mil\\.lv|id\\.lv|net\\.lv|asn\\.lv|conf\\.lv|ly|com\\.ly|net\\.ly|gov\\.ly|plc\\.ly|edu\\.ly|sch\\.ly|med\\.ly|org\\.ly|id\\.ly|ma|co\\.ma|net\\.ma|gov\\.ma|org\\.ma|mc|tm\\.mc|asso\\.mc|md|mg|org\\.mg|nom\\.mg|gov\\.mg|prd\\.mg|tm\\.mg|com\\.mg|edu\\.mg|mil\\.mg|mh|mil|army\\.mil|navy\\.mil|mk|com\\.mk|org\\.mk|ml|mm|mn|mo|com\\.mo|net\\.mo|org\\.mo|edu\\.mo|gov\\.mo|mobi|weather\\.mobi|music\\.mobi|mp|mq|mr|ms|mt|org\\.mt|com\\.mt|gov\\.mt|edu\\.mt|net\\.mt|mu|com\\.mu|co\\.mu|museum|mv|aero\\.mv|biz\\.mv|com\\.mv|coop\\.mv|edu\\.mv|gov\\.mv|info\\.mv|int\\.mv|mil\\.mv|museum\\.mv|name\\.mv|net\\.mv|org\\.mv|pro\\.mv|mw|ac\\.mw|co\\.mw|com\\.mw|coop\\.mw|edu\\.mw|gov\\.mw|int\\.mw|museum\\.mw|net\\.mw|org\\.mw|mx|com\\.mx|net\\.mx|org\\.mx|edu\\.mx|gob\\.mx|my|com\\.my|net\\.my|org\\.my|gov\\.my|edu\\.my|mil\\.my|name\\.my|mz|na|name|nc|ne|net|nf|ng|edu\\.ng|com\\.ng|gov\\.ng|org\\.ng|net\\.ng|ni|gob\\.ni|com\\.ni|edu\\.ni|org\\.ni|nom\\.ni|net\\.ni|nl|no|mil\\.no|stat\\.no|kommune\\.no|herad\\.no|priv\\.no|vgs\\.no|fhs\\.no|museum\\.no|fylkesbibl\\.no|folkebibl\\.no|idrett\\.no|np|com\\.np|org\\.np|edu\\.np|net\\.np|gov\\.np|mil\\.np|nr|gov\\.nr|edu\\.nr|biz\\.nr|info\\.nr|org\\.nr|com\\.nr|net\\.nr|nu|nz|ac\\.nz|co\\.nz|cri\\.nz|gen\\.nz|geek\\.nz|govt\\.nz|iwi\\.nz|maori\\.nz|mil\\.nz|net\\.nz|org\\.nz|school\\.nz|om|com\\.om|co\\.om|edu\\.om|ac\\.com|sch\\.om|gov\\.om|net\\.om|org\\.om|mil\\.om|museum\\.om|biz\\.om|pro\\.om|med\\.om|org|pa|com\\.pa|ac\\.pa|sld\\.pa|gob\\.pa|edu\\.pa|org\\.pa|net\\.pa|abo\\.pa|ing\\.pa|med\\.pa|nom\\.pa|pe|com\\.pe|org\\.pe|net\\.pe|edu\\.pe|mil\\.pe|gob\\.pe|nom\\.pe|pf|com\\.pf|org\\.pf|edu\\.pf|pg|com\\.pg|net\\.pg|ph|com\\.ph|gov\\.ph|pk|com\\.pk|net\\.pk|edu\\.pk|org\\.pk|fam\\.pk|biz\\.pk|web\\.pk|gov\\.pk|gob\\.pk|gok\\.pk|gon\\.pk|gop\\.pk|gos\\.pk|pl|com\\.pl|biz\\.pl|net\\.pl|art\\.pl|edu\\.pl|org\\.pl|ngo\\.pl|gov\\.pl|info\\.pl|mil\\.pl|waw\\.pl|warszawa\\.pl|wroc\\.pl|wroclaw\\.pl|krakow\\.pl|poznan\\.pl|lodz\\.pl|gda\\.pl|gdansk\\.pl|slupsk\\.pl|szczecin\\.pl|lublin\\.pl|bialystok\\.pl|olsztyn\\.pl|pn|pr|biz\\.pr|com\\.pr|edu\\.pr|gov\\.pr|info\\.pr|isla\\.pr|name\\.pr|net\\.pr|org\\.pr|pro\\.pr|pro|law\\.pro|med\\.pro|cpa\\.pro|ps|edu\\.ps|gov\\.ps|sec\\.ps|plo\\.ps|com\\.ps|org\\.ps|net\\.ps|pt|com\\.pt|edu\\.pt|gov\\.pt|int\\.pt|net\\.pt|nome\\.pt|org\\.pt|publ\\.pt|pw|py|net\\.py|org\\.py|gov\\.py|edu\\.py|com\\.py|qa|re|ro|com\\.ro|org\\.ro|tm\\.ro|nt\\.ro|nom\\.ro|info\\.ro|rec\\.ro|arts\\.ro|firm\\.ro|store\\.ro|www\\.ro|ru|com\\.ru|net\\.ru|org\\.ru|pp\\.ru|msk\\.ru|int\\.ru|ac\\.ru|rw|gov\\.rw|net\\.rw|edu\\.rw|ac\\.rw|com\\.rw|co\\.rw|int\\.rw|mil\\.rw|gouv\\.rw|sa|com\\.sa|edu\\.sa|sch\\.sa|med\\.sa|gov\\.sa|net\\.sa|org\\.sa|pub\\.sa|sb|com\\.sb|gov\\.sb|net\\.sb|edu\\.sb|sc|com\\.sc|gov\\.sc|net\\.sc|org\\.sc|edu\\.sc|sd|com\\.sd|net\\.sd|org\\.sd|edu\\.sd|med\\.sd|tv\\.sd|gov\\.sd|info\\.sd|se|org\\.se|pp\\.se|tm\\.se|brand\\.se|parti\\.se|press\\.se|komforb\\.se|kommunalforbund\\.se|komvux\\.se|lanarb\\.se|lanbib\\.se|naturbruksgymn\\.se|sshn\\.se|fhv\\.se|fhsk\\.se|fh\\.se|mil\\.se|ab\\.se|c\\.se|d\\.se|e\\.se|f\\.se|g\\.se|h\\.se|i\\.se|k\\.se|m\\.se|n\\.se|o\\.se|s\\.se|t\\.se|u\\.se|w\\.se|x\\.se|y\\.se|z\\.se|ac\\.se|bd\\.se|sg|com\\.sg|net\\.sg|org\\.sg|gov\\.sg|edu\\.sg|per\\.sg|idn\\.sg|sh|si|sk|sl|sm|sn|sr|st|su|sv|edu\\.sv|com\\.sv|gob\\.sv|org\\.sv|red\\.sv|sy|gov\\.sy|com\\.sy|net\\.sy|sz|tc|td|tf|tg|th|ac\\.th|co\\.th|in\\.th|go\\.th|mi\\.th|or\\.th|net\\.th|tj|ac\\.tj|biz\\.tj|com\\.tj|co\\.tj|edu\\.tj|int\\.tj|name\\.tj|net\\.tj|org\\.tj|web\\.tj|gov\\.tj|go\\.tj|mil\\.tj|tk|tm|tn|com\\.tn|intl\\.tn|gov\\.tn|org\\.tn|ind\\.tn|nat\\.tn|tourism\\.tn|info\\.tn|ens\\.tn|fin\\.tn|net\\.tn|to|gov\\.to|tp|gov\\.tp|tr|com\\.tr|info\\.tr|biz\\.tr|net\\.tr|org\\.tr|web\\.tr|gen\\.tr|av\\.tr|dr\\.tr|bbs\\.tr|name\\.tr|tel\\.tr|gov\\.tr|bel\\.tr|pol\\.tr|mil\\.tr|edu\\.tr|travel|tt|co\\.tt|com\\.tt|org\\.tt|net\\.tt|biz\\.tt|info\\.tt|pro\\.tt|name\\.tt|edu\\.tt|gov\\.tt|tv|gov\\.tv|tw|edu\\.tw|gov\\.tw|mil\\.tw|com\\.tw|net\\.tw|org\\.tw|idv\\.tw|game\\.tw|ebiz\\.tw|club\\.tw|tz|co\\.tz|ac\\.tz|go\\.tz|or\\.tz|ne\\.tz|ua|com\\.ua|gov\\.ua|net\\.ua|edu\\.ua|org\\.ua|cherkassy\\.ua|ck\\.ua|chernigov\\.ua|cn\\.ua|chernovtsy\\.ua|cv\\.ua|crimea\\.ua|dnepropetrovsk\\.ua|dp\\.ua|donetsk\\.ua|dn\\.ua|frankivsk\\.ua|if\\.ua|kharkov\\.ua|kh\\.ua|kherson\\.ua|ks\\.ua|khmelnitskiy\\.ua|km\\.ua|kiev\\.ua|kv\\.ua|kirovograd\\.ua|kr\\.ua|lugansk\\.ua|lg\\.ua|lutsk\\.ua|lviv\\.ua|nikolaev\\.ua|mk\\.ua|odessa\\.ua|od\\.ua|poltava\\.ua|pl\\.ua|rovno\\.ua|rv\\.ua|sebastopol\\.ua|sumy\\.ua|ternopil\\.ua|te\\.ua|uzhgorod\\.ua|vinnica\\.ua|vn\\.ua|zaporizhzhe\\.ua|zp\\.ua|zhitomir\\.ua|zt\\.ua|ug|co\\.ug|ac\\.ug|sc\\.ug|go\\.ug|ne\\.ug|or\\.ug|uk|ac\\.uk|co\\.uk|gov\\.uk|ltd\\.uk|me\\.uk|mil\\.uk|mod\\.uk|net\\.uk|nic\\.uk|nhs\\.uk|org\\.uk|plc\\.uk|police\\.uk|sch\\.uk|library\\.uk|icnet\\.uk|jet\\.uk|nel\\.uk|nls\\.uk|scotland\\.uk|parliament\\.uk|sch\\.uk|um|us|ak\\.us|al\\.us|ar\\.us|az\\.us|ca\\.us|co\\.us|ct\\.us|dc\\.us|de\\.us|dni\\.us|fed\\.us|fl\\.us|ga\\.us|hi\\.us|ia\\.us|id\\.us|il\\.us|in\\.us|isa\\.us|kids\\.us|ks\\.us|ky\\.us|la\\.us|ma\\.us|md\\.us|me\\.us|mi\\.us|mn\\.us|mo\\.us|ms\\.us|mt\\.us|nc\\.us|nd\\.us|ne\\.us|nh\\.us|nj\\.us|nm\\.us|nsn\\.us|nv\\.us|ny\\.us|oh\\.us|ok\\.us|or\\.us|pa\\.us|ri\\.us|sc\\.us|sd\\.us|tn\\.us|tx\\.us|ut\\.us|vt\\.us|va\\.us|wa\\.us|wi\\.us|wv\\.us|wy\\.us|uy|edu\\.uy|gub\\.uy|org\\.uy|com\\.uy|net\\.uy|mil\\.uy|uz|va|vatican\\.va|vc|ve|com\\.ve|net\\.ve|org\\.ve|info\\.ve|co\\.ve|web\\.ve|vg|vi|com\\.vi|org\\.vi|edu\\.vi|gov\\.vi|vn|com\\.vn|net\\.vn|org\\.vn|edu\\.vn|gov\\.vn|int\\.vn|ac\\.vn|biz\\.vn|info\\.vn|name\\.vn|pro\\.vn|health\\.vn|vu|ws|ye|com\\.ye|net\\.ye|yu|ac\\.yu|co\\.yu|org\\.yu|edu\\.yu|za|ac\\.za|city\\.za|co\\.za|edu\\.za|gov\\.za|law\\.za|mil\\.za|nom\\.za|org\\.za|school\\.za|alt\\.za|net\\.za|ngo\\.za|tm\\.za|web\\.za|zm|co\\.zm|org\\.zm|gov\\.zm|sch\\.zm|ac\\.zm|zw|co\\.zw|org\\.zw|gov\\.zw|ac\\.zw|zw|co\\.zw|org\\.zw|gov\\.zw|ac\\.zw)$");