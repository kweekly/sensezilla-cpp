import sys,os,time
from os import path
from subprocess import call
import numpy as np
import matplotlib.pyplot as plt
import scipy as sp
from scipy import stats
import glob
from pfutils import *
import re

TICK = time.time();
DATAPOINTS = 0;

GUI = True
if "-nogui" in sys.argv:
    GUI = False;
    sys.argv.remove("-nogui");

DATA_DIR = "D:\\documents\\ucb\\singapore\\experiments\\GT_05022013\\chairtest\\"
GT_FILE = "../data/ground_truth.txt"
TSTART = 1367506685;

MAXR = 21.0
RBINS = 5

if len(sys.argv) >= 2:
    RBINS = int(sys.argv[1])

BINSIZE = MAXR / RBINS; 

#gaussian smoothing kernel points
X_ks = np.linspace(-125,-10,100);

gtdata = read_gt(GT_FILE);

sensor_positions = load_positions(DATA_DIR+"..\\rssi_system.conf")

files = glob.glob(DATA_DIR+"RSSI_t*.csv");
sensors = set();
tags = set()
for f in files:
    result = re.search('RSSI_t(.+?)_s(.+?).csv',str(f));
    if result == None:
        raise Exception("Error glob matched something that wasn't RE'd: "+f)
    tag,sensor = result.groups()
    sensors.add(sensor)
    tags.add(tag)
    
if GUI: plt.figure()
    
sidx = 0;
for s in sensors:
    distances = np.zeros((0,1));
    measurements = np.zeros((0,1));
    spos = sensor_positions[s]
    
    for t in tags:
        fname = "RSSI_t%s_s%s.csv"%(t,s)
        if not path.exists(DATA_DIR+fname):
            print "Missing file "+DATA_DIR+fname
            
        subdat = np.loadtxt(DATA_DIR+fname, delimiter=',');
        if subdat.size == 0:
            print fname+" is empty"
            continue;
            
        subdat[:,0] = subdat[:,0]/1000 - TSTART;
        
        gti_x = np.interp(subdat[:,0],gtdata[:,0],gtdata[:,1]);
        gti_y = np.interp(subdat[:,0],gtdata[:,0],gtdata[:,2]);
        
        dx = gti_x - spos[0]
        dy = gti_y - spos[1]
        dist = np.sqrt(np.square(dx) + np.square(dy))
        distances = np.append(distances,dist);
        measurements = np.append(measurements,subdat[:,1])
        DATAPOINTS += dx.size
    
    rcells = [np.zeros((0,1)) for i in range(RBINS)]
    for mi in range(len(distances)):
        if not np.isnan(distances[mi]) and not np.isnan(measurements[mi]):
            binidx = int( distances[mi] / BINSIZE )
            if ( binidx >= RBINS ):
                print "Distance %.2f too large, putting in last bin"%(distances[mi])
                binidx = RBINS-1;
            rcells[binidx] = np.append(rcells[binidx], measurements[mi])
            
    F_ks = np.empty((X_ks.size,RBINS))
    if GUI: subplt = plt.subplot( int(len(sensors)/6.0), 6, sidx );
    for bi in range(len(rcells)):
        print "Sensor %s, Bin %.2f-%.2f : %d"%(s,bi*BINSIZE,(bi+1)*BINSIZE, len(rcells[bi]))
        if len(rcells[bi]) > 0:
            gkde = stats.gaussian_kde(rcells[bi])
            geval = gkde(X_ks)
            geval[geval < 1e-100] = 0;
            F_ks[:,bi] = geval
            if GUI: 
                subplt.plot(X_ks,F_ks[:,bi])
                plt.title(s);
    
    print ""
    output = np.concatenate((X_ks.reshape(X_ks.size,1),F_ks),axis=1)
    output = np.concatenate(((np.arange(0,RBINS+1)*BINSIZE).reshape(1,RBINS+1), output), axis=0)
    np.savetxt(DATA_DIR+"rcellparams_s%s.csv"%s,output,fmt='%.10e',delimiter=',')
    
    sidx += 1
    
print "Total datapoints: %d, Execution time: %.2f"%(DATAPOINTS, time.time() - TICK)
if GUI:
    plt.show()
    
    