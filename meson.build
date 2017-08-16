project('qmatrixclient', 'cpp')

qt5_mod = import('qt5')

src = [
   'connectiondata.cpp',
   'connection.cpp',
   'logging.cpp',
   'room.cpp',
   'user.cpp',
   'settings.cpp',
   'events/event.cpp',
   'events/roommessageevent.cpp',
   'events/roomnameevent.cpp',
   'events/roomaliasesevent.cpp',
   'events/roomcanonicalaliasevent.cpp',
   'events/roommemberevent.cpp',
   'events/roomtopicevent.cpp',
   'events/typingevent.cpp',
   'events/receiptevent.cpp',
   'jobs/basejob.cpp',
   'jobs/checkauthmethods.cpp',
   'jobs/passwordlogin.cpp',
   'jobs/sendeventjob.cpp',
   'jobs/postreceiptjob.cpp',
   'jobs/joinroomjob.cpp',
   'jobs/leaveroomjob.cpp',
   'jobs/roommessagesjob.cpp',
   'jobs/syncjob.cpp',
   'jobs/mediathumbnailjob.cpp',
   'jobs/logoutjob.cpp',
]

generated_src = qt5_mod.preprocess(
    moc_headers : [
        'connection.h',
        'events/event.h',
        'events/roommemberevent.h',
        'events/roommessageevent.h',
        'jobs/basejob.h',
        'room.h',
        'settings.h',
        'user.h'
    ]
)

qt5gui = dependency('qt5', modules : 'Gui')
qt5network = dependency('qt5', modules : 'Network')

shared_library('qmatrixclient', src + generated_src, version : '0.0.0', soversion : '0', dependencies : [qt5gui, qt5network])
