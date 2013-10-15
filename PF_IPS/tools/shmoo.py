import sys,os,time
from os import path
from subprocess import call
import numpy as np
import matplotlib.pyplot as plt
from pfutils import *
from ProcessTimer import ProcessTimer

NTESTS = 25

SIMULATE = False;

EXE_PATH = "../Release/PF_IPS.exe"
OUT_DIR = "data/"

DATA_FILE = "../data/5_onetag/rssiparam.conf"
GT_FILE = "../data/pos_5.txt";

#DATA_FILE = "../data/4_onetag/rssiparam.conf"
#GT_FILE = "../data/pos_4.txt"

#DATA_FILE = "../data/test/rssiparam.conf"
#GT_FILE = "../data/ground_truth.txt";


devnull = open(os.devnull, 'w')

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
        self.particles = ShmooVariable("Particles",100)
        self.reposition = ShmooVariable("Reposition",0.1)
        self.movespeed = ShmooVariable("Movespeed",0.5)
        self.groundtruth = GT_FILE
        self.gtorigin = "%.1f,%.1f"%(GT_ORIGIN_X,GT_ORIGIN_Y)
        self.visualize = "-novis"
        self.dt = ShmooVariable("dt",5.0);
        self.maxmethod = ShmooVariable("maxmethod","weighted");
        self.attmax = ShmooVariable("attmax",3.0)
        self.attdiff = ShmooVariable("attdiff",1.5)
        self.nthreads = ShmooVariable("nthreads",5);
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
                         "-nthreads","%d"%self.nthreads.asInt(),
                         self.visualize,
                         "-trajout",self.trajout];
                         
        if SIMULATE: cargs.append('-simulate')
        return cargs;
                        
                        


current_rcell = 0
exec_time = 0
mem_usage = 0
skipped_first = False
exec_times = {}
mem_usages = {}
error_collector = []

def run_test(params, nTests=NTESTS):
    global current_rcell, exec_time, mem_usage, skipped_first
    if nTests > 1:
        exec_time = 0
        mem_usage = 0
        skipped_first = False
        return [run_test(params,1) for i in range(NTESTS)]
    
    if path.exists(params.trajout):
        os.remove(params.trajout)
    
    if ( current_rcell != params.nBins.asInt() ):
        print " ".join(params.rcell_cmd());
        call(params.rcell_cmd(),stdout=devnull,stderr=devnull);
        current_rcell = params.nBins.asInt()
    
    print " ".join(params.cmd());
    ptimer = ProcessTimer(params.cmd());
    try:
        ptimer.execute()
        while ptimer.poll():
            time.sleep(0.01);
    finally:
        ptimer.close()
        
    if skipped_first:
        exec_time += ptimer.t1 - ptimer.t0
        mem_usage += ptimer.max_rss_memory
    else:
        skipped_first = True
    
    if ( not path.exists(params.trajout) ):
        raise Exception("Trajout file not created for command ("+" ".join(params.cmd())+")");
        
    data = np.loadtxt(params.trajout, delimiter=',');
    data[:,0] = data[:,0]-data[1,0];
    data[:,2] = -data[:,2];
    return data;        
        
def run_sweep(params, sweepvar, sweepvals):
    global exec_times
    results = {}
    for v in sweepvals:
        sweepvar.set(v)
        params.trajout = OUT_DIR + "trajout_%s.csv"%str(sweepvar);
        results[v] = run_test(params, nTests=NTESTS)
        exec_times[v] = exec_time / (NTESTS-1);
        mem_usages[v] = mem_usage / float(NTESTS-1);
    
    return results
    
