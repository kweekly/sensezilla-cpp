import glob, os

for file in glob.glob('RSSI*.csv'):
    fin = open(file,'r');
    fout = open(file+'.tmp','w');
    for  line in fin :
        try:
            if ( int(line[:line.index('.')]) > (1367502723000 + 1000*60*6) ):
                continue;
        except:pass
    
        fout.write(line);
        
    
    fin.close();
    fout.close();
    
    os.remove(file);
    os.rename(file+'.tmp',file);
    
    