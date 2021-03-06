#include "memorymodel.h"
#include "debuggerapplication.h"
#include "project.h"

MemoryModel::MemoryModel(QObject *parent) : QAbstractTableModel(parent) {
  connect(dApp->project(), &Project::varNameChanged, this,
          &MemoryModel::onVarTextChanged);
  connect(dApp->project(), &Project::varCommentChanged, this,
          &MemoryModel::onVarTextChanged);
  connect(dApp->project(), &Project::allVarsChanged, this,
          &MemoryModel::onAllVarsChanged);
}

int MemoryModel::columnCount(const QModelIndex &parent) const {
  return (int)ColumnType::NumColumns;
}

int MemoryModel::rowCount(const QModelIndex &parent) const {
  return 8000 + 8000;
}

QVariant MemoryModel::headerData(int section, Qt::Orientation orientation,
                                 int role) const {
  if (role != Qt::DisplayRole) return QVariant();
  switch ((ColumnType)section) {
    case ColumnType::VarType:
      return QVariant("Type");
    case ColumnType::Name:
      return QVariant("Name");
    case ColumnType::Comment:
      return QVariant("Comment");
    default:
      return QVariant();
  }
}

Qt::ItemFlags MemoryModel::flags(const QModelIndex &index) const {
  auto origFlags = QAbstractTableModel::flags(index);
  if (!index.isValid()) return origFlags;
  if ((ColumnType)index.column() == ColumnType::Name ||
      (ColumnType)index.column() == ColumnType::Comment)
    return origFlags | Qt::ItemIsEditable;
  return origFlags;
}

bool MemoryModel::setData(const QModelIndex &index, const QVariant &value,
                          int role) {
  if (index.isValid() && role == Qt::EditRole &&
          ((ColumnType)index.column() == ColumnType::Name) ||
      (ColumnType)index.column() == ColumnType::Comment) {
    VariableRefType type;
    int var;
    std::tie(type, var) = varForIndex(index);
    if ((ColumnType)index.column() == ColumnType::Name) {
      dApp->project()->setVarName(value.toString(), type, var);
    } else {
      dApp->project()->setVarComment(value.toString(), type, var);
    }
    return true;
  }
  return false;
}

QVariant MemoryModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::EditRole))
    return QVariant();
  VariableRefType varType;
  int var;
  std::tie(varType, var) = varForIndex(index);
  if (var < 0) return QVariant();
  switch ((ColumnType)index.column()) {
    case ColumnType::VarType: {
      switch (varType) {
        case VariableRefType::GlobalVar:
          return QVariant("GlobalVar");
        case VariableRefType::Flag:
          return QVariant("Flag");
      }
      break;
    }
    case ColumnType::Name: {
      QString name = dApp->project()->getVarName(varType, var);
      if (role == Qt::DisplayRole && name != QString("%1").arg(var)) {
        return QVariant(QString("%1 (%2)").arg(name).arg(var));
      }
      return QVariant(name);
      break;
    }
    case ColumnType::Comment: {
      return QVariant(dApp->project()->getVarComment(varType, var));
    }
    default:
      return QVariant();
  }
}

QModelIndex MemoryModel::indexForVar(VariableRefType type, int var) const {
  if (var >= 8000 || var < 0 || (int)type < 0 ||
      (int)type > (int)VariableRefType::Flag)
    return QModelIndex();
  if (type == VariableRefType::GlobalVar)
    return createIndex(var, 0, nullptr);
  else
    return createIndex(var + 8000, 0, nullptr);
}
std::pair<VariableRefType, int> MemoryModel::varForIndex(
    const QModelIndex &index) const {
  if (!index.isValid()) return std::make_pair(VariableRefType::GlobalVar, -1);
  if (index.row() < 8000)
    return std::make_pair(VariableRefType::GlobalVar, index.row());
  else
    return std::make_pair(VariableRefType::Flag, index.row() - 8000);
}

void MemoryModel::onVarTextChanged(VariableRefType type, int var,
                                   const QString &text) {
  QModelIndex origIndex = indexForVar(type, var);
  QModelIndex endIndex =
      createIndex(origIndex.row(), (int)ColumnType::NumColumns - 1,
                  origIndex.internalPointer());
  emit dataChanged(origIndex, endIndex);
}

void MemoryModel::onAllVarsChanged() {
  beginResetModel();
  endResetModel();
}