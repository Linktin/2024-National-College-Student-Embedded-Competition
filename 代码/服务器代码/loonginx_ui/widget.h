#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QtCore>
#include <QPainter>
#include <QTcpSocket>
namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
    void paintEvent(QPaintEvent *event);
private slots:

    void on_pushButton_clicked();//连接
    void on_pushButton_2_clicked();//断开


private:
    Ui::Widget *ui;//整个ui界面
    QLabel *label;//画布
    QImage background;//背景图片
    QTcpSocket *socket;

};
#endif // WIDGET_H
