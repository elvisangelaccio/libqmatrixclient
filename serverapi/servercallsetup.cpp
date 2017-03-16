/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "servercallsetup.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QObject> // For tr()

#include <QtNetwork/QNetworkReply>

using namespace QMatrixClient::ServerApi;

Status CallConfigBase::checkReply(const QNetworkReply* reply) const
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

QByteArray FromByteArray::load(QNetworkReply* reply)
{
    return reply->readAll();
}

Result<QJsonObject> FromJsonObject::load(QNetworkReply* reply)
{
    QJsonParseError error;
    QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        return { JsonParseError,
                   QObject::tr("Invalid JSON: %1 at offset %2")
                       .arg(error.errorString()).arg(error.offset)
                 };
    }
    if (!data.isObject())
    {
        return { JsonParseError,
                   QObject::tr("The received JSON has no top-level object") };
    }
    return data.object();
}
