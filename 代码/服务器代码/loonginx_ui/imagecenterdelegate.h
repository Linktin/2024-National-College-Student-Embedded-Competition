#ifndef IMAGECENTERDELEGATE_H
#define IMAGECENTERDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class ImageCenterDelegate : public QStyledItemDelegate
{
public:
    ImageCenterDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

};

#endif // IMAGECENTERDELEGATE_H
