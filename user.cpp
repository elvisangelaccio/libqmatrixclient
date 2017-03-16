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

#include "user.h"

#include "connection.h"
#include "events/event.h"
#include "events/roommemberevent.h"
#include "serverapi/getmediathumbnail.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <algorithm>

using namespace QMatrixClient;

class User::Private
{
    public:
        User* q;
        QString userId;
        QString name;
        QUrl avatarUrl;
        Connection* connection;

        QPixmap avatar;
        QSize requestedSize;
        bool avatarValid;
        bool avatarOngoingRequest;
        QVector<QPixmap> scaledAvatars;

        void requestAvatar();
};

User::User(QString userId, Connection* connection)
    : QObject(connection), d(new Private)
{
    d->connection = connection;
    d->userId = userId;
    d->avatarValid = false;
    d->avatarOngoingRequest = false;
    d->q = this;
}

User::~User()
{
    delete d;
}

QString User::id() const
{
    return d->userId;
}

QString User::name() const
{
    return d->name;
}

QString User::displayname() const
{
    if( !d->name.isEmpty() )
        return d->name;
    return d->userId;
}

QPixmap User::avatar(int width, int height)
{
    return croppedAvatar(width, height); // FIXME: Return an uncropped avatar;
}

QPixmap User::croppedAvatar(int width, int height)
{
    QSize size(width, height);

    if( !d->avatarValid
        || width > d->requestedSize.width()
        || height > d->requestedSize.height() )
    {
        if( !d->avatarOngoingRequest && d->avatarUrl.isValid() )
        {
            qDebug() << "Getting avatar for" << id();
            d->requestedSize = size;
            d->avatarOngoingRequest = true;
            QTimer::singleShot(0, this, SLOT(requestAvatar()));
        }
    }

    if( d->avatar.isNull() )
        return d->avatar;
    for (const QPixmap& p: d->scaledAvatars)
    {
        if (p.size() == size)
            return p;
    }
    QPixmap newlyScaled = d->avatar.scaled(size,
        Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap scaledAndCroped = newlyScaled.copy(
        std::max((newlyScaled.width() - width)/2, 0),
        std::max((newlyScaled.height() - height)/2, 0),
        width, height);
    d->scaledAvatars.push_back(scaledAndCroped);
    return scaledAndCroped;
}

void User::processEvent(Event* event)
{
    if( event->type() == EventType::RoomMember )
    {
        RoomMemberEvent* e = static_cast<RoomMemberEvent*>(event);
        if( d->name != e->displayName() )
        {
            const auto oldName = d->name;
            d->name = e->displayName();
            emit nameChanged(this, oldName);
        }
        if( d->avatarUrl != e->avatarUrl() )
        {
            d->avatarUrl = e->avatarUrl();
            d->avatarValid = false;
        }
    }
}

void User::requestAvatar()
{
    d->requestAvatar();
}

void User::Private::requestAvatar()
{
    connection
        ->callServer(ServerApi::GetMediaThumbnail(avatarUrl, requestedSize))
        ->onSuccess( [=](const QPixmap& thumbnail) {
            avatarOngoingRequest = false;
            avatarValid = true;
            avatar = thumbnail.scaled(requestedSize,
                            Qt::KeepAspectRatio, Qt::SmoothTransformation);
            scaledAvatars.clear();
            emit q->avatarChanged(q);
        });
}
