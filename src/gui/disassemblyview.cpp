#include "disassemblymodel.h"
#include "disassemblyview.h"
#include "disassemblyitemdelegate.h"
#include "project.h"
#include "debuggerapplication.h"
#include <QAction>
#include <QInputDialog>
#include <QShortcut>

DisassemblyView::DisassemblyView(QWidget* parent) : QTreeView(parent) {
  connect(dApp, &DebuggerApplication::projectOpened, this,
          &DisassemblyView::onProjectOpened);
  connect(dApp, &DebuggerApplication::projectClosed, this,
          &DisassemblyView::onProjectClosed);

  connect(header(), &QHeaderView::sectionCountChanged, this,
          &DisassemblyView::adjustHeader);

  header()->setSectionsMovable(false);
  header()->setSectionsClickable(false);

  setUniformRowHeights(true);
  setItemsExpandable(false);
  setWordWrap(false);
  setIndentation(0);
  setRootIsDecorated(false);

  setItemDelegate(new DisassemblyItemDelegate(this));

  QShortcut* commentShortcut = new QShortcut(Qt::Key_C, this);
  connect(commentShortcut, &QShortcut::activated, this,
          &DisassemblyView::onCommentKeyPress);
  QShortcut* nameShortcut = new QShortcut(Qt::Key_N, this);
  connect(nameShortcut, &QShortcut::activated, this,
          &DisassemblyView::onNameKeyPress);
}

void DisassemblyView::setModel(QAbstractItemModel* model) {
  QTreeView::setModel(model);
  connect(model, &QAbstractItemModel::modelReset, this,
          &DisassemblyView::onModelReset);
  connect(model, &QAbstractItemModel::rowsInserted, this,
          &DisassemblyView::onModelReset);

  expandAll();
  scrollToTop();
}

void DisassemblyView::onModelReset() {
  setUniformRowHeights(true);
  setItemsExpandable(false);
  setWordWrap(false);
  setIndentation(0);
  setRootIsDecorated(false);

  expandAll();
}

void DisassemblyView::goToAddress(SCXOffset address) {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  QModelIndex index = disModel->firstIndexForAddress(address);
  setCurrentIndex(index);
  // in case the line was already selected, still scroll there
  scrollTo(index);
}

void DisassemblyView::adjustHeader(int oldCount, int newCount) {
  if (newCount != (int)DisassemblyModel::ColumnType::NumColumns) return;
  header()->setSectionResizeMode((int)DisassemblyModel::ColumnType::Breakpoint,
                                 QHeaderView::Fixed);
  // TODO: make breakpoint bounding rectangle square
  header()->resizeSection((int)DisassemblyModel::ColumnType::Breakpoint, 24);
  header()->setSectionResizeMode((int)DisassemblyModel::ColumnType::Address,
                                 QHeaderView::ResizeToContents);
}

void DisassemblyView::onProjectOpened() {
  connect(dApp->project(), &Project::fileSwitched, this,
          &DisassemblyView::onFileSwitched);
}

void DisassemblyView::onProjectClosed() {
  QAbstractItemModel* oldModel = model();
  setModel(nullptr);
  if (oldModel != nullptr) delete oldModel;
}

void DisassemblyView::onFileSwitched(int previousId) {
  QAbstractItemModel* oldModel = model();
  setModel(new DisassemblyModel(dApp->project()->currentFile()));
  if (oldModel != nullptr) delete oldModel;
}

void DisassemblyView::onCommentKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  SCXOffset address = disModel->addressForIndex(currentIndex());
  if (address < 0) return;

  QString oldComment =
      dApp->project()->getComment(disModel->script()->getId(), address);

  bool ok;
  QString newComment = QInputDialog::getMultiLineText(
      this, "Edit comment", "Comment:", oldComment, &ok);
  if (!ok) return;
  dApp->project()->setComment(disModel->script()->getId(), address, newComment);
}

void DisassemblyView::onNameKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  const DisassemblyRow* row =
      static_cast<DisassemblyRow*>(currentIndex().internalPointer());
  if (row == nullptr) return;
  if (row->type != DisassemblyModel::RowType::Label) return;

  QString oldName = disModel->labelNameForLabel(row->id);

  bool ok;
  QString newName = QInputDialog::getMultiLineText(this, "Edit label name",
                                                   "New name:", oldName, &ok);
  if (!ok) return;
  dApp->project()->setLabelName(disModel->script()->getId(), row->id, newName);
}