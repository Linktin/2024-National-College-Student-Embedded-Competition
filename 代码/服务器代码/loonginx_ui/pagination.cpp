// 包含头文件 "pagination.h"，这里假设它包含了Pagination类的声明
#include "pagination.h"

// Pagination类的构造函数，设置父对象为nullptr
Pagination::Pagination(QObject *parent) : QObject(parent), m_model(nullptr), m_itemsPerPage(10), m_currentPage(1) {}

// 设置模型
void Pagination::setModel(QStandardItemModel *model)
{
    m_model = model;
}

// 设置每页显示的项数
void Pagination::setItemsPerPage(int itemsPerPage)
{
    m_itemsPerPage = itemsPerPage;
}

// 获取每页显示的项数
int Pagination::itemsPerPage() const
{
    return m_itemsPerPage;
}

// 设置当前页码
void Pagination::setPage(int page)
{
    // 如果页码有效，则更新当前页码并发射pageChanged信号
    if (page > 0 && page <= totalPages()) {
        m_currentPage = page;
        emit pageChanged(m_currentPage, totalPages());
    }
}

// 获取总页数
int Pagination::totalPages() const
{
    // 如果模型为空，返回0，否则计算总行数并返回总页数
    if (!m_model)
        return 0;

    int totalRows = m_model->rowCount();
    return (totalRows + m_itemsPerPage - 1) / m_itemsPerPage;
}

// 获取当前页码
int Pagination::currentPage() const
{
    return m_currentPage;
}

// 显示下一页
void Pagination::showNextPage()
{
    // 如果当前页不是最后一页，则将当前页码加1并发射pageChanged信号
    if (m_currentPage < totalPages())
    {
        m_currentPage++;
        emit pageChanged(m_currentPage, totalPages());
    }
}

// 显示上一页
void Pagination::showPreviousPage()
{
    // 如果当前页不是第一页，则将当前页码减1并发射pageChanged信号
    if (m_currentPage > 1) {
        m_currentPage--;
        emit pageChanged(m_currentPage, totalPages());
    }
}
