import sys,os,time
from os import path
from subprocess import call
import numpy as np
import scipy as sp
import re

GT_ORIGIN_X = 1.6
GT_ORIGIN_Y = 1.8


def read_gt(GT_FILE):
    gtdata = np.loadtxt(GT_FILE,delimiter=',')
    gtdata[:,1] = gtdata[:,1] + GT_ORIGIN_X
    gtdata[:,2] = gtdata[:,2] + GT_ORIGIN_Y
    gtdata[:,[1,2]] = gtdata[:,[2,1]]
    return gtdata
    
def load_positions(RSSI_SYSFILE):
    fin = open(RSSI_SYSFILE,'r')
    positions = {}
    for line in fin:
        match = re.search('pos_(\S+)\s+=\s+(\S+)\s+(\S+)',line)
        if match:
            sensor,posy,posx = match.groups()
            #positions[sensor] = (float(posx) + refX,  -float(posy) + refY);
            positions[sensor] = (float(posx) + refX,  float(posy) - refY);
            continue;
        
        match = re.search('refX\s+=\s+(\S+)',line)
        if match:
            refX = float(match.group(1))
        
        match = re.search('refY\s+=\s+(\S+)',line)
        if match:
            refY = float(match.group(1))
                
    
    fin.close()
    return positions