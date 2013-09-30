dat = importdata('trajout.csv');
gt = importdata('pos_5.txt');
gt(:,2) = gt(:,2) + 1.6;
gt(:,3) = gt(:,3) + 1.8;
dat(:,1) = (dat(:,1) - dat(1,1));
dat(:,1) = dat(:,1)/dat(end,1) * gt(end,1);
subplot(2,1,1); plot(dat(:,1),-dat(:,3),gt(:,1),gt(:,2)); title('y-coordinate');
subplot(2,1,2); plot(dat(:,1),dat(:,2),gt(:,1),gt(:,3)); title('x-coordinate');