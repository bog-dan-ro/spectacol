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
                        DisassambleDataType type
                        );
        QColor background, foreground;
        uint16_t address;
        QString bytes;
        QString disassamble;
    };

public:
    DisassambleModel(QObject *parent);

    void disassamble(uint16_t address, uint16_t delta = -10, uint16_t length = 0xff);
    void refresh();

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    static QColor color(ColorType colorType, DisassambleDataType type);

signals:
    void rowCountChanged() const;

private:
    uint16_t m_address;
    uint16_t m_delta;
    uint16_t m_length;
    mutable std::mutex m_mutex;
    std::vector<DisassambleData> m_disassambleData, m_disassambleDataTemp;
};

#endif // DISASSAMBLEMODEL_H
