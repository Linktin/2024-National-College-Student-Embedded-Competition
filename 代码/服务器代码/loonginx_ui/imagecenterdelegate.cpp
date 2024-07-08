#include "imagecenterdelegate.h"
#include <QPainter>


void ImageCenterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)const
  {
      if (index.column() == 3) // Assuming the image is in the 4th column
      {
          QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
          if (!pixmap.isNull())
          {
              QRect rect = option.rect;
              int x = rect.x() + rect.width() / 2 - pixmap.width() / 2;
              int y = rect.y() + rect.height() / 2 - pixmap.height() / 2;
              painter->drawPixmap(x, y, pixmap);
              return;
          }
      }
      QStyledItemDelegate::paint(painter, option, index);
  }
