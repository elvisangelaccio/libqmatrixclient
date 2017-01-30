/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "basejob.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QTimer>

#include "../connectiondata.h"

using namespace QMatrixClient;

struct NetworkReplyDeleter : public QScopedPointerDeleteLater
{
    static inline void cleanup(QNetworkReply* reply)
    {
        if (reply && reply->isRunning())
            reply->abort();
        QScopedPointerDeleteLater::cleanup(reply);
    }
};

class BaseJob::Private
{
    public:
        Private(ConnectionData* c, const RequestConfig& rc)
            : connection(c), reqConfig(rc), reply(nullptr), status(NoError)
        { }

        inline void sendRequest();

        ConnectionData* connection;

        // Contents for the network request
        RequestConfig reqConfig;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        Status status;

        QTimer timer;
};

inline QDebug operator<<(QDebug dbg, BaseJob* j)
{
    return dbg << "Job" << j->objectName();
}

BaseJob::BaseJob(ConnectionData* connection, JobHttpType verb, QString name,
                 QString endpoint, const QUrlQuery& query, const Data& data,
                 bool needsToken)
    : BaseJob(connection,
              RequestConfig(name, verb, endpoint, query, data, needsToken))
{ }

BaseJob::BaseJob(ConnectionData* connection, const RequestConfig& rc)
    : d(new Private(connection, rc))
{
    setObjectName(rc.name());
    connect (&d->timer, &QTimer::timeout, this, &BaseJob::timeout);
    qDebug() << this << "created";
}

BaseJob::~BaseJob()
{
    qDebug() << this << "destroyed";
}

RequestConfig& BaseJob::request()
{
    return d->reqConfig;
}

void BaseJob::Private::sendRequest()
{
    QUrl url = connection->baseUrl();
    url.setPath( url.path() + reqConfig.apiPath() );
    auto query = reqConfig.query();
    if (reqConfig.needsToken())
        query.addQueryItem("access_token", connection->accessToken());
    url.setQuery(query);

    QNetworkRequest req {url};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    switch( reqConfig.type() )
    {
        case JobHttpType::GetJob:
            reply.reset( connection->nam()->get(req) );
            break;
        case JobHttpType::PostJob:
            reply.reset( connection->nam()->post(req, reqConfig.data() ));
            break;
        case JobHttpType::PutJob:
            reply.reset( connection->nam()->put(req, reqConfig.data() ));
            break;
        case JobHttpType::DeleteJob:
            reply.reset( connection->nam()->deleteResource(req) );
            break;
    }
}

void BaseJob::start()
{
    d->sendRequest();
    connect( d->reply.data(), &QNetworkReply::sslErrors, this, &BaseJob::sslErrors );
    connect( d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply );
    d->timer.start( 120*1000 );
}

void BaseJob::gotReply()
{
    setStatus(checkReply(d->reply.data()));
    if (status().good())
        setStatus(parseReply(d->reply->readAll()));

    finishJob(true);
}

BaseJob::Status BaseJob::checkReply(QNetworkReply* reply) const
{
    switch( reply->error() )
    {
    case QNetworkReply::NoError:
        return NoError;

    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
        return { ContentAccessError, reply->errorString() };

    default:
        return { NetworkError, reply->errorString() };
    }
}

BaseJob::Status BaseJob::parseReply(QByteArray data)
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(data, &error);
    if( error.error == QJsonParseError::NoError )
        return parseJson(json);
    else
        return { JsonParseError, error.errorString() };
}

BaseJob::Status BaseJob::parseJson(const QJsonDocument&)
{
    return Success;
}

void BaseJob::finishJob(bool emitResult)
{
    d->timer.stop();
    if (!d->reply)
    {
        qWarning() << this << "finishes with empty network reply";
    }
    else if (d->reply->isRunning())
    {
        qWarning() << this << "finishes without ready network reply";
        d->reply->disconnect(this); // Ignore whatever comes from the reply
    }

    // Notify those that are interested in any completion of the job (including killing)
    emit finished(this);

    if (emitResult) {
        emit result(this);
        if (error())
            emit failure(this);
        else
            emit success(this);
    }

    deleteLater();
}

BaseJob::Status BaseJob::status() const
{
    return d->status;
}

int BaseJob::error() const
{
    return d->status.code;
}

QString BaseJob::errorString() const
{
    return d->status.message;
}

void BaseJob::setStatus(Status s)
{
    d->status = s;
    if (!s.good())
    {
        qWarning() << this << "status" << s.code << ":" << s.message;
    }
}

void BaseJob::setStatus(int code, QString message)
{
    setStatus({ code, message });
}

void BaseJob::abandon()
{
    finishJob(false);
}

void BaseJob::timeout()
{
    setStatus( TimeoutError, "The job has timed out" );
    finishJob(true);
}

void BaseJob::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}
