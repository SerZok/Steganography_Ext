#ifndef LAB9_H
#define LAB9_H

#include <QMainWindow>
#include <QActionGroup>

QT_BEGIN_NAMESPACE
namespace Ui {
class Lab9;
}
QT_END_NAMESPACE

class Lab9 : public QMainWindow
{
    Q_OBJECT

public:
    explicit Lab9(QWidget *parent = nullptr);
    ~Lab9();

private slots:
    void openFile1_triggered();   // Открытие файла
    void encodeBtn_clicked();     // Зашифровать
    void decodeBtn_clicked();     // Расшифровать
    void modeChanged();           // Переключение режимов

private:
    Ui::Lab9 *ui;
    QString OriginPath;
    QString ModifPath;
    QActionGroup *modeGroup;         // Группа действий для режима
    QString encodeMessage(const QString &container, const QString &message, bool useNonBreakingSpace, int bitsPerLine);
    QString decodeMessage(const QString &stegoText, bool useNonBreakingSpace, int bitsPerLine);
};

#endif // LAB9_H
