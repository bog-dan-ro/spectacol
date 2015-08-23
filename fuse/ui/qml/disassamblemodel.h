#ifndef DISASSAMBLEMODEL_H
#define DISASSAMBLEMODEL_H

#include <QAbstractListModel>

#include <debugger/breakpoint.h>

#include <vector>
#include <mutex>
#include <QColor>

class DisassambleModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(Origin)

    Q_PROPERTY(int delta READ delta NOTIFY deltaChanged)
    enum {
        Background = Qt::BackgroundColorRole + 1,
        Foreground = Qt::ForegroundRole + 1,
        SelectedBackground = Qt::UserRole + 1,
        SelectedForeground,
        Address,
        AddressText,
        Bytes,
        Disassable
    };

    enum DisassambleDataType {
        BreakExec = DEBUGGER_BREAKPOINT_TYPE_EXECUTE,
        BreakRead = DEBUGGER_BREAKPOINT_TYPE_READ,
        BreakWrite = DEBUGGER_BREAKPOINT_TYPE_WRITE,
        NormalLine
    };

    enum ColorType {
        Paper,
        Ink
    };

    struct DisassambleData {
        DisassambleData(uint16_t address,
                        const QString &bytes,
                        const QString &disassamble,
                        DisassambleDataType type);
        QColor background, foreground;
        uint16_t address;
        QString bytes;
        QString disassamble;
    };

public:
    enum Origin {
        Start,
        End
    };

public:
    DisassambleModel(QObject *parent);

    void disassamble(uint16_t address, int delta = -10, uint16_t length = 0xff);
    void refresh();
    void disassambleMore(Origin origin, int size);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    int delta() const { return -m_delta; }

signals:
    void deltaChanged();

protected:
    QHash<int, QByteArray> roleNames() const;

private:
    static QColor color(ColorType colorType, DisassambleDataType type);
    void disassambleTemp(uint16_t address, int delta, uint16_t length);

private:
    uint16_t m_address = 0;
    int m_delta = 0;
    uint16_t m_length = 0;
    mutable std::mutex m_mutex;
    std::vector<DisassambleData> m_disassambleData, m_disassambleDataTemp;
};

#endif // DISASSAMBLEMODEL_H
