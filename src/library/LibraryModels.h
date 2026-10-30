#pragma once
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QVariantList>
namespace veluno {
class EntryModel final : public QAbstractListModel {
    Q_OBJECT
public:
    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex &parent={}) const override { return parent.isValid() ? 0 : m_rows.size(); }
    QVariant data(const QModelIndex &index,int role) const override;
    QHash<int,QByteArray> roleNames() const override { return {{Qt::UserRole+1,"entry"}}; }
    void replace(const QVariantList &rows);
    const QVariantList &rows() const { return m_rows; }
private:
    QVariantList m_rows;
};
class CatalogProxy final : public QSortFilterProxyModel {
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    QString kind, order="title";
    int availability=-1;
    size_t seed=0;
    void refresh() { invalidate(); sort(0); }
protected:
    bool filterAcceptsRow(int row,const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left,const QModelIndex &right) const override;
};
}
