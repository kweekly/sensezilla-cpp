import sys,os,time
from os import path
from subprocess import call
import numpy as np
import matplotlib.pyplot as plt
from pfutils import *

NTESTS = 10

SIMULATE = False;

EXE_PATH = "../Release/PF_IPS.exe"
OUT_DIR = "data/"

DATA_FILE = "../data/test5/rssiparam.conf"
GT_FILE = "../data/pos_5.txt";

#DATA_FILE = "../data/test/rssiparam.conf"
#GT_FILE = "../data/ground_truth.txt";


class ShmooVariable:
    def __init__(self, name, init):
        self.n = init;
        self.name = name;
    
    def asInt(self):
        return int(self.n);
    
    def asFloat(self):
        return float(self.n);
        
    def asString(self):
        return str(self.n);
        
    def fmt(self,v):
        if isinstance(v,float) or isinstance(v,float):
            return "%s_%.2f"%(self.name,v)
        else:
            return "%s_%s"%(self.name,v)
    
    def __str__(self):
        if isinstance(self.n,float) or isinstance(self.n,float):
            return "%s_%.2f"%(self.name,self.n)
        else:
            return "%s_%s"%(self.name,self.n)
        
    def set(self, i):
        self.n = i;
        
    def get(self):
        return self.n

class Params:
    def __init__(self):
        self.rssiparam = DATA_FILE
        self.mappng = "../data/floorplan_new.png"
        self.mapbounds = "-16.011,46.85,-30.807,15.954"
        self.cellwidth = ShmooVariable("CellWidth",0.75)
        self.mapcache = "../data/mapcache.dat"
        self.moverwrite = "-moverwrite"
        self.particles = ShmooVariable("Particles",600)
        self.reposition = ShmooVariable("Reposition",0.1)
        self.movespeed = ShmooVariable("Movespeed",1.1)
        self.groundtruth = GT_FILE
        self.gtorigin = "%.1f,%.1f"%(GT_ORIGIN_X,GT_ORIGIN_Y)
        self.visualize = "-novis"
        self.dt = ShmooVariable("dt",5.0);
        self.maxmethod = ShmooVariable("maxmethod","weighted");
        self.attmax = ShmooVariable("attmax",3.0)
        self.attdiff = ShmooVariable("attdiff",1.5)
        
        ## other params!
        self.nBins = ShmooVariable("nbins",5);
        
        self.trajout = OUT_DIR + "trajout_05.csv"
    
    def rcell_cmd(self):
        return ['python',"scatter_rcell.py","%d"%self.nBins.asInt(),"-nogui"]
    
    def cmd(self):
        cargs= [EXE_PATH,"-rssiparam",self.rssiparam,
                         "-mappng",self.mappng,
                         "-mapbounds",self.mapbounds,
                         "-cellwidth","%.2f"%self.cellwidth.asFloat(),
                         "-mapcache",self.mapcache,
                         self.moverwrite,
                         "-particles","%d"%self.particles.asInt(),
                         "-reposition","%.2f"%self.reposition.asFloat(),
                         "-movespeed","%.2f"%self.movespeed.asFloat(),
                         "-groundtruth",self.groundtruth,
                         "-gtorigin",self.gtorigin,
                         "-dt","%.2f"%self.dt.asFloat(),
                         "-maxmethod",self.maxmethod.asString(),
                         "-attenuation","%.2f,%.2f"%(self.attmax.asFloat(),self.attdiff.asFloat()),
                         self.visualize,
                         "-trajout",self.trajout];
                         
        if SIMULATE: cargs.append('-simulate')
        return cargs;
                        
                        
                        
devnull = open(os.devnull, 'w')

current_rcell = 0

