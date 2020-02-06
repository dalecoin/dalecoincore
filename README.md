[![Codacy Badge](https://api.codacy.com/project/badge/Grade/1bac5bbdf2f64cfeb67092bef3e50d6f)](https://www.codacy.com/app/yshurik/dalecoin-core?utm_source=github.com&utm_medium=referral&utm_content=dalecoinmarket/dalecoin-core&utm_campaign=badger)
[![Build Status](https://travis-ci.org/dalecoinmarket/dalecoin-core.svg?branch=master)](https://travis-ci.org/dalecoinmarket/dalecoin-core)
[![Build status](https://ci.appveyor.com/api/projects/status/qdy7pilwdtxehqhw?svg=true)](https://ci.appveyor.com/project/yshurik/dalecoin-core)
[![Open Source Love](https://badges.frapsoft.com/os/mit/mit.svg?v=102)](https://github.com/dalecoinmarket/dalecoin-core/blob/master/COPYING)


DaleCoin development tree

DaleCoin is a PoW/PoS-based cryptocurrency forked from bitbay.

DaleCoin
===========================

DaleCoin is a decentralized digital asset used for payments of goods and services.It was formally an erc20 token but now migrated into its own blockchain.

Development process
===========================

Developers work in their own trees, then submit pull requests when they think their feature or bug fix is ready.


Window wallet setup process
===========================
1. run the dalecoin windows wallet then close it.
2. locate the dalecoin data directory in your pc "C:\Users\YourUserName\Appdata\Roaming\DaleCoin"
3. open the folder and create a file dalecoin.conf and paste the following data in it and save.

rpcuser=dalecoinrpc
rpcpassword=anypassword
rpcallowip=127.0.0.1
server=1
deamon=1
addnode=45.76.42.253
addnode=45.63.8.179


you can download the dalecoin.conf file [here](https://github.com/dalecoin/dalecoincore/blob/master/dalecoin.conf). and paste it in dalecoin data directory.
now run the windows wallet to sync to the network

same process applies to ubuntu. locate the dalecoin data directory and paste the data in the config file and save.


Dalecoin mainchain specs
===========================

initial coin supply= 250,000 coins

maximum coin supply= 5,000,000 coins 

yearly coin reward emission= ~ 60,000 to 100,000 coins
monthly coin reward emission= ~ 6,000 to 10,000 coins

POW reward= 0.05 coins/block
POS reward= 0.20 coins/block

average block time = ~ 60 seconds


Swap from dalc erc20 to dalc mainchain details
================================================
100,000 will be used for swap (10 dalc erc20 to 1 dalc mainchain) click link to swap. 
100,000 will be used to negotiate retaining old exchanges and aquiring new exchanges.
50,000 for marketing and development.


