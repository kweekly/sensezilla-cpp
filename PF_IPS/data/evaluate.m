figure;
COLORS = hsv(10);
gt = importdata('pos_5.txt');
gt(:,2) = gt(:,2) + 1.6;
gt(:,3) = gt(:,3) + 1.8;

hx = subplot(2,1,1); hold on; plot(gt(:,1),gt(:,3),'Color',COLORS(1,:)); title('x-coordinate');
hy = subplot(2,1,2); hold on; plot(gt(:,1),gt(:,2),'Color',COLORS(1,:)); title('y-coordinate');
colidx = 2;
files = dir('trajout*.csv');
for file = files'
    
    dat = importdata(file.name);
    dat(:,1) = (dat(:,1) - dat(1,1));
    dat = dat(dat(:,1)<=gt(end,1),:);
    
    plot(hx, dat(:,1),dat(:,2), 'Color',COLORS(colidx,:));
    plot(hy, dat(:,1),-dat(:,3), 'Color',COLORS(colidx,:));
    
    colidx = colidx + 1;
end