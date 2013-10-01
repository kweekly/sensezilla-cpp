import sys,os,time
from os import path
from subprocess import call
import numpy as np
import matplotlib.pyplot as plt

EXE_PATH = "../Release/PF_IPS.exe"
OUT_DIR = "data/"

DATA_FILE = "../data/test5/rssiparam.conf"
GT_FILE = "../data/pos_5.txt";

#DATA_FILE = "../data/test/rssiparam.conf"
#GT_FILE = "../data/ground_truth.txt";


GT_ORIGIN_X = 1.6
GT_ORIGIN_Y = 1.8

def read_gt():
    gtdata = np.loadtxt(GT_FILE,delimiter=',')
    gtdata[:,1] = gtdata[:,1] + GT_ORIGIN_X
    gtdata[:,2] = gtdata[:,2] + GT_ORIGIN_Y
    gtdata[:,[1,2]] = gtdata[:,[2,1]]
    return gtdata

class NumericVariable:
    def __init__(self, name, init):
        self.n = init;
        self.name = name;
    
    def asInt(self):
        return int(self.n);
    
    def asFloat(self):
        return float(self.n);
    
    def __str__(self):
        return "%s_%.1f"%(self.name,self.n)
        
    def set(self, i):
        self.n = i;

class Params:
    def __init__(self):
        self.rssiparam = DATA_FILE
        self.mappng = "../data/floorplan_new.png"
        self.mapbounds = "-16.011,46.85,-30.807,15.954"
        self.cellwidth = NumericVariable("CellWidth",0.75)
        self.mapcache = "../data/mapcache.dat"
        self.moverwrite = "-moverwrite"
        self.particles = NumericVariable("Particles",5000)
        self.reposition = NumericVariable("Reposition",1.0)
        self.movespeed = NumericVariable("Movespeed",1.4)
        self.groundtruth = GT_FILE
        self.gtorigin = "%.1f,%.1f"%(GT_ORIGIN_X,GT_ORIGIN_Y)
        self.visualize = "-novis"
        self.dt = NumericVariable("dt",5.0);
        
        self.trajout = OUT_DIR + "trajout_05.csv"
        
    def cmd(self):
        return [EXE_PATH,"-rssiparam",self.rssiparam,
                         "-mappng",self.mappng,
                         "-mapbounds",self.mapbounds,
                         "-cellwidth","%.1f"%self.cellwidth.asFloat(),
                         "-mapcache",self.mapcache,
                         self.moverwrite,
                         "-particles","%d"%self.particles.asInt(),
                         "-reposition","%.2f"%self.reposition.asFloat(),
                         "-movespeed","%.2f"%self.movespeed.asFloat(),
                         "-groundtruth",self.groundtruth,
                         "-gtorigin",self.gtorigin,
                         "-dt","%.2f"%self.dt.asFloat(),
                         self.visualize,
                         "-trajout",self.trajout];
                        
def run_test(params, overwrite=False):
    if ( overwrite and path.exists(params.trajout) ):
        os.remove(params.trajout);        
    
    print " ".join(params.cmd());
    if ( path.exists(params.trajout) ):
        print "\tSkipped."
    else:
        call(params.cmd());
    
    if ( not path.exists(params.trajout) ):
        raise Exception("Trajout file not created for command ("+" ".join(params.cmd())+")");
        
    data = np.loadtxt(params.trajout, delimiter=',');
    data[:,0] = data[:,0]-data[1,0];
    data[:,2] = -data[:,2];
    return data;        
        
def run_sweep(params, sweepvar, sweepvals):
    results = {}
    for v in sweepvals:
        sweepvar.set(v)
        params.trajout = OUT_DIR + "trajout_%s.csv"%str(sweepvar);
        results[v] = run_test(params)
    
    return results
    
def evaluate_error(result,gtdata,method="RMS"):
    gtidx = 0;
    residx = 0;
    # align start of data
    while result[residx,0] < gtdata[gtidx,0]:
        residx += 1
     
    accum = 0
    n = 0
    while residx < result.shape[0]:
        while gtidx < gtdata.shape[0] and gtdata[gtidx,0] < result[residx,0] :
            gtidx += 1

        if ( gtidx == gtdata.shape[0] ): break;
        
        pos_res = result[residx,[1,2]]
        tdiff = result[residx,0] - gtdata[gtidx-1,0];
        ttot = gtdata[gtidx,0] - gtdata[gtidx-1,0];
        trat = tdiff / ttot;
        pos_gt = gtdata[gtidx-1,[1,2]]*(1-trat) + gtdata[gtidx,[1,2]]*trat;
        
        #print pos_res,pos_gt
        diffsq = (pos_res[0] - pos_gt[0])**2 + (pos_res[1] - pos_gt[1])**2;
        
        if ( method == "RMS" ):
            accum += diffsq;
        elif ( method == "MAX"):
            if ( diffsq > accum ):
                accum = diffsq;
        elif (method == "MIN"):
            if ( diffsq < accum or accum == 0):
                accum = diffsq
        else:
            raise Exception("Unknown Method "+method);
        
        residx += 1
        n += 1
        
    if ( method == "RMS") :
        return np.sqrt( accum / n )
    elif (method == "MAX" or method == "MIN") :
        return np.sqrt( accum )
  
def plot_vs_gt(params, sweepvar, results, gtdata):
    plt.figure()
    xaxis = plt.subplot(2,1,1);
    plt.xlim(gtdata[0,0],gtdata[-1,0]);
    gtlh = xaxis.plot(gtdata[:,0],gtdata[:,1],label="Ground Truth");
    plt.ylabel('x coordinate')
    
    yaxis = plt.subplot(2,1,2);
    plt.xlim(gtdata[0,0],gtdata[-1,0]);
    yaxis.plot(gtdata[:,0],gtdata[:,2]);
    plt.ylabel('y coordinate')
    plt.xlabel('time')
    
    for v in sorted(results.keys()):
        xaxis.plot(results[v][:,0],results[v][:,1], label="%s=%.2f"%(sweepvar.name,v))
        yaxis.plot(results[v][:,0],results[v][:,2])
        
    
    handles, labels = xaxis.get_legend_handles_labels()
    xaxis.legend(handles,labels);

def plot_errors(params, sweepvar, results, gtdata):
    plt.figure()
    plt.title("Varying "+sweepvar.name);
    ax = plt.axes()
    vals = sorted(results.keys());
    RMSerr = [evaluate_error(results[v],gtdata,method="RMS") for v in vals]
    MAXerr = [evaluate_error(results[v],gtdata,method="MAX") for v in vals]
    MINerr = [evaluate_error(results[v],gtdata,method="MIN") for v in vals]
    ax.plot(vals,RMSerr,'-+',label="RMS Error");
    ax.plot(vals,MAXerr,'-+',label="MAX Error");
    ax.plot(vals,MINerr,'-+',label="MIN Error");
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles,labels);
    plt.xticks(vals);
    
gtdata = read_gt()
params = Params()

svar = params.particles;
#svals = [10,50,100,500,1000,5000,10000];
svals = np.arange(100,1100,100);

#svar = params.reposition;
#svals = np.arange(0,1.1,.1)

#svar = params.dt;
#svals = np.arange(1,21,2)

#svar = params.movespeed
#svals = np.arange(0.8,2,0.1)

#svar = params.cellwidth
#svals = np.arange(0.25,2.25,0.25)

results = run_sweep(params,svar, svals);
plot_vs_gt(params, svar, results, gtdata);
plot_errors(params,svar,results,gtdata);

plt.show();