def evaluate_error(result,gtdata,method="RMS"):
    global error_collector
    if isinstance(result, list):
        rrs =  [evaluate_error(r,gtdata,method) for r in result];
        if ( method == "RMS"):
            return np.mean(rrs);
        elif ( method == "MAX"):
            return np.max(rrs);
        elif (method == "MIN"):
            return np.min(rrs);

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
        error_collector.append(np.sqrt(diffsq))
        
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
    xaxis = plt.subplot(3,1,1);
    plt.xlim(gtdata[0,0],gtdata[-1,0]);
    gtlh = xaxis.plot(gtdata[:,0],gtdata[:,1],'k-.',label="Ground Truth");
    plt.ylabel('x coordinate (m)')
    
    yaxis = plt.subplot(3,1,2);
    plt.xlim(gtdata[0,0],gtdata[-1,0]);
    yaxis.plot(gtdata[:,0],gtdata[:,2],'k-.');
    plt.ylabel('y coordinate (m)')
    
    eaxis = plt.subplot(3,1,3);
    plt.xlim(gtdata[0,0],gtdata[-1,0]);
    plt.ylabel('error (m)')
    plt.xlabel('time (s)')
    
    vals = sorted(results.keys())
    for v in vals:
        if isinstance(results[v],list):
            rv = results[v][-1]
        else:
            rv = results[v]
        xaxis.plot(rv[:,0],rv[:,1],'k-', label=sweepvar.fmt(v).replace('_','='))
        yaxis.plot(rv[:,0],rv[:,2],'k-')
        gtix = np.interp(rv[:,0],gtdata[:,0],gtdata[:,1])
        gtiy = np.interp(rv[:,0],gtdata[:,0],gtdata[:,2])
        eaxis.plot(rv[:,0],np.sqrt( np.square(gtix - rv[:,1]) + np.square(gtiy-rv[:,2])),'k-');
        print sweepvar.fmt(v).replace('_','=')
        print "\tMAX: %.2f RMS: %.2f"%(evaluate_error(results[v],gtdata,method="MAX"),evaluate_error(results[v],gtdata,method="RMS"))
    
    handles, labels = xaxis.get_legend_handles_labels()
    #xaxis.legend(handles,labels);
    
    prefix = ""
    if ( len(sys.argv) >= 2 ):
        prefix = sys.argv[1]
    
    fname = prefix+"_trajs_"+sweepvar.name;
    plt.savefig(fname+".pdf",dpi=300);
    fout = open(fname+".csv",'w')
    fout.write("#t,gt_x,gt_y,"+",".join(["%s_x,%s_y"%(v,v) for v in vals]) + "\n");
    for tidx in range(rv.shape[0]):
        gtix = np.interp(rv[tidx,0],gtdata[:,0],gtdata[:,1])
        gtiy = np.interp(rv[tidx,0],gtdata[:,0],gtdata[:,2])
        fout.write("%.1f,%.2f,%.2f"%(rv[tidx,0],gtix,gtiy))
        for v in vals:
            fout.write(",%.5f,%.5f"%(results[v][-1][tidx,1],results[v][-1][tidx,2]));
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
    EXCtime = [exec_times[v] for v in vals]
    MEMusg = [mem_usages[v] for v in vals]
    if isinstance(vals[0],str):
        valstr = vals
        vals = range(len(vals))
        plt.xticks(vals,valstr);
    
    print "\nRMS:\n\t",RMSerr;
    print "\nMAX:\n\t",MAXerr
    ax.plot(vals,RMSerr,'-+',label="RMS Error");
    ax.plot(vals,MAXerr,'-+',label="MAX Error");
    ax.plot(vals,MINerr,'-+',label="MIN Error");
    ax.plot(vals,EXCtime,'-+',label="Exec. time");
    #ax.plot(vals,MEMusg,'-+',label="Mem. Usage");
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles,labels);
    plt.xticks(vals);
    
    prefix = ""
    if ( len(sys.argv) >= 2 ):
        prefix = sys.argv[1]
        
    fname = prefix+"_shmoo_"+sweepvar.name;
    plt.savefig(fname+".pdf",dpi=300);
    fout = open(fname+".csv",'w')
    for vidx in range(len(vals)):
        fout.write("%s,%.5f,%.5f,%.5f,%.3f,%.1f\n"%(str(vals[vidx]),RMSerr[vidx],MAXerr[vidx],MINerr[vidx],EXCtime[vidx],MEMusg[vidx]));
    fout.close()


    
params = Params()

def doit(svar,svals):
    v = svar.get()
    results = run_sweep(params, svar, svals);
    #plot_vs_gt(params, svar, results, gtdata);
    plot_errors(params,svar,results,gtdata);
    svar.set(v)
    

gtdata = read_gt(GT_FILE)

#doit(params.particles,np.arange(100,2100,100))
#doit(params.cellwidth,np.append(np.arange(0.10,0.21,0.01),np.arange(0.20,1.1,0.1)))
#doit(params.dt,np.arange(0.1,5.1,0.1));
#doit(params.nthreads,np.arange(1,16,1));
doit(params.reposition,np.arange(0,1.1,.1));
if False:
    doit(params.nBins,np.arange(1,11,1))
    doit(params.attmax,np.arange(0,11,1));
    doit(params.attdiff,np.arange(0,5,0.5));
    doit(params.particles,np.arange(10,110,10));
    doit(params.reposition,np.arange(0,1.1,.1));
    doit(params.dt,np.arange(1,21,2));
    doit(params.movespeed,np.arange(0.1,1.6,0.1))
    doit(params.cellwidth,np.arange(0.25,2.25,0.25))
    doit(params.maxmethod,['maxp','mean','weighted'])

plt.show()
sys.exit(1)
# single test
NTESTS = 1000
results = run_sweep(params,params.particles,[params.particles.get()])
#plot_vs_gt(params,params.particles,results,gtdata)
plot_errors(params,params.particles,results,gtdata)
fout = open("hist_out.csv",'w');
for f in error_collector:
    fout.write("%.5f\n"%f);
fout.close();
plt.show()
