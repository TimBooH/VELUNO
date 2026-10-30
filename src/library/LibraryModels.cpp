#include "LibraryModels.h"
#include <QSet>
namespace veluno {
QVariant EntryModel::data(const QModelIndex &index,int role) const {
    if (!index.isValid() || index.row()<0 || index.row()>=m_rows.size() || role!=Qt::UserRole+1) return {};
    return m_rows[index.row()];
}
void EntryModel::replace(const QVariantList &rows) {
    QSet<qint64> ids;
    for (const auto &row : rows) ids.insert(row.toMap().value("id").toLongLong());
    for (int i=m_rows.size()-1;i>=0;--i) if (!ids.contains(m_rows[i].toMap().value("id").toLongLong())) {
        beginRemoveRows({},i,i); m_rows.removeAt(i); endRemoveRows();
    }
    // Repository rows are always in primary-key order; existing identities stay stable.
    int position=0;
    for (const auto &row : rows) {
        const auto id=row.toMap().value("id").toLongLong();
        if (position>=m_rows.size() || m_rows[position].toMap().value("id").toLongLong()!=id) {
            int found=position+1;
            while (found<m_rows.size() && m_rows[found].toMap().value("id").toLongLong()!=id) ++found;
            if (found<m_rows.size()) {
                beginMoveRows({},found,found,{},position);
                m_rows.insert(position,m_rows.takeAt(found)); endMoveRows();
            } else {
                beginInsertRows({},position,position); m_rows.insert(position,row); endInsertRows();
            }
        }
        if (m_rows[position]!=row) { m_rows[position]=row; emit dataChanged(index(position),index(position),{Qt::UserRole+1}); }
        ++position;
    }
}
bool CatalogProxy::filterAcceptsRow(int row,const QModelIndex &parent) const {
    const auto data=sourceModel()->index(row,0,parent).data(Qt::UserRole+1).toMap();
    return (kind.isEmpty() || data.value("kind")==kind) && (availability<0 || data.value("available").toInt()==availability);
}
bool CatalogProxy::lessThan(const QModelIndex &left,const QModelIndex &right) const {
    const auto a=left.data(Qt::UserRole+1).toMap(), b=right.data(Qt::UserRole+1).toMap();
    if (order=="recent" && a.value("added_at")!=b.value("added_at")) return a.value("added_at").toString()>b.value("added_at").toString();
    if (order=="year" && a.value("year")!=b.value("year")) return a.value("year").toInt()>b.value("year").toInt();
    if (order=="random") {
        const auto x=qHash(a.value("id").toLongLong(),seed),y=qHash(b.value("id").toLongLong(),seed);
        if (x!=y) return x<y;
    }
    const auto result=QString::localeAwareCompare(a.value("title").toString(),b.value("title").toString());
    return result ? result<0 : a.value("id").toLongLong()<b.value("id").toLongLong();
}
}