def run_test(params, nTests=NTESTS):
    global current_rcell
    if nTests > 1:
        return [run_test(params,1) for i in range(NTESTS)]
    
    if path.exists(params.trajout):
        os.remove(params.trajout)
    
    if ( current_rcell != params.nBins.asInt() ):
        print " ".join(params.rcell_cmd());
        call(params.rcell_cmd(),stdout=devnull,stderr=devnull);
        current_rcell = params.nBins.asInt()
    
    print " ".join(params.cmd());
    call(params.cmd(),stdout=devnull,stderr=devnull);
    
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
    if isinstance(result, list):
        return np.mean( [evaluate_error(r,gtdata,method) for r in result] );

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
    
    vals = sorted(results.keys())
    for v in vals:
        if isinstance(results[v],list):
            rv = results[v][-1]
        else:
            rv = results[v]
        xaxis.plot(rv[:,0],rv[:,1], label=sweepvar.fmt(v).replace('_','='))
        yaxis.plot(rv[:,0],rv[:,2])
        
    
    handles, labels = xaxis.get_legend_handles_labels()
    xaxis.legend(handles,labels);
    
    prefix = ""
    if ( len(sys.argv) >= 2 ):
        prefix = sys.argv[1]
    
    fname = prefix+"_trajs_"+sweepvar.name;
    plt.savefig(fname+".png",dpi=600);
    fout = open(fname+".csv",'w')
    fout.write("#t,gt_x,gt_y,"+",".join(["%s_x,%s_y"%(v,v) for v in vals]) + "\n");
    for tidx in range(gtdata.shape[0]):
        fout.write("%.1f,%.2f,%.2f"%(gtdata[tidx,0],gtdata[tidx,1],gtdata[tidx,2]))
        for v in vals:
            fout.write(",%.5f,%.5f"%(results[v][-1][tidx,0],results[v][-1][tidx,1]));
        fout.write("\n");
    fout.close()

def plot_errors(params, sweepvar, results, gtdata):
    plt.figure()
    plt.title("Varying "+sweepvar.name);
    ax = plt.axes()
    vals = sorted(results.keys());
    RMSerr = [evaluate_error(results[v],gtdata,method="RMS") for v in vals]
    MAXerr = [evaluate_error(results[v],gtdata,method="MAX") for v in vals]
    MINerr = [evaluate_error(results[v],gtdata,method="MIN") for v in vals]
    if isinstance(vals[0],str):
        valstr = vals
        vals = range(len(vals))
        plt.xticks(vals,valstr);
    
    ax.plot(vals,RMSerr,'-+',label="RMS Error");
    ax.plot(vals,MAXerr,'-+',label="MAX Error");
    ax.plot(vals,MINerr,'-+',label="MIN Error");
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles,labels);
    plt.xticks(vals);
    
    prefix = ""
    if ( len(sys.argv) >= 2 ):
        prefix = sys.argv[1]
        
    fname = prefix+"_shmoo_"+sweepvar.name;
    plt.savefig(fname+".png",dpi=600);
    fout = open(fname+".csv",'w')
    for vidx in range(len(vals)):
        fout.write("%s,%.5f,%.5f,%.5f\n"%(str(vals[vidx]),RMSerr[vidx],MAXerr[vidx],MINerr[vidx]));
    fout.close()


    
params = Params()

def doit(svar,svals):
    v = svar.get()
    results = run_sweep(params, svar, svals);
    #plot_vs_gt(params, svar, results, gtdata);
    plot_errors(params,svar,results,gtdata);
    svar.set(v)
    

gtdata = read_gt(GT_FILE)

if True:
    doit(params.nBins,np.arange(1,11,1))
    doit(params.attmax,np.arange(0,11,1));
    doit(params.attdiff,np.arange(0,5,0.5));
    doit(params.particles,np.arange(100,1100,100));
    doit(params.reposition,np.arange(0,1.1,.1));
    doit(params.dt,np.arange(1,21,2));
    doit(params.movespeed,np.arange(0.8,2,0.1))
    doit(params.cellwidth,np.arange(0.25,2.25,0.25))
    doit(params.maxmethod,['maxp','mean','weighted'])

# single test
NTESTS = 1;
results = run_sweep(params,params.particles,[params.particles.get()])
plot_vs_gt(params,params.particles,results,gtdata)
plt.show()