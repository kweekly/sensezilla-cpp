files = dir('1.*.csv');

figure;
hold on;
pp = [];
for i = 1:numel(files)
   fname = files(i).name; 
   dat = importdata(fname,',');
   if ( size(dat,1) > size(pp,1) ) 
      pp = [pp; zeros(size(dat,1)-size(pp,1),size(pp,2))];
      datx = dat(:,1);
   end
   pp(:,i) = [dat(:,2); zeros(size(pp,1)-size(dat,1),1)];
end

plot(datx-datx(1),pp);