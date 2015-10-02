#ifndef Z80ASSEMBLER_H
#define Z80ASSEMBLER_H

#include <QJSEngine>
#include <QValidator>

#include <vector>
#include <tuple>

class Z80Assembler : public QValidator
{
    Q_OBJECT
public:
    explicit Z80Assembler(QObject *parent = 0);
    Q_INVOKABLE bool write(const QString &asmLine, int address, const QByteArray &assembledBytes = QByteArray()) const;

    // QValidator interface
    State validate(QString &value, int &pos) const override;

private:
    QByteArray assemble(const QString &asmLine, int address, const QByteArray &assembledBytes = QByteArray()) const;

private:
    typedef std::pair<QString, QString> RulesType;
    std::vector<RulesType> m_rules;
    typedef std::pair<QString, QString> AssembleRules;
    std::vector<RulesType> m_assembleRules;
    mutable QJSEngine m_engine;
};

#endif // Z80ASSEMBLER_H
