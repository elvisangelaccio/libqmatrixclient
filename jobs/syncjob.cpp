/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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

#include "syncjob.h"

#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

#include "../connectiondata.h"

using namespace QMatrixClient;

class SyncJob::Private
{
    public:
        QString nextBatch;
        SyncData roomData;
};

static size_t jobId = 0;

SyncJob::SyncJob(ConnectionData* connection,
                 QString since, QString filter, int timeout, QString presence)
    : BaseJob(connection, JobHttpType::GetJob, QString("SyncJob-%1").arg(++jobId),
              "_matrix/client/r0/sync")
    , d(new Private)
{
    QUrlQuery query;
    if( !filter.isEmpty() )
        query.addQueryItem("filter", filter);
    if( !presence.isEmpty() )
        query.addQueryItem("set_presence", presence);
    if( timeout >= 0 )
        query.addQueryItem("timeout", QString::number(timeout));
    if( !since.isEmpty() )
        query.addQueryItem("since", since);
    request().setQuery(query);
}

SyncJob::~SyncJob()
{
    delete d;
}

QString SyncJob::nextBatch() const
{
    return d->nextBatch;
}

SyncData& SyncJob::roomData()
{
    return d->roomData;
}

BaseJob::Status SyncJob::parseJson(const QJsonDocument& data)
{
    QJsonObject json = data.object();
    d->nextBatch = json.value("next_batch").toString();
    // TODO: presence
    // TODO: account_data
    QJsonObject rooms = json.value("rooms").toObject();

    const struct { QString jsonKey; JoinState enumVal; } roomStates[]
    {
        { "join", JoinState::Join },
        { "invite", JoinState::Invite },
        { "leave", JoinState::Leave }
    };
    for (auto roomState: roomStates)
    {
        const QJsonObject rs = rooms.value(roomState.jsonKey).toObject();
        d->roomData.reserve(rs.size());
        for( auto rkey: rs.keys() )
        {
            d->roomData.push_back({rkey, roomState.enumVal, rs[rkey].toObject()});
        }
    }

    return Success;
}

void SyncRoomData::EventList::fromJson(const QJsonObject& roomContents)
{
    assign(eventsFromJson(roomContents[jsonKey].toObject()["events"].toArray()));
}

SyncRoomData::SyncRoomData(QString roomId_, JoinState joinState_, const QJsonObject& room_)
    : roomId(roomId_)
    , joinState(joinState_)
    , state("state")
    , timeline("timeline")
    , ephemeral("ephemeral")
    , accountData("account_data")
    , inviteState("invite_state")
{
    switch (joinState) {
        case JoinState::Invite:
            inviteState.fromJson(room_);
            break;
        case JoinState::Join:
            state.fromJson(room_);
            timeline.fromJson(room_);
            ephemeral.fromJson(room_);
            accountData.fromJson(room_);
            break;
        case JoinState::Leave:
            state.fromJson(room_);
            timeline.fromJson(room_);
            break;
    default:
        qWarning() << "SyncRoomData: Unknown JoinState value, ignoring:" << int(joinState);
    }

    QJsonObject timeline = room_.value("timeline").toObject();
    timelineLimited = timeline.value("limited").toBool();
    timelinePrevBatch = timeline.value("prev_batch").toString();

    QJsonObject unread = room_.value("unread_notifications").toObject();
    highlightCount = unread.value("highlight_count").toInt();
    notificationCount = unread.value("notification_count").toInt();
    qDebug() << "Highlights: " << highlightCount << " Notifications:" << notificationCount;
}
