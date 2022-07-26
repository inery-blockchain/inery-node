#!/usr/bin/python3

import os
import time
import json, argparse

with open("tools/config.json", "r") as config_file:
    config = json.loads(config_file.read())

def log(message):
	print("==================================================")
	print(message.center(50, ' '))
	print("==================================================")

def exportPath() :
    os.chdir('../inery/2.0/bin')            
    path = f'export PATH="$PATH:{os.getcwd()}"'
    user = os.getenv("HOME")
    bashrc_path = os.path.join(user, '.bashrc')
    with open(bashrc_path, 'a') as bashrc :
        bashrc.write(path)

def configIni(nodeType):
    os.chdir(f'./{nodeType}.node')
    with open("./blockchain/config/config.ini", "a") as config_ini:
        config_ini.write("\nmax-transaction-time = 15000")
        config_ini.write("\nchain-state-db-size-mb = 64000")

def instalDep() :
    dep = ["libncurses5"]
    for d in dep :
        os.system(f"apt install {d}")

def master(config) :
    log("Creating master node")

    os.system("rm -rf master.node; mkdir master.node")
    os.system("cp tools/genesis.json ./")
    os.system("cp tools/scripts/* master.node")
    os.system("chmod +x master.node/stop.sh master.node/clean.sh")

    files = ["master.node/genesis_start.sh", "master.node/start.sh",
            "master.node/hard_replay.sh"]

    master = config["MASTER_ACCOUNT"]
    pubKey = master["PUBLIC_KEY"]
    privKey = master["PRIVATE_KEY"]

    genesis = config["GENESIS_ACCOUNT"]["PEER_ADDRESS"]

    for file in files:
        with open(file, "a") as fs:

            fs.write("--producer-name {0} \\\n".format(master["NAME"]))
            fs.write("--http-server-address {0} \\\n".format(master["HTTP_ADDRESS"]))
            fs.write("--p2p-listen-endpoint {0} \\\n".format(master["PEER_ADDRESS"]))
            fs.write(f"--p2p-peer-address {genesis} \\\n")
            fs.write(f"--signature-provider {pubKey}=KEY:{privKey} \\\n")

            for i in range(len(config["PEERS"])):
                peer_addr = config["PEERS"][i]["PEER_ADDRESS"]
                fs.write(f"--p2p-peer-address {peer_addr} \\\n")

            fs.write(">> $DATADIR\"/nodine.log\" 2>&1 & \\\n")
            fs.write("echo $! > $DATADIR\"/ined.pid\"")
    
        os.system(f"chmod +x {file}")
    
    # Initialize master
    log("* STARTING MASTER *")
    os.chdir("./master.node")
    os.system("./genesis_start.sh")
    time.sleep(3)
    os.chdir("..")

def lite(config) :
    log("Creating lite node")
    os.system("rm -rf lite.node; mkdir lite.node")
    os.system("cp tools/scripts/* lite.node")
    os.system("cp tools/genesis.json ./")
    os.system("chmod +x lite.node/stop.sh lite.node/clean.sh")
    files = ["lite.node/genesis_start.sh", "lite.node/start.sh",
            "lite.node/hard_replay.sh"]
    genesis = config["GENESIS_ACCOUNT"]["PEER_ADDRESS"]

    lite_node = config["LITE_NODE"]
    peer_addr = lite_node["PEER_ADDRESS"]
    http_addr = lite_node["HTTP_ADDRESS"]

    for file in files:
        with open(file, "a") as fs:
            fs.write(f"--http-server-address {http_addr} \\\n")
            fs.write(f"--p2p-listen-endpoint {peer_addr} \\\n")
            fs.write(f"--p2p-peer-address {genesis} \\\n")
            for i in range(len(config["PEERS"])):
                peer_addr = config["PEERS"][i]["PEER_ADDRESS"]
                fs.write(f"--p2p-peer-address {peer_addr} \\\n")
            fs.write(">> $DATADIR\"/nodine.log\" 2>&1 & \\\n")
            fs.write("echo $! > $DATADIR\"/ined.pid\"")
        os.system(f"chmod +x {file}")
    # Initialize lite
    log("* STARTING LITE *")
    os.chdir("./lite.node")
    os.system("./genesis_start.sh")
    time.sleep(2)
    os.chdir("..")

parser = argparse.ArgumentParser()

parser.add_argument("--master", help="Create master node", action='store_true')
parser.add_argument("--lite", help="Create lite node", action='store_true')
parser.add_argument("--export", help="Export inery bin path to .bashrc file", action='store_true')

args = parser.parse_args()
            

if args.export :
    exportPath()

if args.master :
    master(config)
    configIni('master')
    os.system('tail -f blockchain/nodine.log')

if args.lite : 
    lite(config)
    configIni('lite')
    os.system('tail -f blockchain/nodine.log')