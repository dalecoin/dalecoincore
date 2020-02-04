// Copyright (c) 2018 yshurik
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef INFOPAGE_H
#define INFOPAGE_H

#include <QDialog>
#include <QPixmap>
#include <QItemDelegate>
#include <QStyledItemDelegate>
#include "bignum.h"

namespace Ui {
    class BlockchainPage;
}

class QTreeView;
class QTreeWidget;
class QModelIndex;
class QTreeWidgetItem;
class BlockchainModel;

class BlockchainPage : public QDialog
{
    Q_OBJECT

public:
    explicit BlockchainPage(QWidget *parent = nullptr);
    ~BlockchainPage();

    BlockchainModel * blockchainModel() const;

private slots:
    void showChainPage();
    void showBlockPage();
    void showTxPage();
    void openBlock(uint256);
    void openBlock(const QModelIndex &);
    void openBlock(QTreeWidgetItem*,int);
    void openTx(QTreeWidgetItem*,int);
    void openTx(uint256 blockhash, uint txidx);
    void openTxFromInput();
    void openFractions(QTreeWidgetItem*,int);
    void openFractionsMenu(const QPoint &);
    void jumpToBlock();
    void openBlockFromInput();
    void updateCurrentBlockIndex();
    void scrollToCurrentBlockIndex();
    void openChainMenu(const QPoint &);
    void openBlockMenu(const QPoint &);
    void openTxMenu(const QPoint &);

private:
    Ui::BlockchainPage *ui;
    BlockchainModel *model;
    QPersistentModelIndex currentBlockIndex;
    uint256 currentBlock;
    QPixmap pmChange;
    QPixmap pmFrozenF;
    QPixmap pmFrozenV;
    QPixmap pmFrozenL;
};

class LeftSideIconItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit LeftSideIconItemDelegate(QWidget *parent = nullptr);
    ~LeftSideIconItemDelegate() override;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

class FractionsItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit FractionsItemDelegate(QWidget *parent = nullptr);
    ~FractionsItemDelegate() override;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

class BlockchainPageChainEvents : public QObject
{
    Q_OBJECT
    QTreeView* treeWidget;
public:
    BlockchainPageChainEvents(QTreeView* w, QObject* parent)
        :QObject(parent), treeWidget(w) {}
    ~BlockchainPageChainEvents() override {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class BlockchainPageBlockEvents : public QObject
{
    Q_OBJECT
    QTreeWidget* treeWidget;
public:
    BlockchainPageBlockEvents(QTreeWidget* w, QObject* parent)
        :QObject(parent), treeWidget(w) {}
    ~BlockchainPageBlockEvents() override {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class BlockchainPageTxEvents : public QObject
{
    Q_OBJECT
    QTreeWidget* treeWidget;
public:
    BlockchainPageTxEvents(QTreeWidget* w, QObject* parent)
        :QObject(parent), treeWidget(w) {}
    ~BlockchainPageTxEvents() override {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class FractionsDialogEvents : public QObject
{
    Q_OBJECT
    QTreeWidget* treeWidget;
public:
    FractionsDialogEvents(QTreeWidget* w, QObject* parent)
        :QObject(parent), treeWidget(w) {}
    ~FractionsDialogEvents() override {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // INFOPAGE_H
