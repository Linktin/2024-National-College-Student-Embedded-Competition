#ifndef PAGINATION_H
#define PAGINATION_H
#include <QObject>
#include <QStandardItemModel>

class Pagination : public QObject
{
  Q_OBJECT
  public:
      explicit Pagination(QObject *parent = nullptr);
      void setModel(QStandardItemModel *model);
      void setItemsPerPage(int itemsPerPage);
      void setPage(int page);
      int totalPages() const;
      int currentPage() const;
      int itemsPerPage() const;

  signals:
      void pageChanged(int currentPage, int totalPages);

  public slots:
      void showNextPage();
      void showPreviousPage();

  private:
      QStandardItemModel *m_model;
      int m_itemsPerPage = 20;
      int m_currentPage = 1;
};

#endif // PAGINATION_H
