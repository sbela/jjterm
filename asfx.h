#ifndef ASFX_H
#define ASFX_H

#include <QDateTime>
#include <QDir>

#define PRINT(format, ...)         do { { printf("%s : %s - %d # ", QDateTime::currentDateTime().toString("yy.MM.dd hh:mm:ss.zzz").toUtf8().constData(), __FILE__, __LINE__); printf(format, ##__VA_ARGS__); printf("\n"); fflush(stdout); } } while (0)
#define PRINTF(format, ...)                                                                                                                                                                           \
    do {                                                                                                                                                                                              \
        QString strFormatDate = QDateTime::currentDateTime().toString("yy.MM.dd hh:mm:ss.zzz");                                                                                                       \
        QString strFormatPath = QString("log/%1/%2/%3").arg(QDateTime::currentDateTime().toString("yy"), QDateTime::currentDateTime().toString("MM"), QDateTime::currentDateTime().toString("dd")); \
        QDir d;                                                                                                                                                                                       \
        d.mkpath(strFormatPath);                                                                                                                                                                      \
        FILE* f = fopen(QString("%1/log.log").arg(strFormatPath).toUtf8().constData(), "a+");                                                                                                         \
        fprintf(f, "%s : %s - %d # ", strFormatDate.toUtf8().constData(), __FILE__, __LINE__);                                                                                                        \
        fprintf(f, format, ##__VA_ARGS__);                                                                                                                                                            \
        fprintf(f, "\n");                                                                                                                                                                             \
        fflush(f);                                                                                                                                                                                    \
        fclose(f);                                                                                                                                                                                    \
        printf("%s : %s - %d # ", strFormatDate.toUtf8().constData(), __FILE__, __LINE__);                                                                                                            \
        printf(format, ##__VA_ARGS__);                                                                                                                                                                \
        printf("\n");                                                                                                                                                                                 \
        fflush(stdout);                                                                                                                                                                               \
    } while (0)

#endif // ASFX_H
