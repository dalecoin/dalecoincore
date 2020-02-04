// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

//
// Main network
//

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x71;
        pchMessageStart[1] = 0x31;
        pchMessageStart[2] = 0x21;
        pchMessageStart[3] = 0x05;
        vAlertPubKey = ParseHex("04042564520c9d79f48dd9face8d8d7a735bdd774da46ac0422cc6f0ab0477f607b6e7ecefa55d805dbb7d0b95994060d90e76e2ac2fa319f22539036bc9d1acc4");
        nDefaultPort = 11114;
        nRPCPort = 11115;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        //CBlock(hash=00000fc3f64b1c0c2044a6d770c94839b925520b2dde5adfdf440c226e91f927, ver=1, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=fd71368ec648e4330b0accd85b5f000ad6bb388c52b4b8464c3552ac386e8850, nTime=1580730503, nBits=1e0fffff, nNonce=1464318, vtx=1, vchBlockSig=)
        //  Coinbase(hash=12630d16a9, nTime=1580730503, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //    CTxIn(COutPoint(0000000000, 4294967295), coinbase 00012a24323020466562203230313420426974636f696e2041544d7320636f6d6520746f20555341)
        //    CTxOut(empty)
        //  vMerkleTree: 12630d16a9

        //Change the text and nTime below if you wish to make a new genesis
        const char* pszTimestamp = "DaleCoin gonna make an Impact on the altcoin market 03/02/2020  12:50 pm";
        CTransaction txNew;
        txNew.nTime = 1580730503;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].SetEmpty();
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1580730503;
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 1464318;

        // Uncommenting this will make a new genesis which you can change the info above
        //uint256 hashTarget = CBigNum().SetCompact(genesis.nBits).getuint256();
        //printf("Making genesis");
        //while (genesis.GetHash() > hashTarget)
        //{
        //  ++genesis.nNonce;
        //  if (genesis.nNonce == 0)
        //             {
        //                  printf("NONCE WRAPPED, incrementing time");
        //                  ++genesis.nTime;
        //             }
        //}
        //printf("genesis.GetHash() == %s\n", genesis.GetHash().ToString().c_str());
        //printf("genesis.hashMerkleRoot == %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("genesis.nTime = %u \n", genesis.nTime);
        //printf("genesis.nNonce = %u \n", genesis.nNonce);
        //hashGenesisBlock = genesis.GetHash();
        //assert(hashGenesisBlock == uint256("0x0"));

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("00000fc3f64b1c0c2044a6d770c94839b925520b2dde5adfdf440c226e91f927"));
        assert(genesis.hashMerkleRoot == uint256("4ec4c29c0737743a0d579922232a8cc5474b1dd037725fc172ed4219162006ae"));
        

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,31);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,30);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,158);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        nLastPOWBlock = 10000;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams *mainParams = nullptr;


//
// Testnet
//

class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x2d;
        pchMessageStart[1] = 0x22;
        pchMessageStart[2] = 0x20;
        pchMessageStart[3] = 0x2f;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 16);
        vAlertPubKey = ParseHex("044a003fe137cee9ef9849f03faea3077a729b2f6e748d237975de186ce2fbbe68143ef6a7780d4e7439852a20d01f8d4d3925268ffe224da80a3416cbf526831f");
        nDefaultPort = 38814;
        nRPCPort = 38815;
        strDataDir = "testnet";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 216178;
        hashGenesisBlock = genesis.GetHash();
        //assert(hashGenesisBlock == uint256("0x0000075685d3be1f253ce777174b1594354e79954d2a32a6f77fe9cba00e6467"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();


        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        nLastPOWBlock = 0x7fffffff;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams *testNetParams = nullptr;


//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0xf1;
        pchMessageStart[1] = 0xb2;
        pchMessageStart[2] = 0xb3;
        pchMessageStart[3] = 0xd4;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1411111111;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 2;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 28844;
        strDataDir = "regtest";
        //assert(hashGenesisBlock == uint256("0x523dda6d336047722cbaf1c5dce622298af791bac21b33bf6e2d5048b2a13e3d"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams *regTestParams = nullptr;

static CChainParams *pCurrentParams = mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void InitParamsOnStart() {
	mainParams = new CMainParams();
	testNetParams = new CTestNetParams(); 
	regTestParams = new CRegTestParams();
	pCurrentParams = mainParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);
#ifdef USE_TESTNET
    fTestNet = true;
#endif

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
