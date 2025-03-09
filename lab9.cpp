#include "lab9.h"
#include "ui_lab9.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

Lab9::Lab9(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Lab9)
{
    ui->setupUi(this);

    // Создаем группу действий для режимов
    modeGroup = new QActionGroup(this);
    modeGroup->addAction(ui->simpleMode);
    modeGroup->addAction(ui->mode2);
    ui->simpleMode->setChecked(true);  // Устанавливаем начальный режим

    // Подключаем переключение режима
    connect(ui->simpleMode, &QAction::triggered, this, &Lab9::modeChanged);
    connect(ui->mode2, &QAction::triggered, this, &Lab9::modeChanged);
    connect(ui->openFile1, &QAction::triggered,  this, &Lab9::openFile1_triggered);
    connect(ui->encodeBtn, &QPushButton::pressed,  this, &Lab9::encodeBtn_clicked);
    connect(ui->decodeBtn, &QPushButton::pressed,  this, &Lab9::decodeBtn_clicked);
}

Lab9::~Lab9()
{
    delete ui;
}

// Открытие файла и загрузка в OriginalFileText
void Lab9::openFile1_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "", "Текстовые файлы (*.txt);;Все файлы (*.*)");
    if (fileName.isEmpty()) return;

    OriginPath = fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл!");
        return;
    }

    QTextStream in(&file);
    ui->OriginalFileText->setText(in.readAll());
    file.close();
}

// Функция шифрования
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

void Lab9::encodeBtn_clicked()
{
    QString originalText = ui->OriginalFileText->toPlainText();
    QString message = ui->openText->toPlainText();

    if (originalText.isEmpty() || message.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите текст и выберите файл!");
        return;
    }

    bool useNonBreakingSpace = ui->mode2->isChecked(); // Используем "неразрывный" режим или нет
    int bitsPerLine = ui->bitCount->value(); // Количество битов на строку

    QString encodedText = encodeMessage(originalText, message, useNonBreakingSpace, bitsPerLine);
    ui->ModifyText->setPlainText(encodedText);

    // Создаем новый путь с припиской _modif
    QFileInfo fileInfo(OriginPath);
    ModifPath = fileInfo.path() + "/" + fileInfo.baseName() + "_modif." + fileInfo.completeSuffix();
    qDebug()<<OriginPath;
    qDebug()<<ModifPath;

    // Записываем измененный текст в новый файл
    QFile file(ModifPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << encodedText;
        file.close();
        QMessageBox::information(this, "Успех", "Файл успешно сохранен: " + ModifPath);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл: " + ModifPath);
    }
}

// Функция расшифровки
void Lab9::decodeBtn_clicked()
{
    QFile file(ModifPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл: " + ModifPath);
        return;
    }

    QTextStream in(&file);
    QString stegoText = in.readAll();
    file.close();

    // Проверяем, есть ли зашифрованный текст
    if (stegoText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Файл с зашифрованным текстом пуст!");
        return;
    }

    // Устанавливаем текст в ui->ModifyText для отображения
    ui->ModifyText->setPlainText(stegoText);

    // Декодируем текст
    bool useNonBreakingSpace = ui->mode2->isChecked(); // Режим декодирования
    int bitsPerLine = ui->bitCount->value();

    QString decodedText = decodeMessage(stegoText, useNonBreakingSpace, bitsPerLine);
    ui->DecodedText->setPlainText(decodedText);
}

// Переключение режима
void Lab9::modeChanged()
{
    // if (ui->simpleMode->isChecked()) {
    //     QMessageBox::information(this, "Режим", "Выбран обычный режим.");
    // } else if (ui->mode2->isChecked()) {
    //     QMessageBox::information(this, "Режим", "Выбран неразрывный режим.");
    // }
}


QString Lab9::encodeMessage(const QString &container, const QString &message, bool useNonBreakingSpace, int bitsPerLine) {
    QString binaryMessage;
    QByteArray messageBytes = message.toUtf8();

    // Преобразуем сообщение в двоичный код
    for (char byte : messageBytes) {
        binaryMessage.append(QString("%1").arg(static_cast<uchar>(byte), 8, 2, QChar('0')));
    }

    qDebug() << "Encoded Message Binary: " << binaryMessage;

    QStringList lines = container.split("\n");
    int bitIndex = 0;

    for (int i = 0; i < lines.size(); i++) {
        if (bitIndex >= binaryMessage.length()) break;  // Если всё сообщение закодировано, выходим

        QString encodedSpaces;
        if (useNonBreakingSpace) {
            for (int j = 0; j < bitsPerLine && bitIndex < binaryMessage.length(); j++, bitIndex++) {
                QChar bit = binaryMessage[bitIndex] == '0' ? ' ' : QChar(0xA0);
                qDebug()<<"Бит: "<<bit;
                encodedSpaces.append(bit);
            }
        } else {// Обычный режим
            if (bitIndex < binaryMessage.length()) {
                encodedSpaces.append(binaryMessage[bitIndex] == '0' ? "  " : " ");
                bitIndex++;
            }
        }

        lines[i] += encodedSpaces;  // Добавляем пробелы в конец строки
    }
    return lines.join("\n");
}

QString Lab9::decodeMessage(const QString &stegoText, bool useNonBreakingSpace, int bitsPerLine) {
    QString binaryMessage;
    QStringList lines = stegoText.split("\n");

    for (const QString &line : lines) {
        int totalLength = line.length();
        int spaceStartIndex = totalLength;

        // Ищем начало последовательности пробелов
        for (int i = totalLength - 1; i >= 0; i--) {
            qDebug()<<"char to code: "<<line[i];
            if (line[i] != ' ' && line[i] != QChar(0xA0)) {
                spaceStartIndex = i + 1;
                break;
            }
        }

        int spaceCount = totalLength - spaceStartIndex;
        if (spaceCount == 0) continue;  // Пропускаем строки без закодированного сообщения

        if (useNonBreakingSpace) {
            // Читаем bitsPerLine битов
            for (int j = 0; j < bitsPerLine && (spaceStartIndex + j) < totalLength; j++) {
                QChar bit = line[spaceStartIndex + j] == QChar(0xA0) ? '1' : '0';
                binaryMessage.append(bit);
            }
        } else {
            // Обычный режим: 1 пробел → 1, 2 пробела → 0
            if (spaceCount == 1) {
                binaryMessage.append('1');
            } else if (spaceCount == 2) {
                binaryMessage.append('0');
            }
        }
    }

    qDebug() << "Decoded Message Binary: " << binaryMessage;

    // Преобразование двоичного кода обратно в строку
    QByteArray messageBytes;
    for (int i = 0; i + 8 <= binaryMessage.length(); i += 8) {
        bool ok;
        char byte = binaryMessage.mid(i, 8).toInt(&ok, 2);
        if (ok) {
            messageBytes.append(byte);
        }
    }

    return QString::fromUtf8(messageBytes);
}

