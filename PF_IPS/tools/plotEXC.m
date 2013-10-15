dat = importdata('_shmoo_Particles.csv');
[ax,h1,h2] = plotyy(dat(:,1),dat(:,6)/1e6,dat(:,1),dat(:,5)*1000);
set(get(ax(1),'Ylabel'),'String','Memory Use (MB)');
set(get(ax(2),'Ylabel'),'String','Execution Time (ms)');
xlabel('No. of particles');
set(h1,'Marker','+');
set(h1,'LineWidth',2);
set(h2,'Marker','o');
set(h2,'LineWidth',2);

%%
dat = importdata('_shmoo_CellWidth.csv');
[ax,h1,h2] = plotyy(dat(:,1)*100,dat(:,6)/1e6,dat(:,1)*100,dat(:,5));
set(get(ax(1),'Ylabel'),'String','Memory Use (MB)');
set(get(ax(2),'Ylabel'),'String','Execution Time (s)');
set(ax(1),'YTick',[7.7 474.2 500]);
set(ax(2),'YTick',[0.12 7.8]);
xlabel('Cell Width (cm)');
set(h1,'Marker','+');
set(h1,'LineWidth',2);
set(h2,'Marker','o');
set(h2,'LineWidth',2);

%%
dat = importdata('_shmoo_dt.csv'); 
[ax,h1,h2] = plotyy(dat(:,1),dat(:,6)/1e6,dat(1:end,1),dat(:,5) );
set(get(ax(1),'Ylabel'),'String','Memory Use (MB)');
set(get(ax(2),'Ylabel'),'String','Execution Time (s)');
xlabel('Time Interval (s)');
set(h1,'Marker','+');
set(h1,'LineWidth',2);
set(h2,'Marker','o');
set(h2,'LineWidth',2);


%%
dat = importdata('_shmoo_nthreads.csv');
plot(dat(:,1),dat(:,5),'o-','LineWidth',2);
ylabel('Execution Time (s)');
xlabel('No. of worker threads');