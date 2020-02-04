// Copyright (c) 2018 yshurik
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockchainpage.h"
#include "ui_blockchainpage.h"
#include "ui_fractionsdialog.h"

#include "main.h"
#include "base58.h"
#include "txdb.h"
#include "guiutil.h"
#include "blockchainmodel.h"
#include "metatypes.h"
#include "qwt/qwt_plot.h"
#include "qwt/qwt_plot_curve.h"
#include "qwt/qwt_plot_barchart.h"

#include <QTime>
#include <QMenu>
#include <QDebug>
#include <QPainter>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>

#include <string>
#include <vector>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"
extern json_spirit::Object blockToJSON(const CBlock& block,
                                       const CBlockIndex* blockindex,
                                       bool fPrintTransactionDetail);
extern void TxToJSON(const CTransaction& tx,
                     const uint256 hashBlock,
                     json_spirit::Object& entry);

BlockchainPage::BlockchainPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BlockchainPage)
{
    ui->setupUi(this);
    GUIUtil::SetDaleCoinFonts(this);

    model = new BlockchainModel(this);
    ui->blockchainView->setModel(model);

    connect(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &,int,int)),
            this, SLOT(updateCurrentBlockIndex()));
    connect(model, SIGNAL(rowsInserted(const QModelIndex &,int,int)),
            this, SLOT(scrollToCurrentBlockIndex()));

    connect(ui->buttonChain, SIGNAL(clicked()), this, SLOT(showChainPage()));
    connect(ui->buttonBlock, SIGNAL(clicked()), this, SLOT(showBlockPage()));
    connect(ui->buttonTx, SIGNAL(clicked()), this, SLOT(showTxPage()));

    ui->blockchainView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->blockValues->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->txValues->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->blockchainView->installEventFilter(new BlockchainPageChainEvents(ui->blockchainView, this));
    ui->blockValues->installEventFilter(new BlockchainPageBlockEvents(ui->blockValues, this));
    ui->txValues->installEventFilter(new BlockchainPageTxEvents(ui->blockValues, this));

    connect(ui->blockchainView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(openChainMenu(const QPoint &)));
    connect(ui->blockValues, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(openBlockMenu(const QPoint &)));
    connect(ui->txValues, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(openTxMenu(const QPoint &)));

    connect(ui->blockchainView, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(openBlock(const QModelIndex &)));
    connect(ui->blockValues, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(openTx(QTreeWidgetItem*,int)));
    connect(ui->blockValues, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(openBlock(QTreeWidgetItem*,int)));
    connect(ui->txInputs, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(openFractions(QTreeWidgetItem*,int)));
    connect(ui->txOutputs, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(openFractions(QTreeWidgetItem*,int)));
    connect(ui->txInputs, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(openTx(QTreeWidgetItem*,int)));
    connect(ui->txOutputs, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(openTx(QTreeWidgetItem*,int)));

    QFont font = GUIUtil::bitcoinAddressFont();
    qreal pt = font.pointSizeF()*0.8;
    if (pt != .0) {
        font.setPointSizeF(pt);
    } else {
        int px = font.pixelSize()*8/10;
        font.setPixelSize(px);
    }

    QString hstyle = R"(
        QHeaderView::section {
            background-color: rgb(204,203,227);
            color: rgb(64,64,64);
            padding-left: 4px;
            border: 0px solid #6c6c6c;
            border-right: 1px solid #6c6c6c;
            border-bottom: 1px solid #6c6c6c;
            min-height: 16px;
            text-align: left;
        }
    )";
    ui->txValues->setStyleSheet(hstyle);
    ui->txInputs->setStyleSheet(hstyle);
    ui->txOutputs->setStyleSheet(hstyle);
    ui->blockValues->setStyleSheet(hstyle);
    ui->blockchainView->setStyleSheet(hstyle);

    ui->txValues->setFont(font);
    ui->txInputs->setFont(font);
    ui->txOutputs->setFont(font);
    ui->blockValues->setFont(font);
    ui->blockchainView->setFont(font);

    ui->txValues->header()->setFont(font);
    ui->txInputs->header()->setFont(font);
    ui->txOutputs->header()->setFont(font);
    ui->blockValues->header()->setFont(font);
    ui->blockchainView->header()->setFont(font);

    ui->txValues->header()->resizeSection(0 /*property*/, 250);
    ui->txInputs->header()->resizeSection(0 /*n*/, 50);
    ui->txOutputs->header()->resizeSection(0 /*n*/, 50);
    ui->txInputs->header()->resizeSection(1 /*tx*/, 140);
    ui->txOutputs->header()->resizeSection(1 /*tx*/, 140);
    ui->txInputs->header()->resizeSection(2 /*addr*/, 280);
    ui->txOutputs->header()->resizeSection(2 /*addr*/, 280);
    ui->txInputs->header()->resizeSection(3 /*value*/, 180);
    ui->txOutputs->header()->resizeSection(3 /*value*/, 180);

    auto txInpValueDelegate = new LeftSideIconItemDelegate(ui->txInputs);
    auto txOutValueDelegate = new LeftSideIconItemDelegate(ui->txOutputs);
    ui->txInputs->setItemDelegateForColumn(3 /*value*/, txInpValueDelegate);
    ui->txOutputs->setItemDelegateForColumn(3 /*value*/, txOutValueDelegate);

    auto txInpFractionsDelegate = new FractionsItemDelegate(ui->txInputs);
    auto txOutFractionsDelegate = new FractionsItemDelegate(ui->txOutputs);
    ui->txInputs->setItemDelegateForColumn(4 /*frac*/, txInpFractionsDelegate);
    ui->txOutputs->setItemDelegateForColumn(4 /*frac*/, txOutFractionsDelegate);

    connect(ui->lineJumpToBlock, SIGNAL(returnPressed()),
            this, SLOT(jumpToBlock()));
    connect(ui->lineFindBlock, SIGNAL(returnPressed()),
            this, SLOT(openBlockFromInput()));
    connect(ui->lineTx, SIGNAL(returnPressed()),
            this, SLOT(openTxFromInput()));

    pmChange = QPixmap(":/icons/change");
    pmChange = pmChange.scaled(32,32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    pmFrozenF = QPixmap(":/icons/frost");
    pmFrozenF = pmFrozenF.scaled(32,32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    pmFrozenV = QPixmap(":/icons/frostr");
    pmFrozenV = pmFrozenV.scaled(32,32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    pmFrozenL = QPixmap(":/icons/frostl");
    pmFrozenL = pmFrozenL.scaled(32,32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

BlockchainPage::~BlockchainPage()
{
    delete ui;
}

BlockchainModel * BlockchainPage::blockchainModel() const
{
    return model;
}

void BlockchainPage::showChainPage()
{
    ui->tabs->setCurrentWidget(ui->pageChain);
}

void BlockchainPage::showBlockPage()
{
    ui->tabs->setCurrentWidget(ui->pageBlock);
}

void BlockchainPage::showTxPage()
{
    ui->tabs->setCurrentWidget(ui->pageTx);
}

void BlockchainPage::jumpToBlock()
{
    bool ok = false;
    int blockNum = ui->lineJumpToBlock->text().toInt(&ok);
    if (!ok) return;

    int n = ui->blockchainView->model()->rowCount();
    int r = n-blockNum;
    if (r<0 || r>=n) return;
    auto mi = ui->blockchainView->model()->index(r, 0);
    ui->blockchainView->setCurrentIndex(mi);
    ui->blockchainView->selectionModel()->select(mi, QItemSelectionModel::Current);
    ui->blockchainView->scrollTo(mi, QAbstractItemView::PositionAtCenter);
    ui->blockchainView->setFocus();
    ui->lineJumpToBlock->clear();
}

void BlockchainPage::openBlockFromInput()
{
    bool ok = false;
    int blockNum = ui->lineFindBlock->text().toInt(&ok);
    if (ok) {
        int n = ui->blockchainView->model()->rowCount();
        int r = n-blockNum;
        if (r<0 || r>=n) return;
        auto mi = ui->blockchainView->model()->index(r, 0);
        openBlock(mi);
        return;
    }
    // consider it as hash
    uint256 hash(ui->lineFindBlock->text().toStdString());
    openBlock(hash);
}

void BlockchainPage::updateCurrentBlockIndex()
{
    currentBlockIndex = ui->blockchainView->currentIndex();
}

void BlockchainPage::scrollToCurrentBlockIndex()
{
    ui->blockchainView->scrollTo(currentBlockIndex, QAbstractItemView::PositionAtCenter);
}

void BlockchainPage::openChainMenu(const QPoint & pos)
{
    QModelIndex mi = ui->blockchainView->indexAt(pos);
    if (!mi.isValid()) return;

    QMenu m;
    auto a = m.addAction(tr("Open Block"));
    connect(a, &QAction::triggered, [&] { openBlock(mi); });
    m.addSeparator();
    a = m.addAction(tr("Copy Block Hash"));
    connect(a, &QAction::triggered, [&] {
        QApplication::clipboard()->setText(
            mi.data(BlockchainModel::HashStringRole).toString()
        );
    });
    a = m.addAction(tr("Copy Block Height"));
    connect(a, &QAction::triggered, [&] {
        QApplication::clipboard()->setText(
            mi.data(BlockchainModel::HeightRole).toString()
        );
    });
    a = m.addAction(tr("Copy Block Info"));
    connect(a, &QAction::triggered, [&] {
        auto shash = mi.data(BlockchainModel::HashStringRole).toString();
        uint256 hash(shash.toStdString());
        if (!mapBlockIndex.count(hash))
            return;

        CBlock block;
        auto pblockindex = mapBlockIndex[hash];
        block.ReadFromDisk(pblockindex, true);

        json_spirit::Value result = blockToJSON(block, pblockindex, false);
        string str = json_spirit::write_string(result, true);

        QApplication::clipboard()->setText(
            QString::fromStdString(str)
        );
    });
    m.exec(ui->blockchainView->viewport()->mapToGlobal(pos));
}

bool BlockchainPageChainEvents::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            QModelIndex mi = treeWidget->currentIndex();
            if (!mi.isValid()) return true;
            auto shash = mi.data(BlockchainModel::HashStringRole).toString();
            uint256 hash(shash.toStdString());
            if (!mapBlockIndex.count(hash))
                return true;

            CBlock block;
            auto pblockindex = mapBlockIndex[hash];
            block.ReadFromDisk(pblockindex, true);

            json_spirit::Value result = blockToJSON(block, pblockindex, false);
            string str = json_spirit::write_string(result, true);

            QApplication::clipboard()->setText(
                QString::fromStdString(str)
            );
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void BlockchainPage::openBlock(const QModelIndex & mi)
{
    if (!mi.isValid())
        return;
    openBlock(mi.data(BlockchainModel::HashRole).value<uint256>());
}

void BlockchainPage::openBlock(QTreeWidgetItem* item, int)
{
    if (item->text(0) == "Next" || item->text(0) == "Previous") {
        uint256 bhash(item->text(1).toStdString());
        openBlock(bhash);
    }
}

void BlockchainPage::openBlock(uint256 hash)
{
    currentBlock = hash;
    QString bhash = QString::fromStdString(currentBlock.ToString());

    LOCK(cs_main);
    if (mapBlockIndex.find(currentBlock) == mapBlockIndex.end())
        return;
    CBlockIndex* pblockindex = mapBlockIndex[currentBlock];
    if (!pblockindex)
        return;
    showBlockPage();
    if (ui->lineFindBlock->text() != bhash && ui->lineFindBlock->text().toInt() != pblockindex->nHeight)
        ui->lineFindBlock->clear();
    ui->blockValues->clear();
    auto topItem = new QTreeWidgetItem(QStringList({"Height",QString::number(pblockindex->nHeight)}));
    QVariant vhash;
    vhash.setValue(hash);
    topItem->setData(0, BlockchainModel::HashRole, vhash);
    ui->blockValues->addTopLevelItem(topItem);
    ui->blockValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Datetime",QString::fromStdString(DateTimeStrFormat(pblockindex->GetBlockTime()))})));
    ui->blockValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Hash",bhash})));
    QString pbhash;
    QString nbhash;
    if (pblockindex->pprev) {
        pbhash = QString::fromStdString(pblockindex->pprev->GetBlockHash().ToString());
    }
    if (pblockindex->pnext) {
        nbhash = QString::fromStdString(pblockindex->pnext->GetBlockHash().ToString());
    }
    ui->blockValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Next",nbhash})));
    ui->blockValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Previous",pbhash})));

    CBlock block;
    block.ReadFromDisk(pblockindex, true);

    int idx = 0;
    for(const CTransaction & tx : block.vtx) {
        QString stx = "tx"+QString::number(idx);
        QString thash = QString::fromStdString(tx.GetHash().ToString());
        ui->blockValues->addTopLevelItem(new QTreeWidgetItem(QStringList({stx,thash})));
        idx++;
    }
}

void BlockchainPage::openBlockMenu(const QPoint & pos)
{
    QModelIndex mi = ui->blockValues->indexAt(pos);
    if (!mi.isValid()) return;
    auto model = mi.model();
    if (!model) return;

    QMenu m;

    auto a = m.addAction(tr("Copy Value"));
    connect(a, &QAction::triggered, [&] {
        QModelIndex mi2 = model->index(mi.row(), 1 /*value column*/);
        QApplication::clipboard()->setText(
            mi2.data(Qt::DisplayRole).toString()
        );
    });
    a = m.addAction(tr("Copy All Rows"));
    connect(a, &QAction::triggered, [&] {
        QString text;
        for(int r=0; r<model->rowCount(); r++) {
            for(int c=0; c<model->columnCount(); c++) {
                if (c>0) text += "\t";
                QModelIndex mi2 = model->index(r, c);
                text += mi2.data(Qt::DisplayRole).toString();
            }
            text += "\n";
        }
        QApplication::clipboard()->setText(text);
    });
    a = m.addAction(tr("Copy Block Info (json)"));
    connect(a, &QAction::triggered, [&] {
        QModelIndex mi2 = model->index(0, 0); // topItem
        auto hash = mi2.data(BlockchainModel::HashRole).value<uint256>();
        if (!mapBlockIndex.count(hash))
            return;

        CBlock block;
        auto pblockindex = mapBlockIndex[hash];
        block.ReadFromDisk(pblockindex, true);

        json_spirit::Value result = blockToJSON(block, pblockindex, false);
        string str = json_spirit::write_string(result, true);

        QApplication::clipboard()->setText(
            QString::fromStdString(str)
        );
    });
    m.exec(ui->blockValues->viewport()->mapToGlobal(pos));
}

bool BlockchainPageBlockEvents::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            QModelIndex mi = treeWidget->currentIndex();
            if (!mi.isValid()) return true;
            auto model = mi.model();
            if (!model) return true;
            QModelIndex mi2 = model->index(mi.row(), 1 /*value column*/);
            QApplication::clipboard()->setText(
                mi2.data(Qt::DisplayRole).toString()
            );
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

static QString scriptToAddress(const CScript& scriptPubKey,
                               bool& is_notary,
                               bool show_alias =true) {
    is_notary = false;
    int nRequired;
    txnouttype type;
    vector<CTxDestination> addresses;
    if (ExtractDestinations(scriptPubKey, type, addresses, nRequired)) {
        std::string str_addr_all;
        bool none = true;
        for(const CTxDestination& addr : addresses) {
            std::string str_addr = CBitcoinAddress(addr).ToString();
            if (show_alias) {
                if (str_addr == "bNyZrPLQAMPvYedrVLDcBSd8fbLdNgnRPz") {
                    str_addr = "peginflate";
                }
                else if (str_addr == "bNyZrP2SbrV6v5HqeBoXZXZDE2e4fe6STo") {
                    str_addr = "pegdeflate";
                }
                else if (str_addr == "bNyZrPeFFNP6GFJZCkE82DDN7JC4K5Vrkk") {
                    str_addr = "pegnochange";
                }
            }
            if (!str_addr_all.empty())
                str_addr_all += "\n";
            str_addr_all += str_addr;
            none = false;
        }
        if (!none)
            return QString::fromStdString(str_addr_all);
    }
    const CScript& script1 = scriptPubKey;

    opcodetype opcode1;
    vector<unsigned char> vch1;
    CScript::const_iterator pc1 = script1.begin();
    if (!script1.GetOp(pc1, opcode1, vch1))
        return QString();

    if (opcode1 == OP_RETURN && script1.size()>1) {
        is_notary = true;
        QString left_bytes;
        unsigned long len_bytes = script1[1];
        if (len_bytes > script1.size()-2)
            len_bytes = script1.size()-2;
        for(unsigned int i=0; i< len_bytes; i++) {
            left_bytes += char(script1[i+2]);
        }
        return left_bytes;
    }

    return QString();
}

static QString displayValue(int64_t nValue) {
    QString sValue = QString::number(nValue);
    if (sValue.length() <8) {
        sValue = sValue.rightJustified(8, QChar(' '));
    }
    sValue.insert(sValue.length()-8, QChar('.'));
    if (sValue.length() > (8+1+3))
        sValue.insert(sValue.length()-8-1-3, QChar(','));
    if (sValue.length() > (8+1+3+1+3))
        sValue.insert(sValue.length()-8-1-3-1-3, QChar(','));
    if (sValue.length() > (8+1+3+1+3+1+3))
        sValue.insert(sValue.length()-8-1-3-1-3-1-3, QChar(','));
    return sValue;
}

static QString displayValueR(int64_t nValue, int len=0) {
    if (len==0) {
        len = 8+1+3+1+3+1+3+5;
    }
    QString sValue = displayValue(nValue);
    sValue = sValue.rightJustified(len, QChar(' '));
    return sValue;
}

static QString txId(CTxDB& txdb, uint256 txhash) {
    QString txid;
    CTxIndex txindex;
    txdb.ReadTxIndex(txhash, txindex);
    uint nTxNum = 0;
    int nHeight = txindex.GetHeightInMainChain(&nTxNum, txhash);
    if (nHeight >0) {
        txid = QString("%1-%2").arg(nHeight).arg(nTxNum);
    }
    return txid;
}

void BlockchainPage::openTxFromInput()
{
    // as height-index
    if (ui->lineTx->text().contains("-")) {
        auto args = ui->lineTx->text().split("-");
        bool ok = false;
        int blockNum = args.front().toInt(&ok);
        if (ok) {
            uint txidx = args.back().toUInt();
            int n = ui->blockchainView->model()->rowCount();
            int r = n-blockNum;
            if (r<0 || r>=n) return;
            auto mi = ui->blockchainView->model()->index(r, 0);
            auto bhash = mi.data(BlockchainModel::HashRole).value<uint256>();
            openTx(bhash, txidx);
        }
        return;
    }
    // consider it as hash
    uint nTxNum = 0;
    uint256 blockhash;
    uint256 txhash(ui->lineTx->text().toStdString());
    {
        LOCK(cs_main);
        CTxDB txdb("r");
        CTxIndex txindex;
        txdb.ReadTxIndex(txhash, txindex);
        txindex.GetHeightInMainChain(&nTxNum, txhash, &blockhash);
    }
    openTx(blockhash, nTxNum);
}

void BlockchainPage::openTx(QTreeWidgetItem * item, int column)
{
    if (item->text(0).startsWith("tx")) { // open from block page
        bool tx_idx_ok = false;
        uint tx_idx = item->text(0).mid(2).toUInt(&tx_idx_ok);
        if (!tx_idx_ok)
            return;

        openTx(currentBlock, tx_idx);
    }

    else if ((sender() == ui->txInputs || sender() == ui->txOutputs) && column == 1) {
        uint256 txhash = item->data(1, BlockchainModel::HashRole).value<uint256>();
        CTxDB txdb("r");
        CTxIndex txindex;
        txdb.ReadTxIndex(txhash, txindex);
        uint nTxNum = 0;
        uint256 blockhash;
        txindex.GetHeightInMainChain(&nTxNum, txhash, &blockhash);
        openTx(blockhash, nTxNum);
    }
}

static bool calculateFeesFractions(CBlockIndex* pblockindex,
                                   CBlock& block,
                                   int64_t& nFeesValue)
{
    MapPrevTx mapInputs;
    map<uint256, CTxIndex> mapUnused;

    for (CTransaction & tx : block.vtx) {

        if (tx.IsCoinBase()) continue;
        if (tx.IsCoinStake()) continue;

        uint256 hash = tx.GetHash();

        CTxDB txdb("r");
        if (!txdb.ContainsTx(hash))
            return false;

        bool fInvalid = false;
        tx.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid);

        int64_t nTxValueIn = tx.GetValueIn(mapInputs);
        int64_t nTxValueOut = tx.GetValueOut();
        nFeesValue += nTxValueIn - nTxValueOut;
    }

    return true;
}

void BlockchainPage::openTx(uint256 blockhash, uint txidx)
{
    LOCK(cs_main);
    if (mapBlockIndex.find(blockhash) == mapBlockIndex.end())
        return;
    CBlockIndex* pblockindex = mapBlockIndex[blockhash];
    if (!pblockindex)
        return;

    CBlock block;
    block.ReadFromDisk(pblockindex, true);
    if (txidx >= block.vtx.size())
        return;

    CTransaction & tx = block.vtx[txidx];
    uint256 hash = tx.GetHash();
    QString thash = QString::fromStdString(hash.ToString());
    QString sheight = QString("%1-%2").arg(pblockindex->nHeight).arg(txidx);

    CTxDB txdb("r");
    if (!txdb.ContainsTx(hash))
        return;

    showTxPage();
    ui->txValues->clear();
    auto topItem = new QTreeWidgetItem(QStringList({"Height",sheight}));
    QVariant vhash;
    vhash.setValue(hash);
    topItem->setData(0, BlockchainModel::HashRole, vhash);
    ui->txValues->addTopLevelItem(topItem);
    ui->txValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Datetime",QString::fromStdString(DateTimeStrFormat(pblockindex->GetBlockTime()))})));
    ui->txValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Hash",thash})));

    QString txtype = tr("Transaction");
    if (tx.IsCoinBase()) txtype = tr("CoinBase");
    if (tx.IsCoinStake()) txtype = tr("CoinStake");
    ui->txValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Type",txtype})));

    // logic

    MapPrevTx mapInputs;
    map<uint256, CTxIndex> mapUnused;
    int64_t nFeesValue = 0;
    bool fInvalid = false;
    tx.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid);

    int64_t nReserveIn = 0;
    int64_t nLiquidityIn = 0;

    // gui

    ui->txInputs->clear();
    QSet<QString> sAddresses;
    size_t n_vin = tx.vin.size();
    if (tx.IsCoinBase()) n_vin = 0;
    int64_t nValueIn = 0;
    for (unsigned int i = 0; i < n_vin; i++)
    {
        COutPoint prevout = tx.vin[i].prevout;
        QStringList row;
        row << QString::number(i); // idx, 0

        QString prev_thash = QString::fromStdString(prevout.hash.ToString());
        QString prev_txid_hash = prev_thash.left(4)+"..."+prev_thash.right(4);
        QString prev_txid_height = txId(txdb, prevout.hash);
        QString prev_txid = prev_txid_height.isEmpty() ? prev_txid_hash : prev_txid_height;

        row << QString("%1:%2").arg(prev_txid).arg(prevout.n); // tx, 1

        int64_t nValue = 0;
        if (mapInputs.find(prevout.hash) != mapInputs.end()) {
            CTransaction& txPrev = mapInputs[prevout.hash].second;
            if (prevout.n < txPrev.vout.size()) {
                bool is_notary = false;
                auto addr = scriptToAddress(txPrev.vout[prevout.n].scriptPubKey, is_notary);
                if (addr.isEmpty())
                    row << "N/A"; // address, 2
                else {
                    row << addr;
                    sAddresses.insert(addr);
                }

                nValue = txPrev.vout[prevout.n].nValue;
                nValueIn += nValue;
                row << displayValue(txPrev.vout[prevout.n].nValue);
            }
            else {
                row << "N/A"; // address, 2
                row << "none"; // value, 3
            }
        }
        else {
            row << "N/A"; // address
            row << "none"; // value
        }

        auto input = new QTreeWidgetItem(row);
        {
            QVariant vhash;
            vhash.setValue(prevout.hash);
            input->setData(1, BlockchainModel::HashRole, vhash);
            input->setData(1, BlockchainModel::OutNumRole, prevout.n);
        }
        QVariant vFractions;
        input->setData(4, BlockchainModel::FractionsRole, vFractions);
        input->setData(4, BlockchainModel::PegSupplyRole, 0);
        input->setData(3, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
        ui->txInputs->addTopLevelItem(input);
    }

    if (tx.IsCoinStake()) {
        uint64_t nCoinAge = 0;
        if (!tx.GetCoinAge(txdb, pblockindex->pprev, nCoinAge)) {
            //something went wrong
        }
        int64_t nCalculatedStakeReward = GetProofOfStakeReward(pblockindex->pprev, nCoinAge, 0 /*fees*/);

        QStringList rowMined;
        rowMined << "Mined"; // idx, 0
        rowMined << "";      // tx, 1
        rowMined << "N/A";   // address, 2
        rowMined << displayValue(nCalculatedStakeReward);

        auto inputMined = new QTreeWidgetItem(rowMined);
        QVariant vfractions;
        inputMined->setData(4, BlockchainModel::FractionsRole, vfractions);
        inputMined->setData(4, BlockchainModel::PegSupplyRole, 0);
        inputMined->setData(3, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
        ui->txInputs->addTopLevelItem(inputMined);

        // need to collect all fees fractions from all tx in the block
        if (!calculateFeesFractions(pblockindex, block, nFeesValue)) {
            // peg violation?
        }

        QStringList rowFees;
        rowFees << "Fees";  // idx, 0
        rowFees << "";      // tx, 1
        rowFees << "N/A";   // address, 2
        rowFees << displayValue(nFeesValue);

        auto inputFees = new QTreeWidgetItem(rowFees);
        inputFees->setData(4, BlockchainModel::FractionsRole, vfractions);
        inputFees->setData(4, BlockchainModel::PegSupplyRole, 0);
        inputFees->setData(3, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
        ui->txInputs->addTopLevelItem(inputFees);
    }

    CTxIndex txindex;
    txdb.ReadTxIndex(hash, txindex);

    ui->txOutputs->clear();
    size_t n_vout = tx.vout.size();
    if (tx.IsCoinBase()) n_vout = 0;
    int64_t nValueOut = 0;
    int64_t nLiquidityOut = 0;
    for (unsigned int i = 0; i < n_vout; i++)
    {
        QStringList row;
        row << QString::number(i); // 0, idx

        bool hasSpend = false;
        QString titleSpend;
        uint256 next_hash;
        uint next_outnum = 0;

        if (i < txindex.vSpent.size()) {
            CDiskTxPos & txpos = txindex.vSpent[i];
            CTransaction txSpend;
            if (txSpend.ReadFromDisk(txpos)) {
                int vin_idx =0;
                for(const CTxIn &txin : txSpend.vin) {
                    if (txin.prevout.hash == hash && txin.prevout.n == i) {
                        next_hash = txSpend.GetHash();
                        next_outnum = i;

                        QString next_thash = QString::fromStdString(next_hash.ToString());
                        QString next_txid_hash = next_thash.left(4)+"..."+next_thash.right(4);
                        QString next_txid_height = txId(txdb, next_hash);
                        QString next_txid = next_txid_height.isEmpty() ? next_txid_hash : next_txid_height;

                        row << QString("%1:%2").arg(next_txid).arg(vin_idx); // 1, spend
                        titleSpend = QString("%1:%2").arg(next_thash).arg(vin_idx);
                        hasSpend = true;
                    }
                    vin_idx++;
                }
            }
        }

        bool is_notary = false;
        auto addr = scriptToAddress(tx.vout[i].scriptPubKey, is_notary);

        if (!hasSpend) {
            if (is_notary) {
                row << "Notary/Burn"; // 1, spend
            }
            else row << ""; // 1, spend
        }

        if (addr.isEmpty())
            row << "N/A"; // 2, address
        else row << addr;

        nValueOut += tx.vout[i].nValue;
        row << displayValue(tx.vout[i].nValue); // 3, value

        bool fIndicateFrozen = false;
        auto output = new QTreeWidgetItem(row);
        if (hasSpend) {
            QVariant vhash;
            vhash.setValue(next_hash);
            output->setData(1, BlockchainModel::HashRole, vhash);
            output->setData(1, BlockchainModel::OutNumRole, next_outnum);
        }
        QVariant vFractions;
        output->setData(4, BlockchainModel::HashRole, titleSpend);
        output->setData(4, BlockchainModel::FractionsRole, vFractions);
        output->setData(4, BlockchainModel::PegSupplyRole, 0);
        output->setData(3, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
        if (!fIndicateFrozen && sAddresses.contains(addr)) {
            output->setData(3, Qt::DecorationPropertyRole, pmChange);
        }
        ui->txOutputs->addTopLevelItem(output);
    }

    int nValueMaxLen = qMax(displayValue(nValueIn).length(),
                            qMax(displayValue(nValueOut).length(),
                                 qMax(displayValue(nReserveIn).length(),
                                      qMax(displayValue(nLiquidityIn).length(),
                                           displayValue(nLiquidityOut).length()))));
    auto sValueIn = displayValueR(nValueIn, nValueMaxLen);
    auto sValueOut = displayValueR(nValueOut, nValueMaxLen);

    ui->txValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Value In",sValueIn})));
    ui->txValues->addTopLevelItem(new QTreeWidgetItem(QStringList({"Value Out",sValueOut})));

    if (!tx.IsCoinBase() && !tx.IsCoinStake() && nValueOut < nValueIn) {
        QStringList row;
        row << "Fee";
        row << ""; // spent
        row << ""; // address (todo)
        row << displayValue(nValueIn - nValueOut);
        auto outputFees = new QTreeWidgetItem(row);
        QVariant vfractions;
        {
            outputFees->setData(4, BlockchainModel::FractionsRole, vfractions);
            outputFees->setData(4, BlockchainModel::PegSupplyRole, 0);
        }
        outputFees->setData(3, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
        ui->txOutputs->addTopLevelItem(outputFees);
    }
}

void BlockchainPage::openTxMenu(const QPoint & pos)
{
    QModelIndex mi = ui->txValues->indexAt(pos);
    if (!mi.isValid()) return;
    auto model = mi.model();
    if (!model) return;

    QMenu m;

    auto a = m.addAction(tr("Copy Value"));
    connect(a, &QAction::triggered, [&] {
        QModelIndex mi2 = model->index(mi.row(), 1 /*value column*/);
        QApplication::clipboard()->setText(
            mi2.data(Qt::DisplayRole).toString()
        );
    });
    a = m.addAction(tr("Copy All Rows"));
    connect(a, &QAction::triggered, [&] {
        QString text;
        for(int r=0; r<model->rowCount(); r++) {
            for(int c=0; c<model->columnCount(); c++) {
                if (c>0) text += "\t";
                QModelIndex mi2 = model->index(r, c);
                text += mi2.data(Qt::DisplayRole).toString();
            }
            text += "\n";
        }
        QApplication::clipboard()->setText(text);
    });
    a = m.addAction(tr("Copy Transaction Info (json)"));
    connect(a, &QAction::triggered, [&] {
        QModelIndex mi2 = model->index(0, 0); // topItem
        auto hash = mi2.data(BlockchainModel::HashRole).value<uint256>();

        CTransaction tx;
        uint256 hashBlock = 0;
        if (!GetTransaction(hash, tx, hashBlock))
            return;

        json_spirit::Object result;
        TxToJSON(tx, hashBlock, result);
        json_spirit::Value vresult = result;
        string str = json_spirit::write_string(vresult, true);

        QApplication::clipboard()->setText(
            QString::fromStdString(str)
        );
    });
    m.exec(ui->txValues->viewport()->mapToGlobal(pos));
}

bool BlockchainPageTxEvents::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            QModelIndex mi = treeWidget->currentIndex();
            if (!mi.isValid()) return true;
            auto model = mi.model();
            if (!model) return true;
            QModelIndex mi2 = model->index(mi.row(), 1 /*value column*/);
            QApplication::clipboard()->setText(
                mi2.data(Qt::DisplayRole).toString()
            );
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void BlockchainPage::openFractions(QTreeWidgetItem * item, int column)
{
    if (column != 4) // only fractions column
        return;

    return; // tmp
    
//    auto dlg = new QDialog(this);
//    Ui::FractionsDialog ui;
//    ui.setupUi(dlg);
//    QwtPlot * fplot = new QwtPlot;
//    QVBoxLayout *fvbox = new QVBoxLayout;
//    fvbox->setMargin(0);
//    fvbox->addWidget(fplot);
//    ui.chart->setLayout(fvbox);

//    QFont font = GUIUtil::bitcoinAddressFont();
//    qreal pt = font.pointSizeF()*0.8;
//    if (pt != .0) {
//        font.setPointSizeF(pt);
//    } else {
//        int px = font.pixelSize()*8/10;
//        font.setPixelSize(px);
//    }

//    QString hstyle = R"(
//        QHeaderView::section {
//            background-color: rgb(204,203,227);
//            color: rgb(64,64,64);
//            padding-left: 4px;
//            border: 0px solid #6c6c6c;
//            border-right: 1px solid #6c6c6c;
//            border-bottom: 1px solid #6c6c6c;
//            min-height: 16px;
//            text-align: left;
//        }
//    )";
//    ui.fractions->setStyleSheet(hstyle);
//    ui.fractions->setFont(font);
//    ui.fractions->header()->setFont(font);
//    ui.fractions->header()->resizeSection(0 /*n*/, 50);
//    ui.fractions->header()->resizeSection(1 /*value*/, 160);

//    ui.fractions->setContextMenuPolicy(Qt::CustomContextMenu);
//    //ui.fractions->installEventFilter(new FractionsDialogEvents(ui.fractions, this));
//    connect(ui.fractions, SIGNAL(customContextMenuRequested(const QPoint &)),
//            this, SLOT(openFractionsMenu(const QPoint &)));

//    auto txhash = item->data(4, BlockchainModel::HashRole).toString();
//    auto supply = item->data(4, BlockchainModel::PegSupplyRole).toInt();
//    auto vfractions = item->data(4, BlockchainModel::FractionsRole);
//    auto fractions = vfractions.value<CFractions>();
//    auto fractions_std = fractions.Std();

////    int64_t fdelta[CPegFractions::PEG_SIZE];
////    int64_t fundelta[CPegFractions::PEG_SIZE];
////    fractions_std.ToDeltas(fdelta);
////    CPegFractions fd;
////    fd.FromDeltas(fdelta);

//    unsigned long len_test = 0;
//    CDataStream fout_test(SER_DISK, CLIENT_VERSION);
//    fractions.Pack(fout_test, &len_test);
//    ui.packedLabel->setText(tr("Packed: %1 bytes").arg(len_test));
//    ui.valueLabel->setText(tr("Value: %1").arg(displayValue(fractions.Total())));
//    ui.reserveLabel->setText(tr("Reserve: %1").arg(displayValue(fractions.Low(supply))));
//    ui.liquidityLabel->setText(tr("Liquidity: %1").arg(displayValue(fractions.High(supply))));

//    qreal xs_reserve[PEG_SIZE*2];
//    qreal ys_reserve[PEG_SIZE*2];
//    qreal xs_liquidity[PEG_SIZE*2];
//    qreal ys_liquidity[PEG_SIZE*2];

//    for (int i=0; i<PEG_SIZE; i++) {
//        QStringList row;
//        row << QString::number(i) << displayValue(fractions_std.f[i]); // << QString::number(fdelta[i]) << QString::number(fd.f[i]);
//        auto row_item = new QTreeWidgetItem(row);
//        row_item->setData(0, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
//        row_item->setData(1, Qt::TextAlignmentRole, int(Qt::AlignVCenter | Qt::AlignRight));
//        ui.fractions->addTopLevelItem(row_item);

//        xs_reserve[i*2] = i;
//        ys_reserve[i*2] = i < supply ? qreal(fractions_std.f[i]) : 0;
//        xs_reserve[i*2+1] = i+1;
//        ys_reserve[i*2+1] = ys_reserve[i*2];

//        xs_liquidity[i*2] = i;
//        ys_liquidity[i*2] = i >= supply ? qreal(fractions_std.f[i]) : 0;
//        xs_liquidity[i*2+1] = i+1;
//        ys_liquidity[i*2+1] = ys_liquidity[i*2];
//    }

//    QPen nopen(Qt::NoPen);

//    auto curve_reserve = new QwtPlotCurve;
//    curve_reserve->setPen(nopen);
//    curve_reserve->setBrush(QColor("#c06a15"));
//    curve_reserve->setSamples(xs_reserve, ys_reserve, supply*2);
//    curve_reserve->setRenderHint(QwtPlotItem::RenderAntialiased);
//    curve_reserve->attach(fplot);

//    auto curve_liquidity = new QwtPlotCurve;
//    curve_liquidity->setPen(nopen);
//    curve_liquidity->setBrush(QColor("#2da5e0"));
//    curve_liquidity->setSamples(xs_liquidity+supply*2,
//                                ys_liquidity+supply*2,
//                                PEG_SIZE*2-supply*2);
//    curve_liquidity->setRenderHint(QwtPlotItem::RenderAntialiased);
//    curve_liquidity->attach(fplot);

//    fplot->replot();

//    dlg->setWindowTitle(txhash+" "+tr("fractions"));
//    dlg->show();
}

void BlockchainPage::openFractionsMenu(const QPoint & pos)
{
    QTreeWidget * table = dynamic_cast<QTreeWidget *>(sender());
    if (!table) return;
    QModelIndex mi = table->indexAt(pos);
    if (!mi.isValid()) return;
    auto model = mi.model();
    if (!model) return;

    QMenu m;

    auto a = m.addAction(tr("Copy Value"));
    connect(a, &QAction::triggered, [&] {
        QModelIndex mi2 = model->index(mi.row(), 1 /*value column*/);
        QApplication::clipboard()->setText(
            mi2.data(Qt::DisplayRole).toString()
        );
    });
    a = m.addAction(tr("Copy All Rows"));
    connect(a, &QAction::triggered, [&] {
        QString text;
        for(int r=0; r<model->rowCount(); r++) {
            for(int c=0; c<model->columnCount(); c++) {
                if (c>0) text += "\t";
                QModelIndex mi2 = model->index(r, c);
                text += mi2.data(Qt::DisplayRole).toString();
            }
            text += "\n";
        }
        QApplication::clipboard()->setText(text);
    });
    m.exec(table->viewport()->mapToGlobal(pos));
}

bool FractionsDialogEvents::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            QModelIndex mi = treeWidget->currentIndex();
            if (!mi.isValid()) return true;
            auto model = mi.model();
            if (!model) return true;
            QModelIndex mi2 = model->index(mi.row(), 1 /*value column*/);
            QApplication::clipboard()->setText(
                mi2.data(Qt::DisplayRole).toString()
            );
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

// delegate to draw icon on left side

LeftSideIconItemDelegate::LeftSideIconItemDelegate(QWidget *parent) :
    QStyledItemDelegate(parent) {}
LeftSideIconItemDelegate::~LeftSideIconItemDelegate() {}

void LeftSideIconItemDelegate::paint(QPainter* p,
                                  const QStyleOptionViewItem& o,
                                  const QModelIndex& index) const
{
    QStyledItemDelegate::paint(p, o, index);
    auto icondata = index.data(Qt::DecorationPropertyRole);
    if (!icondata.isValid()) return;
    QPixmap pm = icondata.value<QPixmap>();
    if (pm.isNull()) return;
    QRect r = o.rect;
    //int rside = r.height()-2;
    QRect pmr = QRect(0,0, 16,16);
    pmr.moveCenter(QPoint(r.left()+pmr.width()/2, r.center().y()));
    //p->setOpacity(0.5);
    p->drawPixmap(pmr, pm);
    //p->setOpacity(1);
}

// delegate to draw fractions

FractionsItemDelegate::FractionsItemDelegate(QWidget *parent) :
    QItemDelegate(parent)
{
}

FractionsItemDelegate::~FractionsItemDelegate()
{
}

void FractionsItemDelegate::paint(QPainter* p,
                                  const QStyleOptionViewItem& o,
                                  const QModelIndex& index) const
{
    return QItemDelegate::paint(p, o, index);
//    auto vfractions = index.data(BlockchainModel::FractionsRole);
//    auto fractions = vfractions.value<CFractions>();
//    auto fractions_std = fractions.Std();

//    int64_t f_max = 0;
//    for (int i=0; i<PEG_SIZE; i++) {
//        auto f = fractions_std.f[i];
//        if (f > f_max) f_max = f;
//    }
//    if (f_max == 0)
//        return; // zero-value fractions

//    auto supply = index.data(BlockchainModel::PegSupplyRole).toInt();

//    QPainterPath path_reserve;
//    QPainterPath path_liquidity;
//    QVector<QPointF> points_reserve;
//    QVector<QPointF> points_liquidity;

//    QRect r = o.rect;
//    qreal rx = r.x();
//    qreal ry = r.y();
//    qreal rw = r.width();
//    qreal rh = r.height();
//    qreal w = PEG_SIZE;
//    qreal h = f_max;
//    qreal pegx = rx + supply*rw/w;

//    points_reserve.push_back(QPointF(rx,r.bottom()));
//    for (int i=0; i<supply; i++) {
//        int64_t f = fractions_std.f[i];
//        qreal x = rx + qreal(i)*rw/w;
//        qreal y = ry + rh - qreal(f)*rh/h;
//        points_reserve.push_back(QPointF(x,y));
//    }
//    points_reserve.push_back(QPointF(pegx,r.bottom()));

//    points_liquidity.push_back(QPointF(pegx,r.bottom()));
//    for (int i=supply; i<PEG_SIZE; i++) {
//        int64_t f = fractions_std.f[i];
//        qreal x = rx + qreal(i)*rw/w;
//        qreal y = ry + rh - qreal(f)*rh/h;
//        points_liquidity.push_back(QPointF(x,y));
//    }
//    points_liquidity.push_back(QPointF(rx+rw,r.bottom()));

//    QPolygonF poly_reserve(points_reserve);
//    path_reserve.addPolygon(poly_reserve);

//    QPolygonF poly_liquidity(points_liquidity);
//    path_liquidity.addPolygon(poly_liquidity);

//    p->setRenderHint( QPainter::Antialiasing );

//    p->setBrush( QColor("#c06a15") );
//    p->setPen( QColor("#c06a15") );
//    p->drawPath( path_reserve );

//    p->setBrush( QColor("#2da5e0") );
//    p->setPen( QColor("#2da5e0") );
//    p->drawPath( path_liquidity );

//    p->setPen( Qt::red );
//    p->drawLine(QPointF(pegx, ry), QPointF(pegx, ry+rh));
}
