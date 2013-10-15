dd = importdata('VER_trajs_Particles.csv');
figure;
axis off;
imdata = imread('floorplan_bw.png');
imdata = (imdata(:,:,3) == 255);
minx = -16.011;
maxx = 46.85;
wid = maxx-minx;
miny = -30.807;
maxy = 15.954;
hei = maxy-miny;
imshow(imdata,'InitialMagnification',25);
imh = size(imdata,1);
imw = size(imdata,2);
ratw = imw/wid;
rath = imh/hei;
axis equal;
for ii = 1:size(dd.data,1)
    x1 = (dd.data(ii,2)-minx)*ratw;
    y1 = (-dd.data(ii,3)-miny)*rath;
    x2 = (dd.data(ii,4)-minx)*ratw;
    y2 = (-dd.data(ii,5)-miny)*rath;
    
    ah = annotation('doublearrow',...
        'head2Style','rectangle',...
        'head1style','none',...
        'LineStyle','none',...
         'headwidth',10000,'headlength',10000,'Color','red');
    set(ah,'LineWidth',0.);
    set(ah,'Parent',gca);
    set(ah,'position',[x1 y1 (x2-x1) (y2-y1)]);
end

for ii = 1:size(dd.data,1)
    x1 = (dd.data(ii,2)-minx)*ratw;
    y1 = (-dd.data(ii,3)-miny)*rath;
    x2 = (dd.data(ii,4)-minx)*ratw;
    y2 = (-dd.data(ii,5)-miny)*rath;
    
    ah = annotation('doublearrow',...
        'head2Style','none',...
        'head1style','ellipse',...
        'LineStyle','-',...
         'headwidth',10000,'headlength',10000,'Color','blue');
    set(ah,'LineWidth',0.);
    set(ah,'Parent',gca);
    set(ah,'position',[x1 y1 (x2-x1) (y2-y1)]);
end
axis equal;

%q = quiver(dd.data(:,2),dd.data(:,3),dd.data(:,4)-dd.data(:,2),dd.data(:,5)-dd.data(:,3));
